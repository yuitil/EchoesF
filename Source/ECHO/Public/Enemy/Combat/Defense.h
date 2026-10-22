#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Enemy/Core/EnemyData.h"
#include "Defense.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDefenseEvent, EDefenseResult, _result, AActor*, _attacker);

UCLASS(ClassGroup = (Enemy), meta = (BlueprintSpawnableComponent))
class ECHO_API UDefense : public UActorComponent
{
	GENERATED_BODY()

public:
	UDefense();

	EDefenseResult ResolveHit(AActor* _attacker, bool _unblockable, bool _heavyAttack = false);
	bool IsObservedHeavyAttack(AActor* _attacker) const;
	void PublishResult(AActor* _attacker, EDefenseResult _result);
	bool ShouldStartGuard() const;
	bool IsFrontFacing(AActor* _attacker) const;
	void ReactToDefendedAttack(EDefenseResult _result);
	static bool PassesChance(float _roll, float _chance);

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Enemy | Defense")
	bool IsTargetAttacking(AActor* target) const;

	UFUNCTION(BlueprintPure, Category = "Enemy | Defense")
	bool IsBusy() const;

	UFUNCTION(BlueprintCallable, Category = "Enemy | Defense")
	void ResetDefense();

	void RememberHit(AActor* _attacker);
	bool HasGuardRequest() const;
	void AttackFinished();
	void ConsumeGuardRequest() { m_guardPending = false; }


public:
	UPROPERTY(BlueprintAssignable, Category = "Enemy | Defense", meta = (DisplayName = "OnDefenseResolved"))
	FDefenseEvent m_onDefense;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "LastResult"))
	EDefenseResult m_lastResult = EDefenseResult::None;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "Attempts"))
	int32 m_attempts = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "Successes"))
	int32 m_successes = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy | GuardBreak", meta = (DisplayName = "GuardedHeavyHits"))
	int32 m_guardedHeavyHits = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy | GuardBreak", meta = (DisplayName = "GuardedLightHits"))
	bool m_bGuardBroken = false;

protected:
	virtual void EndPlay(const EEndPlayReason::Type _reason) override;

private:
	void TryCounter();
	bool m_guardPending = false;
	double m_guardUntil = 0.;
	double m_guardPose = 0.;
	double m_counterUntil = 0.;
	TWeakObjectPtr<AActor> m_guardTarget;
	FTimerHandle m_counterTimer;

	void ClearHeavyChain();
	EDefenseResult ResolveGuardChain(AActor* _attacker, bool _heavyAttack);
	void StartGuardBreak();
	void EndGuardBreak();
	void BeginReaction(UAnimMontage* _montage, float _minduration);

private:
	double m_nextAttemptTime = 0.0;
	double m_busyUntilTime = 0.0;
	double m_lastHeavyTime = -1.0;

	TWeakObjectPtr<AActor> m_heavyAttacker;
	FTimerHandle m_breakTimer;
	FTimerHandle m_heavyTimer;
};
