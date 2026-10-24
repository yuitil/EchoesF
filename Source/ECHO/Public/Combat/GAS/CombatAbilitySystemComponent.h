#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "CombatAbilitySystemComponent.generated.h"

class UCombatAttributeSet;

//敵の能力を準備し、体力の増減を反映する。
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class ECHO_API UCombatAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	//能力と体力の管理を準備する。
	void InitializeCombat(AActor* _ownerActor, AActor* _avatarActor, UCombatAttributeSet* _attribute);

	//体力を初期値に戻す。
	void ResetHealth(float _max);

	//体力の増減を反映する。
	UFUNCTION(BlueprintCallable, Category = "Combat | GAS")
	float ApplyHealthDelta(float _delta, AActor* _instigator, AActor* _damageCauser);
};
