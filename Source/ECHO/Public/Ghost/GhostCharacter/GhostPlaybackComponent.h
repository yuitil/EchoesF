// GhostPlaybackComponent.h
// 記録データの再生と DelayedMirror を管理するコンポーネント
//
// ■ 移動の仕組み（重要）
//   AddMovementInput は呼ばれたフレームだけ有効。
//   そのため Move レコードが来るたびに CurrentMoveDirection を更新し、
//   毎 Tick でその方向に AddMovementInput を呼び続ける。
//   MoveExpireTime 秒間 Move レコードが来なければ方向をゼロにして停止。
//
// ■ 攻撃の仕組み
//   GAS 経由ではなく CombatComponent を直接呼ぶ。
//   前フレームの攻撃フラグとの差分（立ち上がりエッジ）で連打を防止。
//
// ■ モード遷移
//   RecordedPlayback → DelayedMirror → EnemyMode

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Ghost/Data/GhostTypes.h"
#include "GhostPlaybackComponent.generated.h"

class UGhostRecorderComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRecordedPlaybackFinished);

UCLASS(ClassGroup = (Ghost), meta = (BlueprintSpawnableComponent))
class ECHO_API UGhostPlaybackComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGhostPlaybackComponent();

    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    // -----------------------------------------------------------------------
    // 初期化 / モード切替
    // -----------------------------------------------------------------------
    void Initialize(const TArray<FGhostActionData>& Snapshot,
        UGhostRecorderComponent* Recorder);

    void SetEnemyMode();

    // -----------------------------------------------------------------------
    // AnimBP 用の状態取得
    // -----------------------------------------------------------------------
    UFUNCTION(BlueprintPure, Category = "Ghost|Playback")
    float GetSpeed() const { return m_AnimSpeed; }

    UFUNCTION(BlueprintPure, Category = "Ghost|Playback")
    bool GetIsAttacking() const { return m_bAnimAttacking; }

    UFUNCTION(BlueprintPure, Category = "Ghost|Playback")
    bool GetIsDodging() const { return m_bAnimDodging; }

    UFUNCTION(BlueprintPure, Category = "Ghost|Playback")
    EGhostPlaybackMode GetPlaybackMode() const { return CurrentMode; }

    // -----------------------------------------------------------------------
    // イベント
    // -----------------------------------------------------------------------
    UPROPERTY(BlueprintAssignable, Category = "Ghost|Playback")
    FOnRecordedPlaybackFinished OnRecordedPlaybackFinished;

    // -----------------------------------------------------------------------
    // 設定値（BP で調整可能）
    // -----------------------------------------------------------------------

    /** DelayedMirror の遅延秒数 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost|Playback",
        meta = (ClampMin = "0.5", ClampMax = "5.0"))
    float m_MirrorDelay;

    /** Move レコードが来なくなってから何秒後に停止するか */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost|Playback",
        meta = (ClampMin = "0.05", ClampMax = "1.0"))
    float m_MoveExpireTime;

    /** 回転補間速度 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ghost|Playback")
    float m_RotationInterpSpeed;

private:
    // -----------------------------------------------------------------------
    // モード
    // -----------------------------------------------------------------------
    EGhostPlaybackMode CurrentMode = EGhostPlaybackMode::RecordedPlayback;

    // -----------------------------------------------------------------------
    // RecordedPlayback
    // -----------------------------------------------------------------------
    TArray<FGhostActionData> PlaybackQueue;
    int32 m_iPlaybackIndex;
    float m_PlaybackStartTime;
    float m_SnapshotStartTime;
    
    void TickRecordedPlayback(float DeltaTime);
    
public:
    //中里,PlaybackQueueゲッターの追加6.3
    TArray<FGhostActionData> GetPlaybackQueue() const { return PlaybackQueue; }
private:
    // -----------------------------------------------------------------------
    // DelayedMirror
    // -----------------------------------------------------------------------
    UPROPERTY()
    TObjectPtr<UGhostRecorderComponent> RecorderRef;

    float m_LastMirrorFetchTime;

    void TickDelayedMirror(float DeltaTime);

    // -----------------------------------------------------------------------
    // 移動の持続管理
    // 毎 Tick でこの方向に AddMovementInput を呼び続ける
    // -----------------------------------------------------------------------
    FVector CurrentMoveDirection = FVector::ZeroVector;

    /** 最後に Move レコードを受け取った時刻。MoveExpireTime 超えたらゼロクリア */
    float m_LastMoveReceivedTime;

    /** 毎 Tick で移動入力を維持する処理 */
    void TickMovement(float DeltaTime);

    // -----------------------------------------------------------------------
    // アクション実行
    // -----------------------------------------------------------------------
    void ExecuteAction(const FGhostActionData& Action);
    void ExecuteMove(const FGhostActionData& Action);
    void ExecuteAttack();
    void ExecuteDodge(const FGhostActionData& Action);
    void ExecuteJump();

    // -----------------------------------------------------------------------
    // 攻撃の連打防止（エッジ検出用）
    // -----------------------------------------------------------------------
    bool bPrevAttackingCombat = false;

    // -----------------------------------------------------------------------
    // AnimBP 同期キャッシュ
    // -----------------------------------------------------------------------
    float m_AnimSpeed;
    bool  m_bAnimAttacking;
    bool  m_bAnimDodging;
};
