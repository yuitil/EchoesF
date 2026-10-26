#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BossPosture.generated.h"

//HPとは別に体勢の残りと回復を管理する。
UCLASS(ClassGroup=(Enemy), meta=(BlueprintSpawnableComponent))
class ECHO_API UBossPosture : public UActorComponent
{
    GENERATED_BODY()

public:
    //毎フレームの回復を有効にする。
    UBossPosture();

    //受けた攻撃に合わせて体勢を減らす。
    UFUNCTION(BlueprintCallable, Category="Boss|Posture")
    void Hit(float _damage);

    //フェーズ開始時に体勢を戻す。
    UFUNCTION(BlueprintCallable, Category="Boss|Posture")
    void Reset();

    //バーに表示する残りの割合。
    UFUNCTION(BlueprintPure, Category="Boss|Posture")
    float GetRatio() const;

protected:
    //ゲーム開始時に最大値へ戻す。
    virtual void BeginPlay() override;

    //被弾していない間の回復を進める。
    virtual void TickComponent(float _delta, ELevelTick _type, FActorComponentTickFunction* _tick) override;

public:
    //体勢ゲージを使うか。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Posture")
    bool m_enabled = true;

    //体勢の最大値。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Posture", meta=(ClampMin="1"))
    float m_max = 100.f;

    //一回の被弾で減る基本量。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Posture", meta=(ClampMin="0"))
    float m_hitCost = 10.f;

    //ダメージ量に応じて追加する減少量の倍率。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Posture", meta=(ClampMin="0"))
    float m_damageScale = 0.f;

    //最後の被弾から回復開始までの秒数。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Posture", meta=(ClampMin="0"))
    float m_healDelay = 2.f;

    //一秒で回復する体勢の量。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Posture", meta=(ClampMin="0"))
    float m_healSpeed = 20.f;

    //体勢が尽きた時に無防備になる秒数。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Posture", meta=(ClampMin="0.1"))
    float m_breakTime = 2.f;

    //現在の体勢。
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Boss|Posture")
    float m_value = 100.f;

private:
    //最後に攻撃を受けた時刻。
    double m_lastHit = 0.;

    //前回ガードが崩れていたか。
    bool m_wasBroken = false;
};
