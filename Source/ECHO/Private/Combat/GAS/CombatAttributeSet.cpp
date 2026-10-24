#include "Combat/GAS/CombatAttributeSet.h"
#include "GameplayEffectExtension.h"

//初期設定と必要な部品を用意する。
UCombatAttributeSet::UCombatAttributeSet()
{
	InitHealth(100.f);
	InitMaxHealth(100.f);
}

//体力が変わる前に値の範囲を整える。
void UCombatAttributeSet::PreAttributeChange(const FGameplayAttribute& _attribute, float& _value)
{
	Super::PreAttributeChange(_attribute, _value);

	if(_attribute == GetHealthAttribute())
	{
		_value = FMath::Clamp(_value, 0.f, GetMaxHealth());
	}

	if(_attribute == GetMaxHealthAttribute())
	{
		_value = FMath::Max(1.f, _value);
	}
}

//効果で体力が変わった後に値を整える。
void UCombatAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& _data)
{
	Super::PostGameplayEffectExecute(_data);
	if(_data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
	}

	if(_data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		SetMaxHealth(FMath::Max(1.f, GetMaxHealth()));
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
	}
}
