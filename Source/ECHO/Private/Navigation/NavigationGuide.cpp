
#include "Navigation/NavigationGuide.h"

#include "Components/SceneComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

#include "NavigationSystem.h"
#include "NavigationPath.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

#include "Event/StageEventSubsystem/StageEventSubsystem.h"

ANavigationGuide::ANavigationGuide()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(
        TEXT("SceneRoot")
    );

    RootComponent = SceneRoot;
}

void ANavigationGuide::BeginPlay()
{
    Super::BeginPlay();

    m_CurrentDestinationIndex = 0;
    m_HasArrived = false;
    m_IsEventActive = false;

    ResolveTargetPlayer();

    // ステージイベントの通知を登録
    if (UStageEventSubsystem* EventSubsystem =
        GetWorld()->GetSubsystem<UStageEventSubsystem>())
    {
        EventSubsystem->OnEventStarted.AddUniqueDynamic(
            this,
            &ANavigationGuide::OnStageEventStarted
        );

        EventSubsystem->OnEventEnded.AddUniqueDynamic(
            this,
            &ANavigationGuide::OnStageEventEnded
        );
    }

    UpdateNavigation();
}

void ANavigationGuide::EndPlay(
    const EEndPlayReason::Type EndPlayReason)
{
    // ステージイベントの通知を解除
    if (UWorld* World = GetWorld())
    {
        if (UStageEventSubsystem* EventSubsystem =
            World->GetSubsystem<UStageEventSubsystem>())
        {
            EventSubsystem->OnEventStarted.RemoveDynamic(
                this,
                &ANavigationGuide::OnStageEventStarted
            );

            EventSubsystem->OnEventEnded.RemoveDynamic(
                this,
                &ANavigationGuide::OnStageEventEnded
            );
        }
    }

    // 生成したNiagaraComponentを破棄
    for (UNiagaraComponent* Effect : ParticleEffects)
    {
        if (!IsValid(Effect)) continue;

        Effect->DestroyComponent();
    }

    ParticleEffects.Empty();

    Super::EndPlay(EndPlayReason);
}

void ANavigationGuide::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // イベント中は更新しない
    if (m_IsEventActive) return;

    // 到着済みの場合は更新しない
    if (m_HasArrived) return;

    UpdateTimer += DeltaTime;

    if (UpdateTimer < FMath::Max(UpdateInterval, 0.05f))
        return;

    UpdateTimer = 0.0f;

    ResolveTargetPlayer();

    AActor* CurrentDestination = GetCurrentDestination();

    if (!IsValid(TargetPlayer) ||
        !IsValid(CurrentDestination))
    {
        HideParticleEffects();
        return;
    }

    // 目的地への到着判定
    if (FVector::Dist(
        TargetPlayer->GetActorLocation(),
        CurrentDestination->GetActorLocation()) <= ArrivalDistance)
    {
        m_HasArrived = true;

        HideParticleEffects();
        return;
    }

    UpdateNavigation();
}

void ANavigationGuide::ResolveTargetPlayer()
{
    if (IsValid(TargetPlayer)) return;

    UWorld* World = GetWorld();
    if (!World) return;

    APlayerController* PC = World->GetFirstPlayerController();
    if (!IsValid(PC)) return;

    TargetPlayer = PC->GetPawn();
}

void ANavigationGuide::AddDestination(
    AActor* NewDestination)
{
    if (!IsValid(NewDestination)) return;

    Destinations.Add(NewDestination);

    // 最初の目的地が追加された場合
    if (Destinations.Num() == 1)
    {
        m_CurrentDestinationIndex = 0;
        m_HasArrived = false;
    }

    UpdateTimer = 0.0f;

    if (m_IsEventActive) return;

    UpdateNavigation();
}

void ANavigationGuide::ClearDestination()
{
    Destinations.Empty();

    m_CurrentDestinationIndex = 0;
    m_HasArrived = false;
    UpdateTimer = 0.0f;

    HideParticleEffects();
}

AActor* ANavigationGuide::GetCurrentDestination() const
{
    if (!Destinations.IsValidIndex(
        m_CurrentDestinationIndex))
    {
        return nullptr;
    }

    AActor* CurrentDestination =
        Destinations[m_CurrentDestinationIndex].Get();

    return IsValid(CurrentDestination)
        ? CurrentDestination
        : nullptr;
}

void ANavigationGuide::AdvanceDestination()
{
    // 次の目的地へ進む
    ++m_CurrentDestinationIndex;

    m_HasArrived = false;
    UpdateTimer = 0.0f;

    // 次の目的地が存在しない場合
    if (!Destinations.IsValidIndex(
        m_CurrentDestinationIndex))
    {
        HideParticleEffects();
        return;
    }

    // 次の目的地への経路を計算
    UpdateNavigation();
}

