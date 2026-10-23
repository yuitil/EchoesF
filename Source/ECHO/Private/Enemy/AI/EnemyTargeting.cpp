#include "Enemy/AI/EnemyTargeting.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Ghost/GhostCharacter/GhostCharacter.h"
#include "Ghost/GhostEnemy/GhostEnemyComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "AIController.h"
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
    if (enemy->IsActing() && IsCandidate(enemy->m_combatTarget)) return enemy->m_combatTarget;
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
