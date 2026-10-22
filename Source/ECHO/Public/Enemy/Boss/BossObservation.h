
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BossObservation.generated.h"

class UBossData;
class UAnimMontage;

USTRUCT(BlueprintType)
struct ECHO_API FBossPattern
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "Samples"))
	int32 m_samples = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "Attacks"))
	int32 m_attacks = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "Dodges"))
	int32 m_dodges = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "HitsOnBoss"))
	int32 m_hitsOnBoss = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "CloseRatio"))
	float m_closeRatio = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "RetreatRatio"))
	float m_retreatRatio = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "LeftRatio"))
	float m_leftRatio = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "RightRatio"))
	float m_rightRatio = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "MeanAttackInterval"))
	float m_attackInterval = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "AttackAfterBossActionRatio"))
	float m_punishRatio = 0.0f;
};

UCLASS( ClassGroup=(Enemy), meta=(BlueprintSpawnableComponent) )
class ECHO_API UBossObservation : public UActorComponent
{
	GENERATED_BODY()

public:	
	UBossObservation();

	void Observe(AActor* _target, const UBossData* _data);
	void RecordHit();
	void MarkBossActionEnded();
	void Freeze();
	float AdaptWeight(uint8 _actionKind) const;

public:
	UFUNCTION(BlueprintCallable, Category = "Boss")
	void Reset();

public:
	UPROPERTY(BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "Frozen"))
	FBossPattern m_frozenPattern;

	UPROPERTY(BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "IsFrozen"))
	bool m_bIsFrozen = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss", meta = (DisplayName = "MinimumSamples", ClampMin = "0"))
	int32 m_minimumSamples = 30;

private:
	FBossPattern m_pattern;
	TWeakObjectPtr<AActor> m_lastTarget;
	TWeakObjectPtr<UAnimMontage> m_lastMontage;
	float m_lastAnimPos = -1.0f;
	double m_lastAttack = -1.0;
	double m_lastBossEnd = -10.0;
	int32 m_closeCount = 0;
	int32 m_retreatCount = 0;
	int32 m_leftCount = 0;
	int32 m_rightCount = 0;
	int32 m_punishCount = 0;
	int32 m_intervalCount = 0;
	double m_intervalSum = 0.0;
};
