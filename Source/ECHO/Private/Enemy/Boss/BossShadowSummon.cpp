#include "Enemy/Boss/BossShadowSummon.h"
#include "Enemy/Boss/BossShadow.h"
#include "Ghost/GhostCharacter/GhostCharacter.h"
#include "Ghost/GhostEnemy/GhostEnemyComponent.h"
#include "NavigationSystem.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

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
    if(m_shadow.IsValid()) { m_shadow->FadeOut(); }
    m_seen.Empty();
    m_nextTime = 0.;
}

//削除時は影のAIも残さず片付ける。
void UBossShadowSummon::EndPlay(const EEndPlayReason::Type _reason)
{
    if(m_shadow.IsValid())
    {
        if(auto* ai = m_shadow->GetController()) { ai->UnPossess(); ai->Destroy(); }
        m_shadow->Destroy();
    }
    Super::EndPlay(_reason);
}

//歩ける場所が見つかった時だけ影を一体呼ぶ。
void UBossShadowSummon::TickComponent(float _delta, ELevelTick _type, FActorComponentTickFunction* _tick)
{
    Super::TickComponent(_delta, _type, _tick);
    auto* boss = Cast<ABossEnemyCharacter>(GetOwner());
    if(!boss || !boss->HasAuthority() || boss->IsA<ABossShadow>()) { return; }
    if(boss->m_bossPhase != EBossPhase::PhaseTwo || !boss->IsCombatActive())
    {
        if(m_shadow.IsValid()) { m_shadow->FadeOut(); }
        return;
    }
    const double now = GetWorld()->GetTimeSeconds();
    if(!m_shadowClass || m_shadow.IsValid() || now < m_nextTime) { return; }
    m_seen.RemoveAll([](const TWeakObjectPtr<AActor>& _actor) { return !_actor.IsValid(); });

    //味方として活動中の分身から最も近い一体を探す。
    AGhostCharacter* target = nullptr;
    float best = FMath::Square(m_range);
    for(TActorIterator<AGhostCharacter> it(GetWorld()); it; ++it)
    {
        auto* ghost = *it;
        if(!ghost->EnemyComponent || ghost->IsHidden() || !ghost->CanBeDamaged() || m_seen.Contains(ghost)) { continue; }
        const auto state = ghost->EnemyComponent->GetLifecycleState();
        if(state != EGhostLifecycleState::Friendly && state != EGhostLifecycleState::Warning) { continue; }
        const float distance = FVector::DistSquared2D(boss->GetActorLocation(), ghost->GetActorLocation());
        if(distance < best) { best = distance; target = ghost; }
    }
    if(!target) { return; }

    //壁や床に埋まる場所には生成しない。
    auto* nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    FNavLocation floor;
   
    const FVector point = boss->GetActorLocation() + boss->GetActorRightVector() * m_spawnRange;
    if(!nav || !nav->ProjectPointToNavigation(point, floor)) { return; }
   
    const auto* defaults = m_shadowClass->GetDefaultObject<ABossShadow>();
    const FVector location = floor.Location + FVector(0.f, 0.f, defaults->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
    const FTransform pose((target->GetActorLocation() - location).Rotation(), location);
    auto* shadow = GetWorld()->SpawnActorDeferred<ABossShadow>(m_shadowClass, pose, boss, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding);
    if(!shadow) { m_nextTime = now + PrimaryComponentTick.TickInterval; return; }
   
    shadow->Setup(target, FMath::FRandRange(FMath::Max(1.f, m_lifeMin), FMath::Max(m_lifeMin, m_lifeMax)));
    UGameplayStatics::FinishSpawningActor(shadow, pose);
    if(!IsValid(shadow)) { return; }
   
    m_shadow = shadow;
    m_seen.Add(target);
    m_nextTime = now + FMath::Max(0.f, m_cooldown);
}
