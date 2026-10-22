#include "Enemy/Combat/Defense.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Enemy/Combat/Combat.h"
#include "Combat/EBDefenseFeedback.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Player/CombatComponent.h"

UDefense::UDefense()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UDefense::PassesChance(float _roll, float _chance)
{
	return FMath::IsFinite(_roll) && FMath::IsFinite(_chance) && _roll >= 0 && _roll < FMath::Clamp(_chance, 0.0f, 1.0f);
}

bool UDefense::IsBusy() const
{
	return m_bGuardBroken || (GetWorld() && GetWorld()->GetTimeSeconds() < m_guardPose) || (GetWorld() && GetWorld()->GetTimeSeconds() < m_busyUntilTime);
}

bool UDefense::IsFrontFacing(AActor* _attacker) const
{
	const AEnemyCharacterBase* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	if (!enemy || !enemy->m_definition || !IsValid(_attacker) || _attacker == enemy)
	{
		return false;
	}

	const FVector direction = (_attacker->GetActorLocation() - GetOwner()->GetActorLocation()).GetSafeNormal2D();
	if (direction.IsNearlyZero())
	{
		return false;
	}

	const float halfAngle = FMath::Clamp(enemy->m_definition->m_defenseFrontAngle, 0.0f, 360.0f) * .5f;
	return FVector::DotProduct(enemy->GetActorForwardVector().GetSafeNormal2D(), direction) >= FMath::Cos(FMath::DegreesToRadians(halfAngle));
}

bool UDefense::ShouldStartGuard() const
{
	const AEnemyCharacterBase* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	if (!enemy || !enemy->m_definition || !enemy->m_definition->m_useGuard || IsBusy() || enemy->IsActing() || enemy->GetCharacterMovement()->IsFalling()
		|| !IsFrontFacing(enemy->m_combatTarget))
	{
		return false;
	}

	return (IsTargetAttacking(enemy->m_combatTarget) || HasGuardRequest())
		&& FVector::Dist2D(enemy->GetActorLocation(), enemy->m_combatTarget->GetActorLocation()) <= FMath::Max(0.0f, enemy->m_definition->m_guardRange);

}

bool UDefense::IsTargetAttacking_Implementation(AActor* target) const
{
	if (!IsValid(target)) { return false; }
	if (const UCombatComponent* combat = target->FindComponentByClass<UCombatComponent>())
	{
		return combat->IsAttacking() && combat->IsPlayingAttackMontage();
	}
	const AEnemyCharacterBase* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	const ACharacter* character = Cast<ACharacter>(target);
	const UAnimInstance* anim = character && character->GetMesh() ? character->GetMesh()->GetAnimInstance() : nullptr;
	if (!enemy || !enemy->m_definition || !anim) { return false; }
	for (const auto& montage : enemy->m_definition->m_guardAttacks)
	{
		if (montage && anim->Montage_IsPlaying(montage)) { return true; }
	}
	return false;
}


