
#include "Enemy/AI/BossPeformAction.h"

#include "Enemy/AI/BossAIController.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Enemy/Core/EnemyTraversal.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Enemy/Combat/Defense.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Animation/AnimInstance.h"
#include "Enemy/Animation/BossAnimation.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include "Enemy/Minion/MinionFormation.h"

//判定に使う幅や待ち時間。
namespace BossPeformActionTuning
{
    //退避先に必要な余裕。
    constexpr auto retreatMargin = 10.f;
    //歩ける場所を探す横幅。
    constexpr auto navWidth = 25;
    //歩ける場所を探す高さ。
    constexpr auto navHeight = 50;
    //距離の判定に持たせる余裕。
    constexpr auto rangeMargin = 20.f;
    //接近しすぎないための余裕。
    constexpr auto closeMargin = 30.f;
    //相手へ近づく時の速度。
    constexpr auto approachSpeed = 50.f;
    //命中範囲に持たせる余裕。
    constexpr auto hitMargin = .08f;
    //直線距離に対して許す経路の長さ。
    constexpr auto pathRatio = 1.15f;
    //突進中の加速の強さ。
    constexpr auto dashAccel = 8.f;
    //届く距離の判定に使う余裕。
    constexpr auto reachMargin = 35.f;
    //突進を始めるまでの時間。
    constexpr auto dashDelay = .125f;
    //次の確認までの時間。
    constexpr auto checkDelay = .1;
    //停止位置の余裕。
    constexpr auto stopMargin = 5.f;
    //追跡を待つ制限時間。
    constexpr auto chaseTimeout = 4.f;
    //再生速度の下限。
    constexpr auto minRate = .1f;
    //区間の時間の下限。
    constexpr auto minStepTime = .05f;
}

namespace
{
	//後ろに退避できる場所があるか調べる。
	bool HasRetreatSpace(AEnemyCharacterBase* enemy)
	{
		if(!enemy || !IsValid(enemy->m_combatTarget)) { return false; }
		//調べ始める位置。
		const FVector start = enemy->GetActorLocation();
		//相手から離れる方向。
		const FVector away = (start - enemy->m_combatTarget->GetActorLocation()).GetSafeNormal2D();
		//調べ終える位置。
		const FVector end = enemy->m_combatTarget->GetActorLocation() + away * (enemy->GetMeleeDistance() + BossPeformActionTuning::retreatMargin);
		//ぶつかった場所の情報。
		FHitResult hit;
		//生成や当たり判定に使う設定。
		FCollisionQueryParams params(SCENE_QUERY_STAT(EnemyRetreatSpace), false, enemy);
		//キャラクターの当たり判定。
		const auto* capsule = enemy->GetCapsuleComponent();
		if(enemy->GetWorld()->SweepSingleByChannel(hit, start, FVector(end.X, end.Y, start.Z), FQuat::Identity, ECC_Pawn,
			FCollisionShape::MakeCapsule(capsule->GetScaledCapsuleRadius(), capsule->GetScaledCapsuleHalfHeight()), params)) { return false; }
		//歩ける場所を調べる機能。
		auto* nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(enemy->GetWorld());
		//地面に当たった情報。
		FNavLocation floor;
		//足元の位置。
		const float feet = start.Z - capsule->GetScaledCapsuleHalfHeight();
		return nav && nav->ProjectPointToNavigation(FVector(end.X, end.Y, feet), floor, FVector(BossPeformActionTuning::navWidth, BossPeformActionTuning::navWidth, BossPeformActionTuning::navHeight)) && FMath::Abs(floor.Location.Z - feet) <= enemy->GetCharacterMovement()->MaxStepHeight;
	}

	//指定した間合いまで移動する。
	EPathFollowingRequestResult::Type MoveToRange(AAIController* ai, AEnemyCharacterBase* enemy, float distance)
	{
		//AIの移動先と停止条件。
		FAIMoveRequest request(enemy->m_combatTarget);
		request.SetAcceptanceRadius(distance);
		request.SetReachTestIncludesAgentRadius(false);
		request.SetReachTestIncludesGoalRadius(false);
		request.SetUsePathfinding(true);
		request.SetAllowPartialPath(false);
		return ai->MoveTo(request).Code;
	}
}

//初期設定と必要な部品を用意する。
UBossPeformAction::UBossPeformAction()
{
	NodeName = TEXT("Boss Perform Action");
	bCreateNodeInstance = true;
	bNotifyTick = true;
}

