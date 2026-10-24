#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "CombatAttributeSet.generated.h"

//体力の取得・変更・初期化の関数をまとめて用意する。
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
        if(UAbilitySystemComponent* abilitySystem = GetOwningAbilitySystemComponent()) \
        { \
            abilitySystem->SetNumericAttributeBase(Get##Name##Attribute(), _value); \
        } \
    } \
    void Init##Name(float _value) \
    { \
        Member.SetBaseValue(_value); \
        Member.SetCurrentValue(_value); \
    }

//体力と最大体力を管理する。
UCLASS()
class ECHO_API UCombatAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	//体力と最大体力を管理する。
	UCombatAttributeSet();

public:
	//現在の体力。
	UPROPERTY(BlueprintReadOnly, Category = "Combat", meta = (DisplayName = "Health"))
	FGameplayAttributeData m_health;

	//最大体力。
	UPROPERTY(BlueprintReadOnly, Category = "Combat", meta = (DisplayName = "MaxHealth"))
	FGameplayAttributeData m_maxHealth;

public:
	//この体力項目の読み書き用の関数を作る。
	COMBAT_ATTR(Health, m_health)

	//この体力項目の読み書き用の関数を作る。
	COMBAT_ATTR(MaxHealth, m_maxHealth)

	//体力が変わる前に値の範囲を整える。
	virtual void PreAttributeChange(const FGameplayAttribute& _attribute, float& _value) override;

	//効果で体力が変わった後に値を整える。
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& _data) override;
};

#undef COMBAT_ATTR
