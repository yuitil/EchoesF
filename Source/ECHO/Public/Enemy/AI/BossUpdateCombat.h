#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BossUpdateCombat.generated.h"

//
UCLASS()
class ECHO_API UBossUpdateCombat : public UBTService
{
	GENERATED_BODY()

public:
	UBossUpdateCombat();
protected:
	virtual void TickNode(UBehaviorTreeComponent& _ownerComp, uint8* _nodeMemory, float _delta) override;
};


//
