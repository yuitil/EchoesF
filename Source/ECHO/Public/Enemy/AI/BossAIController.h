#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BTService.h"
#include "BossAIController.generated.h"

//敵を操作し、設定した行動の順番を動かす。
UCLASS()
class ECHO_API ABossAIController : public AAIController
{
	GENERATED_BODY()

protected:
	//操作する敵のAIを準備する。
	virtual void OnPossess(APawn* _pawn) override;
};
