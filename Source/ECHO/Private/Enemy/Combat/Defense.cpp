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

//動作を確かめるための幅や待ち時間。
namespace DefenseTuning
{
    //一回転の角度。
    constexpr auto fullTurn = 360.0f;
    //判定時間の下限。
    constexpr auto minWindow = 0.05f;
    //姿勢を保つ時間の下限。
    constexpr auto minHold = 0.1f;
}

//初期設定と必要な部品を用意する。
UDefense::UDefense()
{
	PrimaryComponentTick.bCanEverTick = false;
}

//抽選の値が設定した確率に入るか調べる。
bool UDefense::PassesChance(float _roll, float _chance)
{
	return FMath::IsFinite(_roll) && FMath::IsFinite(_chance) && _roll >= 0 && _roll < FMath::Clamp(_chance, 0.0f, 1.0f);
}

//防御の反応中で次の行動を待つ必要があるか返す。
bool UDefense::IsBusy() const
{
	return m_bGuardBroken || (GetWorld() && GetWorld()->GetTimeSeconds() < m_guardPose) || (GetWorld() && GetWorld()->GetTimeSeconds() < m_busyUntilTime);
}

//相手が防御できる正面の範囲にいるか調べる。
bool UDefense::IsFrontFacing(AActor* _attacker) const
{
	//処理の対象となる敵。
	const AEnemyCharacterBase* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	if(!enemy || !enemy->m_definition || !IsValid(_attacker) || _attacker == enemy) { return false; }

	//移動や判定に使う方向。
	const FVector direction = (_attacker->GetActorLocation() - GetOwner()->GetActorLocation()).GetSafeNormal2D();
	if(direction.IsNearlyZero()) { return false; }

	//防御できる角度の半分。
	const float halfAngle = FMath::Clamp(enemy->m_definition->m_defenseFrontAngle, 0.0f, DefenseTuning::fullTurn) * .5f;
	return FVector::DotProduct(enemy->GetActorForwardVector().GetSafeNormal2D(), direction) >= FMath::Cos(FMath::DegreesToRadians(halfAngle));
}

//相手の攻撃に備えてガードするか調べる。
bool UDefense::ShouldStartGuard() const
{
	//処理の対象となる敵。
	const AEnemyCharacterBase* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	if(!enemy || !enemy->m_definition || !enemy->m_definition->m_useGuard || IsBusy() || enemy->IsActing() || enemy->GetCharacterMovement()->IsFalling()
		|| !IsFrontFacing(enemy->m_combatTarget)) { return false; }

	return (IsTargetAttacking(enemy->m_combatTarget) || HasGuardRequest())
		&& FVector::Dist2D(enemy->GetActorLocation(), enemy->m_combatTarget->GetActorLocation()) <= FMath::Max(0.0f, enemy->m_definition->m_guardRange);

}

//相手が攻撃中か調べる。
bool UDefense::IsTargetAttacking_Implementation(AActor* _target) const
{
	if(!IsValid(_target)) { return false; }
	if(const UCombatComponent* combat = _target->FindComponentByClass<UCombatComponent>()) { return combat->IsAttacking() && combat->IsPlayingAttackMontage(); }
	//処理の対象となる敵。
	const AEnemyCharacterBase* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	//対象のキャラクター。
	const ACharacter* character = Cast<ACharacter>(_target);
	//アニメーションを再生する部品。
	const UAnimInstance* anim = character && character->GetMesh() ? character->GetMesh()->GetAnimInstance() : nullptr;
	if(!enemy || !enemy->m_definition || !anim) { return false; }
	for (const auto& montage : enemy->m_definition->m_guardAttacks)
	{
		if(montage && anim->Montage_IsPlaying(montage)) { return true; }
	}
	return false;
}

