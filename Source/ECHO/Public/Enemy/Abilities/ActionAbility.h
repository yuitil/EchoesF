#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ActionAbility.generated.h"


UCLASS()
class ECHO_API UActionAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UActionAbility();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle _handle, const FGameplayAbilityActorInfo* _actorInfo, const FGameplayAbilityActivationInfo _activationInfo, const FGameplayEventData* _eventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle _handle, const FGameplayAbilityActorInfo* _actorInfo, const FGameplayAbilityActivationInfo _activationInfo, bool _replicateEndAbility, bool _wasCancelled) override;


	void FinishFromNotify(float _blend);
private:
	bool m_finished = false;

	UFUNCTION(Category = "Enemy")
	void Completed();
	
	UFUNCTION(Category = "Enemy")
	void Interrupted();

	FTimerHandle m_timerHandle;
};
