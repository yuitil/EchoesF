// ActionCharacter.h
// プレイヤーのコアクラス
// 入力・カメラ制御・各コンポーネントへの委譲

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Player/LockOnComponent.h"
#include "Ghost/GhostCharacter/GhostCharacter.h"
#include "Player/GhostRecorderComponent.h"
#include "ActionCharacter.generated.h"

class UCombatComponent;
class USpringArmComponent;
class UCameraComponent;
class UGhostManagerComponent;
class UBatManagerComponent;
class USkillComponent;

//エネルギーが変わったとき（現在値, 最大値）
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnEnergyChanged, float, float);
//分身の数・状態が変わったとき
DECLARE_MULTICAST_DELEGATE(FOnGhostsChanged);
//UI用（現在HP, 最大HP）
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnHPChanged, float, float);
//UI用（残り回数, 最大回数）
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnHealCountChanged, int32, int32);

UCLASS()
class ECHO_API AActionCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AActionCharacter(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;
	//オートダッシュの計測に使う
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	//ジャンプが成功したときに呼ばれる関数
	virtual void OnJumped_Implementation() override;
	//地面などに着地したときに呼ばれる関数
	virtual void Landed(const FHitResult& Hit) override;

	// -----------------------------------------------------------------------
	// 入力
	// -----------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* AttackAction;
	void Attack();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* MoveAction;
	void Move(const FInputActionValue& Value);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* LookAction;
	void Look(const FInputActionValue& Value);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* LockOnAction;
	void LockOn();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* DodgeAction;
	void Dodge();

	//召喚アクション
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* SummonAction;

	//ゲージ技：R1を押している間だけ有効になる入力
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Skill")
	class UInputMappingContext* SkillMappingContext;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Skill")
	class UInputAction* SkillModifierAction;   //R1
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Skill")
	class UInputAction* Skill1Action;          //一閃
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Skill")
	class UInputAction* Skill2Action;          //テンペスト
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Skill")
	class UInputAction* Skill3Action;          //最終奥義（ため）

	//回復アクション
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* HealAction;
	void Heal();
	
	
	// -----------------------------------------------------------------------
	// 移動
	// -----------------------------------------------------------------------

	//オートダッシュへの移行時間
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float m_TimeToSprint;

	//走り続けている時間を計測する変数
	float m_CurrentRunTime;
	//走り開始と終了
	void StartSprint();
	void StopSprint();

	//スティックを離したと時に保存した方向をクリアする関数
	void MoveCompleted(const FInputActionValue& Value);

	// -----------------------------------------------------------------------
	// カメラ
	// -----------------------------------------------------------------------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* FollowCamera;

	// -----------------------------------------------------------------------
	// 戦闘
	// -----------------------------------------------------------------------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	class UCombatComponent* CombatComponent;

	//手に持たせる剣のメッシュ
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	UStaticMeshComponent* WeaponMesh;

public:
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	float GetCurrentHP() const { return m_CurrentHP; }
	float GetMaxHP()     const { return m_MaxHP; }
	bool  IsDead()       const { return m_bIsDead; }

	FOnHPChanged OnHPChanged;

	//デバッグ用：コンソールで「DebugDamage 30」と打つとダメージを受ける
	UFUNCTION(Exec)
	void DebugDamage(float Amount = 10.f);

	//佐々木変更点------------------------------------------------------------------------------------------------------
	//Hp加算メソッド
	void AddHp(float  _hpAmount);
	
	//リスポーン時に呼ばれるメソッド（自由に書き換えてくれ）
	void OnRespawn();
	//----------------------------------------------------------------------------------------------------------------
private:
	UPROPERTY(EditAnywhere, Category = "Health", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	float m_MaxHP = 100.f;

	UPROPERTY(VisibleInstanceOnly, Category = "Health", meta = (AllowPrivateAccess = "true"))
	float m_CurrentHP = 0.f;

	//被弾後の無敵時間（多段ヒット防止）
	UPROPERTY(EditAnywhere, Category = "Health", meta = (AllowPrivateAccess = "true"))
	float m_HitInvincibleTime = 0.5f;

	//回避中は無敵にするか
	UPROPERTY(EditAnywhere, Category = "Health", meta = (AllowPrivateAccess = "true"))
	bool m_bDodgeInvincible = true;

	UPROPERTY(EditAnywhere, Category = "Health|Debug", meta = (AllowPrivateAccess = "true"))
	bool m_bDebugHP = true;

	float m_LastDamageTime = -999.f;
	bool  m_bIsDead = false;

	void SetHP(float NewHP);
	void Die();

public:
	UFUNCTION(BlueprintCallable, Category = "Movement")
	class UActionMovementComponent* GetActionMovementComponent() const;

	//戦闘中かどうか（ダッシュの制限などに使う）
	bool IsInCombat() const { return m_bInCombat; }

	// スキル関連

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill")
	USkillComponent* SkillComponent;

	//ゲージ（ストック）を消費する。足りなければ false
	bool TryConsumeEnergyStock(int32 Count);

	//デバッグ用：コンソールで「DebugEnergy 500」と打つとエネルギーが増える
	UFUNCTION(Exec)
	void DebugEnergy(float Amount = 500.f);

private:
	//この範囲内に「誰かを狙っている敵」がいたら戦闘中
	UPROPERTY(EditAnywhere, Category = "Movement|Combat")
	float m_CombatCheckRadius = 1500.f;

	//最後に攻撃を当ててから、この秒数は戦闘中とみなす
	UPROPERTY(EditAnywhere, Category = "Movement|Combat")
	float m_CombatLingerTime = 4.f;

	bool  m_bInCombat = false;
	float m_LastCombatTime = -100.f;
	float m_CombatCheckTimer = 0.f;

	void UpdateCombatState(float DeltaTime);

	// -----------------------------------------------------------------------
	// ロックオン処理
	// -----------------------------------------------------------------------
public:
	//ロックオンコンポーネント
	UPROPERTY(VisibleAnywhere)
	ULockOnComponent* LockOnComponent;

	//ロックオン状態を切り替える関数
	void OnLockOnChanged(AActor* NewTarget);

	//ロックオン中のカメラ補間速度
	UPROPERTY(EditAnywhere, Category = "LockOn")
	float m_LockOnCameraInterpSpeed;

	//ロックオン中のカメラの距離オフセット
	UPROPERTY(EditAnywhere, Category = "LockOn")
	float m_LockOnCameraOffsetY;

	//ロックオン中カメラPitchの下限
	UPROPERTY(EditAnywhere, Category = "LockOn")
	float m_LockOnPitchMin;

	//ロックオン中カメラPitchの上限
	UPROPERTY(EditAnywhere, Category = "LockOn")
	float m_LockOnPitchMax;

	//左右オフセットが最大になる内積の絶対値、小さいほど切り替えが機敏
	UPROPERTY(EditAnywhere, Category = "LockOn")
	float m_SideBlendRange = 0.5f;

	//注視店をどれだけ敵側によせるか(0=プレイヤー 1=敵)
	UPROPERTY(EditAnywhere, Category = "LockOn")
	float m_TargetFocusBias = 0.5f;

	//至近距離での追従速度
	UPROPERTY(EditAnywhere, Category = "LockOn")
	float m_LockOnInterpSpeedNear = 12.f;

	FVector DefaultSocketOffset;

	//ロックオン中のかどうかのフラグ
	bool m_bsLockedOn;

	//ロックオン中のカメラの更新
	void UpdateLockOnCamera(float DeltaTime);

	// -----------------------------------------------------------------------
	// 分身システム
	// -----------------------------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "Ghost")
	void SummonGhost();

	UFUNCTION()
	void OnGhostDestroyed(AActor* DestroyedActor);

	//エネルギー（UI 表示用に public）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost")
	float m_CurrentEnergy;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost")
	float m_MaxEnergy;

	//現在召喚中の分身数（UI 表示用）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost")
	int32 m_iActiveGhostCount;

	//分身の最大召喚数
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost")
	int32 m_iMaxGhostCount;

private:
	//敵に攻撃がヒットした際に通知を受け取り、エネルギーをチャージする
	void OnHitEnemy(float EnergyGain);

	//プレイヤー行動を記録するコンポーネント
	UPROPERTY(VisibleAnywhere, Category = "Ghost")
	UGhostRecorderComponent* GhostRecorder;

	//分身の召喚・管理コンポーネント（TrySummon 経由で利用可能）
	UPROPERTY(VisibleAnywhere, Category = "Ghost")
	UGhostManagerComponent* GhostManager;

	//召喚コスト
	UPROPERTY(EditAnywhere, Category = "Ghost")
	float m_GhostSummonCost;

	//ヒット時のエネルギー増加量
	UPROPERTY(EditAnywhere, Category = "Ghost")
	float m_EnergyGainPerHit;

	// スポーンする GhostCharacter クラス（BP で BP_GhostCharacter を設定）
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = "true"), Category = "Ghost")
	TSubclassOf<AGhostCharacter> GhostCharacterClass;

	//分身の攻撃で得られるエネルギーの倍率（1.0でプレイヤーと同じ）
	UPROPERTY(EditAnywhere, Category = "Ghost")
	float m_GhostEnergyRate = 1.f;

	// private の関数に追加
		//分身が敵に攻撃を当てたときの通知を受け取る
	void OnGhostHitEnemy(float EnergyGain);

	// -----------------------------------------------------------------------
	// 回復
	// -----------------------------------------------------------------------
