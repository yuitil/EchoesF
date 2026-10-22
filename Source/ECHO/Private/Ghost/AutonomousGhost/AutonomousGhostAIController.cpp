//AutonomousGhostAIController.cpp

#include "Ghost/AutonomousGhost/AutonomousGhostAIController.h"
#include "Ghost/GhostCharacter/GhostCharacter.h"
#include "Ghost/GhostCharacter/GhostPlaybackComponent.h"
#include "BehaviorTree/BehaviorTree.h"

void AAutonomousGhostAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	//この分身はBTで自律行動するので、記録再生コンポーネントは止めておく
	if (AGhostCharacter* Ghost = Cast<AGhostCharacter>(InPawn))
	{
		if (Ghost->PlaybackComponent)
		{
			Ghost->PlaybackComponent->SetComponentTickEnabled(false);
		}
	}
	//BTの起動は StartFromSnapshot で行う（この時点ではまだ記録データを受け取っていないため）
}

void AAutonomousGhostAIController::StartFromSnapshot(const TArray<FGhostActionData>& Snapshot)
{
	if (UBehaviorTree* Tree = SelectTree(Snapshot))
	{
		RunBehaviorTree(Tree);
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Ghost: BehaviorTree 未設定"));
	}
}

UBehaviorTree* AAutonomousGhostAIController::SelectTree(const TArray<FGhostActionData>& Snapshot) const
{
	int32 AttackCount = 0;
	int32 DodgeCount = 0;
	for (const FGhostActionData& Action : Snapshot)
	{
		if (Action.Type == EGhostActionType::Attack) ++AttackCount;
		else if (Action.Type == EGhostActionType::Dodge) ++DodgeCount;
	}

	const int32 Total = AttackCount + DodgeCount;
	const float AttackRatio = (Total > 0) ? static_cast<float>(AttackCount) / Total : 0.5f;

	UBehaviorTree* Tree = BalancedTree;
	const TCHAR* TypeName = TEXT("Balanced");
	if (AttackRatio >= m_AttackerRatio)
	{
		Tree = AttackerTree;
		TypeName = TEXT("Attacker");
	}
	else if (AttackRatio <= m_DodgerRatio)
	{
		Tree = DodgerTree;
		TypeName = TEXT("Dodger");
	}

	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green,
		FString::Printf(TEXT("Ghost Type: %s (Attack %d / Dodge %d)"), TypeName, AttackCount, DodgeCount));

	return Tree ? Tree : BalancedTree;
}
