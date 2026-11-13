// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimationNotifyState/ANS_AnimCurvePlayRate.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"

//==================================================
//Notify開始
//==================================================
void UANS_AnimCurvePlayRate::NotifyBegin(
	USkeletalMeshComponent* _meshComp,
	UAnimSequenceBase* _animation,
	float _totalDuration,
	const FAnimNotifyEventReference& _eventReference
)
{
	Super::NotifyBegin(
		_meshComp,
		_animation,
		_totalDuration,
		_eventReference
	);

	if (!IsValid(_meshComp))
	{
		return;
	}

	UAnimInstance* AnimInstance =
		_meshComp->GetAnimInstance();

	if (!IsValid(AnimInstance))
	{
		return;
	}

	UAnimMontage* Montage =
		AnimInstance->GetCurrentActiveMontage();

	if (!IsValid(Montage))
	{
		return;
	}

	FAnimCurvePlayRateRuntimeData RuntimeData;

	RuntimeData.Montage = Montage;

	RuntimeData.OriginalPlayRate =
		AnimInstance->Montage_GetPlayRate(Montage);

	const TWeakObjectPtr<USkeletalMeshComponent> MeshKey =
		_meshComp;

	m_runtimeData.Add(
		MeshKey,
		RuntimeData
	);
}

//==================================================
//Notify更新
//==================================================
void UANS_AnimCurvePlayRate::NotifyTick(
	USkeletalMeshComponent* _meshComp,
	UAnimSequenceBase* _animation,
	float _frameDeltaTime,
	const FAnimNotifyEventReference& _eventReference
)
{
	Super::NotifyTick(
		_meshComp,
		_animation,
		_frameDeltaTime,
		_eventReference
	);

	if (!IsValid(_meshComp))
	{
		return;
	}

	UAnimInstance* AnimInstance =
		_meshComp->GetAnimInstance();

	if (!IsValid(AnimInstance))
	{
		return;
	}

	const TWeakObjectPtr<USkeletalMeshComponent> MeshKey =
		_meshComp;

	FAnimCurvePlayRateRuntimeData* RuntimeData =
		m_runtimeData.Find(MeshKey);

	if (!RuntimeData)
	{
		return;
	}

	UAnimMontage* Montage =
		RuntimeData->Montage.Get();

	if (!IsValid(Montage))
	{
		return;
	}

	//==================================================
	//再生速度カーブを取得
	//カーブが存在しない場合は1倍
	//==================================================
	float PlayRateCurveValue = 1.0f;
	float FoundPlayRateCurveValue = 0.0f;

	const bool bHasPlayRateCurve =
		AnimInstance->GetCurveValue(
			m_playRateCurveName,
			FoundPlayRateCurveValue
		);

	if (bHasPlayRateCurve)
	{
		PlayRateCurveValue =
			FoundPlayRateCurveValue;
	}

	//==================================================
	//再生速度を制限
	//==================================================
	const float PlayRate =
		FMath::Clamp(
			PlayRateCurveValue,
			m_minPlayRate,
			m_maxPlayRate
		);

	//==================================================
	//モンタージュの再生速度に反映
	//==================================================
	AnimInstance->Montage_SetPlayRate(
		Montage,
		PlayRate
	);
}

//==================================================
//Notify終了
//==================================================
void UANS_AnimCurvePlayRate::NotifyEnd(
	USkeletalMeshComponent* _meshComp,
	UAnimSequenceBase* _animation,
	const FAnimNotifyEventReference& _eventReference
)
{
	Super::NotifyEnd(
		_meshComp,
		_animation,
		_eventReference
	);

	if (!IsValid(_meshComp))
	{
		return;
	}

	const TWeakObjectPtr<USkeletalMeshComponent> MeshKey =
		_meshComp;

	FAnimCurvePlayRateRuntimeData* RuntimeData =
		m_runtimeData.Find(MeshKey);

	if (!RuntimeData)
	{
		return;
	}

	UAnimInstance* AnimInstance =
		_meshComp->GetAnimInstance();

	UAnimMontage* Montage =
		RuntimeData->Montage.Get();

	if (IsValid(AnimInstance) && IsValid(Montage))
	{
		AnimInstance->Montage_SetPlayRate(
			Montage,
			RuntimeData->OriginalPlayRate
		);
	}

	m_runtimeData.Remove(MeshKey);
}