//選んだ攻撃を開始する。
bool UBossPeformAction::StartAttack(UBehaviorTreeComponent& _ownerComp)
{
	//処理の対象となる敵。
	auto* enemy = Cast<AEnemyCharacterBase>(_ownerComp.GetAIOwner()->GetPawn());

	if(!enemy || !IsValid(enemy->m_combatTarget) || (!enemy->m_stationary && FVector::Dist2D(enemy->GetActorLocation(), enemy->m_combatTarget->GetActorLocation()) > FMath::Max(m_attackDist, enemy->GetMeleeDistance()) + BossPeformActionTuning::rangeMargin) ||
		(!enemy->m_stationary && !enemy->GetCharacterMovement()->IsFalling() && FVector::Dist2D(enemy->GetActorLocation(), enemy->m_combatTarget->GetActorLocation()) < enemy->GetMeleeDistance() - BossPeformActionTuning::closeMargin && HasRetreatSpace(enemy)) ||
		!enemy->RequestAction()) { return false; }

	m_serial = enemy->m_actionSerial;
	//使用する行動の設定。
	const auto* action = enemy->GetActiveAction();
	m_deadline = enemy->GetWorld()->GetTimeSeconds() + enemy->GetActionTimeout(*action);
	return true;
}

//突進で相手に追いつけるか調べる。
bool UBossPeformAction::CanCatch(AEnemyCharacterBase* _enemy, float& _hitEnd, float& _moveStart) const
{
	//使用する行動の設定。
	const auto* action = _enemy->GetReadyAction(m_rushAction);
	if(!action || !action->m_montage || !_enemy->GetPendingComboId().IsNone()) { return false; }
	//敵の行動を操作するAI。
	AAIController* ai = Cast<AAIController>(_enemy->GetController());
	//狙っている相手。
	AActor* target = _enemy->m_combatTarget;
	//調べ始める位置。
	const FVector start = _enemy->GetActorLocation();
	//相手までの距離。
	const float distance = FVector::Dist2D(start, target->GetActorLocation());
	if(!ai || !ai->LineOfSightTo(target) || distance <= FMath::Max(m_attackDist, m_rushMin) || distance > m_rushRange) { return false; }

	if(_enemy->GetWorld()->GetTimeSeconds() - m_chaseStart < m_chaseDelay) { return false; }
	//相手へ向かう方向。
	const FVector toward = (target->GetActorLocation() - start).GetSafeNormal2D();
	//向いている方向。
	const float facing = FVector::DotProduct(_enemy->GetActorForwardVector().GetSafeNormal2D(), toward);
	if(facing < FMath::Cos(FMath::DegreesToRadians(m_rushAngle))) { return false; }
	//相手が離れる速さ。
	const float escapeSpeed = FVector::DotProduct(target->GetVelocity(), toward);
	if(escapeSpeed < -BossPeformActionTuning::approachSpeed) { return false; }

	_hitEnd = 0.f;
	for (const auto& notify : action->m_montage->Notifies)
	{
		//動作の判定に使う時間の幅。
		const auto* window = Cast<UBossCombatWindow>(notify.NotifyStateClass);
		if (window && window->m_window == EBossWindow::Damage)
		{
			_hitEnd = (notify.GetTriggerTime() + notify.GetDuration()) / FMath::Max(BossPeformActionTuning::minRate, _enemy->GetAttackRate(*action) * action->m_montage->RateScale);
			break;
		}
	}
	//再生する区間。
	const int32 section = action->m_montage->GetSectionIndex(m_moveSection);
	if(section == INDEX_NONE) { return false; }
	//区間が終わる再生位置。
	float sectionEnd = 0.f;
	action->m_montage->GetSectionStartAndEndTime(section, _moveStart, sectionEnd);
	//使用する再生速度。
	const float rate = FMath::Max(BossPeformActionTuning::minRate, _enemy->GetAttackRate(*action) * action->m_montage->RateScale);
	_moveStart /= rate;
	_hitEnd = FMath::Min(_hitEnd, sectionEnd / rate);
	//処理に使う時間。
	const float time = _hitEnd - BossPeformActionTuning::hitMargin;
	if(time <= _moveStart) { return false; }
	//相手の移動を先読みした位置。
	const FVector predicted = target->GetActorLocation() + target->GetVelocity() * time;
	if(FMath::Abs(predicted.Z - start.Z) > m_heightLimit) { return false; }
	//進行方向の障害物の情報。
	FHitResult wall;
	//生成や当たり判定に使う設定。
	FCollisionQueryParams params(SCENE_QUERY_STAT(DashReach), false, _enemy);
	params.AddIgnoredActor(target);
	if(_enemy->GetWorld()->LineTraceSingleByChannel(wall, start, predicted, ECC_Visibility, params)) { return false; }
	//目的地までの移動経路。
	auto* path = UNavigationSystemV1::FindPathToLocationSynchronously(_enemy->GetWorld(), start, predicted, _enemy);
	if(!path || !path->IsValid() || path->IsPartial()) { return false; }
	//目的地までの直線距離。
	const float direct = FVector::Dist2D(start, predicted);
	if(path->GetPathLength() > direct * BossPeformActionTuning::pathRatio + BossPeformActionTuning::rangeMargin) { return false; }
	//加速の強さ。
	const float accel = FMath::Max(_enemy->GetCharacterMovement()->MaxAcceleration, m_rushSpeed * BossPeformActionTuning::dashAccel);
	//移動する速さ。
	const float speed = 0.f;
	//目的地までの移動時間。
	const float travelTime = time - _moveStart;
	//加速にかかる時間。
	const float ramp = FMath::Min(travelTime, (m_rushSpeed - speed) / accel);
	//移動する距離。
	const float travel = speed * ramp + .5f * accel * ramp * ramp + m_rushSpeed * (travelTime - ramp);
	if(path->GetPathLength() > travel + FMath::Max(0.f, m_attackDist - BossPeformActionTuning::reachMargin)) { return false; }

	//相手との距離を縮める速さ。
	const float closing = _enemy->GetCharacterMovement()->MaxWalkSpeed - escapeSpeed;
	//通常移動で近づく時間。
	const float walkTime = closing > 1.f ? (distance - m_attackDist) / closing : BIG_NUMBER;
	//突進で近づく時間。
	const float dashTime = _moveStart + FMath::Max(0.f, path->GetPathLength() - m_attackDist) / m_rushSpeed + BossPeformActionTuning::dashDelay;
	return walkTime - dashTime >= m_minGain;

}

