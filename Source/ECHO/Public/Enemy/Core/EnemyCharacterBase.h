#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Enemy/Core/EnemyData.h"
#include "Combat/EBDefenseFeedback.h"
#include "EnemyCharacterBase.generated.h"

class UAbilitySystemComponent;
class UCombatAbilitySystemComponent;
class UCombatAttributeSet;
class UCombat;
class UEnemyTraversal;
class UDefense;

class UEnemyTargeting;
class UEnemyHitEffect;
class UNiagaraSystem;

class UGameplayAbility;
class UBehaviorTree;
struct FOnAttributeChangeData;

//状態の変化をBPや他の処理へ知らせる通知を用意する。
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEnemyHealthEvent, float, health, float, maxHealth);
//状態の変化をBPや他の処理へ知らせる通知を用意する。
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEnemyStateEvent, EEnemyState, state);

//敵に共通する体力、攻撃、移動を管理する。
UCLASS()
class ECHO_API AEnemyCharacterBase : public ACharacter, public IAbilitySystemInterface, public IEBDefenseFeedback
{
	GENERATED_BODY()

public:
	//敵に共通する体力、攻撃、移動を管理する。
	AEnemyCharacterBase();

	//敵の能力を管理する部品を返す。
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	//防御を確認して体力を減らす。
	virtual float TakeDamage(float _damage, const FDamageEvent& _damageEvent, AController* _instigator, AActor* _causer) override;

	//攻撃を防がれた結果を受け取る。
	virtual void AttackDefended_Implementation(AActor* _defender, EDefenseResult _result) override;

	//受け流しなどを選ぶ確率を返す。
	virtual float GetSpecialDefenseChance() const;

	//相手の攻撃や移動を観察する。
	virtual void ObserveTarget(AActor* _target){}

	//状況に合わせて次のコンボを選ぶ。
	static FName SelectComboBranch(const FEnemyActionData& _action, bool _hit, bool _blocked);

	//今回の攻撃が命中したことを記録する。
	void RecordAttackHit();

	//今回の攻撃が防がれたか返す。
	bool WasAttackBlocked() const { return m_bActionBlocked; }

	//現在実行している攻撃の設定を返す。
	const FEnemyActionData* GetActiveAction() const;

	//設定に合わせた攻撃の再生速度を返す。
	float GetAttackRate(const FEnemyActionData& _action) const;

	//攻撃が終わるまで待つ時間を求める。
	float GetActionTimeout(const FEnemyActionData& _action) const;

	//攻撃の再生速度を通常に戻す。
	void RestoreAttackRate();

	//攻撃を終えて次の行動に備える。
	void FinishAction(bool _cancelled);

	//間合いを取り直しているか返す。
	virtual bool IsRepositioning() const { return false; }

	//今選べる攻撃を返す。
	const FEnemyActionData* GetReadyAction(FName _actionId) const;

	//突進攻撃を開始できるか調べる。
	bool CanRushAttack(float _distance) const;

public:
	//ガード崩れの演出を表示する。
	UFUNCTION(NetMulticast, Unreliable)
	void ShowGuardBreak();

	//ガード崩れの見た目をBPで再生する。
	UFUNCTION(BlueprintImplementableEvent, Category="Enemy|UI")
	void GuardBreakFX();

	//相手の防御と回避の結果を確認する。
	UFUNCTION(BlueprintNativeEvent, Category = "Enemy | Defense")
	EDefenseResult ResolveTargetDefense(AActor* target) const;

	//相手のガードが崩れたことをBPへ伝える。
	UFUNCTION(BlueprintImplementableEvent, Category="Enemy|Defense")
	void TargetGuardBroken(AActor* target);

	//ジャスト回避された攻撃を減速する。
	UFUNCTION(BlueprintCallable, Category = "Enemy | Defense")
	bool ApplyPerfectDodge(AActor* target);

	//攻撃に対して防御できる時間の幅を返す。
	UFUNCTION(BlueprintPure, Category = "Enemy | Timing")
	float GetDefenseWindow(EDefenseResult result, FName actionId = NAME_None) const;

