// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "CombatAbilitySystemComponent.generated.h"

class UCombatAttributeSet;

UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class ECHO_API UCombatAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()
	
public:
	void InitializeCombat(AActor* _ownerActor, AActor* _avatarActor, UCombatAttributeSet* _attribute);

	void ResetHealth(float _max);

	UFUNCTION(BlueprintCallable, Category = "Combat | GAS")
	float ApplyHealthDelta(float _delta, AActor* _instigator, AActor* _damageCauser);
};