EDefenseResult UDefense::ResolveHit(AActor* _attacker, bool _unblockable, bool _heavyAttack)
{
	AEnemyCharacterBase* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	if (!enemy || !enemy->HasAuthority() || !enemy->m_definition || _unblockable || !enemy->IsCombatActive()
		|| m_bGuardBroken || GetWorld()->GetTimeSeconds() < m_busyUntilTime || enemy->GetCharacterMovement()->IsFalling() || !IsFrontFacing(_attacker))
	{
		ClearHeavyChain();
		return EDefenseResult::None;
	}

	const UEnemyData* data = enemy->m_definition;

	const bool remembered = m_guardTarget.Get() == _attacker && GetWorld()->GetTimeSeconds() < m_guardUntil;
	const bool guard = data->m_useGuard && (enemy->m_combat->m_bGuarding || remembered);


	const FEnemyActionData* action = enemy->GetActiveAction();
	const bool canReact = !enemy->IsActing() || (action && action->m_kind == EEnemyAction::Guard);
	const double now = GetWorld()->GetTimeSeconds();
	const float deflectWeight = FMath::Max(0.0f, data->m_deflectWeight);
	const float parryWeight = FMath::Max(0.0f, data->m_parryWeight);
	const float totalWeight = deflectWeight + parryWeight;

	if (data->m_useSpecialDefense && !_heavyAttack && canReact && now >= m_nextAttemptTime && totalWeight > 0.0f)
	{

		m_nextAttemptTime = now + FMath::Max(0.05f, data->m_defenseInterval);


		const float chance = enemy->GetSpecialDefenseChance();
		if (PassesChance(FMath::FRand(), chance))
		{
			ClearHeavyChain();
			++m_successes;


			const bool deflect = deflectWeight > 0 && (parryWeight <= 0 || FMath::FRand() < deflectWeight / totalWeight);
			const EDefenseResult result = deflect ? EDefenseResult::Deflect : EDefenseResult::Parry;
			BeginReaction(deflect ? data->m_deflectMontage : data->m_parryMontage, deflect ? data->m_deflectRecovery : data->m_parryRecovery);


			m_counterUntil = m_busyUntilTime + data->m_counterTime;
			GetWorld()->GetTimerManager().SetTimer(m_counterTimer, this, &UDefense::TryCounter, .05f, true);

			return result;
		}
	}

	if (guard && canReact)

	{
		const EDefenseResult result = ResolveGuardChain(_attacker, _heavyAttack);
		if (result == EDefenseResult::Guard)
		{
			RememberHit(_attacker);
			enemy->CancelAction();
			if (auto* ai = Cast<AAIController>(enemy->GetController())) ai->StopMovement();
			enemy->GetCharacterMovement()->StopMovementImmediately();
			if (data->m_guardHit) enemy->PlayAnimMontage(data->m_guardHit);
			m_guardPose = GetWorld()->GetTimeSeconds() + data->m_guardHold;
		}
		return result;

	}
	ClearHeavyChain();
	return EDefenseResult::None;
}

bool UDefense::IsObservedHeavyAttack(AActor* _attacker) const
{
	const AEnemyCharacterBase* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	const ACharacter* attacker = Cast<ACharacter>(_attacker);
	UAnimInstance* anim = attacker ? attacker->GetMesh()->GetAnimInstance() : nullptr;
	UAnimMontage* montage = anim ? anim->GetCurrentActiveMontage() : nullptr;

	return enemy && enemy->m_definition && montage && enemy->m_definition->m_observedHeavyMontage.Contains(montage);
}

void UDefense::ClearHeavyChain()
{
	m_guardedHeavyHits = 0;
	m_lastHeavyTime = -1;
	m_heavyAttacker.Reset();

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(m_heavyTimer);
	}
}

EDefenseResult UDefense::ResolveGuardChain(AActor* _attacker, bool _heavyAttack)
{
	AEnemyCharacterBase* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	const UEnemyData* data = enemy ? enemy->m_definition : nullptr;
	if (!data->m_useGuardBreak || !_heavyAttack)
	{
		ClearHeavyChain();
		return EDefenseResult::Guard;
	}

	const float timeout = FMath::Max(0.1f, data->m_heavyTimeout);
	const double now = GetWorld()->GetTimeSeconds();
	if (m_heavyAttacker.Get() != _attacker || m_lastHeavyTime < 0.0 || now - m_lastHeavyTime > timeout)
	{
		m_guardedHeavyHits = 0;
	}
	m_heavyAttacker = _attacker;
	m_lastHeavyTime = now;
	++m_guardedHeavyHits;

	if (m_guardedHeavyHits >= FMath::Max(1, data->m_heavyHitsCount))
	{
		ClearHeavyChain();
		StartGuardBreak();
		return EDefenseResult::GuardBreak;
	}
	GetWorld()->GetTimerManager().SetTimer(m_heavyTimer, this, &UDefense::ClearHeavyChain, timeout, false);
	return EDefenseResult::Guard;
}

void UDefense::StartGuardBreak()
{
	AEnemyCharacterBase* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	const float duration = FMath::Max(0.1f, enemy->m_definition->m_guardBreakTime);
	BeginReaction(enemy->m_definition->m_guardBreakMontage, duration);
	m_bGuardBroken = true;

	m_busyUntilTime = GetWorld()->GetTimeSeconds() + duration;
	enemy->GetCharacterMovement()->DisableMovement();
	GetWorld()->GetTimerManager().SetTimer(m_breakTimer, this, &UDefense::EndGuardBreak, duration, false);
}

void UDefense::EndGuardBreak()
{
	AEnemyCharacterBase* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	m_bGuardBroken = false;
	m_busyUntilTime = 0.0;
	if (!enemy || !enemy->IsCombatActive())
	{
		return;
	}

	if (enemy->m_definition && enemy->m_definition->m_guardBreakMontage)
	{
		enemy->StopAnimMontage(enemy->m_definition->m_guardBreakMontage);
	}

	enemy->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
}

