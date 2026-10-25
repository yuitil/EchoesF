#pragma once

#include "CoreMinimal.h"
#include "StageGimmick/StageGimmickBase.h"
#include "Gate.generated.h"

class UStaticMeshComponent;
class USceneComponent;

UENUM(BlueprintType)
enum class EGateState : uint8
{
    Closed UMETA(DisplayName = "Closed"),
    Open UMETA(DisplayName = "Open")
};

// ゲートの開閉Transform
USTRUCT(BlueprintType)
struct FGateMeshTransform
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Param")
    FTransform ClosedTransform = FTransform::Identity;

    UPROPERTY(EditAnywhere, Category = "Param")
    FTransform OpenTransform = FTransform::Identity;
};

UCLASS()
class ECHO_API AGate : public AStageGimmickBase
{
    GENERATED_BODY()

public:
    AGate();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    void OpenGate();
    void CloseGate();

protected:
    // メッシュを目標Transformへ移動
    bool UpdateGateMeshTransform(
        UStaticMeshComponent* _mesh,
        const FTransform& _targetTransform,
        float _deltaTime
    );

protected:
    UPROPERTY(VisibleAnywhere, Category = "Component")
    TObjectPtr<USceneComponent> m_Root;

    UPROPERTY(VisibleAnywhere, Category = "Component")
    TObjectPtr<UStaticMeshComponent> m_GateMesh1;

    UPROPERTY(VisibleAnywhere, Category = "Component")
    TObjectPtr<UStaticMeshComponent> m_GateMesh2;

    UPROPERTY(EditAnywhere, Category = "Param")
    bool m_UseSecondGate = true;

    UPROPERTY(EditAnywhere, Category = "Param")
    EGateState m_DefaultGateState = EGateState::Closed;

    UPROPERTY(VisibleAnywhere, Category = "Param")
    EGateState m_GateState = EGateState::Closed;

    UPROPERTY(VisibleAnywhere, Category = "Param")
    bool m_IsTransformFinished = false;

    UPROPERTY(EditAnywhere, Category = "Param")
    FGateMeshTransform m_Gate1Transform;

    UPROPERTY(EditAnywhere, Category = "Param")
    FGateMeshTransform m_Gate2Transform;

    UPROPERTY(EditAnywhere, Category = "Param")
    float m_MoveSpeed = 100.f;

    UPROPERTY(EditAnywhere, Category = "Param")
    float m_RotationSpeed = 100.f;

    UPROPERTY(EditAnywhere, Category = "Param")
    float m_ScaleSpeed = 1.f;
};