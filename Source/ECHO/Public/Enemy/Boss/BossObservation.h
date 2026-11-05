
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BossObservation.generated.h"

class UBossData;
class UAnimMontage;
struct FEnemyActionData;

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

	//見えていると確認した相手の動きを、予測用に残す。
	void ReadMotion(AActor* _target);

	//相手から攻撃を受けた回数を記録する。
	void RecordHit();

	//ボスの攻撃が終わった時刻を記録する。
	void MarkBossActionEnded();

	//フェーズ1の観察結果を保存する。
	void Freeze();

	//観察した癖に合わせて攻撃の選びやすさを変える。
	float AdaptWeight(uint8 _actionKind) const;

	//今の動きの予測と、覚えた癖を合わせて選ぶ。
	float GetActionWeight(const FEnemyActionData& _action) const;

	//移動先と攻撃姿勢から、この技の使いやすさを返す。
	float PredictWeight(const FEnemyActionData& _action) const;

public:
	//次の戦闘に向けて状態を初期化する。
	UFUNCTION(BlueprintCallable, Category = "Boss")
	void Reset();

    //直近の命中と空振りを技ごとに覚える。
    void RecordMove(FName _move, bool _hit);
    float MoveWeight(FName _move) const;

	//記録をどれだけ判断に使えるか。0なら先読みを中心にする。
	UFUNCTION(BlueprintPure, Category="Boss|Prediction")
	float GetConfidence() const;

public:
    //動きから先読みするか。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Prediction")
    bool m_usePrediction = true;

    //何秒先の位置を予測するか。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Prediction", meta=(ClampMin="0", ClampMax="0.8"))
    float m_lookAhead = .35f;

    //遠すぎる位置まで予測しないための上限。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Prediction", meta=(ClampMin="0"))
    float m_maxLead = 180.f;

    //未観察の時に予測を使う強さ。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Prediction", meta=(ClampMin="0", ClampMax="2"))
    float m_newRead = 1.2f;

    //記録が集まった後も残す予測の強さ。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Prediction", meta=(ClampMin="0", ClampMax="2"))
    float m_knownRead = .6f;

    //十分な記録と考える攻撃数。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Prediction", meta=(ClampMin="1"))
    int32 m_knownAttacks = 12;

    //十分な移動記録と考える観察回数。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Prediction", meta=(ClampMin="1"))
    int32 m_knownSamples = 150;

    //見失った後、予測を使える短い時間。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Prediction", meta=(ClampMin="0", ClampMax="2"))
    float m_sightMemory = .6f;

    //こちらを向いていると判断する角度。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Prediction", meta=(ClampMin="0", ClampMax="90"))
    float m_faceAngle = 60.f;

	//フェーズ1で記録した相手の癖。
	UPROPERTY(BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "Frozen"))
	FBossPattern m_frozenPattern;

	//観察結果を保存済みか。
	UPROPERTY(BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "IsFrozen"))
	bool m_bIsFrozen = false;

	//判断に使う最低限の観察回数。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss", meta = (DisplayName = "MinimumSamples", ClampMin = "0"))
	int32 m_minimumSamples = 30;

public:
    //ステージ開始後の攻撃の癖も判断に使うか。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Learning")
    bool m_useHistory = true;

    //癖を攻撃の選択に反映する強さ。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Learning", meta=(ClampMin="0", ClampMax="2"))
    float m_adaptStrength = .6f;

    //攻撃が偏りすぎないための追加倍率の上限。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Learning", meta=(ClampMin="0", ClampMax="2"))
    float m_maxBonus = 1.f;

    //攻撃の癖を使い始める最低回数。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Learning", meta=(ClampMin="2"))
    int32 m_minAttacks = 3;

    //連続攻撃と考える間隔。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Learning", meta=(ClampMin="0.1"))
    float m_fastGap = 2.f;

private:
    //最近の結果を重くした技ごとの成績。
    TMap<FName,float> m_moveScores;

	//最後に見えた相手の位置。
	FVector m_seenPos = FVector::ZeroVector;

	//最後に見えた相手の移動速度。
	FVector m_seenSpeed = FVector::ZeroVector;

	//最後に相手が見えた時刻。
	double m_seenTime = -100.;

	//相手が攻撃中だったか。
	bool m_seenAttack = false;

	//相手がボスへ向いていたか。
	bool m_seenFacing = false;

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
