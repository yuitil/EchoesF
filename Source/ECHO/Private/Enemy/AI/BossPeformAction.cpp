//
#include "Enemy/AI/BossPeformAction.h"
//
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
//
namespace
{
	bool HasRetreatSpace(AEnemyCharacterBase* enemy)
	{
		if (!enemy || !IsValid(enemy->m_combatTarget)) { return false; }
		const FVector start = enemy->GetActorLocation();
		const FVector away = (start - enemy->m_combatTarget->GetActorLocation()).GetSafeNormal2D();
		const FVector end = enemy->m_combatTarget->GetActorLocation() + away * (enemy->GetMeleeDistance() + 10.f);
		FHitResult hit;
		FCollisionQueryParams params(SCENE_QUERY_STAT(EnemyRetreatSpace), false, enemy);
		const auto* capsule = enemy->GetCapsuleComponent();
		if (enemy->GetWorld()->SweepSingleByChannel(hit, start, FVector(end.X, end.Y, start.Z), FQuat::Identity, ECC_Pawn,
			FCollisionShape::MakeCapsule(capsule->GetScaledCapsuleRadius(), capsule->GetScaledCapsuleHalfHeight()), params)) {
			return false;
		}
		auto* nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(enemy->GetWorld());
		FNavLocation floor;
		const float feet = start.Z - capsule->GetScaledCapsuleHalfHeight();
		return nav && nav->ProjectPointToNavigation(FVector(end.X, end.Y, feet), floor, FVector(25, 25, 50)) && FMath::Abs(floor.Location.Z - feet) <= enemy->GetCharacterMovement()->MaxStepHeight;
	}

	EPathFollowingRequestResult::Type MoveToRange(AAIController* ai, AEnemyCharacterBase* enemy, float distance)
	{
		FAIMoveRequest request(enemy->m_combatTarget);
		request.SetAcceptanceRadius(distance);
		request.SetReachTestIncludesAgentRadius(false);
		request.SetReachTestIncludesGoalRadius(false);
		request.SetUsePathfinding(true);
		request.SetAllowPartialPath(false);
		return ai->MoveTo(request).Code;
	}
}


UBossPeformAction::UBossPeformAction()
{
	NodeName = TEXT("Boss Perform Action");
	bCreateNodeInstance = true;
	bNotifyTick = true;
}

bool UBossPeformAction::StartAttack(UBehaviorTreeComponent& _ownerComp)
{
	auto* enemy = Cast<AEnemyCharacterBase>(_ownerComp.GetAIOwner()->GetPawn());

	if (!enemy || !IsValid(enemy->m_combatTarget) || (!enemy->m_stationary && FVector::Dist2D(enemy->GetActorLocation(), enemy->m_combatTarget->GetActorLocation()) > FMath::Max(m_attackDist, enemy->GetMeleeDistance()) + 20.f) ||
		(!enemy->m_stationary && !enemy->GetCharacterMovement()->IsFalling() && FVector::Dist2D(enemy->GetActorLocation(), enemy->m_combatTarget->GetActorLocation()) < enemy->GetMeleeDistance() - 30.f && HasRetreatSpace(enemy)) ||
		!enemy->RequestAction()) {
		return false;
	}

	m_serial = enemy->m_actionSerial;
	const auto* action = enemy->GetActiveAction();
	m_deadline = enemy->GetWorld()->GetTimeSeconds() + enemy->GetActionTimeout(*action);
	return true;
}