	//防御の入力タイミングが有効か調べる。
	UFUNCTION(BlueprintPure, Category = "Enemy | Timing")
	bool IsDefenseInputValid(EDefenseResult result, float elapsed) const;

	//攻撃を減速しているか返す。
	UFUNCTION(BlueprintPure, Category = "Enemy | Timing")
	bool IsAttackSlowed() const { return m_slowMontage != nullptr; }

	//次に出すコンボの名前を返す。
	UFUNCTION(BlueprintPure, Category = "Enemy | Combo")
	FName GetPendingComboId() const { return m_nextActionId; }

	//今のコンボの段数を返す。
	UFUNCTION(BlueprintPure, Category = "Enemy | Combo")
	int32 GetComboStep() const { return m_comboSteps; }

	//敵が戦闘できる状態か返す。
	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool IsCombatActive() const { return m_state == EEnemyState::Alive; }

	//現在の体力を返す。
	UFUNCTION(BlueprintPure, Category = "Enemy")
	float GetHealth() const;

	//最大体力を返す。
	UFUNCTION(BlueprintPure, Category = "Enemy")
	float GetMaxHealth() const;

	//敵が行動を実行中か返す。
	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool IsActing() const { return m_activeIndex != INDEX_NONE; }

	//次の行動を判断できる状態か調べる。
	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool CanDecide() const;

	//相手を打ち上げられるか調べる。
	UFUNCTION(BlueprintNativeEvent, Category = "Enemy | Air")
	bool CanLiftTarget(AActor* target) const;

	//剣が届く間合いを返す。
	UFUNCTION(BlueprintPure, Category = "Enemy | Movement")
	float GetMeleeDistance() const;

	//指定した攻撃の実行を依頼する。
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	bool RequestAction(FName _actionId = NAME_None);

	//現在の攻撃を中断する。
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void CancelAction();

	//次の戦闘に向けて状態を初期化する。
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	virtual void ResetEncounter();

	//敵を倒したことをBPへ知らせる。
	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy")
	void EnemyDefeted();

public:
	//現在の行動を見分ける番号。
	int32 m_actionSerial = 0;

	//行動が成功して終わったか。
	bool m_actionSuccess = false;

public:
	//ガード崩れに使うエフェクト。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|UI")
	TObjectPtr<UNiagaraSystem> m_guardFx;

	//ガード崩れのエフェクトの大きさ。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|UI")
	FVector m_guardFxScale = FVector(.7f);

	//命中の演出を行う部品。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy|Hit FX")
	TObjectPtr<UEnemyHitEffect> m_hitFx;

	//移動しない敵として扱うか。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Movement", meta=(DisplayName="Stationary"))
	bool m_stationary = false;

	//空中コンボを使用するか。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Air", meta = (DisplayName = "Enable Air Combo"))
	bool m_airCombo = false;

	//近接攻撃を始める基本の間合い。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Movement", meta = (ClampMin = "1", DisplayName = "Melee Distance"))
	float m_meleeDist = 160.f;

	//体が重ならないための余裕。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Movement", meta = (ClampMin = "0", DisplayName = "Body Margin"))
	float m_bodyMargin = 30.f;

	//能力とタグを管理する部品。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (DisplayName = "ASC"))
	TObjectPtr<UCombatAbilitySystemComponent> m_abilitySystemComponent;

	//体力などの値を管理する部品。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (DisplayName = "AttributeSet"))
	TObjectPtr<UCombatAttributeSet> m_attributeSet;

	//攻撃の当たり判定を行う部品。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (DisplayName = "Combat"))
	TObjectPtr<UCombat> m_combat;

	//段差を越える移動を行う部品。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy")
	TObjectPtr<UEnemyTraversal> m_traversal;

	//防御と反撃を判断する部品。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (DisplayName = "Defense"))
	TObjectPtr<UDefense> m_defense;

	//狙う相手を選ぶ部品。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy")
	TObjectPtr<UEnemyTargeting> m_targeting;

	//BPから設定する敵のデータ。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (DisplayName = "Definition"))
	TObjectPtr<UEnemyData> m_definition;

	//行動判断に使うBT。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (DisplayName = "BehaviorTree"))
	TObjectPtr<UBehaviorTree> m_behaviorTree;

