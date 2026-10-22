// GhostEnemyComponent.cpp

#include "Ghost/GhostEnemy/GhostEnemyComponent.h"
#include "Ghost/GhostCharacter/GhostCharacter.h"
#include "Ghost/GhostCharacter/GhostPlaybackComponent.h"

UGhostEnemyComponent::UGhostEnemyComponent():
    m_LifetimeSeconds(30.f),
    m_WarningThreshold(5.f),
    m_ElapsedTime(0.f),
    m_bTimerActive(false)
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UGhostEnemyComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UGhostEnemyComponent::StartLifetimeTimer()
{
    m_ElapsedTime = 0.f;
    m_bTimerActive = true;
    SetComponentTickEnabled(true);
    SetState(EGhostLifecycleState::Friendly);
}

void UGhostEnemyComponent::TickComponent(
    float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!m_bTimerActive) return;

    m_ElapsedTime += DeltaTime;

    if (CurrentState == EGhostLifecycleState::Friendly)
    {
        if (m_ElapsedTime >= m_LifetimeSeconds - m_WarningThreshold)
            SetState(EGhostLifecycleState::Warning);
    }
    else if (CurrentState == EGhostLifecycleState::Warning)
    {
        if (m_ElapsedTime >= m_LifetimeSeconds)
        {
            if (m_bCorruptOnExpire)
            {
                SetState(EGhostLifecycleState::Corrupted);
                Corrupt();
            }
            else
            {
                Vanish();
            }
        }
    }
}

float UGhostEnemyComponent::GetRemainingTime() const
{
    return FMath::Max(0.f, m_LifetimeSeconds - m_ElapsedTime);
}

float UGhostEnemyComponent::GetRemainingTimeNormalized() const
{
    if (m_LifetimeSeconds <= 0.f) return 0.f;
    return FMath::Clamp(GetRemainingTime() / m_LifetimeSeconds, 0.f, 1.f);
}

void UGhostEnemyComponent::SetState(EGhostLifecycleState NewState)
{
    if (CurrentState == NewState) return;
    CurrentState = NewState;
    OnStateChanged.Broadcast(NewState);
}

void UGhostEnemyComponent::Corrupt()
{
    AGhostCharacter* Ghost = Cast<AGhostCharacter>(GetOwner());
    if (!Ghost) return;

    if (Ghost->PlaybackComponent)
        Ghost->PlaybackComponent->SetEnemyMode();

    // SpawnAIFromControllerClass → ActivateEnemyAI に統一
    Ghost->ActivateEnemyAI();

    SetState(EGhostLifecycleState::Enemy);

    m_bTimerActive = false;
    SetComponentTickEnabled(false);
}

void UGhostEnemyComponent::Vanish()
{
    m_bTimerActive = false;
    SetComponentTickEnabled(false);
    SetState(EGhostLifecycleState::Dead);

    //TODO: 消滅エフェクト（Warning中に点滅・フェードさせるなど）
    if (AActor* Owner = GetOwner())
    {
        Owner->Destroy();
    }
}
