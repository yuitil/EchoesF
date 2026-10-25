#include "StageGimmick/Gate/Gate.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

AGate::AGate()
{
    PrimaryActorTick.bCanEverTick = true;

    // Root
    m_Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(m_Root);

    // GateMesh1
    m_GateMesh1 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GateMesh1"));
    m_GateMesh1->SetupAttachment(m_Root);

    // GateMesh2
    m_GateMesh2 = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GateMesh2"));
    m_GateMesh2->SetupAttachment(m_Root);
}

void AGate::BeginPlay()
{
    Super::BeginPlay();

    m_GateState = m_DefaultGateState;

    const bool bIsOpen = (m_GateState == EGateState::Open);

    // Gate1 初期化
    const FTransform& Gate1Transform = bIsOpen
        ? m_Gate1Transform.OpenTransform
        : m_Gate1Transform.ClosedTransform;

    m_GateMesh1->SetRelativeTransform(Gate1Transform);

    // Gate2 初期化
    m_GateMesh2->SetVisibility(m_UseSecondGate);
    m_GateMesh2->SetCollisionEnabled(
        m_UseSecondGate
            ? ECollisionEnabled::QueryAndPhysics
            : ECollisionEnabled::NoCollision
    );

    if (m_UseSecondGate)
    {
        const FTransform& Gate2Transform = bIsOpen
            ? m_Gate2Transform.OpenTransform
            : m_Gate2Transform.ClosedTransform;

        m_GateMesh2->SetRelativeTransform(Gate2Transform);
    }

    m_IsTransformFinished = true;
}

void AGate::OpenGate()
{
    if (m_GateState == EGateState::Open)
        return;

    m_GateState = EGateState::Open;
    m_IsTransformFinished = false;
}

void AGate::CloseGate()
{
    if (m_GateState == EGateState::Closed)
        return;

    m_GateState = EGateState::Closed;
    m_IsTransformFinished = false;
}

void AGate::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (m_IsTransformFinished)
        return;

    const bool bIsOpen = (m_GateState == EGateState::Open);

    // Gate1
    const FTransform& TargetTransform1 = bIsOpen
        ? m_Gate1Transform.OpenTransform
        : m_Gate1Transform.ClosedTransform;

    const bool bGate1Finished = UpdateGateMeshTransform(
        m_GateMesh1,
        TargetTransform1,
        DeltaTime
    );

    // Gate2
    bool bGate2Finished = true;

    if (m_UseSecondGate)
    {
        const FTransform& TargetTransform2 = bIsOpen
            ? m_Gate2Transform.OpenTransform
            : m_Gate2Transform.ClosedTransform;

        bGate2Finished = UpdateGateMeshTransform(
            m_GateMesh2,
            TargetTransform2,
            DeltaTime
        );
    }

    // 両方の移動が完了したら終了
    m_IsTransformFinished = bGate1Finished && bGate2Finished;
}

bool AGate::UpdateGateMeshTransform(
    UStaticMeshComponent* _mesh,
    const FTransform& _targetTransform,
    float _deltaTime
)
{
    if (!_mesh)
        return true;

    const FTransform CurrentTransform = _mesh->GetRelativeTransform();

    const FVector CurrentLocation = CurrentTransform.GetLocation();
    const FRotator CurrentRotation = CurrentTransform.Rotator();
    const FVector CurrentScale = CurrentTransform.GetScale3D();

    const FVector TargetLocation = _targetTransform.GetLocation();
    const FRotator TargetRotation = _targetTransform.Rotator();
    const FVector TargetScale = _targetTransform.GetScale3D();

    // 位置補間
    const FVector NewLocation = FMath::VInterpConstantTo(
        CurrentLocation,
        TargetLocation,
        _deltaTime,
        m_MoveSpeed
    );

    // 回転補間
    const FRotator NewRotation = FMath::RInterpConstantTo(
        CurrentRotation,
        TargetRotation,
        _deltaTime,
        m_RotationSpeed
    );

    // スケール補間
    const FVector NewScale = FMath::VInterpConstantTo(
        CurrentScale,
        TargetScale,
        _deltaTime,
        m_ScaleSpeed
    );

    _mesh->SetRelativeTransform(
        FTransform(NewRotation, NewLocation, NewScale)
    );

    // 到達判定
    const bool bLocationFinished =
        NewLocation.Equals(TargetLocation, 0.01f);

    const bool bRotationFinished =
        NewRotation.Equals(TargetRotation, 0.01f);

    const bool bScaleFinished =
        NewScale.Equals(TargetScale, 0.001f);

    if (bLocationFinished && bRotationFinished && bScaleFinished)
    {
        _mesh->SetRelativeTransform(_targetTransform);
        return true;
    }

    return false;
}