
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BossObservation.generated.h"

class UBossData;
class UAnimMontage;

//観察したプレイヤーの行動の記録。
USTRUCT(BlueprintType)
struct ECHO_API FBossPattern
{
	GENERATED_BODY()

	//行動を観察した回数。
	UPROPERTY(BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "Samples"))
	int32 m_samples = 0;

	//観察した攻撃の回数。
	UPROPERTY(BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "Attacks"))
	int32 m_attacks = 0;

	//観察した回避の回数。
	UPROPERTY(BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "Dodges"))
	int32 m_dodges = 0;

	//ボスに攻撃が当たった回数。
	UPROPERTY(BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "HitsOnBoss"))
	int32 m_hitsOnBoss = 0;

	//近距離にいた割合。
	UPROPERTY(BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "CloseRatio"))
	float m_closeRatio = 0.0f;

	//遠ざかった割合。
	UPROPERTY(BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "RetreatRatio"))
	float m_retreatRatio = 0.0f;

	//左へ動いた割合。
	UPROPERTY(BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "LeftRatio"))
	float m_leftRatio = 0.0f;

	//右へ動いた割合。
	UPROPERTY(BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "RightRatio"))
	float m_rightRatio = 0.0f;

	//攻撃間隔の平均。
	UPROPERTY(BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "MeanAttackInterval"))
	float m_attackInterval = 0.0f;

	//ボスの攻撃後を狙った割合。
	UPROPERTY(BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "AttackAfterBossActionRatio"))
	float m_punishRatio = 0.0f;
};

//相手の攻撃や回避を覚え、次の判断に使う。
UCLASS( ClassGroup=(Enemy), meta=(BlueprintSpawnableComponent) )
class ECHO_API UBossObservation : public UActorComponent
{
	GENERATED_BODY()

public:
	//相手の攻撃や回避を覚え、次の判断に使う。
	UBossObservation();

	//相手の行動を記録する。
	void Observe(AActor* _target, const UBossData* _data);

	//相手から攻撃を受けた回数を記録する。
	void RecordHit();

	//ボスの攻撃が終わった時刻を記録する。
	void MarkBossActionEnded();

	//フェーズ1の観察結果を保存する。
	void Freeze();

	//観察した癖に合わせて攻撃の選びやすさを変える。
	float AdaptWeight(uint8 _actionKind) const;

public:
	//次の戦闘に向けて状態を初期化する。
	UFUNCTION(BlueprintCallable, Category = "Boss")
	void Reset();

public:
	//フェーズ1で記録した相手の癖。
	UPROPERTY(BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "Frozen"))
	FBossPattern m_frozenPattern;

	//観察結果を保存済みか。
	UPROPERTY(BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "IsFrozen"))
	bool m_bIsFrozen = false;

	//判断に使う最低限の観察回数。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss", meta = (DisplayName = "MinimumSamples", ClampMin = "0"))
	int32 m_minimumSamples = 30;

private:
	//現在記録中の相手の癖。
	FBossPattern m_pattern;

	//前回観察した相手。
	TWeakObjectPtr<AActor> m_lastTarget;

	//前回観察したモンタージュ。
	TWeakObjectPtr<UAnimMontage> m_lastMontage;

	//前回のアニメーション再生位置。
	float m_lastAnimPos = -1.0f;

	//前回の攻撃時刻。
	double m_lastAttack = -1.0;

	//ボスの攻撃が終わった時刻。
	double m_lastBossEnd = -10.0;

	//近距離にいた回数。
	int32 m_closeCount = 0;

	//遠ざかった回数。
	int32 m_retreatCount = 0;

	//左へ動いた回数。
	int32 m_leftCount = 0;

	//右へ動いた回数。
	int32 m_rightCount = 0;

	//攻撃後の隙を狙われた回数。
	int32 m_punishCount = 0;

	//攻撃間隔を測った回数。
	int32 m_intervalCount = 0;

	//攻撃間隔の合計。
	double m_intervalSum = 0.0;
};
