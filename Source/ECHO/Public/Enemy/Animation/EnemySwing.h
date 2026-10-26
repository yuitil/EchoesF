#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemySwing.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;

//剣を実際に振った時だけ、残り線と風切り音を出す。
UCLASS(ClassGroup=(Enemy), meta=(BlueprintSpawnableComponent))
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

    //一振りの表示を終える。
    void Stop();

    //剣先の動きに音と残り線を合わせる。
    virtual void TickComponent(float _delta, ELevelTick _type, FActorComponentTickFunction* _tick) override;

    //敵を消した時に残り線も消す。
    virtual void EndPlay(const EEndPlayReason::Type _reason) override;

public:
    //剣先に付けるNiagara。素手の敵は空欄にする。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Swing")
    TObjectPtr<UNiagaraSystem> m_trail;

    //予備動作では出さないための剣先速度。単位はcm/秒。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Swing", meta=(ClampMin="0"))
    float m_minSpeed = 180.f;

    //瞬間移動やセクション切替を残り線でつながない距離。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Swing", meta=(ClampMin="1"))
    float m_maxStep = 100.f;

    //刃のどこを追うか。0が根元、1が剣先。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Swing", meta=(ClampMin="0", ClampMax="1"))
    float m_tipPart = .9f;

    //一振りの風切り音の大きさ。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Swing", meta=(ClampMin="0", ClampMax="2"))
    float m_gain = 1.f;

private:
    //表示中の線を消す。
    void ClearTrail();

private:
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