bool UBossPeformAction::CanCatch(AEnemyCharacterBase* _enemy, float& _hitEnd, float& _moveStart) const
{
	const auto* action = _enemy->GetReadyAction(m_rushAction);
	if (!action || !action->m_montage || !_enemy->GetPendingComboId().IsNone()) { return false; }
	AAIController* ai = Cast<AAIController>(_enemy->GetController());
	AActor* target = _enemy->m_combatTarget;
	const FVector start = _enemy->GetActorLocation();
	const float distance = FVector::Dist2D(start, target->GetActorLocation());
	if (!ai || !ai->LineOfSightTo(target) || distance <= FMath::Max(m_attackDist, m_rushMin) || distance > m_rushRange) { return false; }

	if (_enemy->GetWorld()->GetTimeSeconds() - m_chaseStart < m_chaseDelay) { return false; }
	const FVector toward = (target->GetActorLocation() - start).GetSafeNormal2D();
	const float facing = FVector::DotProduct(_enemy->GetActorForwardVector().GetSafeNormal2D(), toward);
	if (facing < FMath::Cos(FMath::DegreesToRadians(m_rushAngle))) { return false; }
	const float escapeSpeed = FVector::DotProduct(target->GetVelocity(), toward);
	if (escapeSpeed < -50.f) { return false; }

	_hitEnd = 0.f;
	for (const auto& notify : action->m_montage->Notifies)
	{
		const auto* window = Cast<UBossCombatWindow>(notify.NotifyStateClass);
		if (window && window->m_window == EBossWindow::Damage)
		{
			_hitEnd = (notify.GetTriggerTime() + notify.GetDuration()) / FMath::Max(.1f, _enemy->GetAttackRate(*action) * action->m_montage->RateScale);
			break;
		}
	}
	const int32 section = action->m_montage->GetSectionIndex(m_moveSection);
	if (section == INDEX_NONE) { return false; }
	float sectionEnd = 0.f;
	action->m_montage->GetSectionStartAndEndTime(section, _moveStart, sectionEnd);
	const float rate = FMath::Max(.1f, _enemy->GetAttackRate(*action) * action->m_montage->RateScale);
	_moveStart /= rate;
	_hitEnd = FMath::Min(_hitEnd, sectionEnd / rate);
	const float time = _hitEnd - .08f;
	if (time <= _moveStart) { return false; }
	const FVector predicted = target->GetActorLocation() + target->GetVelocity() * time;
	if (FMath::Abs(predicted.Z - start.Z) > m_heightLimit) { return false; }
	FHitResult wall;
	FCollisionQueryParams params(SCENE_QUERY_STAT(DashReach), false, _enemy);
	params.AddIgnoredActor(target);
	if (_enemy->GetWorld()->LineTraceSingleByChannel(wall, start, predicted, ECC_Visibility, params)) { return false; }
	auto* path = UNavigationSystemV1::FindPathToLocationSynchronously(_enemy->GetWorld(), start, predicted, _enemy);
	if (!path || !path->IsValid() || path->IsPartial()) { return false; }
	const float direct = FVector::Dist2D(start, predicted);
	if (path->GetPathLength() > direct * 1.15f + 20.f) { return false; }
	const float accel = FMath::Max(_enemy->GetCharacterMovement()->MaxAcceleration, m_rushSpeed * 8.f);
	const float speed = 0.f;
	const float travelTime = time - _moveStart;
	const float ramp = FMath::Min(travelTime, (m_rushSpeed - speed) / accel);
	const float travel = speed * ramp + .5f * accel * ramp * ramp + m_rushSpeed * (travelTime - ramp);
	if (path->GetPathLength() > travel + FMath::Max(0.f, m_attackDist - 35.f)) { return false; }

	const float closing = _enemy->GetCharacterMovement()->MaxWalkSpeed - escapeSpeed;
	const float walkTime = closing > 1.f ? (distance - m_attackDist) / closing : BIG_NUMBER;
	const float dashTime = _moveStart + FMath::Max(0.f, path->GetPathLength() - m_attackDist) / m_rushSpeed + .125f;
	return walkTime - dashTime >= m_minGain;

}

bool UBossPeformAction::TryDash(UBehaviorTreeComponent& _ownerComp)
{
	auto* ai = _ownerComp.GetAIOwner();
	auto* enemy = ai ? Cast<AEnemyCharacterBase>(ai->GetPawn()) : nullptr;
	float hitEnd = 0.f;
	float moveStart = 0.f;
	if (!enemy || enemy->GetWorld()->GetTimeSeconds() < m_nextCheck) { return false; }
	m_nextCheck = enemy->GetWorld()->GetTimeSeconds() + .1;
	if (!enemy || !CanCatch(enemy, hitEnd, moveStart) || !enemy->RequestAction(m_rushAction)) { return false; }
	m_fast = true;
	m_serial = enemy->m_actionSerial;
	auto* move = enemy->GetCharacterMovement();
	move->MaxWalkSpeed = m_rushSpeed;
	move->MaxAcceleration = FMath::Max(m_oldAccel, m_rushSpeed * 8.f);
	m_hitEnd = enemy->GetWorld()->GetTimeSeconds() + hitEnd;
	m_moveStart = enemy->GetWorld()->GetTimeSeconds() + moveStart;
	const auto* action = enemy->GetActiveAction();
	m_deadline = enemy->GetWorld()->GetTimeSeconds() + enemy->GetActionTimeout(*action);
	ai->StopMovement();
	move->StopMovementImmediately();
	return true;
}

