#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "EnemyGroundHit.generated.h"

class UNiagaraSystem;
class USoundBase;
class UCameraShakeBase;

//地面を叩いた瞬間に、砂煙と音と小さな揺れを出す。
UCLASS(meta=(DisplayName="Enemy Ground Hit"))
class ECHO_API UEnemyGroundHit : public UAnimNotify
{
    GENERATED_BODY()

public:
    //通知を置いたフレームで、足元の床を調べる。
    virtual void Notify(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim, const FAnimNotifyEventReference& _event) override;

public:
    //床を調べる手のボーン。空欄なら敵の前方を使う。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ground")
    FName m_bone;

    //ボーンからの位置。空欄の場合は敵からの位置。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ground")
    FVector m_offset = FVector(100.f, 0.f, 0.f);

    //床を探す上下の距離。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ground", meta=(ClampMin="1"))
    float m_depth = 350.f;

    //煙を床から少し離す高さ。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ground")
    float m_floorGap = 3.f;

    //BPで選ぶ砂煙のNiagara。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ground")
    TObjectPtr<UNiagaraSystem> m_dust;

    //砂煙の大きさ。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ground")
    FVector m_scale = FVector::OneVector;

    //地面を叩く音。BPで選ぶ。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ground")
    TObjectPtr<USoundBase> m_sound;

    //地面を叩く音の大きさ。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ground", meta=(ClampMin="0", ClampMax="2"))
    float m_gain = .6f;

    //BPで作った小さなカメラの揺れ。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ground")
    TSubclassOf<UCameraShakeBase> m_shake;

    //揺れが届く距離。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ground", meta=(ClampMin="1"))
    float m_range = 800.f;
};