//突進で相手に届くか調べて開始する。
bool UBossPeformAction::TryDash(UBehaviorTreeComponent& _ownerComp)
{
	//敵の行動を操作するAI。
	auto* ai = _ownerComp.GetAIOwner();
	//処理の対象となる敵。
	auto* enemy = ai ? Cast<AEnemyCharacterBase>(ai->GetPawn()) : nullptr;
	//攻撃判定を終える再生位置。
	float hitEnd = 0.f;
	//移動を始める再生位置。
	float moveStart = 0.f;
	if(!enemy || enemy->GetWorld()->GetTimeSeconds() < m_nextCheck) { return false; }
	m_nextCheck = enemy->GetWorld()->GetTimeSeconds() + BossPeformActionTuning::checkDelay;
	if(!enemy || !CanCatch(enemy, hitEnd, moveStart) || !enemy->RequestAction(m_rushAction)) { return false; }
	m_fast = true;
	m_serial = enemy->m_actionSerial;
	//移動を管理する部品。
	auto* move = enemy->GetCharacterMovement();
	move->MaxWalkSpeed = m_rushSpeed;
	move->MaxAcceleration = FMath::Max(m_oldAccel, m_rushSpeed * BossPeformActionTuning::dashAccel);
	m_hitEnd = enemy->GetWorld()->GetTimeSeconds() + hitEnd;
	m_moveStart = enemy->GetWorld()->GetTimeSeconds() + moveStart;
	//使用する行動の設定。
	const auto* action = enemy->GetActiveAction();
	m_deadline = enemy->GetWorld()->GetTimeSeconds() + enemy->GetActionTimeout(*action);
	ai->StopMovement();
	move->StopMovementImmediately();
	return true;
}

//突進を止めて移動設定を戻す。
void UBossPeformAction::StopRush(UBehaviorTreeComponent& _ownerComp)
{
	if(!m_rushing) { return; }
	m_rushing = false;
	//敵の行動を操作するAI。
	auto* ai = _ownerComp.GetAIOwner();
	//処理の対象となる敵。
	auto* enemy = ai ? Cast<AEnemyCharacterBase>(ai->GetPawn()) : nullptr;

	//段差を移動中か。
	const bool traversing = enemy && enemy->m_traversal->IsTraversing();
	if (ai && !traversing) { ai->StopMovement(); }

	if (enemy)
	{
		//移動を管理する部品。
		auto* move = enemy->GetCharacterMovement();
		move->MaxWalkSpeed = m_oldSpeed;
		move->MaxAcceleration = m_oldAccel;
		if (!traversing) { move->StopMovementImmediately(); }
	}
}

