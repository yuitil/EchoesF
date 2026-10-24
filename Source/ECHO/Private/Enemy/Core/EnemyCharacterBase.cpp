#include "Enemy/Core/EnemyCharacterBase.h"
#include "Enemy/AI/EnemyTargeting.h"

#include "Enemy/Core/EnemyTraversal.h"
#include "Combat/GAS/CombatAbilitySystemComponent.h"
#include "Combat/GAS/CombatAttributeSet.h"
#include "Combat/GAS/CombatGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Enemy/Combat/Combat.h"

#include "Enemy/Combat/EnemyHitEffect.h"

#include "Enemy/Combat/Defense.h"
#include "Enemy/Abilities/ActionAbility.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "BrainComponent.h"
#include "AIController.h"
#include "TimerManager.h"
#include "NavigationSystem.h"

#include "Player/CombatComponent.h"

//判定に使う幅や待ち時間。
namespace EnemyCharacterBaseTuning
{
    //体の当たり判定の半径。
    constexpr auto bodyRadius = 42.f;
    
	//体の当たり判定の半分の高さ。
    constexpr auto bodyHalfHeight = 96.0f;
    
	//行動終了を待つ時間の余裕。
    constexpr auto timeoutMargin = 3.f;
    
	//歩ける場所を探す横幅。
    constexpr auto navWidth = 30;
   
	//歩ける場所を探す高さ。
    constexpr auto navHeight = 150;
   
	//初期体力の基準。
    constexpr auto baseHealth = 600;
    
	//続けられるコンボの段数。
    constexpr auto maxComboSteps = 10;
    
	//攻撃後の待ち時間の下限。
    constexpr auto minRecovery = 0.05f;
    
	//経路を調べる分割数。
    constexpr auto pathSteps = 5;
    
	//経路を調べる分割数。
    constexpr auto pathStepCount = 5.f;
    
	//経路の横幅。
    constexpr auto pathWidth = 25;
    
	//地面との距離に持たせる余裕。
    constexpr auto floorMargin = 4;
    
	//計算に使う最小の刻み幅。
    constexpr auto minStep = .1f;
    
	//再生速度の下限。
    constexpr auto minRate = .01f;
    
	//抽選に使う重みの下限。
    constexpr auto minWeight = 0.01f;
}

//初期設定と必要な部品を用意する。
AEnemyCharacterBase::AEnemyCharacterBase()
{

	m_targeting = CreateDefaultSubobject<UEnemyTargeting>(TEXT("Targeting"));

	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(EnemyCharacterBaseTuning::bodyRadius, EnemyCharacterBaseTuning::bodyHalfHeight);
	m_abilitySystemComponent = CreateDefaultSubobject<UCombatAbilitySystemComponent>(TEXT("AbilitySystem"));
	m_attributeSet = CreateDefaultSubobject<UCombatAttributeSet>(TEXT("AttributeSet"));
	m_combat = CreateDefaultSubobject<UCombat>(TEXT("Combat"));
	m_traversal = CreateDefaultSubobject<UEnemyTraversal>(TEXT("Traversal"));
	m_defense = CreateDefaultSubobject<UDefense>(TEXT("Defense"));

	m_hitFx = CreateDefaultSubobject<UEnemyHitEffect>(TEXT("HitFX"));

}

//相手の防御結果を確認する標準処理。
EDefenseResult AEnemyCharacterBase::ResolveTargetDefense_Implementation(AActor* _target) const
{
	return EDefenseResult::None;
}

//設定に合わせた攻撃の再生速度を返す。
float AEnemyCharacterBase::GetAttackRate(const FEnemyActionData& _action) const
{
	//対象となる攻撃。
	const bool attack = _action.m_kind != EEnemyAction::Guard && _action.m_kind != EEnemyAction::Dodge
		&& _action.m_kind != EEnemyAction::Recovery && _action.m_kind != EEnemyAction::Presentaiton;
	return FMath::Clamp(m_definition && m_definition->m_commonRate && attack ? m_definition->m_attackRate : _action.m_playRate, EnemyCharacterBaseTuning::minStep, 2.f);
}

