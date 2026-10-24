//Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Combat/CombatTypes.h"
#include "GameFramework/DamageType.h"
#include "EBDefenseFeedback.generated.h"

//攻撃が強攻撃か、防御できるかを伝える。
UCLASS()
class ECHO_API UEBCombatDamageType : public UDamageType
{
	GENERATED_BODY()
public:
	//強攻撃として扱うか。
	bool m_heavyAttack = false;
	
	//ガードで防げる攻撃か。
	bool m_bBlockable = true;
};

//防御できる強攻撃を表す。
UCLASS()
class ECHO_API UEBHeavyDamageType : public UEBCombatDamageType
{
	GENERATED_BODY()
public:
	//防御できる強攻撃を表す。
	UEBHeavyDamageType()
	{
		m_heavyAttack = true;
	}
};

//ガードできない攻撃を表す。
UCLASS()
class ECHO_API UEBUnblockableDamageType : public UEBCombatDamageType
{
	GENERATED_BODY()
public:
	//ガードできない攻撃を表す。
	UEBUnblockableDamageType()
	{
		m_bBlockable = false;
	}
};

//ガードできない強攻撃を表す。
UCLASS()
class ECHO_API UEBHeavyUnblockableDamageType : public UEBCombatDamageType
{
	GENERATED_BODY()
public:
	//ガードできない強攻撃を表す。
	UEBHeavyUnblockableDamageType()
	{
		m_heavyAttack = true;
		m_bBlockable = false;
	}
};

//防御の結果を攻撃した相手へ伝える窓口。
UINTERFACE(BlueprintType, Blueprintable)
class ECHO_API UEBDefenseFeedback : public UInterface
{
	GENERATED_BODY()
};

//防御の結果を攻撃した相手へ伝える窓口。
class ECHO_API IEBDefenseFeedback
{
	GENERATED_BODY()

public:
	//攻撃を防がれた結果を受け取る。
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat | Defense")
	void AttackDefended(AActor* _actor, EDefenseResult _result);
};