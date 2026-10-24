#pragma once

#include "CoreMinimal.h"
#include "Enemy/Core/EnemyData.h"
#include "BossData.generated.h"

//ボス戦のフェーズ。
UENUM(BlueprintType)
enum class EBossPhase : uint8
{
	//フェーズ1。
	PhaseOne,
	//立ち上がり中。
	Transition,
	//フェーズ2。
	PhaseTwo,
	//死亡した状態。
	Dead
};

//ボスの攻撃や体力をエディターで設定する。
UCLASS(BlueprintType)
class ECHO_API UBossData : public UEnemyData
{
	GENERATED_BODY()

public:
	//フェーズ2の移動速度の倍率。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss|Phase Two", meta=(ClampMin="1", ClampMax="2"))
	float m_phaseSpeed = 1.15f;

	//体力が尽きた後にフェーズ2へ進むか。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "SecondLife"))
	bool m_bSecondLife = true;

	//フェーズ2の体力。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "PhaseTwoHealth", ClampMin = "1"))
	float m_phaseTwoHealth = 900.0f;

	//フェーズ切り替えの待ち時間。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "TransitionTime"))
	float m_transitionTime = 3.0f;

	//立ち上がり後に行動を始めるまでの時間。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "Phase Recovery", ClampMin = "0"))
	float m_phaseRecovery = 0.2f;

	//フェーズ切り替えのモンタージュ。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "TransitionMontage"))
	TObjectPtr<UAnimMontage> m_transitionMontage;

	//観察するプレイヤーの攻撃。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "ObservedAttackMontages"))
	TArray<TObjectPtr<UAnimMontage>> m_attackMontages;

	//観察するプレイヤーの回避。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "ObservedDodgeMontages"))
	TArray<TObjectPtr<UAnimMontage>> m_dodgeMontages;

	//フェーズ1の特殊防御の確率。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss | Defense", meta = (DisplayName = "PhaseOneDefenseChance", ClampMin = "0.0", ClampMax = "1.0"))
	float m_phaseOneChance = 0.2f;

	//反撃を使い始める残り体力の割合。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss | Defense", meta = (DisplayName = "Counter Health Ratio", ClampMin = "0", ClampMax = "1"))
	float m_counterHealth = .5f;

	//フェーズ2の特殊防御の確率。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss | Defense", meta = (DisplayName = "PhaseTwoDefenseChance", ClampMin = "0.0", ClampMax = "1.0"))
	float m_phaseTwoChance = 0.35f;

	//フェーズ1で使う攻撃名。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss | Defense", meta = (DisplayName = "PhaseOneActions"))
	TArray<FName> m_phaseOneActions;

	//フェーズ2で使う攻撃名。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss | Defense", meta = (DisplayName = "PhaseTwoActions"))
	TArray<FName> m_phaseTwoActions;
};