//攻撃に対して防御できる時間の幅を返す。
float AEnemyCharacterBase::GetDefenseWindow(EDefenseResult _result, FName _actionId) const
{
	//使用する行動の設定。
	const FEnemyActionData* action = GetActiveAction();
	if (!_actionId.IsNone())
	{
		action = m_definition ? m_definition->m_actions.FindByPredicate([_actionId](const FEnemyActionData& item) { return item.m_actionId == _actionId; }) : nullptr;
	}
	if(!action || action->m_damage <= 0.f) { return 0.f; }
	if(_result == EDefenseResult::PerfectDodge) 
	{ 
		return m_definition && m_definition->m_perfectDodge ? FMath::Clamp(action->m_dodgeWindow > 0.f ? action->m_dodgeWindow : m_definition->m_dodgeWindow, 0.f, .5f) : 0.f; 
	}
	if(action->m_unblockable) { return 0.f; }
	if(_result == EDefenseResult::Parry) { return FMath::Clamp(action->m_parryWindow, 0.f, .5f); }
	if(_result == EDefenseResult::Deflect) { return FMath::Clamp(action->m_deflectWindow, 0.f, .5f); }
	return 0.f;
}

//防御の入力タイミングが有効か調べる。
bool AEnemyCharacterBase::IsDefenseInputValid(EDefenseResult _result, float _elapsed) const
{
	//動作の判定に使う時間の幅。
	const float window = GetDefenseWindow(_result);
	return IsActing() && window > 0.f && FMath::IsFinite(_elapsed) && _elapsed >= 0.f && _elapsed <= window;
}

//攻撃が終わるまで待つ時間を求める。
float AEnemyCharacterBase::GetActionTimeout(const FEnemyActionData& _action) const
{
	//攻撃の減速に使う値。
	const float slow = m_definition && m_definition->m_perfectDodge ? FMath::Clamp(m_definition->m_slowRate, EnemyCharacterBaseTuning::minStep, 1.f) : 1.f;
	return _action.m_montage ? _action.m_montage->GetPlayLength() / FMath::Max(EnemyCharacterBaseTuning::minRate, GetAttackRate(_action) * _action.m_montage->RateScale * slow) + EnemyCharacterBaseTuning::timeoutMargin : EnemyCharacterBaseTuning::timeoutMargin;
}

//ジャスト回避された攻撃を減速する。
bool AEnemyCharacterBase::ApplyPerfectDodge(AActor* _target)
{
	//使用する行動の設定。
	const auto* action = GetActiveAction();
	
	//アニメーションを再生する部品。
	auto* anim = GetMesh()->GetAnimInstance();
	if(!HasAuthority() || !IsActing() || !IsValid(_target) || _target != m_combatTarget || !action || action->m_damage <= 0.f
		|| !m_definition || !m_definition->m_perfectDodge || !anim || !anim->Montage_IsPlaying(action->m_montage) || IsAttackSlowed()) { return false; }
	m_slowMontage = action->m_montage;
	m_savedRate = anim->Montage_GetPlayRate(m_slowMontage);

	//現在の位置や再生位置。
	const float position = anim->Montage_GetPosition(m_slowMontage);
	m_slowStart = 0.f;
	m_slowEnd = m_slowMontage->GetPlayLength();
	m_slowSection = anim->Montage_GetCurrentSection(m_slowMontage);
	for (const auto& track : m_slowMontage->SlotAnimTracks)
	{
		if (const auto* segment = track.AnimTrack.GetSegmentAtTime(position))
		{
			m_slowStart = segment->StartPos;
			m_slowEnd = segment->StartPos + segment->GetLength();
			break;
		}
	}

	anim->Montage_SetPlayRate(m_slowMontage, m_savedRate * FMath::Clamp(m_definition->m_slowRate, EnemyCharacterBaseTuning::minStep, 1.f));
	m_nextActionId = NAME_None;
	m_combat->EndLift();
	return true;
}

//攻撃の再生速度を通常に戻す。
void AEnemyCharacterBase::RestoreAttackRate()
{
	if(!m_slowMontage) { return; }
	if (auto* anim = GetMesh()->GetAnimInstance()) { anim->Montage_SetPlayRate(m_slowMontage, m_savedRate); }
	if (m_combat->m_bDebug) { UE_LOG(LogTemp, Display, TEXT("BossCombat RestoreRate rate=%.3f"), m_savedRate); }
	m_slowMontage = nullptr;
}

