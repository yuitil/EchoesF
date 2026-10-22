//BTTask_GhostDodge.cpp

#include "Ghost/AI/BTTask_GhostDodge.h"
#include "Player/CombatComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_GhostDodge::UBTTask_GhostDodge()
{
	NodeName = TEXT("Ghost Dodge");
}

EBTNodeResult::Type UBTTask_GhostDodge::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;
	UCombatComponent* Combat = Pawn ? Pawn->FindComponentByClass<UCombatComponent>() : nullptr;
	if (!Combat) return EBTNodeResult::Failed;

	FVector Dir = -Pawn->GetActorForwardVector();
	const UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (const AActor* Target = BB ? Cast<AActor>(BB->GetValueAsObject(TEXT("TargetEnemy"))) : nullptr)
	{
		const FVector ToTarget = (Target->GetActorLocation() - Pawn->GetActorLocation()).GetSafeNormal2D();
		Dir = m_bBackStep
			? -ToTarget
			: FVector::CrossProduct(ToTarget, FVector::UpVector) * (FMath::RandBool() ? 1.f : -1.f);
	}

	Combat->ExecuteDodge(Dir);
	return EBTNodeResult::Succeeded;
}

