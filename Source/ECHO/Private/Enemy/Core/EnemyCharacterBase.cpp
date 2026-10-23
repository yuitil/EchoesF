#include "Enemy/Core/EnemyCharacterBase.h"
//
#include "Enemy/AI/EnemyTargeting.h"
//
#include "Enemy/Core/EnemyTraversal.h"
#include "Combat/GAS/CombatAbilitySystemComponent.h"
#include "Combat/GAS/CombatAttributeSet.h"
#include "Combat/GAS/CombatGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Enemy/Combat/Combat.h"
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
//
#include "Player/CombatComponent.h"
//

AEnemyCharacterBase::AEnemyCharacterBase()
{
	//
	m_targeting = CreateDefaultSubobject<UEnemyTargeting>(TEXT("Targeting"));
	//
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
	m_abilitySystemComponent = CreateDefaultSubobject<UCombatAbilitySystemComponent>(TEXT("AbilitySystem"));
	m_attributeSet = CreateDefaultSubobject<UCombatAttributeSet>(TEXT("AttributeSet"));
	m_combat = CreateDefaultSubobject<UCombat>(TEXT("Combat"));
	m_traversal = CreateDefaultSubobject<UEnemyTraversal>(TEXT("Traversal"));
	m_defense = CreateDefaultSubobject<UDefense>(TEXT("Defense"));
}

EDefenseResult AEnemyCharacterBase::ResolveTargetDefense_Implementation(AActor* target) const
{
	return EDefenseResult::None;
}

float AEnemyCharacterBase::GetAttackRate(const FEnemyActionData& action) const
{
	const bool attack = action.m_kind != EEnemyAction::Guard && action.m_kind != EEnemyAction::Dodge 
		&& action.m_kind != EEnemyAction::Recovery && action.m_kind != EEnemyAction::Presentaiton;
	return FMath::Clamp(m_definition && m_definition->m_commonRate && attack ? m_definition->m_attackRate : action.m_playRate, .1f, 2.f);
}

float AEnemyCharacterBase::GetDefenseWindow(EDefenseResult result, FName actionId) const
{
	const FEnemyActionData* action = GetActiveAction();
	if (!actionId.IsNone())
	{
		action = m_definition ? m_definition->m_actions.FindByPredicate([actionId](const FEnemyActionData& item) { return item.m_actionId == actionId; }) : nullptr;
	}
	if (!action || action->m_damage <= 0.f) { return 0.f; }
	if (result == EDefenseResult::PerfectDodge)
	{
		return m_definition && m_definition->m_perfectDodge ? FMath::Clamp(action->m_dodgeWindow > 0.f ? action->m_dodgeWindow : m_definition->m_dodgeWindow, 0.f, .5f) : 0.f;
	}
	if (action->m_unblockable) { return 0.f; }
	if (result == EDefenseResult::Parry) { return FMath::Clamp(action->m_parryWindow, 0.f, .5f); }
	if (result == EDefenseResult::Deflect) { return FMath::Clamp(action->m_deflectWindow, 0.f, .5f); }
	return 0.f;
}

bool AEnemyCharacterBase::IsDefenseInputValid(EDefenseResult result, float elapsed) const
{
	const float window = GetDefenseWindow(result);
	return IsActing() && window > 0.f && FMath::IsFinite(elapsed) && elapsed >= 0.f && elapsed <= window;
}

float AEnemyCharacterBase::GetActionTimeout(const FEnemyActionData& action) const
{
	const float slow = m_definition && m_definition->m_perfectDodge ? FMath::Clamp(m_definition->m_slowRate, .1f, 1.f) : 1.f;
	return action.m_montage ? action.m_montage->GetPlayLength() / FMath::Max(.01f, GetAttackRate(action) * action.m_montage->RateScale * slow) + 3.f : 3.f;
}

bool AEnemyCharacterBase::ApplyPerfectDodge(AActor* target)
{
	const auto* action = GetActiveAction();
	auto* anim = GetMesh()->GetAnimInstance();
	if (!HasAuthority() || !IsActing() || !IsValid(target) || target != m_combatTarget || !action || action->m_damage <= 0.f
		|| !m_definition || !m_definition->m_perfectDodge || !anim || !anim->Montage_IsPlaying(action->m_montage) || IsAttackSlowed()) {
		return false;
	}
	m_slowMontage = action->m_montage;
	m_savedRate = anim->Montage_GetPlayRate(m_slowMontage);
	//
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
	//
	anim->Montage_SetPlayRate(m_slowMontage, m_savedRate * FMath::Clamp(m_definition->m_slowRate, .1f, 1.f));
	m_nextActionId = NAME_None;
	m_combat->EndLift();
	return true;
}