//経過時間に合わせて敵の状態を更新する。
void AEnemyCharacterBase::Tick(float _delta)
{
	Super::Tick(_delta);

	if (m_slowMontage)
	{
		//アニメーションを再生する部品。
		auto* anim = GetMesh()->GetAnimInstance();
		
		//現在の位置や再生位置。
		const float position = anim ? anim->Montage_GetPosition(m_slowMontage) : -1.f;
		if (!anim || !anim->Montage_IsPlaying(m_slowMontage) || position < m_slowStart || position >= m_slowEnd
			|| anim->Montage_GetCurrentSection(m_slowMontage) != m_slowSection)
		{
			RestoreAttackRate();
		}
	}

	//使用する行動の設定。
	const auto* action = GetActiveAction();
	if(!HasAuthority() || !action || action->m_backSpeed <= 0.f || !GetCharacterMovement()->IsMovingOnGround()) { return; }

    //アニメーションを再生する部品。
    const auto* anim = GetMesh()->GetAnimInstance();
    if(!anim || !anim->Montage_IsPlaying(action->m_montage)) { return; }
    
	//処理に使う時間。
    const float time = anim->Montage_GetPosition(action->m_montage);

	if (time >= action->m_backStart && time < action->m_backEnd)
	{
		//地面に当たった情報。
		FNavLocation floor;
		
		//歩ける場所を調べる機能。
		auto* nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
		
		//次に使う区間または候補。
		const FVector next = GetActorLocation() + EvadeDirection(*action) * action->m_backSpeed * FMath::Max(_delta, EnemyCharacterBaseTuning::minStep);
		
		//足元の位置。
		const float feet = GetActorLocation().Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		if (!nav || !nav->ProjectPointToNavigation(next, floor, FVector(EnemyCharacterBaseTuning::navWidth, EnemyCharacterBaseTuning::navWidth, EnemyCharacterBaseTuning::navHeight)) || FMath::Abs(floor.Location.Z - feet) > GetCharacterMovement()->MaxStepHeight)
		{
			GetCharacterMovement()->StopMovementImmediately();
			return;
		}
		GetCharacterMovement()->MaxWalkSpeed = action->m_backSpeed;
		AddMovementInput(EvadeDirection(*action), 1.f);
	}
	else
	{
		GetCharacterMovement()->StopMovementImmediately();
	}
}

//相手を打ち上げられるか調べる標準処理。
bool AEnemyCharacterBase::CanLiftTarget_Implementation(AActor* _target) const
{
	//対象のキャラクター。
	const auto* character = Cast<ACharacter>(_target);
	return m_airCombo && IsValid(character) && character->GetCharacterMovement()->IsMovingOnGround();
}

//敵の能力を管理する部品を返す。
UAbilitySystemComponent* AEnemyCharacterBase::GetAbilitySystemComponent() const
{
	return m_abilitySystemComponent;
}

//ゲーム開始時に必要な状態を準備する。
void AEnemyCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	GetCharacterMovement()->MaxWalkSpeed = m_moveSpeed;
	m_abilitySystemComponent->InitializeCombat(this, this, m_attributeSet);

	if (HasAuthority())
	{
		m_abilitySystemComponent->GiveAbility(FGameplayAbilitySpec(UActionAbility::StaticClass(), 1));
	}

	SetHealthPool(m_definition ? m_definition->m_initialHealth : EnemyCharacterBaseTuning::baseHealth);
	m_healthHandle = m_abilitySystemComponent->GetGameplayAttributeValueChangeDelegate(m_attributeSet->GetHealthAttribute()).AddUObject(this, &AEnemyCharacterBase::HealthChanged);
	m_onHealthChanged.Broadcast(GetHealth(), GetMaxHealth());
}

//現在の体力を返す。
float AEnemyCharacterBase::GetHealth() const
{
	return m_attributeSet->GetHealth();
}

//最大体力を返す。
float AEnemyCharacterBase::GetMaxHealth() const
{
	return m_attributeSet->GetMaxHealth();
}

//体力を設定する。
void AEnemyCharacterBase::SetHealthPool(float _max)
{
	m_abilitySystemComponent->ResetHealth(_max);
}

