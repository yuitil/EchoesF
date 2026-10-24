#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BossUpdateCombat.generated.h"

//ボスの相手と距離を調べ、行動の判断へ渡す。
UCLASS()
class ECHO_API UBossUpdateCombat : public UBTService
{
	GENERATED_BODY()

public:
	//ボスの相手と距離を調べ、行動の判断へ渡す。
	UBossUpdateCombat();

protected:
	//BTが判断に使う敵と相手の状態を更新する。
	virtual void TickNode(UBehaviorTreeComponent& _ownerComp, uint8* _nodeMemory, float _delta) override;
};
