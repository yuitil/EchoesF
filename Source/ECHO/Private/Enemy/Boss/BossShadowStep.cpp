#include "Enemy/Boss/BossShadowSummon.h"
#include "Enemy/Boss/BossShadow.h"
#include "Enemy/Boss/BossApproach.h"
#include "Enemy/Combat/Combat.h"
#include "Enemy/Core/EnemyTraversal.h"
#include "Enemy/Combat/Defense.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Navigation/PathFollowingComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "Kismet/GameplayStatics.h"

namespace
{
    constexpr float stuckTravel = 30.f;
    constexpr float swapGap = 125.f;
    constexpr float panicGain = 60.f;
    constexpr float floorGap = 3.f;

    //床の高さを取り直し、入るキャラの体が壁や他の敵に重ならないか調べる。
    bool Landing(ACharacter& who, const FVector& near, const AActor* other, FVector& spot)
    {
        auto* world = who.GetWorld();
        auto* nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(world);
        auto* cap = who.GetCapsuleComponent();
        FNavLocation floor;
        const float height = cap->GetScaledCapsuleHalfHeight();
        if(!nav || !nav->ProjectPointToNavigation(near, floor, FVector(60.f,60.f,height+60.f))) { return false; }

        FCollisionQueryParams query(SCENE_QUERY_STAT(ShadowLanding),false,&who);
        query.AddIgnoredActor(other);
        FHitResult hit;
        const FVector up = FVector::UpVector * height;
        if(!world->LineTraceSingleByChannel(hit,floor.Location+up,floor.Location-up,ECC_WorldStatic,query)
            || !who.GetCharacterMovement()->IsWalkable(hit)) { return false; }
        spot = hit.ImpactPoint + FVector::UpVector*(height+floorGap);

        return !world->OverlapBlockingTestByProfile(spot,FQuat::Identity,cap->GetCollisionProfileName(),
            FCollisionShape::MakeCapsule(cap->GetScaledCapsuleRadius(),height),query);
    }

    void StopWalk(ACharacter& who)
    {
        if(auto* ai = Cast<AAIController>(who.GetController())) { ai->StopMovement(); }
        who.GetCharacterMovement()->StopMovementImmediately();
    }
}

bool UBossShadowSummon::IsChanging() const
{
    const auto* boss = Cast<ABossEnemyCharacter>(GetOwner());
    return boss && m_changeMove.IsValid() && boss->GetCurrentMontage() == m_changeMove.Get();
}

bool UBossShadowSummon::CanCover(const ABossShadow& _partner) const
{
    const auto* boss = Cast<ABossEnemyCharacter>(GetOwner());
    if(!boss || !boss->IsActing() || !boss->m_combatTarget || boss->m_combatTarget != _partner.m_combatTarget
        || GetWorld()->GetTimeSeconds() < m_nextSwap) { return false; }
    const auto* move = boss->GetActiveAction();
    return move && move->m_damage > 0.f && (!move->m_hitNextId.IsNone() || !move->m_hitChoices.IsEmpty())
        && _partner.GetDistanceTo(boss->m_combatTarget) <= boss->GetMeleeDistance()
        && _partner.GetDistanceTo(boss) >= swapGap;
}

void UBossShadowSummon::QueueRelay()
{
    auto* boss = Cast<ABossEnemyCharacter>(GetOwner());
    if(!boss || !boss->HasAuthority() || !m_shadow.IsValid()) { return; }
    const double now = GetWorld()->GetTimeSeconds();
    m_relayEnd = now + PrimaryComponentTick.TickInterval;
    TryStep(*boss,now);
}