void AEnemyCharacterBase::RestoreAttackRate()
{
	if (!m_slowMontage) { return; }
	if (auto* anim = GetMesh()->GetAnimInstance()) { anim->Montage_SetPlayRate(m_slowMontage, m_savedRate); }
	if (m_combat->m_bDebug) { UE_LOG(LogTemp, Display, TEXT("BossCombat RestoreRate rate=%.3f"), m_savedRate); }
	m_slowMontage = nullptr;
}


void AEnemyCharacterBase::Tick(float _delta)
{
	Super::Tick(_delta);
	//
	if (m_slowMontage)
	{
		auto* anim = GetMesh()->GetAnimInstance();
		const float position = anim ? anim->Montage_GetPosition(m_slowMontage) : -1.f;
		if (!anim || !anim->Montage_IsPlaying(m_slowMontage) || position < m_slowStart || position >= m_slowEnd
			|| anim->Montage_GetCurrentSection(m_slowMontage) != m_slowSection)
		{
			RestoreAttackRate();
		}
	}
	//
	const auto* action = GetActiveAction();
	if (!HasAuthority() || !action || action->m_backSpeed <= 0.f || !GetCharacterMovement()->IsMovingOnGround()) { return; }
	//
    const auto* anim = GetMesh()->GetAnimInstance();
    if (!anim || !anim->Montage_IsPlaying(action->m_montage)) return;
    const float time = anim->Montage_GetPosition(action->m_montage);
    //
	if (time >= action->m_backStart && time < action->m_backEnd)
	{
		FNavLocation floor;
		auto* nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
		const FVector next = GetActorLocation() + EvadeDirection(*action) * action->m_backSpeed * FMath::Max(_delta, .1f);
		const float feet = GetActorLocation().Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		if (!nav || !nav->ProjectPointToNavigation(next, floor, FVector(30, 30, 150)) || FMath::Abs(floor.Location.Z - feet) > GetCharacterMovement()->MaxStepHeight)
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

bool AEnemyCharacterBase::CanLiftTarget_Implementation(AActor* target) const
{
	const auto* character = Cast<ACharacter>(target);
	return m_airCombo && IsValid(character) && character->GetCharacterMovement()->IsMovingOnGround();
}


UAbilitySystemComponent* AEnemyCharacterBase::GetAbilitySystemComponent() const
{
	return m_abilitySystemComponent;
}


void AEnemyCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	GetCharacterMovement()->MaxWalkSpeed = m_moveSpeed;
	m_abilitySystemComponent->InitializeCombat(this, this, m_attributeSet);

	if (HasAuthority())
	{
		m_abilitySystemComponent->GiveAbility(FGameplayAbilitySpec(UActionAbility::StaticClass(), 1));
	}

	SetHealthPool(m_definition ? m_definition->m_initialHealth : 600);
	m_healthHandle = m_abilitySystemComponent->GetGameplayAttributeValueChangeDelegate(m_attributeSet->GetHealthAttribute()).AddUObject(this, &AEnemyCharacterBase::HealthChanged);
	m_onHealthChanged.Broadcast(GetHealth(), GetMaxHealth());
}

float AEnemyCharacterBase::GetHealth() const
{
	return m_attributeSet->GetHealth();
}

float AEnemyCharacterBase::GetMaxHealth() const
{
	return m_attributeSet->GetMaxHealth();
}

void AEnemyCharacterBase::SetHealthPool(float _max)
{
	m_abilitySystemComponent->ResetHealth(_max);
}

float AEnemyCharacterBase::TakeDamage(float _damage, const FDamageEvent& _damageEvent, AController* _instigator, AActor* _causer)
{
	if (!HasAuthority() || !FMath::IsFinite(_damage) || _damage <= 0 || !CanBeDamaged() || !IsCombatActive() || m_combat->m_bInvulnerable)
	{
		return 0.0f;
	}

	float accepted = Super::TakeDamage(_damage, _damageEvent, _instigator, _causer);
	if (!FMath::IsFinite(accepted) || accepted <= 0)
	{
		return 0.0f;
	}

	AActor* attacker = _instigator && _instigator->GetPawn() ? _instigator->GetPawn() : _causer;
	if (attacker && attacker->GetOwner() && !Cast<APawn>(attacker))
	{
		attacker = attacker->GetOwner();
	}

	const UEBCombatDamageType* damageInfo = _damageEvent.DamageTypeClass ? Cast<UEBCombatDamageType>(_damageEvent.DamageTypeClass->GetDefaultObject()) : nullptr;
	const bool unblockable = damageInfo && !damageInfo->m_bBlockable;

	const bool heavyAttack = damageInfo ? damageInfo->m_heavyAttack : m_defense->IsObservedHeavyAttack(attacker);
	const EDefenseResult result = m_defense->ResolveHit(attacker, unblockable, heavyAttack);
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

	const float applied = FMath::Min(GetHealth(), accepted);
	OnAcceptedHit(attacker);

	m_defense->RememberHit(attacker);

	m_abilitySystemComponent->ApplyHealthDelta(-accepted, _instigator ? _instigator->GetPawn() : _causer, _causer);
	m_defense->PublishResult(attacker, result);

	//
	if (applied > 0.f && IsValid(attacker))
	{
		if (auto* combat = attacker->FindComponentByClass<UCombatComponent>())
		{
			if (combat->IsPlayingAttackMontage()) combat->OnHitEnemy.Broadcast(applied);
		}
	}
	//
	return applied;
}

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

void AEnemyCharacterBase::HealthChanged(const FOnAttributeChangeData& _data)
{
	m_onHealthChanged.Broadcast(_data.NewValue, GetMaxHealth());
	if (_data.NewValue <= 0.0f && IsCombatActive())
	{
		HandleHealthDepleted();
	}
}

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
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (m_definition && m_definition->m_deathMontage)
	{
		PlayAnimMontage(m_definition->m_deathMontage);
	}

	EnemyDefeted();
}

