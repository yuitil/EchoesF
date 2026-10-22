#pragma once

#include "CoreMinimal.h"
#include "Enemy/Core/EnemyData.h"
#include "BossData.generated.h"

UENUM(BlueprintType)
enum class EBossPhase : uint8
{
	PhaseOne,
	Transition,
	PhaseTwo,
	Dead
};

UCLASS(BlueprintType)
class ECHO_API UBossData : public UEnemyData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "SecondLife"))
	bool m_bSecondLife = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "PhaseTwoHealth", ClampMin = "1"))
	float m_phaseTwoHealth = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "TransitionTime"))
	float m_transitionTime = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "Phase Recovery", ClampMin = "0"))
	float m_phaseRecovery = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "TransitionMontage"))
	TObjectPtr<UAnimMontage> m_transitionMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "ObservedAttackMontages"))
	TArray<TObjectPtr<UAnimMontage>> m_attackMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "ObservedDodgeMontages"))
	TArray<TObjectPtr<UAnimMontage>> m_dodgeMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss | Defense", meta = (DisplayName = "PhaseOneDefenseChance", ClampMin = "0.0", ClampMax = "1.0"))
	float m_phaseOneChance = 0.2f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss | Defense", meta = (DisplayName = "Counter Health Ratio", ClampMin = "0", ClampMax = "1"))
	float m_counterHealth = .5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss | Defense", meta = (DisplayName = "PhaseTwoDefenseChance", ClampMin = "0.0", ClampMax = "1.0"))
	float m_phaseTwoChance = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss | Defense", meta = (DisplayName = "PhaseOneActions"))
	TArray<FName> m_phaseOneActions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss | Defense", meta = (DisplayName = "PhaseTwoActions"))
	TArray<FName> m_phaseTwoActions;
};