//安全な影の位置と、本体が立っていた位置を一度に交換する。
bool UBossShadowSummon::TryStep(ABossEnemyCharacter& boss, double now)
{
    auto* target = boss.m_combatTarget.Get();
    if(!IsValid(target) || !boss.m_approach->HasDrawn() || !boss.IsCombatActive() || boss.IsRepositioning()
        || boss.m_traversal->IsTraversing() || boss.IsActing() || boss.m_defense->IsBusy() || !boss.GetCharacterMovement()->IsMovingOnGround())
    {
        m_blockedSince = 0.;
        return false;
    }
    const FVector from = boss.GetActorLocation();
    const float distance = FVector::Dist2D(from,target->GetActorLocation());
    auto* ai = Cast<AAIController>(boss.GetController());
    const bool chasing = ai && distance > m_drawRange && (ai->GetMoveStatus() == EPathFollowingStatus::Moving
        || boss.GetCharacterMovement()->GetCurrentAcceleration().SizeSquared2D() > 1.f
        || (boss.CanDecide() && !ai->LineOfSightTo(target)));
    if(!chasing || FVector::Dist2D(from,m_lastPlace) > stuckTravel || m_blockedSince == 0.)
    {
        m_lastPlace = from;
        m_blockedSince = chasing ? now : 0.;
    }
    const bool rescue = now < m_rescueEnd;
    const bool stuck = (rescue && !m_rescuePanic) || (chasing && now-m_blockedSince >= m_stuckTime);
    const bool panic = distance < m_drawRange && boss.GetHealth() <= boss.GetMaxHealth()*m_panicHP
        && ((rescue && m_rescuePanic) || boss.m_defense->IsTargetAttacking(target));
    const bool relay = m_relayEnd > now && !boss.GetPendingComboId().IsNone();
    if(now < m_nextSwap || (!stuck && !panic && !relay)) { return false; }

    ABossShadow* partner = nullptr;
    FVector arrival, returnSpot;
    float bestFit = -TNumericLimits<float>::Max();
    for(auto* shadow : {m_shadow.Get(),m_secondShadow.Get()})
    {
        if(!IsValid(shadow) || !shadow->IsCombatActive() || shadow->IsActing() || (relay ? !shadow->CanRelay() : shadow->IsRepositioning())
            || !shadow->GetCharacterMovement()->IsMovingOnGround()) { continue; }
        if(relay && !stuck && !panic && shadow->m_combatTarget != target) { continue; }
        FVector nextPlace, nextReturn;
        if(!Landing(boss,shadow->GetActorLocation(),shadow,nextPlace)) { continue; }
        if(!Landing(*shadow,from,&boss,nextReturn))
        {
            bool found = false;
            for(int side = 0; side < 8; ++side)
            {
                const FVector offset = FVector::ForwardVector.RotateAngleAxis(side*45.f,FVector::UpVector)*swapGap;
                if(Landing(*shadow,from+offset,&boss,nextReturn) && FVector::Dist2D(nextPlace,nextReturn)>swapGap) { found = true; break; }
            }
            if(!found) { continue; }
        }
        if(FVector::Dist2D(nextPlace,nextReturn) < swapGap) { continue; }
        const float next = FVector::Dist2D(nextPlace,target->GetActorLocation());
        if(panic && !stuck && next < distance+panicGain) { continue; }
        if(relay && !panic && !stuck && next > boss.GetMeleeDistance()) { continue; }
        auto* path = UNavigationSystemV1::FindPathToLocationSynchronously(&boss,nextPlace,target->GetActorLocation(),&boss);
        if(!path || !path->IsValid() || path->IsPartial()) { continue; }
        //危険なら遠い影を、連携なら相手に近い影を選ぶ。
        const float fit = panic ? next : -next;
        if(fit <= bestFit) { continue; }
        bestFit = fit;
        partner = shadow;
        arrival = nextPlace;
        returnSpot = nextReturn;
    }
    if(!partner)
    {
        if((!stuck && !panic) || rescue || m_secondShadow.IsValid() || !m_shadowClass || now<m_nextTime) { return false; }
        //真横が壁なら斜めや後方の床も試す。
        for(const float turn : {90.f,-90.f,135.f,-135.f,180.f,45.f,-45.f,0.f})
        {
            const FVector aim = (target->GetActorLocation()-from).GetSafeNormal2D().RotateAngleAxis(turn,FVector::UpVector);
            FVector place;
            if(!Landing(boss,from+aim*m_spawnRange,nullptr,place) || FVector::Dist2D(from,place)<swapGap) { continue; }
            if(panic && FVector::Dist2D(place,target->GetActorLocation()) < distance+panicGain) { continue; }
            auto* path = UNavigationSystemV1::FindPathToLocationSynchronously(&boss,place,target->GetActorLocation(),&boss);
            if(!path || !path->IsValid() || path->IsPartial()) { continue; }
            const FTransform pose((target->GetActorLocation()-place).GetSafeNormal2D().Rotation(),place);
            auto* shade = GetWorld()->SpawnActorDeferred<ABossShadow>(m_shadowClass,pose,&boss,nullptr,ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding);
            if(!shade) { continue; }
            shade->Setup(target,FMath::Max(m_lifeMin,1.f));
            UGameplayStatics::FinishSpawningActor(shade,pose);
            if(!IsValid(shade)) { continue; }
            if(m_shadow.IsValid()) { m_secondShadow = shade; }
            else { m_shadow = shade; }
            m_rescuePanic = panic && !stuck;
            m_rescueEnd = now + shade->m_riseTime + 2.;
            m_nextTime = now + m_cooldown;
            return true;
        }
        m_nextSwap = now + 1.;
        return false;
    }
    //両方の着地点を確保してから、同じフレーム内で入れ替える。
    const FVector old = partner->GetActorLocation();
    StopWalk(boss);
    StopWalk(*partner);
    if(!partner->SetActorLocation(returnSpot,false,nullptr,ETeleportType::TeleportPhysics)) { return false; }
    if(!boss.SetActorLocation(arrival,false,nullptr,ETeleportType::TeleportPhysics))
    {
        partner->SetActorLocation(old,false,nullptr,ETeleportType::TeleportPhysics);
        return false;
    }
    boss.SetActorRotation((target->GetActorLocation()-arrival).GetSafeNormal2D().Rotation());
    if(IsValid(partner->m_combatTarget)) { partner->SetActorRotation((partner->m_combatTarget->GetActorLocation()-returnSpot).GetSafeNormal2D().Rotation()); }
    partner->PlaySmoke();
    partner->PlaySmokeAt(arrival);
    if(m_swapMove && boss.PlayAnimMontage(m_swapMove)>0.f) { m_changeMove = m_swapMove; }
    m_nextSwap = now + FMath::Max(1.f,m_swapDelay);
    m_rescueEnd = m_relayEnd = m_blockedSince = 0.;
    const bool follow = relay && partner->m_combatTarget == target;
    if(follow) { partner->RelayStrike(m_relayAction); }
    UE_LOG(LogTemp,Verbose,TEXT("Boss shadow swap: stuck=%d panic=%d relay=%d shadowQueued=%d distance=%.0f"),stuck,panic,relay,follow,FVector::Dist2D(from,arrival));
    return true;
}






