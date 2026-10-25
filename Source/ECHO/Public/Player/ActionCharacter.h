// ActionCharacter.h
// �v���C���[�̃R�A�N���X
// ���́E�J��������E�e�R���|�[�l���g�ւ̈Ϗ�

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

//�G�l���M�[���ς�����Ƃ��i���ݒl, �ő�l�j
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnEnergyChanged, float, float);
//���g�̐��E��Ԃ��ς�����Ƃ�
DECLARE_MULTICAST_DELEGATE(FOnGhostsChanged);
//UI�p�i����HP, �ő�HP�j
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnHPChanged, float, float);
//UI�p�i�c���, �ő�񐔁j
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnHealCountChanged, int32, int32);

UCLASS()
class ECHO_API AActionCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AActionCharacter(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;
	//�I�[�g�_�b�V���̌v���Ɏg��
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	//�W�����v�����������Ƃ��ɌĂ΂��֐�
	virtual void OnJumped_Implementation() override;
	//�n�ʂȂǂɒ��n�����Ƃ��ɌĂ΂��֐�
	virtual void Landed(const FHitResult& Hit) override;

	// -----------------------------------------------------------------------
	// ����
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

	//�����A�N�V����
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* SummonAction;

	//�񕜃A�N�V����
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* HealAction;
	void Heal();
	
	
	// -----------------------------------------------------------------------
	// �ړ�
	// -----------------------------------------------------------------------

	//�I�[�g�_�b�V���ւ̈ڍs����
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float m_TimeToSprint;

	//���葱���Ă��鎞�Ԃ��v������ϐ�
	float m_CurrentRunTime;
	//����J�n�ƏI��
	void StartSprint();
	void StopSprint();

	//�X�e�B�b�N�𗣂����Ǝ��ɕۑ������������N���A����֐�
	void MoveCompleted(const FInputActionValue& Value);

	// -----------------------------------------------------------------------
	// �J����
	// -----------------------------------------------------------------------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* FollowCamera;

	// -----------------------------------------------------------------------
	// �퓬
	// -----------------------------------------------------------------------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	class UCombatComponent* CombatComponent;

	//��Ɏ������錕�̃��b�V��
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	UStaticMeshComponent* WeaponMesh;

public:
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

	float GetCurrentHP() const { return m_CurrentHP; }
	float GetMaxHP()     const { return m_MaxHP; }
	bool  IsDead()       const { return m_bIsDead; }

	FOnHPChanged OnHPChanged;

	//�f�o�b�O�p�F�R���\�[���ŁuDebugDamage 30�v�Ƒłƃ_���[�W���󂯂�
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

	//��e��̖��G���ԁi���i�q�b�g�h�~�j
	UPROPERTY(EditAnywhere, Category = "Health", meta = (AllowPrivateAccess = "true"))
	float m_HitInvincibleTime = 0.5f;

	//��𒆂͖��G�ɂ��邩
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

	//�퓬�����ǂ����i�_�b�V���̐����ȂǂɎg���j
	bool IsInCombat() const { return m_bInCombat; }

private:
	//���͈͓̔��Ɂu�N����_���Ă���G�v��������퓬��
	UPROPERTY(EditAnywhere, Category = "Movement|Combat")
	float m_CombatCheckRadius = 1500.f;

	//�Ō�ɍU���𓖂ĂĂ���A���̕b���͐퓬���Ƃ݂Ȃ�
	UPROPERTY(EditAnywhere, Category = "Movement|Combat")
	float m_CombatLingerTime = 4.f;

	bool  m_bInCombat = false;
	float m_LastCombatTime = -100.f;
	float m_CombatCheckTimer = 0.f;

	void UpdateCombatState(float DeltaTime);

	// -----------------------------------------------------------------------
	// ���b�N�I������
	// -----------------------------------------------------------------------
public:
	//���b�N�I���R���|�[�l���g
	UPROPERTY(VisibleAnywhere)
	ULockOnComponent* LockOnComponent;

	//���b�N�I����Ԃ�؂�ւ���֐�
	void OnLockOnChanged(AActor* NewTarget);

	//���b�N�I�����̃J������ԑ��x
	UPROPERTY(EditAnywhere, Category = "LockOn")
	float m_LockOnCameraInterpSpeed;

	//���b�N�I�����̃J�����̋����I�t�Z�b�g
	UPROPERTY(EditAnywhere, Category = "LockOn")
	float m_LockOnCameraOffsetY;

	//���b�N�I�����J����Pitch�̉���
	UPROPERTY(EditAnywhere, Category = "LockOn")
	float m_LockOnPitchMin;

	//���b�N�I�����J����Pitch�̏��
	UPROPERTY(EditAnywhere, Category = "LockOn")
	float m_LockOnPitchMax;

	//���E�I�t�Z�b�g���ő�ɂȂ���ς̐�Βl�A�������قǐ؂�ւ����@�q
	UPROPERTY(EditAnywhere, Category = "LockOn")
	float m_SideBlendRange = 0.5f;

	//�����X���ǂꂾ���G���ɂ悹�邩(0=�v���C���[ 1=�G)
	UPROPERTY(EditAnywhere, Category = "LockOn")
	float m_TargetFocusBias = 0.5f;

	//���ߋ����ł̒Ǐ]���x
	UPROPERTY(EditAnywhere, Category = "LockOn")
	float m_LockOnInterpSpeedNear = 12.f;

	FVector DefaultSocketOffset;

	//���b�N�I�����̂��ǂ����̃t���O
	bool m_bsLockedOn;

	//���b�N�I�����̃J�����̍X�V
	void UpdateLockOnCamera(float DeltaTime);

	// -----------------------------------------------------------------------
	// ���g�V�X�e��
	// -----------------------------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "Ghost")
	void SummonGhost();

	UFUNCTION()
	void OnGhostDestroyed(AActor* DestroyedActor);

	//�G�l���M�[�iUI �\���p�� public�j
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost")
	float m_CurrentEnergy;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost")
	float m_MaxEnergy;

	//���ݏ������̕��g���iUI �\���p�j
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ghost")
	int32 m_iActiveGhostCount;

	//���g�̍ő叢����
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ghost")
	int32 m_iMaxGhostCount;

private:
	//�G�ɍU�����q�b�g�����ۂɒʒm���󂯎��A�G�l���M�[���`���[�W����
	void OnHitEnemy(float EnergyGain);

	//�v���C���[�s�����L�^����R���|�[�l���g
	UPROPERTY(VisibleAnywhere, Category = "Ghost")
	UGhostRecorderComponent* GhostRecorder;

	//���g�̏����E�Ǘ��R���|�[�l���g�iTrySummon �o�R�ŗ��p�\�j
	UPROPERTY(VisibleAnywhere, Category = "Ghost")
	UGhostManagerComponent* GhostManager;

	//�����R�X�g
	UPROPERTY(EditAnywhere, Category = "Ghost")
	float m_GhostSummonCost;

	//�q�b�g���̃G�l���M�[������
	UPROPERTY(EditAnywhere, Category = "Ghost")
	float m_EnergyGainPerHit;

	// �X�|�[������ GhostCharacter �N���X�iBP �� BP_GhostCharacter ��ݒ�j
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = "true"), Category = "Ghost")
	TSubclassOf<AGhostCharacter> GhostCharacterClass;

	//���g�̍U���œ�����G�l���M�[�̔{���i1.0�Ńv���C���[�Ɠ����j
	UPROPERTY(EditAnywhere, Category = "Ghost")
	float m_GhostEnergyRate = 1.f;

	// private �̊֐��ɒǉ�
		//���g���G�ɍU���𓖂Ă��Ƃ��̒ʒm���󂯎��
	void OnGhostHitEnemy(float EnergyGain);

	// -----------------------------------------------------------------------
	// �� 
	// -----------------------------------------------------------------------
