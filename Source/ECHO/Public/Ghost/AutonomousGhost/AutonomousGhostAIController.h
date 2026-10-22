//AutonomousGhostAIController.h

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Ghost/Data/GhostTypes.h"
#include "AutonomousGhostAIController.generated.h"

class UBehaviorTree;

UCLASS()
class ECHO_API AAutonomousGhostAIController : public AAIController
{
	GENERATED_BODY()

public:
	//召喚時に呼ばれる：プレイヤーの行動記録からタイプを決めてBTを起動する
	void StartFromSnapshot(const TArray<FGhostActionData>& Snapshot);

protected:
	virtual void OnPossess(APawn* InPawn) override;

	UPROPERTY(EditDefaultsOnly, Category = "Ghost|AI")
	UBehaviorTree* AttackerTree;

	UPROPERTY(EditDefaultsOnly, Category = "Ghost|AI")
	UBehaviorTree* BalancedTree;

	UPROPERTY(EditDefaultsOnly, Category = "Ghost|AI")
	UBehaviorTree* DodgerTree;

	//攻撃の割合がこれ以上ならAttacker
	UPROPERTY(EditDefaultsOnly, Category = "Ghost|AI", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float m_AttackerRatio = 0.7f;

	//攻撃の割合がこれ以下ならDodger
	UPROPERTY(EditDefaultsOnly, Category = "Ghost|AI", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float m_DodgerRatio = 0.3f;

private:
	UBehaviorTree* SelectTree(const TArray<FGhostActionData>& Snapshot) const;
};