//防御を確認して体力を減らす。
float AEnemyCharacterBase::TakeDamage(float _damage, const FDamageEvent& _damageEvent, AController* _instigator, AActor* _causer)
{
	if(!HasAuthority() || !FMath::IsFinite(_damage) || _damage <= 0 || !CanBeDamaged() || !IsCombatActive() || m_combat->m_bInvulnerable) { return 0.0f; }

	//実際に反映できた体力の変化量。
	float accepted = Super::TakeDamage(_damage, _damageEvent, _instigator, _causer);
	if(!FMath::IsFinite(accepted) || accepted <= 0) { return 0.0f; }

	//攻撃してきた相手。
	AActor* attacker = _instigator && _instigator->GetPawn() ? _instigator->GetPawn() : _causer;
	if (attacker && attacker->GetOwner() && !Cast<APawn>(attacker))
	{
		attacker = attacker->GetOwner();
	}

	//受けたダメージの情報。
	const UEBCombatDamageType* damageInfo = _damageEvent.DamageTypeClass ? Cast<UEBCombatDamageType>(_damageEvent.DamageTypeClass->GetDefaultObject()) : nullptr;
	
	//ガードできない攻撃か。
	const bool unblockable = damageInfo && !damageInfo->m_bBlockable;

	//強攻撃として扱うか。
	const bool heavyAttack = damageInfo ? damageInfo->m_heavyAttack : m_defense->IsObservedHeavyAttack(attacker);
	
	//処理の結果。
	const EDefenseResult result = m_defense->ResolveHit(attacker, unblockable, heavyAttack);
	
	//防御で受けた時は体の命中と別の火花を出す。
	if(result == EDefenseResult::Guard || result == EDefenseResult::Deflect || result == EDefenseResult::Parry)
	{
		m_hitFx->PlayClash(attacker);
	}

	if (result == EDefenseResult::Deflect || result == EDefenseResult::Parry)
	{
		m_defense->PublishResult(attacker, result);
		return 0.0f;
	}

	if (result == EDefenseResult::Guard)

	{
		accepted *= FMath::Clamp(m_definition->m_guardDamageScale, 0.0f, 1.0f);
	}

	if (accepted <= 0.0f)
	{
		m_defense->PublishResult(attacker, result);
		return 0.0f;
	}

	//反映したダメージ。
	const float applied = FMath::Min(GetHealth(), accepted);

	if (applied > 0.f && result != EDefenseResult::Guard)
	{
		//ぶつかった場所の情報。
		const FHitResult* hit = _damageEvent.IsOfType(FPointDamageEvent::ClassID) ? &static_cast<const FPointDamageEvent&>(_damageEvent).HitInfo : nullptr;
		m_hitFx->PlayHit(attacker, hit);
	}

	OnAcceptedHit(attacker);

	m_defense->RememberHit(attacker);

	m_abilitySystemComponent->ApplyHealthDelta(-accepted, _instigator ? _instigator->GetPawn() : _causer, _causer);
	m_defense->PublishResult(attacker, result);

	if (applied > 0.f && IsValid(attacker))
	{
		if (auto* combat = attacker->FindComponentByClass<UCombatComponent>())
		{
			if (combat->IsPlayingAttackMontage()) combat->OnHitEnemy.Broadcast(applied);
		}
	}

	return applied;
}

//敵の行動状態を切り替える。
void AEnemyCharacterBase::SetState(EEnemyState _newState)
{
	m_state = _newState;
	if (!IsCombatActive())
	{
		m_defense->ResetDefense();
	}

	m_abilitySystemComponent->SetLooseGameplayTagCount(CombatTags::StateDead, m_state == EEnemyState::Dead ? 1 : 0);
	m_abilitySystemComponent->SetLooseGameplayTagCount(CombatTags::StateSuspended, m_state == EEnemyState::Suspended ? 1 : 0);
	m_onStateChanged.Broadcast(m_state);
}

//体力の変化を受けて死亡などを確認する。
void AEnemyCharacterBase::HealthChanged(const FOnAttributeChangeData& _data)
{
	m_onHealthChanged.Broadcast(_data.NewValue, GetMaxHealth());
	if (_data.NewValue <= 0.0f && IsCombatActive())
	{
		HandleHealthDepleted();
	}
}

//体力がなくなった時の処理を行う。
void AEnemyCharacterBase::HandleHealthDepleted()
{
	SetState(EEnemyState::Dead);
	CancelAction();
	SetCanBeDamaged(false);

	if (AAIController* aiController = Cast<AAIController>(GetController()))
	{
		aiController->StopMovement();

		if (UBrainComponent* brain = aiController->GetBrainComponent())
		{
			brain->StopLogic(TEXT("Dead"));
		}
	}
	
	//死亡後に移動や接触が残らないように止める。
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	//死亡用のモンタージュが設定されていれば再生する。
	if (m_definition && m_definition->m_deathMontage)
	{
		PlayAnimMontage(m_definition->m_deathMontage);
	}

	EnemyDefeted();
}

//受け流しなどを選ぶ確率を返す。
float AEnemyCharacterBase::GetSpecialDefenseChance() const
{
	return m_definition ? m_definition->m_defenseChance : 0.0f;
}

//剣が届く間合いを返す。
float AEnemyCharacterBase::GetMeleeDistance() const
{
	//キャラクターの当たり判定。
	const auto* capsule = IsValid(m_combatTarget) ? m_combatTarget->FindComponentByClass<UCapsuleComponent>() : nullptr;
	
	//判定に使う半径。
	const float radius = capsule ? capsule->GetScaledCapsuleRadius() : 0.f;
	return FMath::Max(m_meleeDist, GetCapsuleComponent()->GetScaledCapsuleRadius() + radius + m_bodyMargin);
}

//次の行動を判断できる状態か調べる。
bool AEnemyCharacterBase::CanDecide() const
{
	return !IsRepositioning() && !m_traversal->IsTraversing() && IsCombatActive() && !IsActing() && !m_defense->IsBusy() && IsValid(m_combatTarget) && GetWorld()->GetTimeSeconds() >= m_readyTime;
}

