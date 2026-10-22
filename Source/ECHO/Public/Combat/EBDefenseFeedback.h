// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Combat/CombatTypes.h"
#include "GameFramework/DamageType.h"
#include "EBDefenseFeedback.generated.h"

UCLASS()
class ECHO_API UEBCombatDamageType : public UDamageType
{
	GENERATED_BODY()
public:
	bool m_heavyAttack = false;
	bool m_bBlockable = true;
};

UCLASS()
class ECHO_API UEBHeavyDamageType : public UEBCombatDamageType
{
	GENERATED_BODY()
public:
	UEBHeavyDamageType()
	{
		m_heavyAttack = true;
	}
};

UCLASS()
class ECHO_API UEBUnblockableDamageType : public UEBCombatDamageType
{
	GENERATED_BODY()
public:
	UEBUnblockableDamageType()
	{
		m_bBlockable = false;
	}
};

UCLASS()
class ECHO_API UEBHeavyUnblockableDamageType : public UEBCombatDamageType
{
	GENERATED_BODY()
public:
	UEBHeavyUnblockableDamageType()
	{
		m_heavyAttack = true;
		m_bBlockable = false;
	}
};

UINTERFACE(BlueprintType, Blueprintable)
class ECHO_API UEBDefenseFeedback : public UInterface
{
	GENERATED_BODY()
};

class ECHO_API IEBDefenseFeedback
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat | Defense")
	void AttackDefended(AActor* _actor, EDefenseResult _result);
};