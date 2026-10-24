#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ActionAbility.generated.h"

//敵の攻撃を開始し、終わったら後片付けする。
UCLASS()
class ECHO_API UActionAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	//敵の攻撃を開始し、終わったら後片付けする。
	UActionAbility();

	//攻撃の能力を開始する。
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle _handle, const FGameplayAbilityActorInfo* _actorInfo, const FGameplayAbilityActivationInfo _activationInfo, const FGameplayEventData* _eventData) override;

	//攻撃の能力を終えて後片付けする。
	virtual void EndAbility(const FGameplayAbilitySpecHandle _handle, const FGameplayAbilityActorInfo* _actorInfo, const FGameplayAbilityActivationInfo _activationInfo, bool _replicateEndAbility, bool _wasCancelled) override;

	//通知を受けて攻撃を終了する。
	void FinishFromNotify(float _blend);

private:
	//再生完了を受けて攻撃を終了する。
	UFUNCTION(Category = "Enemy")
	void Completed();

	//再生中断を受けて攻撃を終了する。
	UFUNCTION(Category = "Enemy")
	void Interrupted();

private:
	//攻撃の能力を終了済みか。
	bool m_finished = false;

	//攻撃の終了や待ち時間を管理するタイマー。
	FTimerHandle m_timerHandle;
};