//BTから呼ばれた行動を開始する。
EBTNodeResult::Type UBossPeformAction::ExecuteTask(UBehaviorTreeComponent& _ownerComp, uint8* _nodeMemory)
{
	//敵の行動を操作するAI。
	auto* ai = _ownerComp.GetAIOwner();
	//処理の対象となる敵。
	auto* enemy = ai ? Cast<AEnemyCharacterBase>(ai->GetPawn()) : nullptr;
	m_rushing = false;
	m_fast = false;
	m_serial = INDEX_NONE;
	m_nextCheck = 0.;
	if(!enemy || !enemy->CanDecide()) { return EBTNodeResult::Failed; }
	if(const auto* formation=enemy->FindComponentByClass<UMinionFormation>(); formation && !formation->HasTurn()) { return EBTNodeResult::Failed; }

	if(enemy->m_stationary) { return StartAttack(_ownerComp) ? EBTNodeResult::InProgress : EBTNodeResult::Failed; }

	m_chaseStart = enemy->GetWorld()->GetTimeSeconds();
	//相手までの距離。
	const float distance = FVector::Dist2D(enemy->GetActorLocation(), enemy->m_combatTarget->GetActorLocation());

	if (distance < enemy->GetMeleeDistance() - BossPeformActionTuning::closeMargin && enemy->GetCharacterMovement()->IsMovingOnGround() && HasRetreatSpace(enemy))
	{

		if (!m_retreatAction.IsNone() && enemy->RequestAction(m_retreatAction))
		{
			m_serial = enemy->m_actionSerial;
			//使用する行動の設定。
			const auto* action = enemy->GetActiveAction();
			m_deadline = enemy->GetWorld()->GetTimeSeconds() + enemy->GetActionTimeout(*action);
			return EBTNodeResult::InProgress;
		}

		//相手から離れる方向。
		FVector away = (enemy->GetActorLocation() - enemy->m_combatTarget->GetActorLocation()).GetSafeNormal2D();
		if (away.IsNearlyZero()) { away = -enemy->GetActorForwardVector(); }
		//移動の目的地。
		const FVector goal = enemy->m_combatTarget->GetActorLocation() + away * enemy->GetMeleeDistance();
		m_oldSpeed = enemy->GetCharacterMovement()->MaxWalkSpeed;
		m_oldAccel = enemy->GetCharacterMovement()->MaxAcceleration;
		m_rushing = true;
		m_deadline = enemy->GetWorld()->GetTimeSeconds() + 2.f;
		if (ai->MoveToLocation(goal, BossPeformActionTuning::stopMargin, false, true, true, true, nullptr, false) == EPathFollowingRequestResult::Failed)
		{
			StopRush(_ownerComp);
			return EBTNodeResult::Failed;
		}
		return EBTNodeResult::InProgress;
	}

	if(distance <= m_rushMin && StartAttack(_ownerComp)) { return EBTNodeResult::InProgress; }

	if (distance > FMath::Max(m_attackDist, enemy->GetMeleeDistance()) + BossPeformActionTuning::rangeMargin && enemy->GetPendingComboId().IsNone() && enemy->GetCharacterMovement()->IsMovingOnGround())
	{
		m_oldSpeed = enemy->GetCharacterMovement()->MaxWalkSpeed;
		m_oldAccel = enemy->GetCharacterMovement()->MaxAcceleration;
		m_rushing = true;
		m_deadline = enemy->GetWorld()->GetTimeSeconds() + distance / FMath::Max(1.f, m_oldSpeed) + BossPeformActionTuning::chaseTimeout;
		if(TryDash(_ownerComp)) { return EBTNodeResult::InProgress; }
		if (MoveToRange(ai, enemy, FMath::Max(m_attackDist, enemy->GetMeleeDistance())) == EPathFollowingRequestResult::Failed)
		{
			StopRush(_ownerComp);
			return EBTNodeResult::Failed;
		}
		return EBTNodeResult::InProgress;
	}
	return StartAttack(_ownerComp) ? EBTNodeResult::InProgress : EBTNodeResult::Failed;
}

