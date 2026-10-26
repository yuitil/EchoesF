#include "Enemy/Audio/EnemyAudio.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundConcurrency.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

//足音や命中時だけ呼び出す。
UEnemyAudio::UEnemyAudio()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

//判定を行う側から一度だけ通知する。
void UEnemyAudio::Play(EEnemySound _sound, FVector _point, float _gain)
{
    if(!GetOwner() || !GetOwner()->HasAuthority()) { return; }
    Emit(_sound, _point, _gain);
}

//距離と同時再生数を守って音を鳴らす。
void UEnemyAudio::Emit_Implementation(EEnemySound _sound, FVector _point, float _gain)
{
    if(GetNetMode() == NM_DedicatedServer || !GetWorld() || _point.ContainsNaN()) { return; }

    //今回の音の設定。
    const FEnemySoundSet* cue = m_cues.Find(_sound);
    if(!cue || cue->m_sounds.IsEmpty() || cue->m_volume <= 0.f || _gain <= 0.f) { return; }

    //通知が重なった時は同じ音を連続で鳴らさない。
    const double now = GetWorld()->GetTimeSeconds();
    if(now < m_nextTime.FindRef(_sound)) { return; }
    m_nextTime.Add(_sound, now + FMath::Max(0.f, cue->m_gap));

    //登録した素材から音を一つ選ぶ。
    USoundBase* sound = cue->m_sounds[FMath::RandRange(0, cue->m_sounds.Num() - 1)];
    if(!sound) { return; }

    //音の高さを少しだけ変える。
    const float pitch = FMath::Max(KINDA_SMALL_NUMBER, cue->m_pitch + FMath::FRandRange(-m_pitchSpread, m_pitchSpread));
    UGameplayStatics::PlaySoundAtLocation(this, sound, _point, FRotator::ZeroRotator, cue->m_volume * _gain, pitch, 0.f,
        m_range, _sound == EEnemySound::Step ? m_stepLimit.Get() : m_limit.Get(), GetOwner());
    if(m_debug) { UE_LOG(LogTemp, Display, TEXT("EnemyAudio %s event=%d sound=%s"), *GetNameSafe(GetOwner()), int32(_sound), *GetNameSafe(sound)); }
}