//命中した攻撃を防げるか判断する。
EDefenseResult UDefense::ResolveHit(AActor* _attacker, bool _unblockable, bool _heavyAttack)
{
	//処理の対象となる敵。
	AEnemyCharacterBase* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	if (!enemy || !enemy->HasAuthority() || !enemy->m_definition || _unblockable || !enemy->IsCombatActive()
		|| m_bGuardBroken || GetWorld()->GetTimeSeconds() < m_busyUntilTime || enemy->GetCharacterMovement()->IsFalling() || !IsFrontFacing(_attacker))
	{
		ClearHeavyChain();
		return EDefenseResult::None;
	}

	//BPから設定した敵のデータ。
	const UEnemyData* data = enemy->m_definition;

	//前の攻撃を覚えている相手か。
	const bool remembered = m_guardTarget.Get() == _attacker && GetWorld()->GetTimeSeconds() < m_guardUntil;
	//通常ガードで防げる状態か。
	const bool guard = data->m_useGuard && (enemy->m_combat->m_bGuarding || remembered);

	//使用する行動の設定。
	const FEnemyActionData* action = enemy->GetActiveAction();
	//攻撃を中断せず防御できるか。
	const bool canReact = !enemy->IsActing() || (action && action->m_kind == EEnemyAction::Guard);
	//現在の時刻。
	const double now = GetWorld()->GetTimeSeconds();
	//弾きの選ばれやすさ。
	const float deflectWeight = FMath::Max(0.0f, data->m_deflectWeight);
	//受け流しの選ばれやすさ。
	const float parryWeight = FMath::Max(0.0f, data->m_parryWeight);
	//選ばれやすさの合計。
	const float totalWeight = deflectWeight + parryWeight;

	//使える状態と間隔を確認してから、特殊防御を抽選する。
	if (data->m_useSpecialDefense && !_heavyAttack && canReact && now >= m_nextAttemptTime && totalWeight > 0.0f)
	{

		//連続で抽選し続けないよう、次の時刻を決める。
		m_nextAttemptTime = now + FMath::Max(DefenseTuning::minWindow, data->m_defenseInterval);

		//抽選に使う確率。
		const float chance = enemy->GetSpecialDefenseChance();
		if (PassesChance(FMath::FRand(), chance))
		{
			ClearHeavyChain();
			++m_successes;

			//弾きを選んだか。
			const bool deflect = deflectWeight > 0 && (parryWeight <= 0 || FMath::FRand() < deflectWeight / totalWeight);
			//処理の結果。
			const EDefenseResult result = deflect ? EDefenseResult::Deflect : EDefenseResult::Parry;
			BeginReaction(deflect ? data->m_deflectMontage : data->m_parryMontage, deflect ? data->m_deflectRecovery : data->m_parryRecovery);

			m_counterUntil = m_busyUntilTime + data->m_counterTime;
			GetWorld()->GetTimerManager().SetTimer(m_counterTimer, this, &UDefense::TryCounter, DefenseTuning::minWindow, true);

			return result;
		}
	}

	if (guard && canReact)

	{
		//処理の結果。
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

//相手の再生中の動作が強攻撃か調べる。
bool UDefense::IsObservedHeavyAttack(AActor* _attacker) const
{
	//処理の対象となる敵。
	const AEnemyCharacterBase* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	//攻撃してきた相手。
	const ACharacter* attacker = Cast<ACharacter>(_attacker);
	//アニメーションを再生する部品。
	UAnimInstance* anim = attacker ? attacker->GetMesh()->GetAnimInstance() : nullptr;
	//対象のモンタージュ。
	UAnimMontage* montage = anim ? anim->GetCurrentActiveMontage() : nullptr;

	return enemy && enemy->m_definition && montage && enemy->m_definition->m_observedHeavyMontage.Contains(montage);
}

//連続して受けた強攻撃の記録を消す。
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

//強攻撃の連続命中によるガード崩れを調べる。
EDefenseResult UDefense::ResolveGuardChain(AActor* _attacker, bool _heavyAttack)
{
	//処理の対象となる敵。
	AEnemyCharacterBase* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	//BPから設定した敵のデータ。
	const UEnemyData* data = enemy ? enemy->m_definition : nullptr;

	if(!data) { return EDefenseResult::None; }

	if (!data->m_useGuardBreak || !_heavyAttack)
	{
		ClearHeavyChain();
		return EDefenseResult::Guard;
	}

	//処理を待つ制限時間。
	const float timeout = FMath::Max(DefenseTuning::minHold, data->m_heavyTimeout);
	//現在の時刻。
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

//ガードを崩して無防備な時間を開始する。
void UDefense::StartGuardBreak()
{
	//処理の対象となる敵。
	AEnemyCharacterBase* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	//動作を続ける時間。
	const float duration = FMath::Max(DefenseTuning::minHold, enemy->m_definition->m_guardBreakTime);
	BeginReaction(enemy->m_definition->m_guardBreakMontage, duration);
	m_bGuardBroken = true;
	enemy->ShowGuardBreak();

	m_busyUntilTime = GetWorld()->GetTimeSeconds() + duration;
	enemy->GetCharacterMovement()->DisableMovement();
	GetWorld()->GetTimerManager().SetTimer(m_breakTimer, this, &UDefense::EndGuardBreak, duration, false);
}

//ガード崩れを終えて行動できる状態へ戻す。
void UDefense::EndGuardBreak()
{
	//処理の対象となる敵。
	AEnemyCharacterBase* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	m_bGuardBroken = false;
	m_busyUntilTime = 0.0;
	if(!enemy || !enemy->IsCombatActive()) { return; }

	if (enemy->m_definition && enemy->m_definition->m_guardBreakMontage)
	{
		enemy->StopAnimMontage(enemy->m_definition->m_guardBreakMontage);
	}

	enemy->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
}

//防御後の反応アニメーションを開始する。
void UDefense::BeginReaction(UAnimMontage* _montage, float _minduration)
{
	//処理の対象となる敵。
	AEnemyCharacterBase* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	if(!enemy) { return; }

	enemy->CancelAction();
	if (AAIController* aiController = Cast<AAIController>(enemy->GetController()))
	{
		aiController->StopMovement();
	}

	enemy->GetCharacterMovement()->StopMovementImmediately();

	//動作を続ける時間。
	const float  duration = _montage ? enemy->PlayAnimMontage(_montage) : 0;
	m_busyUntilTime = GetWorld()->GetTimeSeconds() + FMath::Max(DefenseTuning::minWindow, FMath::Max(_minduration, duration));
}

//防御の結果を通知する。
void UDefense::PublishResult(AActor* _attacker, EDefenseResult _result)
{
	m_lastResult = _result;
	if(_result == EDefenseResult::None) { return; }

	if (IsValid(_attacker) && _attacker->GetClass()->ImplementsInterface(UEBDefenseFeedback::StaticClass()))
	{
		IEBDefenseFeedback::Execute_AttackDefended(_attacker, GetOwner(), _result);
	}
	m_onDefense.Broadcast(_result, _attacker);
}

//攻撃を防がれた時の反応を行う。
void UDefense::ReactToDefendedAttack(EDefenseResult _result)
{
	//処理の対象となる敵。
	AEnemyCharacterBase* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	if(!enemy || !enemy->IsCombatActive() || !enemy->HasAuthority() || !enemy->m_definition || !enemy->IsActing()) { return; }

	if (_result == EDefenseResult::Deflect || _result == EDefenseResult::Parry)
	{
		BeginReaction(enemy->m_definition->m_staggerMontage, _result == EDefenseResult::Deflect ? enemy->m_definition->m_deflectedStun : enemy->m_definition->m_parriedStun);
		enemy->ShowGuardBreak();
	}
}

//ガードと反撃の状態を初期化する。
void UDefense::ResetDefense()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(m_breakTimer);
	}

	ClearHeavyChain();

	//処理の対象となる敵。
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
	//連続で抽選し続けないよう、次の時刻を決める。
	m_nextAttemptTime = 0.0;
	m_busyUntilTime = 0.0;
	m_attempts = 0;
	m_successes = 0;
	m_lastResult = EDefenseResult::None;

}

//終了時にタイマーや登録を解除する。
void UDefense::EndPlay(const EEndPlayReason::Type _reason)
{
	ResetDefense();
	Super::EndPlay(_reason);
}

//受けた攻撃を覚えて次の防御に備える。
void UDefense::RememberHit(AActor* _attacker)
{
	//処理の対象となる敵。
	auto* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	if(!enemy || !enemy->m_definition || !IsValid(_attacker)) { return; }
	//使用する行動の設定。
	const auto* action = enemy->GetActiveAction();
	if (action && action->m_kind != EEnemyAction::Guard) m_guardPending = true;
	m_guardTarget = _attacker;
	m_guardUntil = GetWorld()->GetTimeSeconds() + enemy->m_definition->m_guardMemory;
}

//予約した反撃を開始できるか調べる。
void UDefense::TryCounter()
{
	//処理の対象となる敵。
	auto* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	if (!enemy || !enemy->IsCombatActive() || !enemy->m_definition || GetWorld()->GetTimeSeconds() > m_counterUntil)
	{
		GetWorld()->GetTimerManager().ClearTimer(m_counterTimer); return;
	}
	if (enemy->CanDecide() && !IsTargetAttacking(enemy->m_combatTarget) && !enemy->m_definition->m_counter.IsNone()
		&& enemy->RequestAction(enemy->m_definition->m_counter)) GetWorld()->GetTimerManager().ClearTimer(m_counterTimer);
}

//攻撃後のガード予約が残っているか調べる。
bool UDefense::HasGuardRequest() const
{
	//処理の対象となる敵。
	const auto* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	return m_guardPending && enemy && m_guardTarget.IsValid() && m_guardTarget.Get() == enemy->m_combatTarget && GetWorld() && GetWorld()->GetTimeSeconds() < m_guardUntil;
}

//攻撃を終えた後に防御へつなぐ。
void UDefense::AttackFinished()
{
	if (m_guardPending && m_guardTarget.IsValid()) RememberHit(m_guardTarget.Get());
}
