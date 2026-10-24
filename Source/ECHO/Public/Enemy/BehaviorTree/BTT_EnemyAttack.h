#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTT_EnemyAttack.generated.h"


//BTから敵の攻撃を呼び出す。
UCLASS()
class ECHO_API UBTT_EnemyAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	//BTから敵の攻撃を呼び出す。
	UBTT_EnemyAttack();

	//BTから呼ばれた行動を開始する。
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& _ownerComp, uint8* _nodeMemory) override;
	

	//BTの画面に表示する説明を返す。
	virtual FString GetStaticDescription() const override;
};
