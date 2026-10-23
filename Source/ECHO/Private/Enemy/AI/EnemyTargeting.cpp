#include "Enemy/AI/EnemyTargeting.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Ghost/GhostCharacter/GhostCharacter.h"
#include "Ghost/GhostEnemy/GhostEnemyComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Enemy/Minion/MinionFormation.h"
#include "Enemy/Minion/MinionCombatDirector.h"
//
bool UEnemyTargeting::IsCandidate(AActor* actor) const
{
    if (!IsValid(actor) || actor->IsHidden() || !actor->CanBeDamaged()) return false;
    if (auto* ghost = Cast<AGhostCharacter>(actor))
    {
        if (!m_targetClones || !ghost->EnemyComponent) return false;
        const auto state = ghost->EnemyComponent->GetLifecycleState();
        return state == EGhostLifecycleState::Friendly || state == EGhostLifecycleState::Warning;
    }
    return actor == UGameplayStatics::GetPlayerPawn(this,0);
}
AActor* UEnemyTargeting::SelectTarget()
{
    auto* enemy = Cast<AEnemyCharacterBase>(GetOwner());
    if (!enemy) return nullptr;
    if (const auto* formation=enemy->FindComponentByClass<UMinionFormation>(); formation && formation->IsProvoked() && IsCandidate(enemy->m_combatTarget)
        && FVector::Dist2D(enemy->GetActorLocation(),enemy->m_combatTarget->GetActorLocation()) <= m_range) return enemy->m_combatTarget;
    if (enemy->IsActing()) return IsCandidate(enemy->m_combatTarget) ? enemy->m_combatTarget.Get() : nullptr;
    //
    if (m_followTarget && IsCandidate(enemy->m_combatTarget)
        && (m_chaseRange <= 0.f || FVector::Dist2D(enemy->GetActorLocation(), enemy->m_combatTarget->GetActorLocation()) <= m_chaseRange))
        return enemy->m_combatTarget;
    //
    auto* ai = Cast<AAIController>(enemy->GetController());
    AActor* best = nullptr;
    float score = TNumericLimits<float>::Max();
    auto consider = [&](AActor* actor)
    {
        if (!IsCandidate(actor)) return;
        float distance = FVector::Dist2D(actor->GetActorLocation(),enemy->GetActorLocation());
        if (distance > FMath::Max(0.f,m_range) || (ai && !ai->LineOfSightTo(actor))) return;
        if (actor == enemy->m_combatTarget) distance *= FMath::Clamp(m_keepWeight,0.f,1.f);
        if (distance < score) { score = distance; best = actor; }
    };
    consider(UGameplayStatics::GetPlayerPawn(this,0));
    if (m_targetClones) for (TActorIterator<AGhostCharacter> it(GetWorld()); it; ++it) consider(*it);
    if (!best && IsCandidate(enemy->m_combatTarget)
        && FVector::Dist2D(enemy->GetActorLocation(), enemy->m_combatTarget->GetActorLocation()) <= m_range)
        return enemy->m_combatTarget;
    return best;
}
//

//
AActor* UEnemyTargeting::SelectMinionTarget()
{
    auto* enemy = Cast<AEnemyCharacterBase>(GetOwner());
    if (!enemy) return nullptr;
    auto* formation = enemy->FindComponentByClass<UMinionFormation>();
    if (enemy->IsActing()) return IsCandidate(enemy->m_combatTarget) ? enemy->m_combatTarget.Get() : nullptr;
    const float keepRange = FMath::Max(m_range,m_giveUpRange);
    auto keep = [&](AActor* actor) { return IsCandidate(actor) && FVector::DistSquared2D(enemy->GetActorLocation(),actor->GetActorLocation()) <= FMath::Square(keepRange); };
    if (formation && formation->IsProvoked() && keep(formation->GetCounterTarget())) return formation->GetCounterTarget();
    if (formation && formation->HasTurn() && keep(enemy->m_combatTarget)) return enemy->m_combatTarget;
    if (!m_preferClones || !m_targetClones) return SelectTarget();
    auto* ai = Cast<AAIController>(enemy->GetController());
    AActor* best = nullptr;
    float score = TNumericLimits<float>::Max();
    for (TActorIterator<AGhostCharacter> it(GetWorld()); it; ++it)
    {
        auto* target = *it;
        if (!IsCandidate(target)) continue;
        float distance = FVector::Dist2D(enemy->GetActorLocation(), target->GetActorLocation());
        if (distance > m_range || (ai && !ai->LineOfSightTo(target))) continue;
        if (!enemy->m_stationary)
        {
            auto* path = UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(), enemy->GetActorLocation(), target->GetActorLocation(), enemy);
            if (!path || !path->IsValid() || path->IsPartial()) continue;
        }
        if (target == enemy->m_combatTarget) distance *= FMath::Clamp(m_keepWeight, 0.f, 1.f);
        if (distance < score) { score = distance; best = target; }
    }
    //
    auto* player = UGameplayStatics::GetPlayerPawn(this,0);
    if (best && m_keepPlayerDuel && IsCandidate(player)
        && GetWorld()->GetSubsystem<UMinionCombatDirector>()->GetPlayerGuard(player) == enemy) return player;
    //
    return best ? best : SelectTarget();
}
//
