#pragma once

#include "CoreMinimal.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Enemy/Boss/BossData.h"
#include "BossEnemyCharacter.generated.h"

class UBossObservation;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBossLifeEvent, EBossPhase, lifePhase);

UCLASS()
class ECHO_API ABossEnemyCharacter : public AEnemyCharacterBase
{
	GENERATED_BODY()

public:
	ABossEnemyCharacter();

	virtual void ResetEncounter() override;
	virtual float GetSpecialDefenseChance() const override;
	virtual void ObserveTarget(AActor* _target) override;

public:
	UFUNCTION(BlueprintPure, Category = "Boss")
	UBossData* GetBossData() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss")
	void PhaseTransitionStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss")
	void BossDefeated();

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "Observation"))
	TObjectPtr<UBossObservation> m_observation;

	UPROPERTY(BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "Life"))
	EBossPhase m_bossPhase = EBossPhase::PhaseOne;

	UPROPERTY(BlueprintAssignable, Category = "Boss", meta = (DisplayName = "OnLifeChanged"))
	FBossLifeEvent m_onLifeChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type _reason) override;
	virtual void HandleHealthDepleted() override;
	virtual bool IsActionAllowed(const FEnemyActionData& _action) const override;
	virtual float GetActionWeightMultiplier(const FEnemyActionData& _action) const override;
	virtual void OnAcceptedHit(AActor* _attacker) override;
	virtual void OnActionEnded() override;

private:
	void SetLife(EBossPhase _newLife);
	void FinishTransition();
	FTimerHandle m_transitionTimer;
};
