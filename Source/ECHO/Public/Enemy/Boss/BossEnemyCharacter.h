#pragma once

#include "CoreMinimal.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Enemy/Boss/BossData.h"
#include "BossEnemyCharacter.generated.h"

class UBossObservation;
class UBossApproach;

class UBossSpacing;
class UBossPosture;
class UEnemyAfterimageTrail;
class UBossShadowSummon;

//状態の変化をBPや他の処理へ知らせる通知を用意する。
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBossLifeEvent, EBossPhase, lifePhase);

//ボスの体力と二つの段階を管理する。
UCLASS()
class ECHO_API ABossEnemyCharacter : public AEnemyCharacterBase
{
	GENERATED_BODY()

public:
	//ボスの体力と二つの段階を管理する。
	ABossEnemyCharacter();

	//間合いを取り直しているか返す。
	virtual bool IsRepositioning() const override;

	//被弾した分を体勢ゲージにも伝える。
	virtual float TakeDamage(float _damage, const FDamageEvent& _event, AController* _instigator, AActor* _causer) override;

	//次の戦闘に向けて状態を初期化する。
	virtual void ResetEncounter() override;

	//受け流しなどを選ぶ確率を返す。
	virtual float GetSpecialDefenseChance() const override;

	//相手の攻撃や移動を観察する。
	virtual void ObserveTarget(AActor* _target) override;

public:
	//フェーズに合わせた見た目をBPで切り替える。
	UFUNCTION(BlueprintImplementableEvent, Category="Boss|Look")
	void PhaseLook(bool _evolved);

	//ボス専用の設定を返す。
	UFUNCTION(BlueprintPure, Category = "Boss")
	UBossData* GetBossData() const;

	//フェーズ切り替え開始をBPへ知らせる。
	UFUNCTION(BlueprintImplementableEvent, Category = "Boss")
	void PhaseTransitionStarted();

	//敵を倒したことをBPへ知らせる。
	UFUNCTION(BlueprintImplementableEvent, Category = "Boss")
	void BossDefeated();

public:
	//抜刀と接近を管理する部品。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Boss")
	TObjectPtr<UBossApproach> m_approach;

	//フェーズ2でプレイヤーの分身に対抗する影を呼ぶ。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Boss")
	TObjectPtr<UBossShadowSummon> m_summon;

	//HPと別に体勢を管理する部品。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Boss")
	TObjectPtr<UBossPosture> m_posture;

	//通知の区間でメッシュ形の残像を出す部品。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Boss")
	TObjectPtr<UEnemyAfterimageTrail> m_afterimage;

	//間合いの調整を行う部品。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Boss")
	TObjectPtr<UBossSpacing> m_spacing;

	//相手の行動を覚える部品。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "Observation"))
	TObjectPtr<UBossObservation> m_observation;

	//現在のボスのフェーズ。
	UPROPERTY(BlueprintReadOnly, Category = "Boss", meta = (DisplayName = "Life"))
	EBossPhase m_bossPhase = EBossPhase::PhaseOne;

	//フェーズが変わった時の通知。
	UPROPERTY(BlueprintAssignable, Category = "Boss", meta = (DisplayName = "OnLifeChanged"))
	FBossLifeEvent m_onLifeChanged;

protected:
	//ゲーム開始時に必要な状態を準備する。
	virtual void BeginPlay() override;

	//終了時にタイマーや登録を解除する。
	virtual void EndPlay(const EEndPlayReason::Type _reason) override;

	//体力がなくなった時の処理を行う。
	virtual void HandleHealthDepleted() override;

	//現在の状態で選べる攻撃か調べる。
	virtual bool IsActionAllowed(const FEnemyActionData& _action) const override;

	//状況に合わせて攻撃の選びやすさを返す。
	virtual float GetActionWeightMultiplier(const FEnemyActionData& _action) const override;

	//ダメージを受けた後の反応を行う。
	virtual void OnAcceptedHit(AActor* _attacker) override;

	//行動終了後の状態を整える。
	virtual void OnActionEnded() override;

	//攻撃終了後の次の行動を準備する。
	virtual void OnAttackFinished() override;

private:
	//ボスのフェーズを切り替える。
	void SetLife(EBossPhase _newLife);

	//立ち上がりを終えてフェーズ2を開始する。
	void FinishTransition();

private:
	//フェーズ切り替えを終えるタイマー。
	FTimerHandle m_transitionTimer;
};
