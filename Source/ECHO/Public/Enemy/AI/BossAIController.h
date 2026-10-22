#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BTService.h"
#include "BossAIController.generated.h"


UCLASS()
class ECHO_API ABossAIController : public AAIController
{
	GENERATED_BODY()
	
protected:
	virtual void OnPossess(APawn* _pawn) override;
};

UCLASS()
class ECHO_API UBossUpdateCombat : public UBTService
{
	GENERATED_BODY()

public:
	UBossUpdateCombat();
protected:
	virtual void TickNode(UBehaviorTreeComponent& _ownerComp, uint8* _nodeMemory, float _delta) override;
};

UCLASS()
class ECHO_API UBossPeformAction : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBossPeformAction();
protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& _ownerComp, uint8* _nodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& _ownerComp, uint8* _nodeMemory, float _delta) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& _ownerComp, uint8* _nodeMemory) override;
public:
	UPROPERTY(EditAnywhere, Category = "Rush", meta = (DisplayName = "Rush Action"))
	FName m_rushAction;

	UPROPERTY(EditAnywhere, Category = "Combat", meta = (DisplayName = "Retreat Action"))
	FName m_retreatAction;
	
	UPROPERTY(EditAnywhere, Category = "Rush", meta = (DisplayName = "Move Section"))
	FName m_moveSection;
	
	UPROPERTY(EditAnywhere, Category = "Rush", meta = (ClampMin = "0", DisplayName = "Rush Range"))
	float m_rushRange = 800.f;
	
	UPROPERTY(EditAnywhere, Category = "Rush", meta = (ClampMin = "1", DisplayName = "Rush Speed"))
	float m_rushSpeed = 1400.f;

	UPROPERTY(EditAnywhere, Category = "Rush", meta = (ClampMin = "0", DisplayName = "Start Speed"))
	float m_startSpeed = 400.f;
	
	UPROPERTY(EditAnywhere, Category = "Rush", meta = (ClampMin = "1", DisplayName = "Attack Distance"))
	float m_attackDist = 140.f;
	UPROPERTY(EditAnywhere, Category = "Rush", meta = (ClampMin = "0", DisplayName = "Height Limit"))
	float m_heightLimit = 80.f;
	
	UPROPERTY(EditAnywhere, Category = "Rush", meta = (ClampMin = "0", DisplayName = "Rush Min Distance"))
	float m_rushMin = 280.f;
	
	UPROPERTY(EditAnywhere, Category = "Rush", meta = (ClampMin = "0", DisplayName = "Chase Delay"))
	float m_chaseDelay = .25f;
	
	UPROPERTY(EditAnywhere, Category = "Rush", meta = (ClampMin = "0", DisplayName = "Minimum Time Gain"))
	float m_minGain = .15f;
	
	UPROPERTY(EditAnywhere, Category = "Rush", meta = (ClampMin = "1", ClampMax = "90", DisplayName = "Rush Angle"))
	float m_rushAngle = 60.f;
	
private:
	bool StartAttack(UBehaviorTreeComponent& _ownerComp);
	bool TryDash(UBehaviorTreeComponent& _ownerComp);
	bool CanCatch(class AEnemyCharacterBase* _enemy, float& _hitEnd, float& _moveStart) const;
	void StopRush(UBehaviorTreeComponent& _ownerComp);
	bool m_rushing = false;
	bool m_fast = false;
	float m_oldSpeed = 0.f;
	float m_oldAccel = 0.f;
	int32 m_serial = INDEX_NONE;
	double m_deadline = 0.;
	double m_hitEnd = 0.;
	double m_moveStart = 0.;
	double m_nextCheck = 0.;
	double m_chaseStart = 0.;

};
