//BTService_GhostFindTarget.cpp

#include "Ghost/AI/BTService_GhostFindTarget.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Enemy/EnemyChara.h"
#include "Engine/OverlapResult.h"
#include "Kismet/GameplayStatics.h"

UBTService_GhostFindTarget::UBTService_GhostFindTarget()
{
	NodeName = TEXT("Ghost Find Target");
	Interval = 0.2f;         //0.2秒ごとに更新
	RandomDeviation = 0.05f; //分身同士で処理タイミングを少しずらす
}

void UBTService_GhostFindTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIC = OwnerComp.GetAIOwner();
	APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!Pawn || !BB) return;

	const FVector MyLoc = Pawn->GetActorLocation();

	//ついていく相手（プレイヤー）
	APawn* Leader = UGameplayStatics::GetPlayerPawn(Pawn, 0);
	BB->SetValueAsObject(TEXT("FollowTarget"), Leader);

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Pawn);
	Pawn->GetWorld()->OverlapMultiByObjectType(Overlaps, MyLoc, FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(m_SearchRadius), Params);

	//生きている敵の中で一番近いものを選ぶ
	AEnemyChara* Nearest = nullptr;
	float NearestDistSq = TNumericLimits<float>::Max();
	for (const FOverlapResult& Result : Overlaps)
	{
		AEnemyChara* Enemy = Cast<AEnemyChara>(Result.GetActor());
		if (!Enemy || Enemy->IsHidden() || Enemy->GetCurrentState() == FEEnemyState::Dead) continue;

		//プレイヤーから離れすぎた敵は無視する
		if (Leader && FVector::DistSquared2D(Leader->GetActorLocation(), Enemy->GetActorLocation())
		> FMath::Square(m_LeashRadius))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared2D(MyLoc, Enemy->GetActorLocation());
		if (DistSq < NearestDistSq)
		{
			NearestDistSq = DistSq;
			Nearest = Enemy;
		}
	}

	BB->SetValueAsObject(TEXT("TargetEnemy"), Nearest);
	BB->SetValueAsBool(TEXT("IsInAttackRange"), Nearest && NearestDistSq <= FMath::Square(m_AttackRange));
}

