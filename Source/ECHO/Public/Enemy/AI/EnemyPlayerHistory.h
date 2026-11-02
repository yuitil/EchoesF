#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EnemyPlayerHistory.generated.h"

class UAnimMontage;

//ボスが出る前から、プレイヤーの攻撃の癖を少量だけ覚える。
UCLASS()
class ECHO_API UEnemyPlayerHistory : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    //ゲーム開始後に観察を始める。
    virtual void OnWorldBeginPlay(UWorld& _world) override;

    //マップ終了時に観察を止める。
    virtual void Deinitialize() override;

public:
    //記録した攻撃数。直近の傾向だけを残す。
    UPROPERTY(BlueprintReadOnly, Category="Enemy|History")
    int32 m_attacks = 0;

    //前の攻撃と同じ技を使う割合。
    UPROPERTY(BlueprintReadOnly, Category="Enemy|History")
    float m_repeatRate = 0.f;

    //攻撃を始める間隔の平均。
    UPROPERTY(BlueprintReadOnly, Category="Enemy|History")
    float m_attackGap = 0.f;

private:
    //プレイヤー本体の再生中の攻撃だけを読む。
    void Sample();

private:
    //観察する間隔を管理する。
    FTimerHandle m_timer;

    //前回観察したプレイヤー。
    TWeakObjectPtr<AActor> m_player;

    //前に開始した攻撃。
    TWeakObjectPtr<UAnimMontage> m_attack;

    //現在再生している攻撃。
    TWeakObjectPtr<UAnimMontage> m_playing;

    //前の再生位置。
    float m_position = -1.f;

    //前の攻撃開始時刻。
    double m_lastAttack = -1.;
};
