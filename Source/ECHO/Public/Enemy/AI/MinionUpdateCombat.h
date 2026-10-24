#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "MinionUpdateCombat.generated.h"

//雑魚敵の相手と攻撃の順番を調べる。
UCLASS()
class ECHO_API UMinionUpdateCombat : public UBTService
{
	GENERATED_BODY()

public:
	//雑魚敵の相手と攻撃の順番を調べる。
	UMinionUpdateCombat();

protected:
	//BTが判断に使う敵と相手の状態を更新する。
	virtual void TickNode(UBehaviorTreeComponent& _ownerComp, uint8* _nodeMemory, float _delta) override;
};
