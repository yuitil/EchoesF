#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Enemy/Core/EnemyData.h"
#include "BossAnimation.generated.h"

//移動の速さや向きをアニメーションへ渡す。
UCLASS(Transient, Blueprintable)
class ECHO_API UBossAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	//移動速度と空中の状態をアニメーションへ渡す。
	virtual void NativeUpdateAnimation(float _deltaSeconds) override;

public:
	//移動や再生に使う速さ。
	UPROPERTY(BlueprintReadOnly, Category = "Enemy", meta = (DisplayName = "Speed"))
	float m_speed = 0.f;

	//正面を基準にした移動方向。
	UPROPERTY(BlueprintReadOnly, Category = "Enemy", meta = (DisplayName = "Direction"))
	float m_direction = 0.f;

	//空中にいるか。
	UPROPERTY(BlueprintReadOnly, Category = "Enemy", meta = (DisplayName = "InAir"))
	bool m_bInAir = false;

	//現在の状態。
	UPROPERTY(BlueprintReadOnly, Category = "Enemy", meta = (DisplayName = "State"))
	EEnemyState m_state = EEnemyState::Alive;
};

//通知で有効にする攻撃や防御の種類。
UENUM(BlueprintType)
enum class EBossWindow : uint8
{
	//ダメージの判定。
	Damage,
	//回避の無敵時間。
	DodgeInvulnerability,
	//通常のガード。
	Guard
};

//攻撃が当たる時間や、ガードする時間を決める。
UCLASS(meta = (DisplayName = "BossCombatWindow"))
class ECHO_API UBossCombatWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	//通知の区間に入った時の処理を行う。
	virtual void NotifyBegin(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim, float _duration, const FAnimNotifyEventReference& _reference) override;

	//通知の区間を出た時の処理を行う。
	virtual void NotifyEnd(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim, const FAnimNotifyEventReference& _reference) override;

public:
	//動作の判定に使う時間の幅。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (DisplayName = "Window"))
	EBossWindow m_window = EBossWindow::Damage;

	//通知区間でのダメージ倍率。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemey", meta = (DisplayAfter = "DamageMultiplier"))
	float m_damageScale = 1.0f;

    //この一振りに使うボーン。空欄なら攻撃データを使う。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Hit", meta=(DisplayName="Hit Bone"))
    FName m_hitBone;

    //指定したボーンから剣先までの位置。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Hit", meta=(DisplayName="Hit Tip Offset"))
    FVector m_tipOffset = FVector::ZeroVector;
};

//アニメーションから音や見た目の合図を送る。
UCLASS(meta = (DisplayName = "BossPresentationCue"))
class ECHO_API UBossPresentationCue : public UAnimNotify
{
	GENERATED_BODY()

public:
	//アニメーションの指定位置で処理を呼ぶ。
	virtual void Notify(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim, const FAnimNotifyEventReference& _reference) override;

public:
	//BPへ伝える演出の名前。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (DisplayName = "Cue"))
	FName m_cue = TEXT("SwordWhoosh");
};

//攻撃の戻り動作を終え、次の動きへつなぐ。
UCLASS(meta = (DisplayName = "BossRecovery"))
class ECHO_API UBossRecovery : public UAnimNotify
{
	GENERATED_BODY()

public:
	//アニメーションの指定位置で処理を呼ぶ。
	virtual void Notify(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim, const FAnimNotifyEventReference& _reference) override;

public:
	//次につなぐ区間名。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recovery", meta = (DisplayName = "Next Section"))
	FName m_next;

	//動作を切り替えるためのなじませ時間。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recovery", meta = (ClampMin = "0.05", DisplayName = "Blend Out"))
	float m_blend = .18f;
};

//空中攻撃で前へ進み、地面へ降りる。
UCLASS(meta=(DisplayName="BossDive"))
class ECHO_API UBossDive : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	//通知の区間内で移動を更新する。
	virtual void NotifyTick(USkeletalMeshComponent* _mesh,UAnimSequenceBase* _anim,float _delta,const FAnimNotifyEventReference& _reference) override;

public:
	//移動や再生に使う速さ。
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Dive",meta=(ClampMin="0",DisplayName="Forward Speed"))
	float m_speed=350.f;

	//落下する速さ。
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Dive",meta=(ClampMin="0",DisplayName="Fall Speed"))
	float m_fall=900.f;

	//着地で動作を終了するか。
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Dive",meta=(DisplayName="Land Section"))
	FName m_land;
};