//攻撃を使う条件がそろっているか調べる。
bool AEnemyCharacterBase::CanUseAction(int32 _actionIndex, float _distance) const
{
	if(!m_definition || !m_definition->m_actions.IsValidIndex(_actionIndex) || !IsValid(m_combatTarget)) { return false; }

	//使用する行動の設定。
	const FEnemyActionData& action = m_definition->m_actions[_actionIndex];

    //| !m_abilitySystemComponent->HasAllMatchingGameplayTags|必要なタグがない場合や、禁止タグがある場合は選ばない。
    if(!m_abilitySystemComponent || !m_abilitySystemComponent->HasAllMatchingGameplayTags(action.m_requiredTags)
        || m_abilitySystemComponent->HasAnyMatchingGameplayTags(action.m_blockedTags)) { return false; }

	//対象のキャラクター。
	const auto* character = Cast<ACharacter>(m_combatTarget.Get());
	//地上・空中・相手の状態が攻撃の条件に合うか調べる。
	if(action.m_airOnly != GetCharacterMovement()->IsFalling()) { return false; }

	if(action.m_targetGround && (!character || !character->GetCharacterMovement()->IsMovingOnGround())) { return false; }
	if(action.m_reactOnly && !m_defense->IsTargetAttacking(m_combatTarget)) { return false; }

	if(action.m_targetAir && (!character || !character->GetCharacterMovement()->IsFalling())) { return false; }
	if(FMath::Abs(GetActorLocation().Z - m_combatTarget->GetActorLocation().Z) > action.m_heightGap) { return false; }
	if(action.m_launch && !CanLiftTarget(m_combatTarget)) { return false; }
	if(action.m_backSpeed > 0.f && !CanEvade(action)) { return false; }

	//再使用までの待ち時間。
	const double* cooldown = m_actionCooldowns.Find(action.m_actionId);
	
	//相手までの距離。
	const float distance = _distance >= 0.f ? _distance : FVector::Dist2D(GetActorLocation(), m_combatTarget->GetActorLocation());

	return action.m_reviewed && action.m_montage && !action.m_actionId.IsNone() && action.m_kind != EEnemyAction::Presentaiton
		&& IsActionAllowed(action) && (!cooldown || *cooldown <= GetWorld()->GetTimeSeconds()) && distance >= action.m_minRange && distance <= action.m_maxRange;
}

//突進攻撃を開始できるか調べる。
bool AEnemyCharacterBase::CanRushAttack(float _distance) const
{
	if(!CanDecide() || !m_definition || !m_nextActionId.IsNone()) { return false; }
	for (int32 i = 0; i < m_definition->m_actions.Num(); ++i)
	{
		//使用する行動の設定。
		const FEnemyActionData& action = m_definition->m_actions[i];
		if(action.m_comboEntry && action.m_kind != EEnemyAction::Guard && CanUseAction(i, _distance)) { return true; }
	}
	return false;
}

//次のコンボの設定を探す。
int32 AEnemyCharacterBase::FindComboAction(FName _actionId) const
{
	if(!m_definition || _actionId.IsNone()) { return INDEX_NONE; }
	for (int32 i = 0; i < m_definition->m_actions.Num(); ++i)
	{
		if(m_definition->m_actions[i].m_actionId == _actionId && CanUseAction(i) && m_definition->m_actions[i].m_kind != EEnemyAction::Guard) { return i; }
	}
	return INDEX_NONE;
}