void UDefense::BeginReaction(UAnimMontage* _montage, float _minduration)
{
	AEnemyCharacterBase* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	if (!enemy)
	{
		return;
	}

	enemy->CancelAction();
	if (AAIController* aiController = Cast<AAIController>(enemy->GetController()))
	{
		aiController->StopMovement();
	}

	enemy->GetCharacterMovement()->StopMovementImmediately();

	const float  duration = _montage ? enemy->PlayAnimMontage(_montage) : 0;
	m_busyUntilTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.05f, FMath::Max(_minduration, duration));
}

void UDefense::PublishResult(AActor* _attacker, EDefenseResult _result)
{
	m_lastResult = _result;
	if (_result == EDefenseResult::None)
	{
		return;
	}

	if (IsValid(_attacker) && _attacker->GetClass()->ImplementsInterface(UEBDefenseFeedback::StaticClass()))
	{
		IEBDefenseFeedback::Execute_AttackDefended(_attacker, GetOwner(), _result);
	}
	m_onDefense.Broadcast(_result, _attacker);
}

void UDefense::ReactToDefendedAttack(EDefenseResult _result)
{
	AEnemyCharacterBase* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	if (!enemy || !enemy->IsCombatActive() || !enemy->HasAuthority() || !enemy->m_definition || !enemy->IsActing())
	{
		return;
	}

	if (_result == EDefenseResult::Deflect || _result == EDefenseResult::Parry)
	{
		BeginReaction(enemy->m_definition->m_staggerMontage, _result == EDefenseResult::Deflect ? enemy->m_definition->m_deflectedStun : enemy->m_definition->m_parriedStun);
	}
}

void UDefense::ResetDefense()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(m_breakTimer);
	}

	ClearHeavyChain();

	AEnemyCharacterBase* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	if (enemy && enemy->m_definition && enemy->m_definition->m_guardBreakMontage && m_bGuardBroken)
	{
		enemy->StopAnimMontage(enemy->m_definition->m_guardBreakMontage);
	}

	if (m_bGuardBroken && enemy && enemy->IsCombatActive())
	{
		enemy->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}


	GetWorld()->GetTimerManager().ClearTimer(m_counterTimer);
	m_guardPending = false;
	m_guardUntil = m_guardPose = m_counterUntil = 0.; m_guardTarget.Reset();

	m_bGuardBroken = false;
	m_nextAttemptTime = 0.0;
	m_busyUntilTime = 0.0;
	m_attempts = 0;
	m_successes = 0;
	m_lastResult = EDefenseResult::None;

}

void UDefense::EndPlay(const EEndPlayReason::Type _reason)
{
	ResetDefense();
	Super::EndPlay(_reason);
}


void UDefense::RememberHit(AActor* _attacker)
{
	auto* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	if (!enemy || !enemy->m_definition || !IsValid(_attacker)) return;
	const auto* action = enemy->GetActiveAction();
	if (action && action->m_kind != EEnemyAction::Guard) m_guardPending = true;
	m_guardTarget = _attacker;
	m_guardUntil = GetWorld()->GetTimeSeconds() + enemy->m_definition->m_guardMemory;
}
void UDefense::TryCounter()
{
	auto* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	if (!enemy || !enemy->IsCombatActive() || !enemy->m_definition || GetWorld()->GetTimeSeconds() > m_counterUntil)
	{
		GetWorld()->GetTimerManager().ClearTimer(m_counterTimer); return;
	}
	if (enemy->CanDecide() && !IsTargetAttacking(enemy->m_combatTarget) && !enemy->m_definition->m_counter.IsNone()
		&& enemy->RequestAction(enemy->m_definition->m_counter)) GetWorld()->GetTimerManager().ClearTimer(m_counterTimer);
}

bool UDefense::HasGuardRequest() const
{
	const auto* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	return m_guardPending && enemy && m_guardTarget.IsValid() && m_guardTarget.Get() == enemy->m_combatTarget && GetWorld() && GetWorld()->GetTimeSeconds() < m_guardUntil;
}

void UDefense::AttackFinished()
{
	if (m_guardPending && m_guardTarget.IsValid()) RememberHit(m_guardTarget.Get());
}

