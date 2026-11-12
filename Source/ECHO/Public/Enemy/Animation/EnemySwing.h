#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemySwing.generated.h"

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

    //攻撃判定が開いた手だけ斬撃を描く。
    void StartHand(bool left);

    //攻撃判定が閉じた手だけ斬撃を消す。
    void EndHand(bool left);

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

    //風切り音を鳴らす剣先速度。単位はcm/秒。
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
    struct FTrail;

    //表示中の線を消す。
    void ClearTrail(FTrail& hand);

    //通知中に実際に通った刃の根元と先端を記録する。
    struct FBladePose
    {
        //この瞬間の刃の根元と先端。
        FVector base, tip;
        double time;
    };

    //一振りの時間と軌道を、左右の手で別々に持つ。
    struct FTrail
    {
        //斬撃の表示に使う、直近の刃の位置。
        TArray<FBladePose> m_poses;

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

        //この敵だけが使う表示中のNiagara。
        TWeakObjectPtr<UNiagaraComponent> m_fx;
    };

    //右手と左手の表示状態。
    FTrail m_hands[2];

    //通知中の刃の位置を追う。
    void UpdateHand(FTrail& hand, float _delta);

    //刃が通った範囲を描く。
    void UpdateSweep(FTrail& hand, FVector _base, FVector _tip);

    //片方の斬撃を片付ける。
    void CloseHand(FTrail& hand);
};