float AEnemyCharacterBase::GetSpecialDefenseChance() const
{
	return m_definition ? m_definition->m_defenseChance : 0.0f;
}

float AEnemyCharacterBase::GetMeleeDistance() const
{
	const auto* capsule = IsValid(m_combatTarget) ? m_combatTarget->FindComponentByClass<UCapsuleComponent>() : nullptr;
	const float radius = capsule ? capsule->GetScaledCapsuleRadius() : 0.f;
	return FMath::Max(m_meleeDist, GetCapsuleComponent()->GetScaledCapsuleRadius() + radius + m_bodyMargin);
}

bool AEnemyCharacterBase::CanDecide() const
{
	return !IsRepositioning() && !m_traversal->IsTraversing() && IsCombatActive() && !IsActing() && !m_defense->IsBusy() && IsValid(m_combatTarget) && GetWorld()->GetTimeSeconds() >= m_readyTime;
}

bool AEnemyCharacterBase::CanUseAction(int32 _actionIndex, float _distance) const
{
	if (!m_definition || !m_definition->m_actions.IsValidIndex(_actionIndex) || !IsValid(m_combatTarget))
	{
		return false;
	}

	const FEnemyActionData& action = m_definition->m_actions[_actionIndex];
    //
    if (!m_abilitySystemComponent || !m_abilitySystemComponent->HasAllMatchingGameplayTags(action.m_requiredTags)
        || m_abilitySystemComponent->HasAnyMatchingGameplayTags(action.m_blockedTags)) return false;
    //
	const auto* character = Cast<ACharacter>(m_combatTarget.Get());
	if (action.m_airOnly != GetCharacterMovement()->IsFalling()) { return false; }

	if (action.m_targetGround && (!character || !character->GetCharacterMovement()->IsMovingOnGround())) return false;
	if (action.m_reactOnly && !m_defense->IsTargetAttacking(m_combatTarget)) return false;

	if (action.m_targetAir && (!character || !character->GetCharacterMovement()->IsFalling())) { return false; }
	if (FMath::Abs(GetActorLocation().Z - m_combatTarget->GetActorLocation().Z) > action.m_heightGap) { return false; }
	if (action.m_launch && !CanLiftTarget(m_combatTarget)) { return false; }
	if (action.m_backSpeed > 0.f && !CanEvade(action)) return false;

	const double* cooldown = m_actionCooldowns.Find(action.m_actionId);
	const float distance = _distance >= 0.f ? _distance : FVector::Dist2D(GetActorLocation(), m_combatTarget->GetActorLocation());

	return action.m_reviewed && action.m_montage && !action.m_actionId.IsNone() && action.m_kind != EEnemyAction::Presentaiton
		&& IsActionAllowed(action) && (!cooldown || *cooldown <= GetWorld()->GetTimeSeconds()) && distance >= action.m_minRange && distance <= action.m_maxRange;
}