void ANavigationGuide::OnStageEventStarted()
{
    m_IsEventActive = true;

    // イベント中はナビゲーションを非表示
    HideParticleEffects();
}

void ANavigationGuide::OnStageEventEnded()
{
    m_IsEventActive = false;

    // イベント終了時に次の目的地へ進む
    AdvanceDestination();
}

void ANavigationGuide::UpdateNavigation()
{
    // イベント中は経路探索・表示を行わない
    if (m_IsEventActive)
    {
        HideParticleEffects();
        return;
    }

    // 到着済みの場合は非表示
    if (m_HasArrived)
    {
        HideParticleEffects();
        return;
    }

    ResolveTargetPlayer();

    AActor* CurrentDestination = GetCurrentDestination();

    if (!IsValid(TargetPlayer) ||
        !IsValid(CurrentDestination) ||
        !IsValid(NavigationEffect))
    {
        HideParticleEffects();
        return;
    }

    UWorld* World = GetWorld();
    if (!World) return;

    // NavMeshを使用して経路探索
    UNavigationPath* Path =
        UNavigationSystemV1::FindPathToLocationSynchronously(
            World,
            TargetPlayer->GetActorLocation(),
            CurrentDestination->GetActorLocation(),
            TargetPlayer.Get()
        );

    if (!IsValid(Path) ||
        !Path->IsValid() ||
        Path->IsPartial() ||
        Path->PathPoints.Num() < 2)
    {
        HideParticleEffects();
        return;
    }

    TArray<FVector> Positions;

    BuildParticlePositions(
        Path->PathPoints,
        Positions
    );

    UpdateParticleEffects(Positions);
}

void ANavigationGuide::BuildParticlePositions(
    const TArray<FVector>& PathPoints,
    TArray<FVector>& OutPositions) const
{
    OutPositions.Reset();

    if (PathPoints.Num() < 2) return;

    const float Spacing =
        FMath::Max(ParticleSpacing, 10.0f);

    const float DisplayDistance =
        FMath::Max(MaxDisplayDistance, 0.0f);

    float TotalDistance = 0.0f;
    float NextSampleDistance = 0.0f;

    for (int32 i = 0; i < PathPoints.Num() - 1; ++i)
    {
        const FVector Start = PathPoints[i];
        const FVector End = PathPoints[i + 1];

        const FVector Segment = End - Start;
        const float SegmentLength = Segment.Size();

        if (SegmentLength <= KINDA_SMALL_NUMBER)
            continue;

        const FVector Direction =
            Segment / SegmentLength;

        const float SegmentEndDistance =
            TotalDistance + SegmentLength;

        while (NextSampleDistance <= SegmentEndDistance &&
            NextSampleDistance <= DisplayDistance)
        {
            const float LocalDistance =
                FMath::Clamp(
                    NextSampleDistance - TotalDistance,
                    0.0f,
                    SegmentLength
                );

            FVector Position =
                Start + Direction * LocalDistance;

            Position.Z += HeightOffset;

            OutPositions.Add(Position);

            NextSampleDistance += Spacing;
        }

        TotalDistance = SegmentEndDistance;

        if (TotalDistance >= DisplayDistance)
            break;
    }
}

void ANavigationGuide::UpdateParticleEffects(
    const TArray<FVector>& Positions)
{
    if (!IsValid(NavigationEffect))
    {
        HideParticleEffects();
        return;
    }

    UWorld* World = GetWorld();
    if (!World) return;

    // 必要な数だけNiagaraComponentを生成
    while (ParticleEffects.Num() < Positions.Num())
    {
        UNiagaraComponent* Effect =
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                World,
                NavigationEffect,
                FVector::ZeroVector,
                FRotator::ZeroRotator,
                FVector(1.0f),
                false, // AutoDestroyしない
                false, // AutoActivateしない
                ENCPoolMethod::None,
                true
            );

        if (!IsValid(Effect)) break;

        ParticleEffects.Add(Effect);
    }

    // 各エフェクトの位置を更新
    for (int32 i = 0; i < Positions.Num(); ++i)
    {
        if (!ParticleEffects.IsValidIndex(i))
            break;

        UNiagaraComponent* Effect = ParticleEffects[i];

        if (!IsValid(Effect)) continue;

        Effect->SetWorldLocation(Positions[i]);
        Effect->SetVisibility(true);

        if (!Effect->IsActive())
        {
            Effect->Activate(true);
        }
    }

    // 余ったエフェクトを非表示
    for (int32 i = Positions.Num();
        i < ParticleEffects.Num();
        ++i)
    {
        UNiagaraComponent* Effect = ParticleEffects[i];

        if (!IsValid(Effect)) continue;

        Effect->SetVisibility(false);
    }
}

void ANavigationGuide::HideParticleEffects()
{
    for (UNiagaraComponent* Effect : ParticleEffects)
    {
        if (!IsValid(Effect)) continue;

        Effect->SetVisibility(false);
    }
}
