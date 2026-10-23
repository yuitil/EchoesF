#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "MinionUpdateCombat.generated.h"

//
UCLASS()
class ECHO_API UMinionUpdateCombat : public UBTService
{
	GENERATED_BODY()

public:
	UMinionUpdateCombat();
protected:
	virtual void TickNode(UBehaviorTreeComponent& _ownerComp, uint8* _nodeMemory, float _delta) override;
};


//