//進行中の行動を確認して終了を判断する。
void UBossPeformAction::TickTask(UBehaviorTreeComponent& _ownerComp, uint8* _nodeMemory, float _delta)
{
	//敵の行動を操作するAI。
	auto* ai = _ownerComp.GetAIOwner();
	//処理の対象となる敵。
	auto* enemy = ai ? Cast<AEnemyCharacterBase>(ai->GetPawn()) : nullptr;
	if (!enemy) { StopRush(_ownerComp); FinishLatentTask(_ownerComp, EBTNodeResult::Failed); return; }
	if (const auto* formation=enemy->FindComponentByClass<UMinionFormation>(); formation && !formation->HasTurn() && !enemy->IsActing())
	{ StopRush(_ownerComp); FinishLatentTask(_ownerComp, EBTNodeResult::Succeeded); return; }
	//現在の時刻。
	const double now = enemy->GetWorld()->GetTimeSeconds();
	if (m_rushing)
	{
		if (!enemy->IsCombatActive() || !IsValid(enemy->m_combatTarget) || !enemy->GetCharacterMovement()->IsMovingOnGround() || now > m_deadline)
		{
			StopRush(_ownerComp);
			if (m_fast && enemy->m_actionSerial == m_serial) { enemy->CancelAction(); }
			FinishLatentTask(_ownerComp, EBTNodeResult::Failed);
			return;
		}
		//相手までの距離。
		const float distance = FVector::Dist2D(enemy->GetActorLocation(), enemy->m_combatTarget->GetActorLocation());
		if (m_fast)
		{
			if (enemy->m_actionSerial != m_serial || !enemy->IsActing())
			{
				StopRush(_ownerComp);
				if(enemy->m_actionSerial == m_serial && enemy->m_actionSuccess && !enemy->GetPendingComboId().IsNone() && enemy->CanDecide() && StartAttack(_ownerComp)) { return; }
				FinishLatentTask(_ownerComp, enemy->m_actionSerial == m_serial && enemy->m_actionSuccess ? EBTNodeResult::Succeeded : EBTNodeResult::Failed);
				return;
			}

			//移動を止める距離。
			const float stop = FMath::Max(m_attackDist, enemy->GetMeleeDistance());
			//移動を管理する部品。
			auto* move = enemy->GetCharacterMovement();

			//処理に使う上限。
			const float limit = now < m_moveStart ? FMath::Clamp(m_startSpeed, 0.f, m_rushSpeed) : m_rushSpeed;
			//移動する速さ。
			const float speed = FMath::Min(limit, FMath::Max(0.f, distance - stop) / FMath::Max(BossPeformActionTuning::minStepTime, _delta * 2.f));
			move->MaxWalkSpeed = speed;
			move->Velocity = move->Velocity.GetClampedToMaxSize(speed);

			if (distance <= stop + BossPeformActionTuning::stopMargin || now >= m_hitEnd)
			{
				ai->StopMovement();
				enemy->GetCharacterMovement()->StopMovementImmediately();
			}
			else if (ai->GetMoveStatus() != EPathFollowingStatus::Moving)
			{
				MoveToRange(ai, enemy, FMath::Max(m_attackDist, enemy->GetMeleeDistance()));
			}
			return;
		}
		if (!enemy->CanDecide()) { StopRush(_ownerComp); FinishLatentTask(_ownerComp, EBTNodeResult::Failed); return; }
		if (distance <= m_rushMin && ai->LineOfSightTo(enemy->m_combatTarget) && StartAttack(_ownerComp))
		{
			StopRush(_ownerComp);
		}
		else if (!TryDash(_ownerComp) && ai->GetMoveStatus() != EPathFollowingStatus::Moving)
		{
			StopRush(_ownerComp);
			FinishLatentTask(_ownerComp, EBTNodeResult::Failed);
		}
		return;
	}
	if (enemy->m_actionSerial != m_serial || !enemy->IsActing())
	{
		if(enemy->m_actionSerial == m_serial && enemy->m_actionSuccess && !enemy->GetPendingComboId().IsNone() && enemy->CanDecide() && StartAttack(_ownerComp)) { return; }

		FinishLatentTask(_ownerComp, enemy->m_actionSerial == m_serial && enemy->m_actionSuccess ? EBTNodeResult::Succeeded : EBTNodeResult::Failed);
	}
	else if (now > m_deadline)
	{
		enemy->CancelAction();
		FinishLatentTask(_ownerComp, EBTNodeResult::Failed);
	}
}

//BTの行動を中断して状態を戻す。
EBTNodeResult::Type UBossPeformAction::AbortTask(UBehaviorTreeComponent& _ownerComp, uint8* _nodeMemory)
{
	StopRush(_ownerComp);
	//敵の行動を操作するAI。
	auto* ai = _ownerComp.GetAIOwner();
	//処理の対象となる敵。
	auto* enemy = ai ? Cast<AEnemyCharacterBase>(ai->GetPawn()) : nullptr;
	if (enemy && enemy->m_actionSerial == m_serial) { enemy->CancelAction(); }
	return EBTNodeResult::Aborted;
}