//距離と状態に合う攻撃を抽選する。
int32 AEnemyCharacterBase::ChooseAction() const
{
	if(!m_definition || !m_combatTarget) { return INDEX_NONE; }

	//攻撃された後のガード予約があれば先に使う。
	if (m_defense->HasGuardRequest() && m_defense->ShouldStartGuard())
		for (int32 i = 0; i < m_definition->m_actions.Num(); ++i)
		{
			if(m_definition->m_actions[i].m_kind == EEnemyAction::Guard && CanUseAction(i)) 
			{ 
				return i; 
			}
		}

	//次につなぐ攻撃。
	const int32 followUp = FindComboAction(m_nextActionId);
	if(followUp != INDEX_NONE) { return followUp; }

	//相手が攻撃中なら、設定した確率で回避を選ぶ。
	if (m_defense->IsTargetAttacking(m_combatTarget) && FMath::FRand() < m_definition->m_evadeChance)
	{
		//選択できる候補。
		TArray<int32> options;
		for (int32 i = 0;i < m_definition->m_actions.Num();++i)
			if (m_definition->m_actions[i].m_kind == EEnemyAction::Dodge && m_definition->m_actions[i].m_reactOnly && CanUseAction(i))
			{
				options.Add(i);
			}
		if(!options.IsEmpty()) { return options[FMath::RandRange(0, options.Num() - 1)]; }
	}

	//ガードが必要な時はガード用の行動を探す。
	if (m_defense->ShouldStartGuard())
	{
		for (int32 i = 0; i < m_definition->m_actions.Num(); ++i)
		{
			if(m_definition->m_actions[i].m_kind == EEnemyAction::Guard && CanUseAction(i)) 
			{
				return i;
			}
		}
	}

    //必殺技の候補。
    TArray<int32> specials;
	for (int32 i = 0; i < m_definition->m_actions.Num(); ++i)
	{
		if (m_definition->m_actions[i].m_special && m_definition->m_actions[i].m_weight > 0.f && m_definition->m_actions[i].m_comboEntry && CanUseAction(i))
		{
			specials.Add(i);
		}
	}

    if(!specials.IsEmpty() && FMath::FRand() < FMath::Clamp(m_definition->m_specialChance,0.f,1.f)) 
	{ 
		return specials[FMath::RandRange(0,specials.Num()-1)];
	}

	//抽選に使う値の合計。
	float sum = 0;
	
	//今回抽選する候補。
	TArray<TPair<int32, float>> choices;
	for (int32 i = 0; i < m_definition->m_actions.Num(); ++i)
	{
		//使用する行動の設定。
		const FEnemyActionData& action = m_definition->m_actions[i];

		//| action.m_special|開始条件を満たさない行動は抽選から外す。
		if (!CanUseAction(i) || action.m_special || !action.m_comboEntry || action.m_kind == EEnemyAction::Guard || action.m_reactOnly)
		{
			continue;
		}

		//同じ攻撃が続く時の選ばれやすさの倍率。
		const float repeat = action.m_actionId == m_lastAction ? FMath::Clamp(m_definition->m_repeatWeight, EnemyCharacterBaseTuning::minWeight, 1.f) : 1.f;

		//抽選での選ばれやすさ。
		const float weight = FMath::Max(0.f, action.m_weight) * GetActionWeightMultiplier(action) * repeat;

		if (weight > 0)
		{
			sum += weight;
			choices.Emplace(i, weight);
		}
	}

	//抽選で選ぶための値。
	float pick = FMath::FRandRange(0.0f, sum);
	for (const auto& choice : choices)
	{
		pick -= choice.Value;
		if(pick <= 0) { return choice.Key; }
	}

	return choices.IsEmpty() ? INDEX_NONE : choices.Last().Key;
}

//状況に合わせて次のコンボを選ぶ。
FName AEnemyCharacterBase::SelectComboBranch(const FEnemyActionData& _action, bool _hit, bool _blocked)
{
	return _blocked ? _action.m_blockedNextId : (_hit ? _action.m_hitNextId : _action.m_missNextId);
}

//今回の攻撃が命中したことを記録する。
void AEnemyCharacterBase::RecordAttackHit()
{
	if (IsActing())
	{
		m_bActionHit = true;
	}
}

//ガード崩れのエフェクトを表示する。
void AEnemyCharacterBase::ShowGuardBreak_Implementation()
{
	if(GetNetMode() != NM_DedicatedServer) { GuardBreakFX(); }
}

//攻撃を防がれた結果を受け取る。
void AEnemyCharacterBase::AttackDefended_Implementation(AActor* _defender, EDefenseResult _result)
{
	if(!IsActing() || !HasAuthority() || _defender != m_combatTarget || _result == EDefenseResult::None) { return; }

	//ジャスト回避された時は、現在の攻撃だけ減速する。
	if (_result == EDefenseResult::PerfectDodge) 
	{ 
		ApplyPerfectDodge(_defender); 
		return; 
	}

	//使用する行動の設定。
	const auto* action = GetActiveAction();
	if(action && action->m_unblockable && (_result == EDefenseResult::Guard || _result == EDefenseResult::Deflect || _result == EDefenseResult::Parry)) { return; }

	//相手のガードを崩せた時は命中として記録する。
	if (_result == EDefenseResult::GuardBreak)
	{
		m_bActionBlocked = false;
		m_bActionHit = true;
	}
	else
	{
		m_bActionBlocked = true;
	}
	m_defense->ReactToDefendedAttack(_result);
}

//今選べる攻撃を返す。
const FEnemyActionData* AEnemyCharacterBase::GetReadyAction(FName _actionId) const
{
	if(!CanDecide()) { return nullptr; }
	
	//配列の中の位置。
	const int32 index = FindComboAction(_actionId);
	return index != INDEX_NONE ? &m_definition->m_actions[index] : nullptr;
}

