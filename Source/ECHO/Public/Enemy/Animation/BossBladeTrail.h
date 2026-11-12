#pragma once

#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "BossBladeTrail.generated.h"

//旧モンタージュの目印。斬撃はBossCombatWindowのDamageで表示する。
UCLASS(meta = (DisplayName = "Boss Blade Trail"))
class ECHO_API UBossBladeTrail : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	//左の刀を振る区間ではオンにする。
	UPROPERTY(EditAnywhere, Category = "Sword")
	bool m_left = false;

};
