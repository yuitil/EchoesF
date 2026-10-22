//BTService_GhostFindTarget.h
//一定期間で一番近い敵を探してブラックボードのTargetEnemyとIsInAttackRangeを更新する

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_GhostFindTarget.generated.h"

UCLASS()
class ECHO_API UBTService_GhostFindTarget : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_GhostFindTarget();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

private:
	//敵を探す半径
	UPROPERTY(EditAnywhere, Category = "Ghost")
	float m_SearchRadius = 2000.f;

	//プレイヤーからこの距離より離れた敵は狙わない（分身がはぐれないように）
	UPROPERTY(EditAnywhere, Category = "Ghost")
	float m_LeashRadius = 1500.f;

	//この距離以内なら攻撃範囲内とみなす
	UPROPERTY(EditAnywhere, Category = "Ghost")
	float m_AttackRange = 180.f;
};
