//BTTask_GhostAttack.h

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_GhostAttack.generated.h"

class UCombatComponent;

//タスク実行中の状態(分身ごとに別々に持つ)
struct FBTGhostAttackMemory
{
    float Elapsed = 0.f;
};

UCLASS()
class ECHO_API UBTTask_GhostAttack : public UBTTaskNode
{
	GENERATED_BODY()

public:
    UBTTask_GhostAttack();

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual uint16 GetInstanceMemorySize() const override { return sizeof(FBTGhostAttackMemory); }

protected:
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

private:
    //何段目まで振るか
    UPROPERTY(EditAnywhere, Category = "Ghost", meta = (ClampMin = "1"))
    int32 m_ComboCount = 3;

    //念のためのタイムアウト
    UPROPERTY(EditAnywhere, Category = "Ghost")
    float m_MaxDuration = 5.f;

    static UCombatComponent* GetCombat(UBehaviorTreeComponent& OwnerComp);
    static void AimAtTarget(UBehaviorTreeComponent& OwnerComp, UCombatComponent* Combat);
};