//指定した攻撃の実行を依頼する。
bool AEnemyCharacterBase::RequestAction(FName _actionId)
{
	if(!CanDecide() || !HasAuthority()) { return false; }

	m_activeIndex = _actionId.IsNone() ? ChooseAction() : FindComboAction(_actionId);

	if (m_activeIndex == INDEX_NONE)
	{
		m_nextActionId = NAME_None;
		m_comboSteps = 0;
		return false;
	}

	if (m_definition->m_actions[m_activeIndex].m_kind == EEnemyAction::Guard) m_defense->ConsumeGuardRequest();

	//コンボを続けるか。
	const bool continuing = !m_nextActionId.IsNone() && m_definition->m_actions[m_activeIndex].m_actionId == m_nextActionId;
	m_comboSteps = continuing ? m_comboSteps + 1 : 1;
	m_nextActionId = NAME_None;
	m_bActionHit = false;
	m_bActionBlocked = false;

	m_savedSpeed = GetCharacterMovement()->MaxWalkSpeed;
	m_savedTurn = GetCharacterMovement()->bOrientRotationToMovement;
	if (m_definition->m_actions[m_activeIndex].m_backSpeed > 0.f) 
	{ 
		GetCharacterMovement()->bOrientRotationToMovement = false; 
	}
	m_actionStart = GetWorld()->GetTimeSeconds();

	++m_actionSerial;
	m_actionSuccess = false;

	//空中にいるか。
	const bool airborne = GetCharacterMovement()->IsFalling();
	
	//上下方向の速度。
	const float verticalSpeed = GetCharacterMovement()->Velocity.Z;

	if (AAIController* aiController = Cast<AAIController>(GetController()))
	{
		aiController->StopMovement();
	}

	GetCharacterMovement()->StopMovementImmediately();
	if (airborne) { GetCharacterMovement()->Velocity.Z = verticalSpeed; }

	//相手へ向かう方向。
	FVector targetDirection = m_combatTarget->GetActorLocation() - GetActorLocation();
	targetDirection.Z = 0;
	if (!targetDirection.IsNearlyZero())
	{
		SetActorRotation(targetDirection.Rotation());
	}

    m_abilitySystemComponent->AddLooseGameplayTags(m_definition->m_actions[m_activeIndex].m_activeTags);

    m_abilitySystemComponent->SetLooseGameplayTagCount(CombatTags::StateActing, 1);
	if (!m_abilitySystemComponent->TryActivateAbilityByClass(UActionAbility::StaticClass()))
	{
		FinishAction(true);
		return false;
	}

	return IsActing();
}

//現在実行している攻撃の設定を返す。
const FEnemyActionData* AEnemyCharacterBase::GetActiveAction() const
{
	if(!m_definition || !m_definition->m_actions.IsValidIndex(m_activeIndex)) { return nullptr; }
	return &m_definition->m_actions[m_activeIndex];
}

//攻撃を終えて次の行動に備える。
void AEnemyCharacterBase::FinishAction(bool _cancelled)
{
	//使用する行動の設定。
	const FEnemyActionData* action = GetActiveAction();

	//相手が回避したか。
	const bool dodged = IsAttackSlowed();
	RestoreAttackRate();

	if (action && action->m_backSpeed > 0.f)
	{
		GetCharacterMovement()->MaxWalkSpeed = m_savedSpeed;
		GetCharacterMovement()->bOrientRotationToMovement = m_savedTurn;
		GetCharacterMovement()->StopMovementImmediately();
	}

	m_nextActionId = NAME_None;

	if (action)
	{

		if (!_cancelled && action->m_kind != EEnemyAction::Guard && action->m_kind != EEnemyAction::Dodge) 
		{ 
			m_lastAction = action->m_actionId;
		}
		m_actionCooldowns.Add(action->m_actionId, GetWorld()->GetTimeSeconds() + action->m_cooldown);

		if (!_cancelled && !dodged && m_definition && m_comboSteps < FMath::Clamp(m_definition->m_maxComboSteps, 1, EnemyCharacterBaseTuning::maxComboSteps))
		{
			m_nextActionId = SelectComboBranch(*action, m_bActionHit, m_bActionBlocked);
		}

		if (FindComboAction(m_nextActionId) == INDEX_NONE)
		{
			m_nextActionId = NAME_None;
		}

		m_readyTime = GetWorld()->GetTimeSeconds() + (m_nextActionId.IsNone() ? FMath::Max(EnemyCharacterBaseTuning::minRecovery, action->m_recovery) : FMath::Max(0.f, action->m_comboGap));
	}

	if (action && action->m_airOnly && m_nextActionId.IsNone()) 
	{ 
		m_combat->EndLift(); 
	}

	if (m_nextActionId.IsNone())
	{
		m_comboSteps = 0;
	}

    if (action) m_abilitySystemComponent->RemoveLooseGameplayTags(action->m_activeTags);

	m_activeIndex = INDEX_NONE;

	if (!_cancelled) { m_defense->AttackFinished(); }

	m_combat->ResetWindows();
	m_abilitySystemComponent->SetLooseGameplayTagCount(CombatTags::StateActing, 0);
	m_actionSuccess = !_cancelled;
	OnActionEnded();

	if (!_cancelled && action && action->m_damage > 0.f && m_nextActionId.IsNone()) { OnAttackFinished(); }

}

