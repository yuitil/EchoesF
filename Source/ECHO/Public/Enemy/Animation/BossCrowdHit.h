#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "BossCrowdHit.generated.h"

//Montageで指定した瞬間に、周りの相手を押し返す。
UCLASS(meta=(DisplayName="Boss Crowd Hit"))
class ECHO_API UBossCrowdHit : public UAnimNotify
{
    GENERATED_BODY()

public:
    //ボスのメインのメッシュからだけ実行する。
    virtual void Notify(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim, const FAnimNotifyEventReference& _event) override;
};
