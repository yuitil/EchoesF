#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "BossAfterimageWindow.generated.h"

//モンタージュ上の帯で残像を出す時間を指定する。
UCLASS(meta=(DisplayName="Boss Afterimage Window"))
class ECHO_API UBossAfterimageWindow : public UAnimNotifyState
{
    GENERATED_BODY()

public:
    //その場のPowerAttackでも姿勢の残像を出す。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Afterimage")
    bool m_allowStill = false;

    //残像の区間を開始する。
    virtual void NotifyBegin(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _clip, float _time, const FAnimNotifyEventReference& _ref) override;

    //残像の区間を終了する。
    virtual void NotifyEnd(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _clip, const FAnimNotifyEventReference& _ref) override;
};
