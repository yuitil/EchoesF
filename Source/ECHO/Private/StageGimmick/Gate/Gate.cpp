// Fill out your copyright notice in the Description page of Project Settings.


#include "StageGimmick/Gate/Gate.h"

AGate::AGate()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AGate::BeginPlay()
{
	Super::BeginPlay();

	// 初期状態を設定
	m_GateState = m_DefaultGateState;

	const FTransform& InitialTransform =
		(m_GateState == EGateState::Open)
		? m_OpenTransform
		: m_ClosedTransform;

	SetActorTransform(InitialTransform);

	m_IsTransformFinished = true;
}
void AGate::OpenGate()
{
	if (m_GateState == EGateState::Open)return;

	m_GateState = EGateState::Open;
	m_IsTransformFinished = false;
}

void AGate::CloseGate()
{
	if (m_GateState == EGateState::Closed)return;

	m_GateState = EGateState::Closed;
	m_IsTransformFinished = false;
}

void AGate::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (m_IsTransformFinished)return;

	// 目標トランスフォームを取得
	const FTransform& TargetTransform =
		(m_GateState == EGateState::Open)
		? m_OpenTransform
		: m_ClosedTransform;

	// 現在のトランスフォームを取得
	const FTransform CurrentTransform = GetActorTransform();

	const FVector CurrentLocation = CurrentTransform.GetLocation();
	const FRotator CurrentRotation = CurrentTransform.Rotator();
	const FVector CurrentScale = CurrentTransform.GetScale3D();

	const FVector TargetLocation = TargetTransform.GetLocation();
	const FRotator TargetRotation = TargetTransform.Rotator();
	const FVector TargetScale = TargetTransform.GetScale3D();

	// 位置を一定速度で補間
	const FVector NewLocation = FMath::VInterpConstantTo(
		CurrentLocation,
		TargetLocation,
		DeltaTime,
		m_MoveSpeed
	);

	// 回転を一定速度で補間
	const FRotator NewRotation = FMath::RInterpConstantTo(
		CurrentRotation,
		TargetRotation,
		DeltaTime,
		m_RotationSpeed
	);

	// スケールを一定速度で補間
	const FVector NewScale = FMath::VInterpConstantTo(
		CurrentScale,
		TargetScale,
		DeltaTime,
		m_ScaleSpeed
	);

	// 更新したトランスフォームを適用
	SetActorTransform(FTransform(NewRotation, NewLocation, NewScale));

	// すべての要素が目標に到達したか確認
	const bool bLocationFinished = NewLocation.Equals(TargetLocation, 0.01f);

	const bool bRotationFinished = NewRotation.Equals(TargetRotation, 0.01f);

	const bool bScaleFinished = NewScale.Equals(TargetScale, 0.001f);

	if (bLocationFinished && bRotationFinished && bScaleFinished)
	{
		// 誤差を残さず目標値に合わせる
		SetActorTransform(TargetTransform);

		m_IsTransformFinished = true;
	}
}