#include "Enemy/Boss/BossSpacing.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Enemy/Core/EnemyTraversal.h"
#include "Enemy/Combat/Defense.h"
#include "Combat/GAS/CombatAttributeSet.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"

//
UBossSpacing::UBossSpacing()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UBossSpacing::Queue()
{
    auto* enemy = Cast<AEnemyCharacterBase>(GetOwner());
    if (!m_enabled || m_active || !enemy || !enemy->HasAuthority() || !enemy->IsCombatActive()
        || enemy->IsActing() || !enemy->GetPendingComboId().IsNone()
        || !enemy->GetCharacterMovement()->IsMovingOnGround() || GetWorld()->GetTimeSeconds() < m_next) return;
    m_active = true;
    m_start = GetWorld()->GetTimeSeconds();
    m_repath = m_start + .3;
    m_side = FMath::RandBool() ? 1.f : -1.f;
}

void UBossSpacing::Stop()
{
    if (!m_active) return;
    m_active = false;
    m_next = GetWorld()->GetTimeSeconds() + FMath::Max(0.f, m_cooldown);
    if (auto* enemy = Cast<AEnemyCharacterBase>(GetOwner()))
    {
        if (m_moving)
        {
            if (auto* ai = Cast<AAIController>(enemy->GetController())) ai->StopMovement();
            enemy->GetCharacterMovement()->MaxWalkSpeed = m_oldSpeed;
            enemy->GetCharacterMovement()->bOrientRotationToMovement = m_oldTurn;
        }
    }
    m_moving = false;
}

void UBossSpacing::Reset()
{
    Stop();
    m_healed = 0.f;
    m_next = 0.;
}

void UBossSpacing::TickComponent(float delta, ELevelTick type, FActorComponentTickFunction* tick)
{
    Super::TickComponent(delta, type, tick);
    if (!m_active) return;
    auto* enemy = Cast<AEnemyCharacterBase>(GetOwner());
    auto* target = enemy ? enemy->m_combatTarget.Get() : nullptr;
    auto* ai = enemy ? Cast<AAIController>(enemy->GetController()) : nullptr;
    const double now = GetWorld()->GetTimeSeconds();
    if (!m_enabled || !enemy || !enemy->HasAuthority() || !IsValid(target) || !ai
        || !enemy->IsCombatActive() || enemy->IsActing() || enemy->m_defense->IsBusy()
        || enemy->m_traversal->IsTraversing() || !enemy->GetCharacterMovement()->IsMovingOnGround()
        || now - m_start >= FMath::Max(.1f, m_duration)) { Stop(); return; }
    const float distance = FVector::Dist2D(enemy->GetActorLocation(), target->GetActorLocation());
    const bool attacking = enemy->m_defense->IsTargetAttacking(target);
    if (attacking && distance < enemy->GetMeleeDistance() + 120.f) { Stop(); return; }
    if (now < m_start + .3) return;
    if (!m_moving)
    {
        m_oldSpeed = enemy->GetCharacterMovement()->MaxWalkSpeed;
        m_oldTurn = enemy->GetCharacterMovement()->bOrientRotationToMovement;
        enemy->GetCharacterMovement()->MaxWalkSpeed = FMath::Max(1.f, m_speed);
        enemy->GetCharacterMovement()->bOrientRotationToMovement = false;
        m_moving = true;
    }
    const FVector away = (enemy->GetActorLocation() - target->GetActorLocation()).GetSafeNormal2D();
    enemy->SetActorRotation(FMath::RInterpTo(enemy->GetActorRotation(), (-away).Rotation(), delta, 5.f));
    if (now >= m_repath)
    {
        m_repath = now + .5;
        auto* nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
        const float radius = FMath::Max(enemy->GetMeleeDistance() + 60.f, m_distance);
        const float angle = distance < radius * .85f ? 0.f : FMath::Clamp(m_orbitAngle, 0.f, 60.f) * m_side;
        const FVector desired = target->GetActorLocation() + away.RotateAngleAxis(angle, FVector::UpVector) * radius;
        FNavLocation goal;
        if (!nav || !nav->ProjectPointToNavigation(desired, goal, FVector(100,100,200))) { Stop(); return; }
        auto* path = UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(), enemy->GetNavAgentLocation(), goal.Location, enemy);
        if (!path || !path->IsValid() || path->IsPartial()) { Stop(); return; }
        for (int32 i=1; i<path->PathPoints.Num(); ++i)
        {
            const FVector closest = FMath::ClosestPointOnSegment(target->GetActorLocation(), path->PathPoints[i-1], path->PathPoints[i]);
            if (FVector::Dist2D(closest, target->GetActorLocation()) < FMath::Min(distance - 10.f, enemy->GetMeleeDistance())) { Stop(); return; }
        }
        if (ai->MoveToLocation(goal.Location, 30.f, false, true, true, true, nullptr, false) == EPathFollowingRequestResult::Failed) { Stop(); return; }
    }
    if (!attacking && distance >= FMath::Max(m_distance * .8f, enemy->GetMeleeDistance() + 60.f)
        && now - m_start >= FMath::Max(0.f, m_healDelay) && enemy->m_attributeSet)
    {
        const float amount = FMath::Max(0.f, FMath::Min3(FMath::Max(0.f, m_healRate) * delta,
            enemy->GetMaxHealth() * FMath::Clamp(m_healLimit, 0.f, 1.f) - m_healed, enemy->GetMaxHealth() - enemy->GetHealth()));
        enemy->m_attributeSet->SetHealth(enemy->GetHealth() + amount);
        m_healed += amount;
    }
}

void UBossSpacing::EndPlay(const EEndPlayReason::Type reason)
{
    Stop();
    Super::EndPlay(reason);
}
//
