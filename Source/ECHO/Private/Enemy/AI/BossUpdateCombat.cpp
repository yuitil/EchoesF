//
#include "Enemy/AI/BossUpdateCombat.h"
//
#include "Enemy/AI/BossAIController.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Enemy/Core/EnemyTraversal.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Enemy/Combat/Defense.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Animation/AnimInstance.h"
#include "Enemy/Animation/BossAnimation.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include "Enemy/AI/EnemyTargeting.h"

//
UBossUpdateCombat::UBossUpdateCombat()
{
	NodeName = TEXT("Boss Update Combat");
	Interval = 0.2f;
	RandomDeviation = 0.0f;
	bNotifyTick = true;
}

void UBossUpdateCombat::TickNode(UBehaviorTreeComponent& _ownerComp, uint8* _nodeMemory, float _delta)
{
	Super::TickNode(_ownerComp, _nodeMemory, _delta);

	AAIController* aiController = _ownerComp.GetAIOwner();
	AEnemyCharacterBase* enemy = aiController ? Cast<AEnemyCharacterBase>(aiController->GetPawn()) : nullptr;
	UBlackboardComponent* blackboard = _ownerComp.GetBlackboardComponent();
	if (!enemy || !blackboard)
	{
		return;
	}

	//
	AActor* target = enemy->m_targeting->SelectTarget();
	//
    //
    if (!IsValid(target) && enemy->IsActing()) enemy->CancelAction();
    //
	enemy->m_combatTarget = target;
	bool alive = enemy->IsCombatActive();

	blackboard->SetValueAsObject(TEXT("TargetActor"), target);
	const bool turn=true;
	blackboard->SetValueAsBool(TEXT("CanDecide"), turn && enemy->CanDecide());
	blackboard->SetValueAsBool(TEXT("CanMove"), turn && !enemy->m_stationary && enemy->CanDecide() && !enemy->m_traversal->IsTraversing() && !enemy->IsActing() && !enemy->m_defense->IsBusy() && enemy->GetPendingComboId().IsNone() && IsValid(target) && FVector::Dist2D(enemy->GetActorLocation(), target->GetActorLocation()) > enemy->GetMeleeDistance() + 20.f);
	blackboard->SetValueAsFloat(TEXT("Distance"), target ? FVector::Dist2D(enemy->GetActorLocation(), target->GetActorLocation()) : 99999.0f);

	if (alive && target && aiController->LineOfSightTo(target))
	{
		enemy->ObserveTarget(target);
	}
}


//
