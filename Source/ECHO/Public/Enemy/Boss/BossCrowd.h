#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BossCrowd.generated.h"

class ACharacter;
class UNiagaraSystem;
class USoundBase;

//分身に囲まれた時に、周囲を押し返して足場を作る。
UCLASS(ClassGroup = (Enemy), meta = (BlueprintSpawnableComponent))
class ECHO_API UBossCrowd : public UActorComponent
{
    GENERATED_BODY()

public:

    //今の人数と待ち時間で使えるかを調べる。
    UFUNCTION(BlueprintPure, Category = "Crowd")
    bool CanPush() const;

    //BTから呼び、行動できる時だけ専用技を始める。
    UFUNCTION(BlueprintCallable, Category = "Crowd")
    bool TryPush();

    //Montageの通知から一度だけ吹き飛ばす。
    UFUNCTION(BlueprintCallable, Category = "Crowd")
    void Release();

    //戦闘のやり直しで待ち時間を戻す。
    void Reset();

public:

    //囲まれた時の技を使うか。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd")
    bool m_enabled = true;

    //DataAssetに登録する専用技の名前。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd")
    FName m_action = TEXT("CrowdPush");

    //必要なプレイヤー分身の数。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd", meta = (ClampMin = "1"))
    int32 m_count = 4;

    //追跡を止められた時に押し返す、近くの分身の数。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd|Blocked", meta = (ClampMin = "1"))
    int32 m_blockCount = 2;

    //この秒数進めなければ、分身を押し返して道を開ける。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd|Blocked", meta = (ClampMin = "0.5", Units = "s"))
    float m_blockWait = 1.5f;

    //この距離を進めたら、足止めの時間を数え直す。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd|Blocked", meta = (ClampMin = "1", Units = "cm"))
    float m_blockTravel = 30.f;

    //分身を数え、押し返す範囲。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd", meta = (ClampMin = "1", Units = "cm"))
    float m_radius = 500.f;

    //別の階の相手を数えないための高さ。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd", meta = (ClampMin = "1", Units = "cm"))
    float m_height = 180.f;

    //平らな床で飛ばす距離の目安。壁に当たると短くなる。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd", meta = (ClampMin = "300", ClampMax = "500", Units = "cm"))
    float m_distance = 400.f;

    //空中に浮く時間の目安。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd", meta = (ClampMin = "0.2", ClampMax = "1.5", Units = "s"))
    float m_airTime = .7f;

    //次に使えるまでの時間。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd", meta = (ClampMin = "1", Units = "s"))
    float m_cooldown = 12.f;

    //通知の瞬間に出すNiagara。BPで指定する。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd")
    TObjectPtr<UNiagaraSystem> m_fx;

    //通知の瞬間に鳴らす音。BPで指定する。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd")
    TObjectPtr<USoundBase> m_sound;

private:
    //追跡を指示しているのに進めない時間を数える。
    void UpdateBlock();

    //相手が近くにいて壁に遮られていないか。
    bool IsNear(ACharacter* _target) const;

    //対象を重複せず集める。
    void GetTargets(TArray<ACharacter*>& _targets, int32& _clones) const;

private:
    //次に技を使える時刻。
    double m_nextTime = 0.;

    //同じ通知で二度押し返さないための攻撃番号。
    int32 m_lastSerial = INDEX_NONE;

    //追跡が止まり始めた時刻。負の値なら足止めされていない。
    double m_blockSince = -1.;

    //追跡で最後に進んだ位置。
    FVector m_lastPlace = FVector::ZeroVector;
};
