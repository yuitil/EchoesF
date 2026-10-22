// GhostPlaybackComponent.cpp

#include "Ghost/GhostCharacter/GhostPlaybackComponent.h"
#include "Ghost/GhostCharacter/GhostCharacter.h"
#include "Player/GhostRecorderComponent.h"
#include "Ghost/AbilitySystem/GhostAbilitySystemComponent.h"
#include "Ghost/Data/GhostGameplayTags.h"
#include "Player/CombatComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UGhostPlaybackComponent::UGhostPlaybackComponent() :
    m_MirrorDelay(2.f),
    m_MoveExpireTime(2.f),
    m_RotationInterpSpeed(15.f),
    m_iPlaybackIndex(0),
    m_PlaybackStartTime(0.f),
    m_SnapshotStartTime(0.f),
    m_LastMirrorFetchTime(0.f),
    m_LastMoveReceivedTime(-999.f),
    m_AnimSpeed(0.f),
    m_bAnimAttacking(false),
    m_bAnimDodging(false)
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

// -----------------------------------------------------------------------
// Initialize
// -----------------------------------------------------------------------
void UGhostPlaybackComponent::Initialize(
    const TArray<FGhostActionData>& Snapshot,
    UGhostRecorderComponent* Recorder)
{
    PlaybackQueue = Snapshot;
    m_iPlaybackIndex = 0;
    RecorderRef = Recorder;
    CurrentMode = EGhostPlaybackMode::RecordedPlayback;
    m_PlaybackStartTime = GetWorld()->GetTimeSeconds();

    m_SnapshotStartTime = (PlaybackQueue.Num() > 0)
        ? PlaybackQueue[0].m_Timestamp
        : m_PlaybackStartTime;

    m_LastMirrorFetchTime = m_PlaybackStartTime - m_MirrorDelay;
    CurrentMoveDirection = FVector::ZeroVector;
    m_LastMoveReceivedTime = -999.f;
    m_bAnimAttacking = false;
    m_bAnimDodging = false;
    bPrevAttackingCombat = false;

    SetComponentTickEnabled(true);

    //中里6.3、ミラー分身ではないのでスポーン位置切り替えをコメントアウト
    //// 初期位置をスナップショット先頭にテレポート
    //if (PlaybackQueue.Num() > 0)
    //{
    //    if (AActor* Owner = GetOwner())
    //    {
    //        Owner->SetActorLocation(PlaybackQueue[0].Location,
    //            false, nullptr, ETeleportType::TeleportPhysics);
    //        Owner->SetActorRotation(PlaybackQueue[0].Rotation);
    //    }
    //}
}

// -----------------------------------------------------------------------
// SetEnemyMode
// -----------------------------------------------------------------------
void UGhostPlaybackComponent::SetEnemyMode()
{
    CurrentMode = EGhostPlaybackMode::EnemyMode;
    CurrentMoveDirection = FVector::ZeroVector;
    m_AnimSpeed = 0.f;
    m_bAnimAttacking = false;
    m_bAnimDodging = false;
    SetComponentTickEnabled(false);
}

// -----------------------------------------------------------------------
// TickComponent
// -----------------------------------------------------------------------
void UGhostPlaybackComponent::TickComponent(
    float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // 移動の持続は全モード共通で毎 Tick 実行
    TickMovement(DeltaTime);

    switch (CurrentMode)
    {
    case EGhostPlaybackMode::RecordedPlayback:
        TickRecordedPlayback(DeltaTime);
        break;
    case EGhostPlaybackMode::DelayedMirror:
        TickDelayedMirror(DeltaTime);
        break;
    default:
        break;
    }

    // AnimBP 用の攻撃フラグ更新
    // CombatComponent の IsAttacking() と同期させる
    if (AGhostCharacter* Ghost = Cast<AGhostCharacter>(GetOwner()))
    {
        if (Ghost->CombatComponent)
        {
            m_bAnimAttacking = Ghost->CombatComponent->IsAttacking();
        }
    }
}

// -----------------------------------------------------------------------
// TickMovement
// CurrentMoveDirection を毎フレーム AddMovementInput に渡し続ける。
// MoveExpireTime 秒間 Move レコードが来なければ停止。
// -----------------------------------------------------------------------
void UGhostPlaybackComponent::TickMovement(float DeltaTime)
{
    const float Now = GetWorld()->GetTimeSeconds();

    // Move レコードが一定時間来なければ方向をクリアして停止
    if (!CurrentMoveDirection.IsNearlyZero())
    {
        if (Now - m_LastMoveReceivedTime > m_MoveExpireTime)
        {
            CurrentMoveDirection = FVector::ZeroVector;
        }
    }

    ACharacter* Owner = Cast<ACharacter>(GetOwner());
    if (!Owner) return;

    if (!CurrentMoveDirection.IsNearlyZero())
    {
        Owner->AddMovementInput(CurrentMoveDirection, 1.f);

        // 回転を移動方向に補間
        const FRotator TargetRot = CurrentMoveDirection.Rotation();
        const FRotator NewRot = FMath::RInterpTo(
            Owner->GetActorRotation(), TargetRot,
            DeltaTime, m_RotationInterpSpeed);
        Owner->SetActorRotation(NewRot);
    }

    // AnimBP 用の速度更新
    if (UCharacterMovementComponent* CMC = Owner->GetCharacterMovement())
    {
        m_AnimSpeed = CMC->Velocity.Size2D();
    }
}

