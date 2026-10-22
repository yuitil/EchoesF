#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Enemy/Core/EnemyData.h"
#include "BossAnimation.generated.h"

UCLASS(Transient, Blueprintable)
class ECHO_API UBossAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	virtual void NativeUpdateAnimation(float _deltaSeconds) override;

public:
	UPROPERTY(BlueprintReadOnly, Category = "Enemy", meta = (DisplayName = "Speed"))
	float m_speed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy", meta = (DisplayName = "Direction"))
	float m_direction = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy", meta = (DisplayName = "InAir"))
	bool m_bInAir = false;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy", meta = (DisplayName = "State"))
	EEnemyState m_state = EEnemyState::Alive;
};

UENUM(BlueprintType)
enum class EBossWindow : uint8
{
	Damage,
	DodgeInvulnerability,
	Guard
};

UCLASS(meta = (DisplayName = "BossCombatWindow"))
class ECHO_API UBossCombatWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim, float _duration, const FAnimNotifyEventReference& _reference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim, const FAnimNotifyEventReference& _reference) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (DisplayName = "Window"))
	EBossWindow m_window = EBossWindow::Damage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemey", meta = (DisplayAfter = "DamageMultiplier"))
	float m_damageScale = 1.0f;
};

UCLASS(meta = (DisplayName = "BossPresentationCue"))
class ECHO_API UBossPresentationCue : public UAnimNotify
{
	GENERATED_BODY()
public:
	virtual void Notify(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim, const FAnimNotifyEventReference& _reference) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (DisplayName = "Cue"))
	FName m_cue = TEXT("SwordWhoosh");
};

UCLASS(meta = (DisplayName = "BossRecovery"))
class ECHO_API UBossRecovery : public UAnimNotify
{
 GENERATED_BODY()
public:
 virtual void Notify(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim, const FAnimNotifyEventReference& _reference) override;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recovery", meta = (DisplayName = "Next Section"))
 FName m_next;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recovery", meta = (ClampMin = "0.05", DisplayName = "Blend Out"))
 float m_blend = .18f;
};


UCLASS(meta=(DisplayName="BossDive"))
class ECHO_API UBossDive : public UAnimNotifyState
{
 GENERATED_BODY()
public:
 virtual void NotifyTick(USkeletalMeshComponent* _mesh,UAnimSequenceBase* _anim,float _delta,const FAnimNotifyEventReference& _reference) override;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Dive",meta=(ClampMin="0",DisplayName="Forward Speed")) float m_speed=350.f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Dive",meta=(ClampMin="0",DisplayName="Fall Speed")) float m_fall=900.f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Dive",meta=(DisplayName="Land Section")) FName m_land;
};