void UBossPeformAction::StopRush(UBehaviorTreeComponent& _ownerComp)
{
	if (!m_rushing) { return; }
	m_rushing = false;
	auto* ai = _ownerComp.GetAIOwner();
	auto* enemy = ai ? Cast<AEnemyCharacterBase>(ai->GetPawn()) : nullptr;

	const bool traversing = enemy && enemy->m_traversal->IsTraversing();
	if (ai && !traversing) { ai->StopMovement(); }

	if (enemy)
	{
		auto* move = enemy->GetCharacterMovement();
		move->MaxWalkSpeed = m_oldSpeed;
		move->MaxAcceleration = m_oldAccel;
		if (!traversing) { move->StopMovementImmediately(); }
	}
}

EBTNodeResult::Type UBossPeformAction::ExecuteTask(UBehaviorTreeComponent& _ownerComp, uint8* _nodeMemory)
{
	auto* ai = _ownerComp.GetAIOwner();
	auto* enemy = ai ? Cast<AEnemyCharacterBase>(ai->GetPawn()) : nullptr;
	m_rushing = false;
	m_fast = false;
	m_serial = INDEX_NONE;
	m_nextCheck = 0.;
	if (!enemy || !enemy->CanDecide()) { return EBTNodeResult::Failed; }
	if (const auto* formation=enemy->FindComponentByClass<UMinionFormation>(); formation && !formation->HasTurn()) return EBTNodeResult::Failed;
	//
	if (enemy->m_stationary) return StartAttack(_ownerComp) ? EBTNodeResult::InProgress : EBTNodeResult::Failed;
	//
	m_chaseStart = enemy->GetWorld()->GetTimeSeconds();
	const float distance = FVector::Dist2D(enemy->GetActorLocation(), enemy->m_combatTarget->GetActorLocation());

	if (distance < enemy->GetMeleeDistance() - 30.f && enemy->GetCharacterMovement()->IsMovingOnGround() && HasRetreatSpace(enemy))
	{

		if (!m_retreatAction.IsNone() && enemy->RequestAction(m_retreatAction))
		{
			m_serial = enemy->m_actionSerial;
			const auto* action = enemy->GetActiveAction();
			m_deadline = enemy->GetWorld()->GetTimeSeconds() + enemy->GetActionTimeout(*action);
			return EBTNodeResult::InProgress;
		}

		FVector away = (enemy->GetActorLocation() - enemy->m_combatTarget->GetActorLocation()).GetSafeNormal2D();
		if (away.IsNearlyZero()) { away = -enemy->GetActorForwardVector(); }
		const FVector goal = enemy->m_combatTarget->GetActorLocation() + away * enemy->GetMeleeDistance();
		m_oldSpeed = enemy->GetCharacterMovement()->MaxWalkSpeed;
		m_oldAccel = enemy->GetCharacterMovement()->MaxAcceleration;
		m_rushing = true;
		m_deadline = enemy->GetWorld()->GetTimeSeconds() + 2.f;
		if (ai->MoveToLocation(goal, 5.f, false, true, true, true, nullptr, false) == EPathFollowingRequestResult::Failed)
		{
			StopRush(_ownerComp);
			return EBTNodeResult::Failed;
		}
		return EBTNodeResult::InProgress;
	}


	if (distance <= m_rushMin && StartAttack(_ownerComp)) { return EBTNodeResult::InProgress; }

	if (distance > FMath::Max(m_attackDist, enemy->GetMeleeDistance()) + 20.f && enemy->GetPendingComboId().IsNone() && enemy->GetCharacterMovement()->IsMovingOnGround())
	{
		m_oldSpeed = enemy->GetCharacterMovement()->MaxWalkSpeed;
		m_oldAccel = enemy->GetCharacterMovement()->MaxAcceleration;
		m_rushing = true;
		m_deadline = enemy->GetWorld()->GetTimeSeconds() + distance / FMath::Max(1.f, m_oldSpeed) + 4.f;
		if (TryDash(_ownerComp)) { return EBTNodeResult::InProgress; }
		if (MoveToRange(ai, enemy, FMath::Max(m_attackDist, enemy->GetMeleeDistance())) == EPathFollowingRequestResult::Failed)
		{
			StopRush(_ownerComp);
			return EBTNodeResult::Failed;
		}
		return EBTNodeResult::InProgress;
	}
	return StartAttack(_ownerComp) ? EBTNodeResult::InProgress : EBTNodeResult::Failed;
}