public:
	int32 GetHealCount()    const { return m_iHealCount; }
	int32 GetMaxHealCount() const { return m_iMaxHealCount; }

	//回数を全回復（チェックポイントなどで使う予定）
	void RefillHeals();

	FOnHealCountChanged OnHealCountChanged;

	bool IsHitStunned() const { return m_bIsHitStunned; }

private:
	UPROPERTY(EditAnywhere, Category = "Health|Heal", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	int32 m_iMaxHealCount = 5;

	UPROPERTY(EditAnywhere, Category = "Health|Heal", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float m_HealAmount = 40.f;

	int32 m_iHealCount = 0;

	enum class EHealPhase : uint8 { None, Windup, Recovery };
	EHealPhase m_HealPhase = EHealPhase::None;
	FTimerHandle m_HealTimer;

	//ボタンを押してから回復するまで（この間に被弾すると失敗）
	UPROPERTY(EditAnywhere, Category = "Health|Heal", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float m_HealWindupTime = 0.5f;

	//回復してから動作が終わるまで（被弾すると動作だけ中断）
	UPROPERTY(EditAnywhere, Category = "Health|Heal", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float m_HealRecoveryTime = 0.5f;

	//回復動作中の移動速度の倍率
	UPROPERTY(EditAnywhere, Category = "Health|Heal", meta = (AllowPrivateAccess = "true", ClampMin = "0", ClampMax = "1"))
	float m_HealMoveSpeedRate = 0.5f;

	void ApplyHeal();      //構えが終わった瞬間に回復する
	void EndHeal();        //回復動作の終了（中断でも呼ぶ）
	void InterruptHeal();  //被弾・回避で割り込まれたとき

	//スキル関連
	void OnSkillModifierPressed();
	void OnSkillModifierReleased();
	void OnSkill1();
	void OnSkill2();
	void OnSkill3Pressed();
	void OnSkill3Released();
	bool CanUseSkill() const;

	//被弾モーション
	UPROPERTY(EditAnywhere, Category = "Health|HitReact", meta = (AllowPrivateAccess = "true"))
	class UAnimMontage* m_HitReactMontage;

	UPROPERTY(EditAnywhere, Category = "Health|HitReact", meta = (AllowPrivateAccess = "true", ClampMin = "0.1"))
	float m_HitReactPlayRate = 1.f;

	//のけぞり時間（この間は操作できない）
	UPROPERTY(EditAnywhere, Category = "Health|HitReact", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float m_HitStunTime = 0.4f;

	bool m_bIsHitStunned = false;
	FTimerHandle m_HitStunTimer;

	void PlayHitReaction(AActor* DamageCauser);
	void EndHitStun();

public:
	bool IsHealing() const { return m_HealPhase != EHealPhase::None; }

	// -----------------------------------------------------------------------
	// UI 用
	// -----------------------------------------------------------------------
	FOnEnergyChanged OnEnergyChanged;
	FOnGhostsChanged OnGhostsChanged;

	float GetCurrentEnergy() const { return m_CurrentEnergy; }
	float GetMaxEnergy() const { return m_MaxEnergy; }
	float GetEnergyPerStock() const { return m_EnergyPerStock; }
	int32 GetMaxEnergyStock() const { return m_iMaxEnergyStock; }
	int32 GetMaxGhostCount() const { return m_iMaxGhostCount; }

	//今のストック数（ゲージが何本溜まっているか）
	int32 GetEnergyStock() const
	{
		return (m_EnergyPerStock > 0.f)
			? FMath::Min(FMath::FloorToInt(m_CurrentEnergy / m_EnergyPerStock), m_iMaxEnergyStock)
			: 0;
	}

	//召喚中の分身の状態を、召喚した順に返す
	void GetGhostStates(TArray<EGhostLifecycleState>& OutStates) const;

private:
	//ゲージ1本分のエネルギー量
	UPROPERTY(EditAnywhere, Category = "Ghost")
	float m_EnergyPerStock = 100.f;

	//ストックできる最大本数
	UPROPERTY(EditAnywhere, Category = "Ghost")
	int32 m_iMaxEnergyStock = 5;

	//召喚中の分身の一覧（UIでカプセルの色を決めるのに使う）
	TArray<TWeakObjectPtr<AGhostCharacter>> m_ActiveGhosts;

	//エネルギーを変更する窓口（変更したら必ずUIに通知する）
	void SetEnergy(float NewEnergy);

	//分身の状態が変わったとき（消える直前など）
	UFUNCTION()
	void OnGhostStateChanged(EGhostLifecycleState NewState);

	// -----------------------------------------------------------------------
	// ジャンプ挙動のカスタマイズ（長押し制御など）
	// -----------------------------------------------------------------------
protected:
	//ジャンプ可能かどうかの判定（攻撃中はジャンプさせない）
	virtual bool CanJumpInternal_Implementation() const override;

private:
	//ジャンプボタンが押された時間
	float m_JumpPressedTime;

	//長ジャンプと判定するためのボタン長押し時間
	UPROPERTY(EditAnywhere, Category = "Jump")
	float m_JumpHoldThreshold;

	//短ジャンプの上方向最大速度
	UPROPERTY(EditAnywhere, Category = "Jump")
	float m_JumpZVelocityShort;

	//長押しの上方向最大速度
	UPROPERTY(EditAnywhere, Category = "Jump")
	float m_JumpZVelocityLong;

	void OnJumpPressed();	//ボタンを入力時
	void OnJumpReleased();	//ボタンを離したとき

	//二段ジャンプ時に再生するモーション
	UPROPERTY(EditAnywhere, Category = "Jump", meta = (AllowPrivateAccess = "true"))
	class UAnimMontage* m_SecondJumpMontage;

	UPROPERTY(EditAnywhere, Category = "Jump", meta = (AllowPrivateAccess = "true", ClampMin = "0.1"))
	float m_SecondJumpPlayRate = 1.f;
};