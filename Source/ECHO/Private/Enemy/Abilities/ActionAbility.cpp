#include "Enemy/Abilities/ActionAbility.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "TimerManager.h"
#include "Engine/World.h"

#include "AbilitySystemComponent.h"

//動作を確かめるための幅や待ち時間。
namespace ActionAbilityTuning
{
    //次の確認までの時間。
    constexpr auto checkDelay = .05f;
}

//初期設定と必要な部品を用意する。
UActionAbility::UActionAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

//攻撃の能力を開始する。
void UActionAbility::ActivateAbility(const FGameplayAbilitySpecHandle _handle, const FGameplayAbilityActorInfo* _actorInfo, const FGameplayAbilityActivationInfo _activationInfo, const FGameplayEventData* _eventData)
{

	m_finished = false;

	//処理の対象となる敵。
	AEnemyCharacterBase* enemy = _actorInfo ? Cast<AEnemyCharacterBase>(_actorInfo->AvatarActor) : nullptr;
	
	//使用する行動の設定。
	const FEnemyActionData* actionData = enemy ? enemy->GetActiveAction() : nullptr;

	if (!actionData || !actionData->m_montage || !CommitAbility(_handle, _actorInfo, _activationInfo))
	{
		EndAbility(_handle, _actorInfo, _activationInfo, true, true);
		return;
	}

	//BTで実行する行動。
	auto* task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, actionData->m_montage, enemy->GetAttackRate(*actionData), NAME_None, true, actionData->m_rootScale);
	task->OnCompleted.AddDynamic(this, &UActionAbility::Completed);
	task->OnInterrupted.AddDynamic(this, &UActionAbility::Interrupted);
	task->OnCancelled.AddDynamic(this, &UActionAbility::Interrupted);

	GetWorld()->GetTimerManager().SetTimer(m_timerHandle, this, &UActionAbility::Interrupted, enemy->GetActionTimeout(*actionData), false);

	task->ReadyForActivation();
}

//再生完了を受けて攻撃を終了する。
void UActionAbility::Completed()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

//通知を受けて攻撃を終了する。
void UActionAbility::FinishFromNotify(float _blend)
{
	if(!IsActive() || m_finished) { return; }
	m_finished = true;
	if (auto* asc = GetAbilitySystemComponentFromActorInfo()) asc->CurrentMontageStop(FMath::Max(ActionAbilityTuning::checkDelay, _blend));
	if (IsActive()) Completed();
}

//再生中断を受けて攻撃を終了する。
void UActionAbility::Interrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, !m_finished);
}

//攻撃の能力を終えて後片付けする。
void UActionAbility::EndAbility(const FGameplayAbilitySpecHandle _handle, const FGameplayAbilityActorInfo* _actorInfo, const FGameplayAbilityActivationInfo _activationInfo, bool _replicateEndAbility, bool _wasCancelled)
{
	if(!IsActive()) { return; }
	GetWorld()->GetTimerManager().ClearTimer(m_timerHandle);

	if (AEnemyCharacterBase* enemy = _actorInfo ? Cast<AEnemyCharacterBase>(_actorInfo->AvatarActor) : nullptr)
	{
		enemy->FinishAction(_wasCancelled);
	}
	Super::EndAbility(_handle, _actorInfo, _activationInfo, _replicateEndAbility, _wasCancelled);
}
