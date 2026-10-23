// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StageGimmick/StageGimmickBase.h"
#include "Gate.generated.h"

/**
 *
 */

UENUM(BlueprintType)
enum class EGateState : uint8
{
	Closed UMETA(DisplayName = "Closed"),
	Open   UMETA(DisplayName = "Open")
};


UCLASS()
class ECHO_API AGate : public AStageGimmickBase
{
	GENERATED_BODY()
public:
	AGate();

	virtual void BeginPlay()override;

	virtual void Tick(float DeltaTime) override;
	void OpenGate();

	void CloseGate();

protected:
	UPROPERTY(EditAnywhere, Category = "Param")
	EGateState m_DefaultGateState = EGateState::Closed;

	UPROPERTY(VisibleAnywhere, Category = "Param")
	EGateState m_GateState = EGateState::Closed;

	UPROPERTY(VisibleAnywhere, Category = "Param")
	bool m_IsTransformFinished = false;

	// ゲートが閉じているときのトランスフォーム
	UPROPERTY(EditAnywhere, Category = "Param")
	FTransform m_ClosedTransform = FTransform::Identity;

	// ゲートが開いているときのトランスフォーム
	UPROPERTY(EditAnywhere, Category = "Param")
	FTransform m_OpenTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, Category = "Param")
	float m_MoveSpeed = 100.f;

	UPROPERTY(EditAnywhere, Category = "Param")
	float m_RotationSpeed = 100.f;

	UPROPERTY(EditAnywhere, Category = "Param")
	float m_ScaleSpeed = 1.f;
};
