#include "Enemy/Animation/BossComboRoute.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"

//初期設定と必要な部品を用意する。
UBossComboRoute::UBossComboRoute()
{
	bIsNativeBranchingPoint = true;
}

//アニメーションの指定位置で処理を呼ぶ。
void UBossComboRoute::Notify(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim, const FAnimNotifyEventReference& _reference)
{
	Super::Notify(_mesh, _anim, _reference);
	//処理の対象となる敵。
	AEnemyCharacterBase* enemy = _mesh ? Cast<AEnemyCharacterBase>(_mesh->GetOwner()) : nullptr;
	
	//対象のモンタージュ。
	UAnimMontage* montage = Cast<UAnimMontage>(_anim);
	if(!enemy || !enemy->HasAuthority() || !enemy->IsCombatActive() || !enemy->IsActing() || !montage) { return; }

	//能力とタグを管理する部品。
	UAbilitySystemComponent* asc = enemy->GetAbilitySystemComponent();
	
	//使用する行動の設定。
	const FEnemyActionData* action = enemy->GetActiveAction();
	if(!asc || !action || action->m_montage != montage || asc->GetCurrentMontage() != montage || asc->GetCurrentMontageSectionName() != m_from) { return; }

	//今のコンボの区間。
	const int32 from = montage->GetSectionIndex(m_from);
	
	//次に使う区間または候補。
	const int32 next = montage->GetSectionIndex(m_next);
	
	//攻撃をやめる時の区間。
	const int32 recover = montage->GetSectionIndex(m_recover);
	if (from == INDEX_NONE || next == INDEX_NONE || recover == INDEX_NONE || next <= recover || recover != from + 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("BossComboRoute: expected From, Recovery, then Next in %s"), *GetNameSafe(montage));
		return;
	}

	//狙っている相手。
	AActor* target = enemy->m_combatTarget;
	
	//攻撃が届く範囲内か。
	const bool inRange = IsValid(target) && FVector::Dist2D(enemy->GetActorLocation(), target->GetActorLocation()) <= FMath::Max(0.0f, m_range);
	
	//コンボを続けるか。
	const bool chain = inRange && FMath::FRand() < FMath::Clamp(m_chance, 0.0f, 1.0f);
	asc->CurrentMontageSetNextSectionName(m_from, chain ? m_next : m_recover);
}
