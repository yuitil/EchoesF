#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Enemy/Core/EnemyData.h"
#include "Defense.generated.h"

//状態の変化をBPや他の処理へ知らせる通知を用意する。
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDefenseEvent, EDefenseResult, _result, AActor*, _attacker);

//ガード、受け流し、ガード崩しを管理する。
UCLASS(ClassGroup = (Enemy), meta = (BlueprintSpawnableComponent))
class ECHO_API UDefense : public UActorComponent
{
	GENERATED_BODY()

public:
	//ガード、受け流し、ガード崩しを管理する。
	UDefense();

	//命中した攻撃を防げるか判断する。
	EDefenseResult ResolveHit(AActor* _attacker, bool _unblockable, bool _heavyAttack = false);

	//相手の再生中の動作が強攻撃か調べる。
	bool IsObservedHeavyAttack(AActor* _attacker) const;

	//防御の結果を通知する。
	void PublishResult(AActor* _attacker, EDefenseResult _result);

	//相手の攻撃に備えてガードするか調べる。
	bool ShouldStartGuard() const;

	//相手が防御できる正面の範囲にいるか調べる。
	bool IsFrontFacing(AActor* _attacker) const;

	//攻撃を防がれた時の反応を行う。
	void ReactToDefendedAttack(EDefenseResult _result);

	//抽選の値が設定した確率に入るか調べる。
	static bool PassesChance(float _roll, float _chance);

	//受けた攻撃を覚えて次の防御に備える。
	void RememberHit(AActor* _attacker);

	//攻撃後のガード予約が残っているか調べる。
	bool HasGuardRequest() const;

	//攻撃を終えた後に防御へつなぐ。
	void AttackFinished();

	//予約していたガードを使用する。
	void ConsumeGuardRequest() { m_guardPending = false; }

public:
	//相手が攻撃中か調べる。
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Enemy | Defense")
	bool IsTargetAttacking(AActor* target) const;

	//防御の反応中で次の行動を待つ必要があるか返す。
	UFUNCTION(BlueprintPure, Category = "Enemy | Defense")
	bool IsBusy() const;

	//ガードと反撃の状態を初期化する。
	UFUNCTION(BlueprintCallable, Category = "Enemy | Defense")
	void ResetDefense();

public:
	//防御の結果を伝える通知。
	UPROPERTY(BlueprintAssignable, Category = "Enemy | Defense", meta = (DisplayName = "OnDefenseResolved"))
	FDefenseEvent m_onDefense;

	//最後の防御結果。
	UPROPERTY(BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "LastResult"))
	EDefenseResult m_lastResult = EDefenseResult::None;

	//特殊防御を試した回数。
	UPROPERTY(BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "Attempts"))
	int32 m_attempts = 0;

	//特殊防御が成功した回数。
	UPROPERTY(BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "Successes"))
	int32 m_successes = 0;

	//連続でガードした強攻撃の回数。
	UPROPERTY(BlueprintReadOnly, Category = "Enemy | GuardBreak", meta = (DisplayName = "GuardedHeavyHits"))
	int32 m_guardedHeavyHits = 0;

	//ガードが崩れているか。
	UPROPERTY(BlueprintReadOnly, Category = "Enemy | GuardBreak", meta = (DisplayName = "GuardedLightHits"))
	bool m_bGuardBroken = false;

protected:
	//終了時にタイマーや登録を解除する。
	virtual void EndPlay(const EEndPlayReason::Type _reason) override;

private:
	//予約した反撃を開始できるか調べる。
	void TryCounter();

	//連続して受けた強攻撃の記録を消す。
	void ClearHeavyChain();

	//強攻撃の連続命中によるガード崩れを調べる。
	EDefenseResult ResolveGuardChain(AActor* _attacker, bool _heavyAttack);

	//ガードを崩して無防備な時間を開始する。
	void StartGuardBreak();

	//ガード崩れを終えて行動できる状態へ戻す。
	void EndGuardBreak();

	//防御後の反応アニメーションを開始する。
	void BeginReaction(UAnimMontage* _montage, float _minduration);

private:
	//攻撃終了後のガードを予約しているか。
	bool m_guardPending = false;

	//攻撃してきた相手を覚えておく時刻。
	double m_guardUntil = 0.;

	//ガード姿勢を保つ終了時刻。
	double m_guardPose = 0.;

	//反撃できる期限。
	double m_counterUntil = 0.;

	//攻撃してきた相手。
	TWeakObjectPtr<AActor> m_guardTarget;

	//反撃を開始するタイマー。
	FTimerHandle m_counterTimer;

	//次に特殊防御を試せる時刻。
	double m_nextAttemptTime = 0.0;

	//防御後の反応が終わる時刻。
	double m_busyUntilTime = 0.0;

	//最後に強攻撃を受けた時刻。
	double m_lastHeavyTime = -1.0;

	//連続強攻撃をしている相手。
	TWeakObjectPtr<AActor> m_heavyAttacker;

	//ガード崩れを解除するタイマー。
	FTimerHandle m_breakTimer;

	//連続強攻撃の記録を消すタイマー。
	FTimerHandle m_heavyTimer;
};