	//現在の状態。
	UPROPERTY(BlueprintReadOnly, Category = "Enemy", meta = (DisplayName = "State"))
	EEnemyState m_state = EEnemyState::Alive;

	//戦闘で狙っている相手。
	UPROPERTY(BlueprintReadOnly, Category = "Enemy", meta = (DisplayName = "CombatTarget"))
	TObjectPtr<AActor> m_combatTarget;

	//体力が変わった時の通知。
	UPROPERTY(BlueprintAssignable, Category = "Enemy", meta = (DisplayName = "OnHealthChanged"))
	FEnemyHealthEvent m_onHealthChanged;

	//敵の状態が変わった時の通知。
	UPROPERTY(BlueprintAssignable, Category = "Enemy", meta = (DisplayName = "OnStateChanged"))
	FEnemyStateEvent m_onStateChanged;

protected:
	//ゲーム開始時に必要な状態を準備する。
	virtual void BeginPlay() override;

	//経過時間に合わせて敵の状態を更新する。
	virtual void Tick(float _delta) override;

	//終了時にタイマーや登録を解除する。
	virtual void EndPlay(const EEndPlayReason::Type _reason) override;

	//体力がなくなった時の処理を行う。
	virtual void HandleHealthDepleted();

	//現在の状態で選べる攻撃か調べる。
	virtual bool IsActionAllowed(const FEnemyActionData& _action) const { return true; }

	//状況に合わせて攻撃の選びやすさを返す。
	virtual float GetActionWeightMultiplier(const FEnemyActionData& _action) const { return 1.0f; }

	//ダメージを受けた後の反応を行う。
	virtual void OnAcceptedHit(AActor* _attacker){}

	//行動終了後の状態を整える。
	virtual void OnActionEnded(){}

	//攻撃終了後の次の行動を準備する。
	virtual void OnAttackFinished() {}

	//体力の変化を受けて死亡などを確認する。
	void HealthChanged(const FOnAttributeChangeData& _data);

	//敵の行動状態を切り替える。
	void SetState(EEnemyState _newState);

	//体力を設定する。
	void SetHealthPool(float _max);

	//距離と状態に合う攻撃を抽選する。
	int32 ChooseAction() const;

	//回避する方向を求める。
	FVector EvadeDirection(const FEnemyActionData& _action) const;

	//障害物を避けて回避できるか調べる。
	bool CanEvade(const FEnemyActionData& _action) const;

	//攻撃を使う条件がそろっているか調べる。
	bool CanUseAction(int32 _actionIndex, float _distance = -1.0f) const;

	//次のコンボの設定を探す。
	int32 FindComboAction(FName _actionId) const;

protected:
	//変更前の移動速度。
	float m_savedSpeed = 0.f;

	//変更前の向きの設定。
	bool m_savedTurn = false;

	//現在の攻撃を始めた時刻。
	double m_actionStart = 0.;

	//次につなぐ攻撃名。
	FName m_nextActionId;

	//直前に使った攻撃名。
	FName m_lastAction;

	//減速前の再生速度。
	float m_savedRate = 1.f;

	//減速を始めた再生位置。
	float m_slowStart = 0.f;

	//減速を終える再生位置。
	float m_slowEnd = 0.f;

	//減速の対象となる区間名。
	FName m_slowSection;

	//現在のコンボの段数。
	int32 m_comboSteps = 0;

	//今回の攻撃が命中したか。
	bool m_bActionHit = false;

	//今回の攻撃が防がれたか。
	bool m_bActionBlocked = false;

	//現在使っている攻撃の番号。
	int32 m_activeIndex = INDEX_NONE;

	//次の行動を開始できる時刻。
	double m_readyTime = 0.0;

	//各攻撃を再使用できる時刻。
	TMap<FName, double> m_actionCooldowns;

	//体力変更の通知を解除するための記録。
	FDelegateHandle m_healthHandle;

protected:
	//減速しているモンタージュ。
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> m_slowMontage;

	//通常の移動速度。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy | Movement", meta = (ClampMin = "0"))
	float m_moveSpeed = 350.0f;
};
