#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BossShadowSummon.generated.h"

class ABossShadow;

//フェーズ2で分身を見つけ、影を一体だけ呼ぶ。
UCLASS(ClassGroup=(Enemy), meta=(BlueprintSpawnableComponent))
class ECHO_API UBossShadowSummon : public UActorComponent
{
    GENERATED_BODY()

public:
    //一定間隔で分身を確認する。
    UBossShadowSummon();
    //戦闘をやり直す時に影と履歴を片付ける。
    void Reset();

public:
    //召喚する影のBP。影自身では未設定にする。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow")
    TSubclassOf<ABossShadow> m_shadowClass;
    //分身を見つける範囲。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow", meta=(ClampMin="0"))
    float m_range = 3000.f;
    //影を出す本体からの距離。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow", meta=(ClampMin="100"))
    float m_spawnRange = 250.f;
    //影の活動時間の最小値。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow", meta=(ClampMin="1"))
    float m_lifeMin = 30.f;
    //影の活動時間の最大値。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow", meta=(ClampMin="1"))
    float m_lifeMax = 60.f;
    //別の分身へ影を呼ぶまでの最短間隔。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shadow", meta=(ClampMin="0"))
    float m_cooldown = 10.f;
    //現在呼び出している影。
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Shadow")
    TWeakObjectPtr<ABossShadow> m_shadow;

protected:
    //分身とボスの状態に合わせて影を管理する。
    virtual void TickComponent(float _delta, ELevelTick _type, FActorComponentTickFunction* _tick) override;
    //マップを閉じた時にも影を残さない。
    virtual void EndPlay(const EEndPlayReason::Type _reason) override;

private:
    //同じ分身へ何度も影を出さないための履歴。
    TArray<TWeakObjectPtr<AActor>> m_seen;
    //次に影を呼べる時刻。
    double m_nextTime = 0.;
};