//現在の攻撃を中断する。
void AEnemyCharacterBase::CancelAction()
{
	m_combat->EndLift();

	m_abilitySystemComponent->CancelAbilities();
	if (IsActing())
	{
		FinishAction(true);
	}

	m_nextActionId = NAME_None;
	m_comboSteps = 0;
	m_combat->ResetWindows();
}

//次の戦闘に向けて状態を初期化する。
void AEnemyCharacterBase::ResetEncounter()
{
	if(!HasAuthority()) { return; }
	CancelAction();
	StopAnimMontage();
	m_actionCooldowns.Empty();

	m_lastAction = NAME_None;

	m_defense->ResetDefense();
	m_readyTime = 0.0;
	m_combatTarget = nullptr;
	SetCanBeDamaged(true);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	SetState(EEnemyState::Alive);
	SetHealthPool(m_definition ? m_definition->m_initialHealth : EnemyCharacterBaseTuning::baseHealth);
	if (AAIController* aiController = Cast<AAIController>(GetController()))
	{
		if (UBrainComponent* brain = aiController->GetBrainComponent())
		{
			brain->RestartLogic();
		}
	}
}

//終了時にタイマーや登録を解除する。
void AEnemyCharacterBase::EndPlay(const EEndPlayReason::Type _reason)
{
	m_abilitySystemComponent->GetGameplayAttributeValueChangeDelegate(m_attributeSet->GetHealthAttribute()).Remove(m_healthHandle);
	CancelAction();
	Super::EndPlay(_reason);
}

//回避する方向を求める。
FVector AEnemyCharacterBase::EvadeDirection(const FEnemyActionData& _action) const
{
	//正面の方向。
	FVector forward = IsValid(m_combatTarget) ? (m_combatTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal2D() : GetActorForwardVector();
	return forward.RotateAngleAxis(_action.m_evadeAngle, FVector::UpVector);
}

//障害物を避けて回避できるか調べる。
bool AEnemyCharacterBase::CanEvade(const FEnemyActionData& _action) const
{
	//歩ける場所を調べる機能。
	auto* nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if(!nav || !_action.m_montage || !GetCharacterMovement()->IsMovingOnGround()) { return false; }
	
	//使用する再生速度。
	const float rate = FMath::Max(EnemyCharacterBaseTuning::minStep, GetAttackRate(_action) * _action.m_montage->RateScale);
	
	//調べ始める位置。
	const FVector start = GetActorLocation();
	
	//前回からの経過時間や値の差。
	const FVector delta = EvadeDirection(_action) * _action.m_backSpeed * FMath::Max(0.f, _action.m_backEnd - _action.m_backStart) / rate;
	
	//足元の位置。
	const float feet = start.Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	for (int i = 1;i <= EnemyCharacterBaseTuning::pathSteps;++i)
	{
		//地面に当たった情報。
		FNavLocation floor;
		if(!nav->ProjectPointToNavigation(start + delta * (i / EnemyCharacterBaseTuning::pathStepCount), floor, FVector(EnemyCharacterBaseTuning::pathWidth, EnemyCharacterBaseTuning::pathWidth, EnemyCharacterBaseTuning::navHeight)) || FMath::Abs(floor.Location.Z - feet) > GetCharacterMovement()->MaxStepHeight) { return false; }
	}
	
	//ぶつかった場所の情報。
	FHitResult hit;FCollisionQueryParams params(SCENE_QUERY_STAT(EnemyEvade), false, this);
	return !GetWorld()->SweepSingleByChannel(hit, start + FVector(0, 0, EnemyCharacterBaseTuning::floorMargin), start + delta + FVector(0, 0, EnemyCharacterBaseTuning::floorMargin), FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeCapsule(FMath::Max(1.f, GetCapsuleComponent()->GetScaledCapsuleRadius() - 2), FMath::Max(1.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - EnemyCharacterBaseTuning::floorMargin)), params);
}
