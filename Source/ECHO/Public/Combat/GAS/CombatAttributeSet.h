// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "CombatAttributeSet.generated.h"

#define COMBAT_ATTR(Name, Member) \
    static FGameplayAttribute Get##Name##Attribute() \
    { \
        return FGameplayAttribute(FindFieldChecked<FProperty>(StaticClass(), GET_MEMBER_NAME_CHECKED(UCombatAttributeSet, Member))); \
    } \
    float Get##Name() const \
    { \
        return Member.GetCurrentValue(); \
    } \
    void Set##Name(float _value) \
    { \
        if (UAbilitySystemComponent* abilitySystem = GetOwningAbilitySystemComponent()) \
        { \
            abilitySystem->SetNumericAttributeBase(Get##Name##Attribute(), _value); \
        } \
    } \
    void Init##Name(float _value) \
    { \
        Member.SetBaseValue(_value); \
        Member.SetCurrentValue(_value); \
    }

UCLASS()
class ECHO_API UCombatAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UCombatAttributeSet();
	
	UPROPERTY(BlueprintReadOnly, Category = "Combat", meta = (DisplayName = "Health"))
	FGameplayAttributeData m_health;
	COMBAT_ATTR(Health, m_health)

	UPROPERTY(BlueprintReadOnly, Category = "Combat", meta = (DisplayName = "MaxHealth"))
	FGameplayAttributeData m_maxHealth;
	COMBAT_ATTR(MaxHealth, m_maxHealth)

	virtual void PreAttributeChange(const FGameplayAttribute& _attribute, float& _value) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& _data) override;
};

#undef COMBAT_ATTR