bool AEnemyCharacterBase::CanRushAttack(float _distance) const
{
	if (!CanDecide() || !m_definition || !m_nextActionId.IsNone()) { return false; }
	for (int32 i = 0; i < m_definition->m_actions.Num(); ++i)
	{
		const FEnemyActionData& action = m_definition->m_actions[i];
		if (action.m_comboEntry && action.m_kind != EEnemyAction::Guard && CanUseAction(i, _distance)) { return true; }
	}
	return false;
}

int32 AEnemyCharacterBase::FindComboAction(FName _actionId) const
{
	if (!m_definition || _actionId.IsNone())
	{
		return INDEX_NONE;
	}
	for (int32 i = 0; i < m_definition->m_actions.Num(); ++i)
	{
		if (m_definition->m_actions[i].m_actionId == _actionId && CanUseAction(i) && m_definition->m_actions[i].m_kind != EEnemyAction::Guard)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

int32 AEnemyCharacterBase::ChooseAction() const
{
	if (!m_definition || !m_combatTarget)
	{
		return INDEX_NONE;
	}

	if (m_defense->HasGuardRequest() && m_defense->ShouldStartGuard())
		for (int32 i = 0;i < m_definition->m_actions.Num();++i)
			if (m_definition->m_actions[i].m_kind == EEnemyAction::Guard && CanUseAction(i)) return i;

	const int32 followUp = FindComboAction(m_nextActionId);
	if (followUp != INDEX_NONE)
	{
		return followUp;
	}

	if (m_defense->IsTargetAttacking(m_combatTarget) && FMath::FRand() < m_definition->m_evadeChance)
	{
		TArray<int32> options;
		for (int32 i = 0;i < m_definition->m_actions.Num();++i)
			if (m_definition->m_actions[i].m_kind == EEnemyAction::Dodge && m_definition->m_actions[i].m_reactOnly && CanUseAction(i)) options.Add(i);
		if (!options.IsEmpty()) return options[FMath::RandRange(0, options.Num() - 1)];
	}

	if (m_defense->ShouldStartGuard())
	{
		for (int32 i = 0; i < m_definition->m_actions.Num(); ++i)
		{
			if (m_definition->m_actions[i].m_kind == EEnemyAction::Guard && CanUseAction(i))
			{
				return i;
			}
		}
	}

    //
    TArray<int32> specials;
    for (int32 i=0; i<m_definition->m_actions.Num(); ++i)
        if (m_definition->m_actions[i].m_special && m_definition->m_actions[i].m_weight > 0.f && m_definition->m_actions[i].m_comboEntry && CanUseAction(i)) specials.Add(i);
    if (!specials.IsEmpty() && FMath::FRand() < FMath::Clamp(m_definition->m_specialChance,0.f,1.f))
        return specials[FMath::RandRange(0,specials.Num()-1)];
    //
	float sum = 0;
	TArray<TPair<int32, float>> choices;
	for (int32 i = 0; i < m_definition->m_actions.Num(); ++i)
	{
		const FEnemyActionData& action = m_definition->m_actions[i];

		if (!CanUseAction(i) || action.m_special || !action.m_comboEntry || action.m_kind == EEnemyAction::Guard || action.m_reactOnly)
		{
			continue;
		}

		const float repeat = action.m_actionId == m_lastAction ? FMath::Clamp(m_definition->m_repeatWeight, 0.01f, 1.f) : 1.f;
		//
		const float weight = FMath::Max(0.f, action.m_weight) * GetActionWeightMultiplier(action) * repeat;
		//

		if (weight > 0)
		{
			sum += weight;
			choices.Emplace(i, weight);
		}
	}

	float pick = FMath::FRandRange(0.0f, sum);
	for (const auto& choice : choices)
	{
		pick -= choice.Value;
		if (pick <= 0)
		{
			return choice.Key;
		}
	}

	return choices.IsEmpty() ? INDEX_NONE : choices.Last().Key;
}

FName AEnemyCharacterBase::SelectComboBranch(const FEnemyActionData& _action, bool _hit, bool _blocked)
{
	return _blocked ? _action.m_blockedNextId : (_hit ? _action.m_hitNextId : _action.m_missNextId);
}

void AEnemyCharacterBase::RecordAttackHit()
{
	if (IsActing())
	{
		m_bActionHit = true;
	}
}

void AEnemyCharacterBase::AttackDefended_Implementation(AActor* _defender, EDefenseResult _result)
{
	if (!IsActing() || !HasAuthority() || _defender != m_combatTarget || _result == EDefenseResult::None)
	{
		return;
	}

	if (_result == EDefenseResult::PerfectDodge) { ApplyPerfectDodge(_defender); return; }

	const auto* action = GetActiveAction();
	if (action && action->m_unblockable && (_result == EDefenseResult::Guard || _result == EDefenseResult::Deflect || _result == EDefenseResult::Parry)) { return; }


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

const FEnemyActionData* AEnemyCharacterBase::GetReadyAction(FName _actionId) const
{
	if (!CanDecide()) { return nullptr; }
	const int32 index = FindComboAction(_actionId);
	return index != INDEX_NONE ? &m_definition->m_actions[index] : nullptr;
}


bool AEnemyCharacterBase::RequestAction(FName _actionId)
{
	if (!CanDecide() || !HasAuthority())
	{
		return false;
	}


	m_activeIndex = _actionId.IsNone() ? ChooseAction() : FindComboAction(_actionId);

	if (m_activeIndex == INDEX_NONE)
	{
		m_nextActionId = NAME_None;
		m_comboSteps = 0;
		return false;
	}

	if (m_definition->m_actions[m_activeIndex].m_kind == EEnemyAction::Guard) m_defense->ConsumeGuardRequest();

	const bool continuing = !m_nextActionId.IsNone() && m_definition->m_actions[m_activeIndex].m_actionId == m_nextActionId;
	m_comboSteps = continuing ? m_comboSteps + 1 : 1;
	m_nextActionId = NAME_None;
	m_bActionHit = false;
	m_bActionBlocked = false;

	m_savedSpeed = GetCharacterMovement()->MaxWalkSpeed;
	m_savedTurn = GetCharacterMovement()->bOrientRotationToMovement;
	if (m_definition->m_actions[m_activeIndex].m_backSpeed > 0.f) { GetCharacterMovement()->bOrientRotationToMovement = false; }
	m_actionStart = GetWorld()->GetTimeSeconds();

	++m_actionSerial;
	m_actionSuccess = false;

	const bool airborne = GetCharacterMovement()->IsFalling();
	const float verticalSpeed = GetCharacterMovement()->Velocity.Z;

	if (AAIController* aiController = Cast<AAIController>(GetController()))
	{
		aiController->StopMovement();
	}

	GetCharacterMovement()->StopMovementImmediately();
	if (airborne) { GetCharacterMovement()->Velocity.Z = verticalSpeed; }

	FVector targetDirection = m_combatTarget->GetActorLocation() - GetActorLocation();
	targetDirection.Z = 0;
	if (!targetDirection.IsNearlyZero())
	{
		SetActorRotation(targetDirection.Rotation());
	}

	//
    m_abilitySystemComponent->AddLooseGameplayTags(m_definition->m_actions[m_activeIndex].m_activeTags);
    //
    m_abilitySystemComponent->SetLooseGameplayTagCount(CombatTags::StateActing, 1);
	if (!m_abilitySystemComponent->TryActivateAbilityByClass(UActionAbility::StaticClass()))
	{
		FinishAction(true);
		return false;
	}

	return IsActing();
}

const FEnemyActionData* AEnemyCharacterBase::GetActiveAction() const
{
	if (!m_definition || !m_definition->m_actions.IsValidIndex(m_activeIndex))
	{
		return nullptr;
	}
	return &m_definition->m_actions[m_activeIndex];
}

void AEnemyCharacterBase::FinishAction(bool _cancelled)
{
	const FEnemyActionData* action = GetActiveAction();

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

		if (!_cancelled && action->m_kind != EEnemyAction::Guard && action->m_kind != EEnemyAction::Dodge) { m_lastAction = action->m_actionId; }
		m_actionCooldowns.Add(action->m_actionId, GetWorld()->GetTimeSeconds() + action->m_cooldown);
		
		if (!_cancelled && !dodged && m_definition && m_comboSteps < FMath::Clamp(m_definition->m_maxComboSteps, 1, 10))
		{
			m_nextActionId = SelectComboBranch(*action, m_bActionHit, m_bActionBlocked);
		}

		if (FindComboAction(m_nextActionId) == INDEX_NONE)
		{
			m_nextActionId = NAME_None;
		}

		m_readyTime = GetWorld()->GetTimeSeconds() + (m_nextActionId.IsNone() ? FMath::Max(0.05f, action->m_recovery) : FMath::Max(0.f, action->m_comboGap));
	}

	if (action && action->m_airOnly && m_nextActionId.IsNone()) { m_combat->EndLift(); }

	if (m_nextActionId.IsNone())
	{
		m_comboSteps = 0;
	}

    //
    if (action) m_abilitySystemComponent->RemoveLooseGameplayTags(action->m_activeTags);
    //
	m_activeIndex = INDEX_NONE;
	
	if (!_cancelled) m_defense->AttackFinished();

	m_combat->ResetWindows();
	m_abilitySystemComponent->SetLooseGameplayTagCount(CombatTags::StateActing, 0);
	m_actionSuccess = !_cancelled;
	OnActionEnded();
	//
	if (!_cancelled && action && action->m_damage > 0.f && m_nextActionId.IsNone()) { OnAttackFinished(); }
	//
}

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

void AEnemyCharacterBase::ResetEncounter()
{
	if (!HasAuthority())
	{
		return;
	}
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
	SetHealthPool(m_definition ? m_definition->m_initialHealth : 600);
	if (AAIController* aiController = Cast<AAIController>(GetController()))
	{
		if (UBrainComponent* brain = aiController->GetBrainComponent())
		{
			brain->RestartLogic();
		}
	}
}

void AEnemyCharacterBase::EndPlay(const EEndPlayReason::Type _reason)
{
	m_abilitySystemComponent->GetGameplayAttributeValueChangeDelegate(m_attributeSet->GetHealthAttribute()).Remove(m_healthHandle);
	CancelAction();
	Super::EndPlay(_reason);
}


FVector AEnemyCharacterBase::EvadeDirection(const FEnemyActionData& action) const
{
	FVector forward = IsValid(m_combatTarget) ? (m_combatTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal2D() : GetActorForwardVector();
	return forward.RotateAngleAxis(action.m_evadeAngle, FVector::UpVector);
}
bool AEnemyCharacterBase::CanEvade(const FEnemyActionData& action) const
{
	auto* nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!nav || !action.m_montage || !GetCharacterMovement()->IsMovingOnGround()) return false;
	const float rate = FMath::Max(.1f, GetAttackRate(action) * action.m_montage->RateScale);
	const FVector start = GetActorLocation();
	const FVector delta = EvadeDirection(action) * action.m_backSpeed * FMath::Max(0.f, action.m_backEnd - action.m_backStart) / rate;
	const float feet = start.Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	for (int i = 1;i <= 5;++i)
	{
		FNavLocation floor;
		if (!nav->ProjectPointToNavigation(start + delta * (i / 5.f), floor, FVector(25, 25, 150)) || FMath::Abs(floor.Location.Z - feet) > GetCharacterMovement()->MaxStepHeight) return false;
	}
	FHitResult hit;FCollisionQueryParams params(SCENE_QUERY_STAT(EnemyEvade), false, this);
	return !GetWorld()->SweepSingleByChannel(hit, start + FVector(0, 0, 4), start + delta + FVector(0, 0, 4), FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeCapsule(FMath::Max(1.f, GetCapsuleComponent()->GetScaledCapsuleRadius() - 2), FMath::Max(1.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - 4)), params);
}