void UBossPeformAction::TickTask(UBehaviorTreeComponent& _ownerComp, uint8* _nodeMemory, float _delta)
{
	auto* ai = _ownerComp.GetAIOwner();
	auto* enemy = ai ? Cast<AEnemyCharacterBase>(ai->GetPawn()) : nullptr;
	if (!enemy) { StopRush(_ownerComp); FinishLatentTask(_ownerComp, EBTNodeResult::Failed); return; }
	if (const auto* formation=enemy->FindComponentByClass<UMinionFormation>(); formation && !formation->HasTurn() && !enemy->IsActing())
	{ StopRush(_ownerComp); FinishLatentTask(_ownerComp, EBTNodeResult::Succeeded); return; }
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
		const float distance = FVector::Dist2D(enemy->GetActorLocation(), enemy->m_combatTarget->GetActorLocation());
		if (m_fast)
		{
			if (enemy->m_actionSerial != m_serial || !enemy->IsActing())
			{
				StopRush(_ownerComp);
				if (enemy->m_actionSerial == m_serial && enemy->m_actionSuccess && !enemy->GetPendingComboId().IsNone() && enemy->CanDecide() && StartAttack(_ownerComp)) { return; }
				FinishLatentTask(_ownerComp, enemy->m_actionSerial == m_serial && enemy->m_actionSuccess ? EBTNodeResult::Succeeded : EBTNodeResult::Failed);
				return;
			}

			const float stop = FMath::Max(m_attackDist, enemy->GetMeleeDistance());
			auto* move = enemy->GetCharacterMovement();

			const float limit = now < m_moveStart ? FMath::Clamp(m_startSpeed, 0.f, m_rushSpeed) : m_rushSpeed;
			const float speed = FMath::Min(limit, FMath::Max(0.f, distance - stop) / FMath::Max(.05f, _delta * 2.f));
			move->MaxWalkSpeed = speed;
			move->Velocity = move->Velocity.GetClampedToMaxSize(speed);


			if (distance <= stop + 5.f || now >= m_hitEnd)
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
		if (enemy->m_actionSerial == m_serial && enemy->m_actionSuccess && !enemy->GetPendingComboId().IsNone() && enemy->CanDecide() && StartAttack(_ownerComp)) { return; }

		FinishLatentTask(_ownerComp, enemy->m_actionSerial == m_serial && enemy->m_actionSuccess ? EBTNodeResult::Succeeded : EBTNodeResult::Failed);
	}
	else if (now > m_deadline)
	{
		enemy->CancelAction();
		FinishLatentTask(_ownerComp, EBTNodeResult::Failed);
	}
}

EBTNodeResult::Type UBossPeformAction::AbortTask(UBehaviorTreeComponent& _ownerComp, uint8* _nodeMemory)
{
	StopRush(_ownerComp);
	auto* ai = _ownerComp.GetAIOwner();
	auto* enemy = ai ? Cast<AEnemyCharacterBase>(ai->GetPawn()) : nullptr;
	if (enemy && enemy->m_actionSerial == m_serial) { enemy->CancelAction(); }
	return EBTNodeResult::Aborted;
}
//
