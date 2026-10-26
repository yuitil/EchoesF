#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyAudio.generated.h"

class USoundBase;
class USoundAttenuation;
class USoundConcurrency;

//音を鳴らす場面。
UENUM(BlueprintType)
enum class EEnemySound : uint8
{
    Step, Swing, Hit, Armor, Clash, Break, Draw, Dodge, Land, Death, Rise, Spawn, Fade, Cast, Blast
};

//一つの場面で使う音と大きさ。
USTRUCT(BlueprintType)
struct FEnemySoundSet
{
    GENERATED_BODY()

    //この中から一つ選ぶ音。
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<TObjectPtr<USoundBase>> m_sounds;

    //音の大きさ。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", ClampMax="2"))
    float m_volume = 0.5f;

    //音の高さ。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.25", ClampMax="2"))
    float m_pitch = 1.f;

    //同じ音が重ならないための間隔。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0"))
    float m_gap = 0.1f;
};

//敵の場面に合う音を、BPの設定で鳴らす。
UCLASS(ClassGroup=(Enemy), meta=(BlueprintSpawnableComponent))
class ECHO_API UEnemyAudio : public UActorComponent
{
    GENERATED_BODY()

public:
    //毎フレームの処理は使わない。
    UEnemyAudio();

public:
    //指定した場所で音を鳴らす。
    UFUNCTION(BlueprintCallable, Category="Enemy|Audio")
    void Play(EEnemySound _sound, FVector _point, float _gain = 1.f);

    //全員に同じ場面の音を知らせる。
    UFUNCTION(NetMulticast, Unreliable)
    void Emit(EEnemySound _sound, FVector _point, float _gain);

public:
    //場面ごとの音素材と調整値。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Audio")
    TMap<EEnemySound, FEnemySoundSet> m_cues;

    //遠くで小さくなる設定。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Audio")
    TObjectPtr<USoundAttenuation> m_range;

    //同時に鳴らせる数。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Audio")
    TObjectPtr<USoundConcurrency> m_limit;

    //足音だけの同時再生数。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Audio")
    TObjectPtr<USoundConcurrency> m_stepLimit;

    //同じ音の繰り返しを和らげる高さの変化量。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Audio", meta=(ClampMin="0", ClampMax="0.2"))
    float m_pitchSpread = 0.04f;

    //再生した場面をログに出す。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Audio")
    bool m_debug = false;

private:
    //場面ごとに次の音を鳴らせる時刻。
    TMap<EEnemySound, double> m_nextTime;
};
