#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "BossFaceTarget.generated.h"

//攻撃のための間にだけ相手へ向き直る。区間はモンタージュで決める。
UCLASS(meta=(DisplayName="BossFaceTarget"))
class ECHO_API UBossFaceTarget : public UAnimNotifyState
{
    GENERATED_BODY()

public:
    //移動補正を始める。
    virtual void NotifyBegin(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim, float _duration, const FAnimNotifyEventReference& _reference) override;

    //移動補正を終える。
    virtual void NotifyEnd(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim, const FAnimNotifyEventReference& _reference) override;

    //通知の間だけ、横方向の向きを少しずつ変える。
    virtual void NotifyTick(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim, float _delta, const FAnimNotifyEventReference& _reference) override;

public:
    //一秒で向き直れる角度。急に背後へ振り向くのを防ぐ。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aim", meta=(ClampMin="0"))
    float m_turnSpeed = 180.f;

    //移動を含む攻撃では、素材の進む方向を相手へ向ける。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aim")
    bool m_aimMove = false;

    //相手との間に残す距離。近づきすぎを防ぐ。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aim", meta=(ClampMin="0"))
    float m_stopRange = 140.f;
};
