// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/GAS/CombatAbilitySystemComponent.h"
#include "Combat/GAS/CombatAttributeSet.h"
#include "GameplayEffect.h"

void UCombatAbilitySystemComponent::InitializeCombat(AActor* _ownerActor, AActor* _avatarActor, UCombatAttributeSet* _attribute)
{
	if (!_ownerActor || !_avatarActor || !_attribute) { return; }

	InitAbilityActorInfo(_ownerActor, _avatarActor);
	AddAttributeSetSubobject(_attribute);
}

void UCombatAbilitySystemComponent::ResetHealth(float _max)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !GetSet<UCombatAttributeSet>() || !FMath::IsFinite(_max)) { return; }

	_max = FMath::Max(1.f, _max);
	SetNumericAttributeBase(UCombatAttributeSet::GetMaxHealthAttribute(), _max);
	SetNumericAttributeBase(UCombatAttributeSet::GetHealthAttribute(), _max);
}

float UCombatAbilitySystemComponent::ApplyHealthDelta(float _delta, AActor* _instigator, AActor* _damageCauser)
{
	const UCombatAttributeSet* attributeSet = GetSet<UCombatAttributeSet>();
	if (!attributeSet || !GetOwner() || !GetOwner()->HasAuthority() || !FMath::IsFinite(_delta)) { return 0.0f; }

	const float accepted = FMath::Clamp( _delta, -attributeSet->GetHealth(), attributeSet->GetMaxHealth() - attributeSet->GetHealth());
	if(FMath::IsNearlyZero(accepted)) { return 0.0f; }
	
	UGameplayEffect* effect = NewObject<UGameplayEffect>();
	effect->DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo modifier;
	modifier.Attribute = UCombatAttributeSet::GetHealthAttribute();
	modifier.ModifierOp = EGameplayModOp::Additive;
	modifier.ModifierMagnitude = FScalableFloat(accepted);
	effect->Modifiers.Add(modifier);

	FGameplayEffectContextHandle context = MakeEffectContext();
	context.AddInstigator(_instigator, _damageCauser);
	ApplyGameplayEffectToSelf(effect, 1.f, context);

	return accepted;
}

