#include "Enemy/Boss/BossShadowSummon.h"
#include "Enemy/Boss/BossShadow.h"
#include "Ghost/GhostCharacter/GhostCharacter.h"
#include "Ghost/GhostEnemy/GhostEnemyComponent.h"
#include "NavigationSystem.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Enemy/Combat/Defense.h"

namespace BossShadowTuning
{
    //分身を探す間隔。
    constexpr float checkDelay = .5f;
}

//分身の確認は毎フレームではなく一定間隔で行う。
UBossShadowSummon::UBossShadowSummon()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = BossShadowTuning::checkDelay;
}

//本体の戦闘が終われば影も消す。
void UBossShadowSummon::Reset()
{
    for(auto* shadow : {m_shadow.Get(), m_secondShadow.Get()})
    {
        if(IsValid(shadow)) { shadow->FadeOut(); }
    }
    m_nextTime = 0.;
    m_lastJoin = INDEX_NONE;
    m_cloneCount = 0;
}

//削除時は影のAIも残さず片付ける。
void UBossShadowSummon::EndPlay(const EEndPlayReason::Type _reason)
{
    for(auto* shadow : {m_shadow.Get(), m_secondShadow.Get()})
    {
        if(!IsValid(shadow)) { continue; }
        if(auto* ai = shadow->GetController()) { ai->UnPossess(); ai->Destroy(); }
        shadow->Destroy();
    }
    Super::EndPlay(_reason);
}

//空いている場所へ影を呼び、同時に二体までにする。
void UBossShadowSummon::TickComponent(float _delta, ELevelTick _type, FActorComponentTickFunction* _tick)
{
    Super::TickComponent(_delta, _type, _tick);
    auto* boss = Cast<ABossEnemyCharacter>(GetOwner());
    if(!boss || !boss->HasAuthority() || boss->IsA<ABossShadow>()) { return; }
    if(!boss->IsCombatActive())
    {
        for(auto* shadow : {m_shadow.Get(), m_secondShadow.Get()})
        {
            if(IsValid(shadow)) { shadow->FadeOut(); }
        }
        return;
    }
    const double now = GetWorld()->GetTimeSeconds();

    //一体目が消えたら、残った影を一体目として扱う。
    if(!m_shadow.IsValid() && m_secondShadow.IsValid())
    {
        m_shadow = m_secondShadow;
        m_secondShadow.Reset();
    }

    //味方として活動中の分身から、担当が重ならない相手を探す。
    AActor* target = nullptr;
    float best = FMath::Square(m_range);
    m_cloneCount = 0;
    for(TActorIterator<AGhostCharacter> it(GetWorld()); it; ++it)
    {
        auto* ghost = *it;
        if(!ghost->EnemyComponent || ghost->IsHidden() || !ghost->CanBeDamaged()) { continue; }
        const auto state = ghost->EnemyComponent->GetLifecycleState();
        if(state != EGhostLifecycleState::Friendly && state != EGhostLifecycleState::Warning) { continue; }
        const float distance = FVector::DistSquared2D(boss->GetActorLocation(), ghost->GetActorLocation());
        if(distance <= FMath::Square(m_range)) { ++m_cloneCount; }
        if(m_shadow.IsValid() && m_shadow->m_combatTarget == ghost) { continue; }
        if(distance < best) { best = distance; target = ghost; }
    }
    const bool second = m_shadow.IsValid();
    if(!m_shadowClass || m_secondShadow.IsValid() || now < m_nextTime || boss->m_defense->IsBusy()) { return; }
    if(second && (!target || m_cloneCount < FMath::Max(2, m_twoCloneCount))) { return; }
    const bool firstPhase = boss->m_bossPhase == EBossPhase::PhaseOne;
    
    //序盤は複数の分身に囲まれた時だけ、影に分担させる。
    if(firstPhase && target && m_cloneCount < m_cloneLimit) { return; }
    
    //指定した本体の攻撃に合わせ、分身がいなければ連携する。
    const bool join = !target;
    if(join)
    {
        const auto* action = boss->GetActiveAction();
        auto* player = UGameplayStatics::GetPlayerPawn(this, 0);
        if(!m_enableJoin || !action || !m_joinMoves.Contains(action->m_actionId) || !IsValid(player) || !player->CanBeDamaged()) { return; }
        if(FVector::DistSquared2D(boss->GetActorLocation(), player->GetActorLocation()) > FMath::Square(m_joinRange)) { return; }
        if(firstPhase && !m_phaseOneJoin) { return; }
        if(m_lastJoin == boss->m_actionSerial) { return; }
        m_lastJoin = boss->m_actionSerial;
        if(FMath::FRand() >= FMath::Clamp(m_joinChance, 0.f, 1.f)) { return; }
        target = player;
    }

    //壁や床に埋まる場所には生成しない。
    auto* nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    FNavLocation floor;
   
    const FVector point = boss->GetActorLocation() + boss->GetActorRightVector() * m_spawnRange * (second ? -1.f : 1.f);
    if(!nav || !nav->ProjectPointToNavigation(point, floor)) { return; }
   
    const auto* defaults = m_shadowClass->GetDefaultObject<ABossShadow>();
    const FVector location = floor.Location + FVector(0.f, 0.f, defaults->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
    const FTransform pose((target->GetActorLocation() - location).Rotation(), location);
    auto* shadow = GetWorld()->SpawnActorDeferred<ABossShadow>(m_shadowClass, pose, boss, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding);
    if(!shadow) { m_nextTime = now + PrimaryComponentTick.TickInterval; return; }
   
    shadow->Setup(target, FMath::FRandRange(FMath::Max(1.f, m_lifeMin), FMath::Max(m_lifeMin, m_lifeMax)));
    if(join) { shadow->StartJoin(m_joinAction, m_joinDelay, m_range); }
    UGameplayStatics::FinishSpawningActor(shadow, pose);
    if(!IsValid(shadow)) { return; }
   
    if(second) { m_secondShadow = shadow; }
    else { m_shadow = shadow; }
    //五体以上なら、次の確認で二体目も呼べるようにする。
    const bool needSecond = !second && !join && m_cloneCount >= FMath::Max(2, m_twoCloneCount);
    m_nextTime = now + (needSecond ? PrimaryComponentTick.TickInterval : FMath::Max(0.f, join ? m_joinCooldown : m_cooldown));
}
