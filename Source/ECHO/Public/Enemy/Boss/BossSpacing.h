#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BossSpacing.generated.h"

//相手の様子に合わせて退避や周回を行う。
UCLASS(ClassGroup=(Enemy), meta=(BlueprintSpawnableComponent))
class ECHO_API UBossSpacing : public UActorComponent
{
	GENERATED_BODY()

public:
	//相手の様子に合わせて退避や周回を行う。
	UBossSpacing();

	//攻撃後に間合いを取る行動を予約する。
	void Queue();

	//間合い調整を止めて移動設定を戻す。
	void Stop();

	//次の戦闘に向けて状態を初期化する。
	void Reset();

public:
	//相手が倒れて起き上がる途中か調べる。
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category="Boss|Spacing")
	bool IsTargetDown(AActor* target) const;

	//間合いを取り直しているか返す。
	UFUNCTION(BlueprintPure, Category="Boss|Spacing")
	bool IsSpacing() const { return m_active; }

	//様子見を終えて攻撃を優先しているか。
	UFUNCTION(BlueprintPure, Category="Boss|Spacing")
	bool IsPressing() const { return m_press; }

public:
	//相手のダウン中だけ周回するか。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Spacing")
	bool m_onlyDown = false;

	//相手の様子見中にも周回するか。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Spacing")
	bool m_watchIdle = false;

	//様子見と判断するまでの時間。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Spacing", meta=(ClampMin="0.2"))
	float m_idleDelay = 1.2f;

	//立ち止まっていると判断する速度。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Spacing", meta=(ClampMin="0"))
	float m_idleSpeed = 80.f;

	//様子を見る距離。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Spacing", meta=(ClampMin="100"))
	float m_watchRange = 900.f;

	//ダウン中と判断するタグ。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Spacing")
	FName m_downTag;

	//ダウン中と判断するモンタージュ。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Spacing")
	TArray<TObjectPtr<class UAnimMontage>> m_downMontages;

	//この機能を使うか。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Spacing")
	bool m_enabled = false;

	//行動を続ける時間。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Spacing", meta=(ClampMin="0.1"))
	float m_duration = 2.5f;

	//再使用までの待ち時間。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Spacing", meta=(ClampMin="0"))
	float m_cooldown = 6.f;

	//保ちたい間合い。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Spacing", meta=(ClampMin="1"))
	float m_distance = 400.f;

	//退避に使う行動名。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Spacing")
	FName m_retreatAction;

	//遠ざかった割合。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Spacing", meta=(ClampMin="0.1",ClampMax="1"))
	float m_retreatRatio = .85f;

	//移動や再生に使う速さ。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Spacing", meta=(ClampMin="1"))
	float m_speed = 200.f;

	//周回で進む角度。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Spacing", meta=(ClampMin="0", ClampMax="60"))
	float m_orbitAngle = 25.f;

	//体力を回復する速さ。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Recovery", meta=(ClampMin="0"))
	float m_healRate = 5.f;

	//一度に回復できる割合。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Recovery", meta=(ClampMin="0", ClampMax="1"))
	float m_healLimit = .1f;

	//回復を始めるまでの時間。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Recovery", meta=(ClampMin="0"))
	float m_healDelay = 1.f;

protected:
	//経過時間に合わせて敵の状態を更新する。
	virtual void TickComponent(float _delta, ELevelTick _type, FActorComponentTickFunction* _tick) override;

	//終了時にタイマーや登録を解除する。
	virtual void EndPlay(const EEndPlayReason::Type _reason) override;

private:
	//相手が静止している時間と、攻撃へ移る時期を調べる。
	void UpdateWatch(class AEnemyCharacterBase* _enemy, double _now);

	//周回を始めてもほかの行動を邪魔しないか。
	bool CanSpace(class AEnemyCharacterBase* _enemy) const;

private:
	//一度様子を見たら、攻撃するまで周回を繰り返さない。
	bool m_press = false;

	//相手が変わった時は様子見の時間を数え直す。
	TWeakObjectPtr<AActor> m_watched;

	//相手が様子見を始めた時刻。
	double m_idleSince = -1.;

	//相手の様子を見ているか。
	bool m_watching = false;

	//行動を実行中か。
	bool m_active = false;

	//移動を開始したか。
	bool m_moving = false;

	//変更前の向きの設定。
	bool m_oldTurn = false;

	//変更前の移動速度。
	float m_oldSpeed = 0.f;

	//周回する左右の向き。
	float m_side = 1.f;

	//今回回復した体力。
	float m_healed = 0.f;

	//行動を開始した時刻。
	double m_start = 0.;

	//次に間合い調整を始められる時刻。
	double m_next = 0.;

	//次に経路を更新する時刻。
	double m_repath = 0.;
};
