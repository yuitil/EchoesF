#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_GhostDodge.generated.h"

UCLASS()
class ECHO_API UGA_GhostDodge : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_GhostDodge();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

};
