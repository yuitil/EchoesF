#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "BossDrawSword.generated.h"

//刀が手に来る位置に置き、刀を見えるようにする。
UCLASS(meta=(DisplayName="BossDrawSword"))
class ECHO_API UBossDrawSword : public UAnimNotify
{
    GENERATED_BODY()

public:
    //アニメーションの指定位置で処理を呼ぶ。
    virtual void Notify(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim, const FAnimNotifyEventReference& _event) override;
};
