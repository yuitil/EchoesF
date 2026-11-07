#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemySwing.generated.h"

class UAnimMontage;
class UNiagaraSystem;
class UNiagaraComponent;

//剣を実際に振った時だけ、残り線と風切り音を出す。
UCLASS(ClassGroup = (Enemy), meta = (BlueprintSpawnableComponent))
class ECHO_API UEnemySwing : public UActorComponent
{
    GENERATED_BODY()

public:
    //剣の姿勢が更新された後に位置を読む。
    UEnemySwing();

    //敵のメッシュより後に更新する。
    virtual void BeginPlay() override;

    //一振りの開始位置を記録する。
    void Start();

    //見た目専用の通知が付いたモンタージュか調べる。
    bool UsesTrailNotify() const;

    //左右の刃と、光を出す区間を受け取る。
    void StartBlade(UAnimMontage* _move, bool _left, float _start, float _end);

    //モンタージュで斬撃を消す時刻を受け取る。
    void SetSwingEnd(UAnimMontage* _move, float _end);

    //一振りの表示を終える。
    void Stop();

    //剣先の動きに音と残り線を合わせる。
    virtual void TickComponent(float _delta, ELevelTick _type, FActorComponentTickFunction* _tick) override;

    //敵を消した時に残り線も消す。
    virtual void EndPlay(const EEndPlayReason::Type _reason) override;

public:
    //剣先に付けるNiagara。素手の敵は空欄にする。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Swing")
    TObjectPtr<UNiagaraSystem> m_trail;

    //予備動作では出さないための剣先速度。単位はcm/秒。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Swing", meta = (ClampMin = "0"))
    float m_minSpeed = 180.f;

    //瞬間移動やセクション切替を残り線でつながない距離。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Swing", meta = (ClampMin = "1"))
    float m_maxStep = 100.f;

    //刃のどこを追うか。0が根元、1が剣先。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Swing", meta = (ClampMin = "0", ClampMax = "1"))
    float m_tipPart = .9f;

    //刃と振る方向に沿って、一振りに一枚の斬撃を出す。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Swing")
    bool m_bladeArc = false;

    //斬撃を出す手と時間をモンタージュの通知で決める。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Swing")
    bool m_useBladeNotify = false;

    //一振りの開始位置を通知の終わりまで残す。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Swing")
    bool m_fullSwing = false;

    //一振りの風切り音の大きさ。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Swing", meta = (ClampMin = "0", ClampMax = "2"))
    float m_gain = 1.f;

    //剣が通った場所を何秒分だけ見せるか。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Swing", meta = (ClampMin = "0.01", ClampMax = "0.15"))
    float m_sweepTime = .07f;

private:
    //表示中の線を消す。
    void ClearTrail();

    //通知中に実際に通った刃の根元と先端を記録する。
    struct FBladePose
    {
        //この瞬間の刃の根元と先端。
        FVector base, tip;
        double time;
    };

    //斬撃の表示に使う、直近の刃の位置。
    TArray<FBladePose> m_poses;

    //前回から今までに刃が通った範囲を描く。
    void UpdateSweep(FVector _base, FVector _tip);

private:
    //現在の斬撃を出すモンタージュと終了時刻。
    TWeakObjectPtr<UAnimMontage> m_swingMove;

    //斬撃の表示を終える再生位置。
    float m_swingEnd = 0.f;

    //斬撃通知の開始時刻と振る手。
    float m_swingStart = 0.f;

    //モンタージュの通知から斬撃を開始したか。
    bool m_notifyBlade = false;

    //左手の刀を使うか。
    bool m_left = false;

    //前回の剣先。敵の移動量を含めない位置。
    FVector m_lastTip = FVector::ZeroVector;

    //位置を一回以上記録したか。
    bool m_hasTip = false;

    //今回の一振りで音を鳴らしたか。
    bool m_played = false;

    //通知の区間内か。
    bool m_open = false;

private:
    //この敵だけが使う表示中のNiagara。
    UPROPERTY(Transient)
    TObjectPtr<UNiagaraComponent> m_fx;
};
