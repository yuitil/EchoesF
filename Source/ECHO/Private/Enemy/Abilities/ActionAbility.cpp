#include "Enemy/Abilities/ActionAbility.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "TimerManager.h"
#include "Engine/World.h"

#include "AbilitySystemComponent.h"


UActionAbility::UActionAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UActionAbility::ActivateAbility(const FGameplayAbilitySpecHandle _handle, const FGameplayAbilityActorInfo* _actorInfo, const FGameplayAbilityActivationInfo _activationInfo, const FGameplayEventData* _eventData)
{

	m_finished = false;

	AEnemyCharacterBase* enemy = _actorInfo ? Cast<AEnemyCharacterBase>(_actorInfo->AvatarActor) : nullptr;
	const FEnemyActionData* actionData = enemy ? enemy->GetActiveAction() : nullptr;

	if (!actionData || !actionData->m_montage || !CommitAbility(_handle, _actorInfo, _activationInfo))
	{
		EndAbility(_handle, _actorInfo, _activationInfo, true, true);
		return;
	}

	auto* task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, actionData->m_montage, enemy->GetAttackRate(*actionData), NAME_None, true, actionData->m_rootScale);
	task->OnCompleted.AddDynamic(this, &UActionAbility::Completed);
	task->OnInterrupted.AddDynamic(this, &UActionAbility::Interrupted);
	task->OnCancelled.AddDynamic(this, &UActionAbility::Interrupted);

	GetWorld()->GetTimerManager().SetTimer(m_timerHandle, this, &UActionAbility::Interrupted, enemy->GetActionTimeout(*actionData), false);

	task->ReadyForActivation();
}

void UActionAbility::Completed()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UActionAbility::FinishFromNotify(float _blend)
{
	if (!IsActive() || m_finished) return;
	m_finished = true;
	if (auto* asc = GetAbilitySystemComponentFromActorInfo()) asc->CurrentMontageStop(FMath::Max(.05f, _blend));
	if (IsActive()) Completed();
}
void UActionAbility::Interrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, !m_finished);
}


void UActionAbility::EndAbility(const FGameplayAbilitySpecHandle _handle, const FGameplayAbilityActorInfo* _actorInfo, const FGameplayAbilityActivationInfo _activationInfo, bool _replicateEndAbility, bool _wasCancelled)
{
	if (!IsActive())
	{
		return;
	}
	GetWorld()->GetTimerManager().ClearTimer(m_timerHandle);

	if (AEnemyCharacterBase* enemy = _actorInfo ? Cast<AEnemyCharacterBase>(_actorInfo->AvatarActor) : nullptr)
	{
		enemy->FinishAction(_wasCancelled);
	}
	Super::EndAbility(_handle, _actorInfo, _activationInfo, _replicateEndAbility, _wasCancelled);
}
