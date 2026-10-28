//SkillComponent.h
//ゲージを消費して出す強力な技（一閃・テンペスト・最終奥義）を管理するコンポーネント

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SkillComponent.generated.h"

class USkillDataAsset;
class UAnimMontage;
class AActionCharacter;
class USpringArmComponent;
class UNiagaraComponent;
struct FUltimateLevelData;

UENUM()
enum class ESkillState : uint8
{
	None,
	DashSlash,   //一閃
	Tempest,     //テンペスト
	Charging,    //最終奥義：ため中
	Ultimate     //最終奥義：発動中
};

//ため段階が変わったとき（UI用）
DECLARE_MULTICAST_DELEGATE_OneParam(FOnChargeLevelChanged, int32);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ECHO_API USkillComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USkillComponent();

	//各技の発動。条件を満たさなければ false
	bool UseDashSlash();
	bool UseTempest();
	bool StartCharge();
	void ReleaseCharge();

	//被弾したとき：ため中ならキャンセル（ゲージは減らない）
	void Interrupt();

	bool IsBusy()       const { return m_State != ESkillState::None; }
	bool IsCharging()   const { return m_State == ESkillState::Charging; }
	//ため中以外の技の最中は無敵
	bool IsInvincible() const { return IsBusy() && !IsCharging(); }
	int32 GetChargeLevel() const { return m_ChargeLevel; }

	//アニメーション通知から呼ぶ（ABP の AnimNotify_SkillHit / AnimNotify_SkillFinish）
	UFUNCTION(BlueprintCallable, Category = "Skill")
	void SkillHit();

	UFUNCTION(BlueprintCallable, Category = "Skill")
	void SkillFinish();

	//技でカメラを動かしている間 true（ロックオンのカメラ処理を止めるため）
	bool IsCameraOverridden() const { return m_CamBlendAlpha > 0.f || m_CamBlendDir != 0; }

	FOnChargeLevelChanged OnChargeLevelChanged;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, Category = "Skill")
	TObjectPtr<USkillDataAsset> SkillData;

	//当たり判定を表示する
	UPROPERTY(EditAnywhere, Category = "Skill|Debug")
	bool m_bDebugDraw = false;

private:
	ESkillState m_State = ESkillState::None;
	FTimerHandle m_StepTimer;
	FTimerHandle m_EndTimer;

	// --- 一閃 ---
	TArray<TWeakObjectPtr<AActor>> m_PendingTargets;   //通り道にいた敵（遅れて斬る）

	// --- テンペスト ---
	TWeakObjectPtr<AActor> m_TempestTarget;
	FVector m_TempestCenter = FVector::ZeroVector;
	float m_TempestAngle = 0.f;
	int32 m_TempestStep = 0;

	// --- カメラ固定 ---
	TWeakObjectPtr<USpringArmComponent> m_FrozenBoom;
	FVector m_BoomRelativeLocation = FVector::ZeroVector;

	// --- 俯瞰カメラ ---
	float m_CamBlendAlpha = 0.f;     //0 = 通常、1 = 俯瞰
	int32 m_CamBlendDir = 0;         //+1 = 俯瞰へ、-1 = 通常へ、0 = 止まっている
	float m_CamBlendTime = 0.25f;
	float m_SavedArmLength = 0.f;
	float m_SavedPitch = 0.f;
	float m_TargetArmLength = 0.f;
	float m_TargetPitch = 0.f;

	// --- 最終奥義 ---
	float m_ChargeTime = 0.f;
	int32 m_ChargeLevel = 0;
	int32 m_UltLevel = 0;
	int32 m_UltHitsLeft = 0;
	FVector m_UltOrigin = FVector::ZeroVector;   //放った位置
	FVector m_UltDir = FVector::ForwardVector;   //放った向き
	TWeakObjectPtr<UNiagaraComponent> m_ChargeFX;   //剣にまとわせているエフェクト

	// --- 共通 ---
	bool m_bHovering = false;                         //空中で発動したので重力を止めている
	TEnumAsByte<ECollisionResponse> m_SavedPawnResponse = ECR_Block;

	AActionCharacter* GetOwnerCharacter() const;
	bool CanStart() const;
	bool TryConsume(int32 Cost);
	void BeginSkill(ESkillState NewState);
	void FinishSkill(float RecoveryTime);
	void EndSkill();

	AActor* FindTarget(float Radius, const FVector& PreferDir) const;
	FVector GetInputOrForward() const;
	void DashTo(const FVector& Location, float Duration);
	void SetPassThroughEnemies(bool bEnable);
	void SetMoveSpeedRate(float Rate);
	void PlayMontage(UAnimMontage* Montage);
	void FreezeCamera(bool bFreeze, const FVector& Offset = FVector::ZeroVector);
	void StartOverheadCamera(float ArmLength, float Pitch, float BlendTime);
	void StopOverheadCamera();
	void UpdateOverheadCamera(float DeltaTime);
	void OnTempestMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	int32 DamageInSphere(const FVector& Center, float Radius, float Damage, TArray<AActor*>* OutHitActors = nullptr);
	void CollectAlongPath(const FVector& Start, const FVector& End, float Radius);

	void DashSlash_Go();
	void DashSlash_ApplyDamage();
	void Tempest_Step();
	void Ultimate_Fire();
	void Ultimate_Hit();
	void UpdateChargeEffect();
	void ClearChargeEffect();
	int32 DamageInWedge(const FVector& Origin, const FVector& Dir, const FUltimateLevelData& Level);
};
