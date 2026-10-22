//BTTask_GhostDodge.h

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_GhostDodge.generated.h"

UCLASS()
class ECHO_API UBTTask_GhostDodge : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_GhostDodge();
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:
	//true:敵から離れる方向（バックステップ） / false:敵の左右どちらか
	UPROPERTY(EditAnywhere, Category = "Ghost")
	bool m_bBackStep = true;
};
