#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Enemy/Audio/EnemyAudio.h"
#include "EnemySoundNotify.generated.h"

//モンタージュや移動アニメーションの指定位置で音を鳴らす。
UCLASS(meta=(DisplayName="Enemy Sound"))
class ECHO_API UEnemySoundNotify : public UAnimNotify
{
    GENERATED_BODY()

public:
    //再生中の敵へ音の場面を伝える。
    virtual void Notify(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim, const FAnimNotifyEventReference& _event) override;

public:
    //鳴らす場面。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio")
    EEnemySound m_sound = EEnemySound::Swing;

    //音を鳴らす足や武器の位置。空欄なら体の中心。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio")
    FName m_socket;

    //この通知だけの音量の倍率。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio", meta=(ClampMin="0", ClampMax="2"))
    float m_gain = 1.f;
};