// -----------------------------------------------------------------------
// TickRecordedPlayback
// タイムスタンプに基づいてキューからアクションを順番に実行する
// -----------------------------------------------------------------------
void UGhostPlaybackComponent::TickRecordedPlayback(float DeltaTime)
{
    if (m_iPlaybackIndex >= PlaybackQueue.Num())
    {
        // 全アクション再生完了 → DelayedMirror へ遷移
        CurrentMode = EGhostPlaybackMode::DelayedMirror;
        OnRecordedPlaybackFinished.Broadcast();
        return;
    }

    const float Elapsed = GetWorld()->GetTimeSeconds() - m_PlaybackStartTime;
    const float QueueTime = PlaybackQueue[m_iPlaybackIndex].m_Timestamp - m_SnapshotStartTime;

    if (Elapsed < QueueTime) return;

    // 現在時刻以前のアクションをまとめて処理（フレーム落ち対応）
    while (m_iPlaybackIndex < PlaybackQueue.Num())
    {
        const float T = PlaybackQueue[m_iPlaybackIndex].m_Timestamp - m_SnapshotStartTime;
        if (Elapsed < T) break;
        ExecuteAction(PlaybackQueue[m_iPlaybackIndex]);
        m_iPlaybackIndex++;
    }
}

// -----------------------------------------------------------------------
// TickDelayedMirror
// Recorder から「MirrorDelay 秒前のアクション」を取得して実行し続ける
// -----------------------------------------------------------------------
void UGhostPlaybackComponent::TickDelayedMirror(float DeltaTime)
{
    if (!RecorderRef) return;

    const float Now = GetWorld()->GetTimeSeconds();
    const float FetchBefore = Now - m_MirrorDelay;

    if (FetchBefore <= m_LastMirrorFetchTime) return;

    TArray<FGhostActionData> NewActions =
        RecorderRef->GetActionsSince(m_LastMirrorFetchTime);
    m_LastMirrorFetchTime = FetchBefore;

    for (const FGhostActionData& Action : NewActions)
    {
        if (Action.m_Timestamp > FetchBefore) break;
        ExecuteAction(Action);
    }
}

// -----------------------------------------------------------------------
// ExecuteAction：Type ごとに処理を振り分ける
// -----------------------------------------------------------------------
void UGhostPlaybackComponent::ExecuteAction(const FGhostActionData& Action)
{
    switch (Action.Type)
    {
    case EGhostActionType::Move:
        ExecuteMove(Action);
        break;
    case EGhostActionType::Attack:
        ExecuteAttack();
        break;
    case EGhostActionType::Dodge:
        ExecuteDodge(Action);
        break;
    case EGhostActionType::Jump:
        ExecuteJump();
        break;
    case EGhostActionType::Land:
        m_bAnimDodging = false;
        break;
    }
}

// -----------------------------------------------------------------------
// ExecuteMove
// CurrentMoveDirection を更新するだけ。
// 実際の AddMovementInput は TickMovement が毎フレーム行う。
// -----------------------------------------------------------------------
void UGhostPlaybackComponent::ExecuteMove(const FGhostActionData& Action)
{
    FVector Dir = Action.Direction;

    // Direction が空なら記録座標への方向を補完
    if (Dir.IsNearlyZero())
    {
        if (ACharacter* Owner = Cast<ACharacter>(GetOwner()))
        {
            Dir = (Action.Location - Owner->GetActorLocation()).GetSafeNormal2D();
        }
    }

    if (!Dir.IsNearlyZero())
    {
        CurrentMoveDirection = Dir;
        m_LastMoveReceivedTime = GetWorld()->GetTimeSeconds();
    }
}

// -----------------------------------------------------------------------
// ExecuteAttack
// GAS を経由せず CombatComponent を直接呼ぶ。
// IsAttacking() で連打防止。
// -----------------------------------------------------------------------
void UGhostPlaybackComponent::ExecuteAttack()
{
    AGhostCharacter* Ghost = Cast<AGhostCharacter>(GetOwner());
    if (!Ghost || !Ghost->CombatComponent) return;

    // すでに攻撃中なら無視（連打防止）
    if (Ghost->CombatComponent->IsAttacking()) return;

    Ghost->CombatComponent->ExecuteAttack();
}

// -----------------------------------------------------------------------
// ExecuteDodge
// CombatComponent を直接呼ぶ。方向を先にセットしてから実行。
// -----------------------------------------------------------------------
void UGhostPlaybackComponent::ExecuteDodge(const FGhostActionData& Action)
{
    AGhostCharacter* Ghost = Cast<AGhostCharacter>(GetOwner());
    if (!Ghost || !Ghost->CombatComponent) return;

    // 回避方向を先にキャラに向かせる
    if (!Action.Direction.IsNearlyZero())
    {
        Ghost->SetActorRotation(Action.Direction.Rotation());
        // 回避中は移動方向もセットしておく
        CurrentMoveDirection = Action.Direction;
        m_LastMoveReceivedTime = GetWorld()->GetTimeSeconds();
    }

    Ghost->CombatComponent->ExecuteDodge();
    m_bAnimDodging = true;
}

// -----------------------------------------------------------------------
// ExecuteJump
// -----------------------------------------------------------------------
void UGhostPlaybackComponent::ExecuteJump()
{
    if (ACharacter* Owner = Cast<ACharacter>(GetOwner()))
    {
        Owner->Jump();
    }
}