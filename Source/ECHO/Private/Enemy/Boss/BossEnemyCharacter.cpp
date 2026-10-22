#include "Enemy/Boss/BossEnemyCharacter.h"
#include "Enemy/Boss/BossObservation.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "BrainComponent.h"

ABossEnemyCharacter::ABossEnemyCharacter()
{
	m_observation = CreateDefaultSubobject<UBossObservation>(TEXT("Observation"));
}

UBossData* ABossEnemyCharacter::GetBossData() const
{
	return Cast<UBossData>(m_definition);
}

void ABossEnemyCharacter::BeginPlay()
{
	if (!m_definition)
	{
		m_definition = NewObject<UBossData>(this);
	}
	if (!GetBossData())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: Boss requires an EB Boss Definition asset; using single-life fallback."), *GetName());
	}

	Super::BeginPlay();
}

void ABossEnemyCharacter::HandleHealthDepleted()
{
	const UBossData* data = GetBossData();
	if (m_bossPhase == EBossPhase::PhaseOne && data && data->m_bSecondLife)
	{
		SetLife(EBossPhase::Transition);
		m_observation->Freeze();

		if (AAIController* controller = Cast<AAIController>(GetController()))
		{
			if (UBrainComponent* brain = controller->GetBrainComponent()) { brain->StopLogic(TEXT("Phase transition")); }
		}
		CancelAction();
		if (AAIController* controller = Cast<AAIController>(GetController()))
		{
			controller->StopMovement();
		}
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
		SetHealthPool(data->m_phaseTwoHealth);
		float delay = FMath::Max(.2f, data->m_transitionTime);
		if (data->m_transitionMontage)
		{
			delay = FMath::Max(delay, PlayAnimMontage(data->m_transitionMontage));
		}
		GetWorldTimerManager().SetTimer(m_transitionTimer, this, &ABossEnemyCharacter::FinishTransition, delay, false);
		return;
	}
	m_bossPhase = EBossPhase::Dead;
	Super::HandleHealthDepleted();
	m_onLifeChanged.Broadcast(m_bossPhase);
	BossDefeated();
}

void ABossEnemyCharacter::SetLife(EBossPhase _newLife)
{
	m_bossPhase = _newLife;
	SetState(m_bossPhase == EBossPhase::Dead ? EEnemyState::Dead : m_bossPhase == EBossPhase::Transition ? EEnemyState::Suspended : EEnemyState::Alive);
	m_onLifeChanged.Broadcast(m_bossPhase);
}

void ABossEnemyCharacter::FinishTransition()
{
	if (m_bossPhase != EBossPhase::Transition) { return; }

	StopAnimMontage();
	CancelAction();
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->MaxWalkSpeed = m_moveSpeed;
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	SetLife(EBossPhase::PhaseTwo);
	m_readyTime = GetWorld()->GetTimeSeconds() + (GetBossData() ? FMath::Max(0.f, GetBossData()->m_phaseRecovery) : 0.f);
	if (AAIController* controller = Cast<AAIController>(GetController()))
	{
		if (UBrainComponent* brain = controller->GetBrainComponent()) { brain->RestartLogic(); }
	}
}

float ABossEnemyCharacter::GetSpecialDefenseChance() const
{
	const UBossData* data = GetBossData();
	if (data && m_bossPhase == EBossPhase::PhaseOne && GetHealth() > GetMaxHealth() * data->m_counterHealth) { return 0.f; }
	return data ? (m_bossPhase == EBossPhase::PhaseTwo ? data->m_phaseTwoChance : data->m_phaseOneChance) : Super::GetSpecialDefenseChance();
}

bool ABossEnemyCharacter::IsActionAllowed(const FEnemyActionData& _action) const
{
	const UBossData* data = GetBossData();
	if (!data) { return true; }

	const TArray<FName>& allowedActions = m_bossPhase == EBossPhase::PhaseTwo ? data->m_phaseTwoActions : data->m_phaseOneActions;
	return allowedActions.IsEmpty() || allowedActions.Contains(_action.m_actionId);
}

float ABossEnemyCharacter::GetActionWeightMultiplier(const FEnemyActionData& _action) const
{
	return m_bossPhase == EBossPhase::PhaseTwo ? m_observation->AdaptWeight(uint8(_action.m_kind)) : 1.0f;
}

void ABossEnemyCharacter::ObserveTarget(AActor* _target)
{
	if (m_bossPhase == EBossPhase::PhaseOne)
	{
		m_observation->Observe(_target, GetBossData());
	}
}

void ABossEnemyCharacter::OnAcceptedHit(AActor* _attacker)
{
	if (m_bossPhase == EBossPhase::PhaseOne && _attacker == m_combatTarget)
	{
		m_observation->RecordHit();
	}
}

void ABossEnemyCharacter::OnActionEnded()
{
	m_observation->MarkBossActionEnded();
}

void ABossEnemyCharacter::ResetEncounter()
{
	if (!HasAuthority()) { return; }

	GetWorld()->GetTimerManager().ClearTimer(m_transitionTimer);
	Super::ResetEncounter();
	m_observation->Reset();
	SetLife(EBossPhase::PhaseOne);
}

void ABossEnemyCharacter::EndPlay(const EEndPlayReason::Type _reason)
{
	GetWorld()->GetTimerManager().ClearTimer(m_transitionTimer);
	Super::EndPlay(_reason);
}
