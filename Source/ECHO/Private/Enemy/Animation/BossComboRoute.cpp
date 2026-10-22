#include "Enemy/Animation/BossComboRoute.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"

UBossComboRoute::UBossComboRoute()
{
	bIsNativeBranchingPoint = true;
}

void UBossComboRoute::Notify(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim, const FAnimNotifyEventReference& _reference)
{
	Super::Notify(_mesh, _anim, _reference);
	AEnemyCharacterBase* enemy = _mesh ? Cast<AEnemyCharacterBase>(_mesh->GetOwner()) : nullptr;
	UAnimMontage* montage = Cast<UAnimMontage>(_anim);
	if (!enemy || !enemy->HasAuthority() || !enemy->IsCombatActive() || !enemy->IsActing() || !montage)
	{
		return;
	}

	UAbilitySystemComponent* asc = enemy->GetAbilitySystemComponent();
	const FEnemyActionData* action = enemy->GetActiveAction();
	if (!asc || !action || action->m_montage != montage || asc->GetCurrentMontage() != montage || asc->GetCurrentMontageSectionName() != m_from)
	{
		return;
	}

	const int32 from = montage->GetSectionIndex(m_from);
	const int32 next = montage->GetSectionIndex(m_next);
	const int32 recover = montage->GetSectionIndex(m_recover);
	if (from == INDEX_NONE || next == INDEX_NONE || recover == INDEX_NONE || next <= recover || recover != from + 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("BossComboRoute: expected From, Recovery, then Next in %s"), *GetNameSafe(montage));
		return;
	}

	AActor* target = enemy->m_combatTarget;
	const bool inRange = IsValid(target) && FVector::Dist2D(enemy->GetActorLocation(), target->GetActorLocation()) <= FMath::Max(0.0f, m_range);
	const bool chain = inRange && FMath::FRand() < FMath::Clamp(m_chance, 0.0f, 1.0f);
	asc->CurrentMontageSetNextSectionName(m_from, chain ? m_next : m_recover);
}
