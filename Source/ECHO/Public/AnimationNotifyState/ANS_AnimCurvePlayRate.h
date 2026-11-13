// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_AnimCurvePlayRate.generated.h"

class UAnimMontage;
class USkeletalMeshComponent;

//==================================================
//再生速度カーブ用ランタイムデータ
//==================================================
USTRUCT()
struct FAnimCurvePlayRateRuntimeData
{
	GENERATED_BODY()

	//対象モンタージュ
	TWeakObjectPtr<UAnimMontage> Montage = nullptr;

	//Notify開始前の再生速度
	float OriginalPlayRate = 1.0f;
};

UCLASS(meta = (DisplayName = "Animation Curve Play Rate"))
class ECHO_API UANS_AnimCurvePlayRate : public UAnimNotifyState
{
	GENERATED_BODY()

public:

	//==================================================
	//Notify開始
	//==================================================
	virtual void NotifyBegin(
		USkeletalMeshComponent* _meshComp,
		UAnimSequenceBase* _animation,
		float _totalDuration,
		const FAnimNotifyEventReference& _eventReference
	) override;

	//==================================================
	//Notify更新
	//==================================================
	virtual void NotifyTick(
		USkeletalMeshComponent* _meshComp,
		UAnimSequenceBase* _animation,
		float _frameDeltaTime,
		const FAnimNotifyEventReference& _eventReference
	) override;

	//==================================================
	//Notify終了
	//==================================================
	virtual void NotifyEnd(
		USkeletalMeshComponent* _meshComp,
		UAnimSequenceBase* _animation,
		const FAnimNotifyEventReference& _eventReference
	) override;

private:

	//再生速度に使用するアニメーションカーブ
	UPROPERTY(EditAnywhere, Category = "Curve")
	FName m_playRateCurveName = TEXT("PlayRate");

	//最低再生速度
	UPROPERTY(EditAnywhere, Category = "Curve", meta = (ClampMin = "0.0"))
	float m_minPlayRate = 0.01f;

	//最大再生速度
	UPROPERTY(EditAnywhere, Category = "Curve", meta = (ClampMin = "0.0"))
	float m_maxPlayRate = 10.0f;

	//メッシュごとのランタイムデータ
	TMap<
		TWeakObjectPtr<USkeletalMeshComponent>,
		FAnimCurvePlayRateRuntimeData
	> m_runtimeData;
};