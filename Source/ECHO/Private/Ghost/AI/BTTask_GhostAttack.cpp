//BTTask_GhostAttack.cpp

#include "Ghost/AI/BTTask_GhostAttack.h"
#include "Player/CombatComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_GhostAttack::UBTTask_GhostAttack()
{
	NodeName = TEXT("Ghost Attack");
	bNotifyTick = true;   //攻撃が終わるまで毎フレーム監視する
}

UCombatComponent* UBTTask_GhostAttack::GetCombat(UBehaviorTreeComponent& OwnerComp)
{
	const AAIController* AIC = OwnerComp.GetAIOwner();
	const APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;
	return Pawn ? Pawn->FindComponentByClass<UCombatComponent>() : nullptr;
}

//ターゲットの方向を「入力方向」として渡す（向き直り・吸い付き・踏み込みはCombatComponentが行う）
void UBTTask_GhostAttack::AimAtTarget(UBehaviorTreeComponent& OwnerComp, UCombatComponent* Combat)
{
	const APawn* Pawn = OwnerComp.GetAIOwner() ? OwnerComp.GetAIOwner()->GetPawn() : nullptr;
	const UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!Pawn || !BB) return;

	if (const AActor* Target = Cast<AActor>(BB->GetValueAsObject(TEXT("TargetEnemy"))))
	{
		Combat->SetDesiredDirection((Target->GetActorLocation() - Pawn->GetActorLocation()).GetSafeNormal2D());
	}
}

EBTNodeResult::Type UBTTask_GhostAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UCombatComponent* Combat = GetCombat(OwnerComp);
	if (!Combat) return EBTNodeResult::Failed;

	*CastInstanceNodeMemory<FBTGhostAttackMemory>(NodeMemory) = FBTGhostAttackMemory();

	AimAtTarget(OwnerComp, Combat);
	Combat->ExecuteAttack();

	//攻撃が出なかった（ComboStepsが空など）
	if (!Combat->IsPlayingAttackMontage()) return EBTNodeResult::Failed;

	return EBTNodeResult::InProgress;
}

void UBTTask_GhostAttack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	UCombatComponent* Combat = GetCombat(OwnerComp);
	if (!Combat)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FBTGhostAttackMemory* Mem = CastInstanceNodeMemory<FBTGhostAttackMemory>(NodeMemory);
	Mem->Elapsed += DeltaSeconds;

	const bool bPlaying = Combat->IsPlayingAttackMontage();

	//振る段数が残っていれば先行入力を入れ続ける（プレイヤーの連打と同じ扱い）
	if (bPlaying && Combat->GetCurrentComboIndex() + 1 < m_ComboCount)
	{
		AimAtTarget(OwnerComp, Combat);
		Combat->ExecuteAttack();
	}

	//モーションが終わったら成功。長引いたらタイムアウト
	if (!bPlaying || Mem->Elapsed >= m_MaxDuration)
	{
		Combat->SetDesiredDirection(FVector::ZeroVector);
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}