public:
	int32 GetHealCount()    const { return m_iHealCount; }
	int32 GetMaxHealCount() const { return m_iMaxHealCount; }

	//�񐔂�S�񕜁i�`�F�b�N�|�C���g�ȂǂŎg���\��j
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

	//�{�^���������Ă���񕜂���܂Łi���̊Ԃɔ�e����Ǝ��s�j
	UPROPERTY(EditAnywhere, Category = "Health|Heal", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float m_HealWindupTime = 0.5f;

	//�񕜂��Ă��瓮�삪�I���܂Łi��e����Ɠ��삾�����f�j
	UPROPERTY(EditAnywhere, Category = "Health|Heal", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float m_HealRecoveryTime = 0.5f;

	//�񕜓��쒆�̈ړ����x�̔{��
	UPROPERTY(EditAnywhere, Category = "Health|Heal", meta = (AllowPrivateAccess = "true", ClampMin = "0", ClampMax = "1"))
	float m_HealMoveSpeedRate = 0.5f;

	void ApplyHeal();      //�\�����I������u�Ԃɉ񕜂���
	void EndHeal();        //�񕜓���̏I���i���f�ł��Ăԁj
	void InterruptHeal();  //��e�E����Ŋ��荞�܂ꂽ�Ƃ�

	//��e���[�V����
	UPROPERTY(EditAnywhere, Category = "Health|HitReact", meta = (AllowPrivateAccess = "true"))
	class UAnimMontage* m_HitReactMontage;

	UPROPERTY(EditAnywhere, Category = "Health|HitReact", meta = (AllowPrivateAccess = "true", ClampMin = "0.1"))
	float m_HitReactPlayRate = 1.f;

	//�̂����莞�ԁi���̊Ԃ͑���ł��Ȃ��j
	UPROPERTY(EditAnywhere, Category = "Health|HitReact", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	float m_HitStunTime = 0.4f;

	bool m_bIsHitStunned = false;
	FTimerHandle m_HitStunTimer;

	void PlayHitReaction(AActor* DamageCauser);
	void EndHitStun();

public:
	bool IsHealing() const { return m_HealPhase != EHealPhase::None; }

	// -----------------------------------------------------------------------
	// UI �p
	// -----------------------------------------------------------------------
	FOnEnergyChanged OnEnergyChanged;
	FOnGhostsChanged OnGhostsChanged;

	float GetCurrentEnergy() const { return m_CurrentEnergy; }
	float GetMaxEnergy() const { return m_MaxEnergy; }
	float GetEnergyPerStock() const { return m_EnergyPerStock; }
	int32 GetMaxEnergyStock() const { return m_iMaxEnergyStock; }
	int32 GetMaxGhostCount() const { return m_iMaxGhostCount; }

	//���̃X�g�b�N���i�Q�[�W�����{���܂��Ă��邩�j
	int32 GetEnergyStock() const
	{
		return (m_EnergyPerStock > 0.f)
			? FMath::Min(FMath::FloorToInt(m_CurrentEnergy / m_EnergyPerStock), m_iMaxEnergyStock)
			: 0;
	}

	//�������̕��g�̏�Ԃ��A�����������ɕԂ�
	void GetGhostStates(TArray<EGhostLifecycleState>& OutStates) const;

private:
	//�Q�[�W1�{���̃G�l���M�[��
	UPROPERTY(EditAnywhere, Category = "Ghost")
	float m_EnergyPerStock = 100.f;

	//�X�g�b�N�ł���ő�{��
	UPROPERTY(EditAnywhere, Category = "Ghost")
	int32 m_iMaxEnergyStock = 5;

	//�������̕��g�̈ꗗ�iUI�ŃJ�v�Z���̐F�����߂�̂Ɏg���j
	TArray<TWeakObjectPtr<AGhostCharacter>> m_ActiveGhosts;

	//�G�l���M�[��ύX���鑋���i�ύX������K��UI�ɒʒm����j
	void SetEnergy(float NewEnergy);

	//���g�̏�Ԃ��ς�����Ƃ��i�����钼�O�Ȃǁj
	UFUNCTION()
	void OnGhostStateChanged(EGhostLifecycleState NewState);

	// -----------------------------------------------------------------------
	// �W�����v�����̃J�X�^�}�C�Y�i����������Ȃǁj
	// -----------------------------------------------------------------------
protected:
	//�W�����v�\���ǂ����̔���i�U�����̓W�����v�����Ȃ��j
	virtual bool CanJumpInternal_Implementation() const override;

private:
	//�W�����v�{�^���������ꂽ����
	float m_JumpPressedTime;

	//���W�����v�Ɣ��肷�邽�߂̃{�^������������
	UPROPERTY(EditAnywhere, Category = "Jump")
	float m_JumpHoldThreshold;

	//�Z�W�����v�̏�����ő呬�x
	UPROPERTY(EditAnywhere, Category = "Jump")
	float m_JumpZVelocityShort;

	//�������̏�����ő呬�x
	UPROPERTY(EditAnywhere, Category = "Jump")
	float m_JumpZVelocityLong;

	void OnJumpPressed();	//�{�^������͎�
	void OnJumpReleased();	//�{�^���𗣂����Ƃ�

	//��i�W�����v���ɍĐ����郂�[�V����
	UPROPERTY(EditAnywhere, Category = "Jump", meta = (AllowPrivateAccess = "true"))
	class UAnimMontage* m_SecondJumpMontage;

	UPROPERTY(EditAnywhere, Category = "Jump", meta = (AllowPrivateAccess = "true", ClampMin = "0.1"))
	float m_SecondJumpPlayRate = 1.f;
};