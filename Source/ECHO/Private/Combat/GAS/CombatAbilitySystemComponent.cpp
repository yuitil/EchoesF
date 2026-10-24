#include "Combat/GAS/CombatAbilitySystemComponent.h"
#include "Combat/GAS/CombatAttributeSet.h"
#include "GameplayEffect.h"

//能力と体力の管理を準備する。
void UCombatAbilitySystemComponent::InitializeCombat(AActor* _ownerActor, AActor* _avatarActor, UCombatAttributeSet* _attribute)
{
	if(!_ownerActor || !_avatarActor || !_attribute) { return; }

	//能力を持つ敵と、能力を実行する相手を結び付ける。
	InitAbilityActorInfo(_ownerActor, _avatarActor);
	AddAttributeSetSubobject(_attribute);
}

//体力を初期値に戻す。
void UCombatAbilitySystemComponent::ResetHealth(float _max)
{
	if(!GetOwner() || !GetOwner()->HasAuthority() || !GetSet<UCombatAttributeSet>() || !FMath::IsFinite(_max)) { return; }

	//最大体力を有効な範囲に収めて全回復する。
	_max = FMath::Max(1.f, _max);
	SetNumericAttributeBase(UCombatAttributeSet::GetMaxHealthAttribute(), _max);
	SetNumericAttributeBase(UCombatAttributeSet::GetHealthAttribute(), _max);
}

//体力の増減を反映する。
float UCombatAbilitySystemComponent::ApplyHealthDelta(float _delta, AActor* _instigator, AActor* _damageCauser)
{
	//体力の値を持つ部品。
	const UCombatAttributeSet* attributeSet = GetSet<UCombatAttributeSet>();
	if(!attributeSet || !GetOwner() || !GetOwner()->HasAuthority() || !FMath::IsFinite(_delta)) { return 0.0f; }

	//実際に反映できた体力の変化量。
	const float accepted = FMath::Clamp( _delta, -attributeSet->GetHealth(), attributeSet->GetMaxHealth() - attributeSet->GetHealth());
	if(FMath::IsNearlyZero(accepted)) { return 0.0f; }

	//体力に適用する効果。
	UGameplayEffect* effect = NewObject<UGameplayEffect>();
	effect->DurationPolicy = EGameplayEffectDurationType::Instant;

	//効果で変更する値の設定。
	FGameplayModifierInfo modifier;
	//体力へ加算する効果として変更量を設定する。
	modifier.Attribute = UCombatAttributeSet::GetHealthAttribute();
	modifier.ModifierOp = EGameplayModOp::Additive;
	modifier.ModifierMagnitude = FScalableFloat(accepted);
	effect->Modifiers.Add(modifier);

	//効果を起こした相手の情報。
	FGameplayEffectContextHandle context = MakeEffectContext();
	context.AddInstigator(_instigator, _damageCauser);
	ApplyGameplayEffectToSelf(effect, 1.f, context);

	return accepted;
}
