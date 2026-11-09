#pragma once

#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "BossBladeTrail.generated.h"

//剣を振る間だけ、その手の斬撃を表示する。
UCLASS(meta = (DisplayName = "Boss Blade Trail"))
class ECHO_API UBossBladeTrail : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	//左の刀を振る区間ではオンにする。
	UPROPERTY(EditAnywhere, Category = "Sword")
	bool m_left = false;

	//モンタージュで決めた開始と終了を斬撃へ渡す。
	virtual void NotifyBegin(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim, float _duration, const FAnimNotifyEventReference& _ref) override;
};
