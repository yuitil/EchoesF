#include "Enemy/Boss/BossShadowSummon.h"
#include "Enemy/Boss/BossShadow.h"
#include "Enemy/Boss/BossPosture.h"
#include "Enemy/Boss/BossSpacing.h"
#include "Enemy/Boss/BossApproach.h"
#include "Enemy/Combat/Combat.h"
#include "Enemy/Core/EnemyTraversal.h"
#include "Enemy/Combat/Defense.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Navigation/PathFollowingComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	//追跡中に、この距離を進めたら足止め時間を数え直す。
	constexpr float stuckTravel = 30.f;
	//交代後に二人の体が重ならないための距離。
	constexpr float swapGap = 125.f;
	//足が床に埋まらないための余白。
	constexpr float floorGap = 3.f;
	//候補の近くから歩ける床を探す幅。
	constexpr float floorSearch = 60.f;

	//床を探し、その上に体が入るか調べる。
	bool Landing(ACharacter& _who, const FVector& _near, const AActor* _other, FVector& _spot)
	{
		auto* world = _who.GetWorld();
		auto* nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(world);
		if (!nav) { return false; }

		//キャラの背の高さに合わせて歩ける床を探す。
		const auto* capsule = _who.GetCapsuleComponent();
		const float height = capsule->GetScaledCapsuleHalfHeight();
		const FVector extent(floorSearch, floorSearch, height + floorSearch);
		FNavLocation floor;
		if (!nav->ProjectPointToNavigation(_near, floor, extent)) { return false; }

		//入れ替わる二人を除いて、実際の床の傾きを調べる。
		FCollisionQueryParams query(SCENE_QUERY_STAT(ShadowLanding), false, &_who);
		query.AddIgnoredActor(_other);
		const FVector up = FVector::UpVector * height;
		const FVector start = floor.Location + up;
		const FVector end = floor.Location - up;
		FHitResult hit;
		if (!world->LineTraceSingleByChannel(hit, start, end, ECC_WorldStatic, query)) { return false; }
		if (!_who.GetCharacterMovement()->IsWalkable(hit)) { return false; }

		//体と同じ形で壁や他の敵に当たらないことを確認する。
		_spot = hit.ImpactPoint + FVector::UpVector * (height + floorGap);
		const auto body = capsule->GetCollisionShape();
		const auto profile = capsule->GetCollisionProfileName();
		return !world->OverlapBlockingTestByProfile(_spot, FQuat::Identity, profile, body, query);
	}

	//交代の前に移動指示と実際の速度を止める。
	void StopWalk(ACharacter& _who)
	{
		if (auto* ai = Cast<AAIController>(_who.GetController())) { ai->StopMovement(); }
		_who.GetCharacterMovement()->StopMovementImmediately();
	}

	//着地点から相手まで歩いて戻れるか調べる。
	bool HasPath(ABossEnemyCharacter& _boss, const FVector& _place, const FVector& _target)
	{
		const auto* path = UNavigationSystemV1::FindPathToLocationSynchronously(&_boss, _place, _target, &_boss);
		return path && path->IsValid() && !path->IsPartial();
	}

	//本体がいた場所に影を置けない時は、周りの八方向を試す。
	bool FindReturn(ABossEnemyCharacter& _boss, ABossShadow& _shadow, const FVector& _arrival, FVector& _spot)
	{
		const FVector from = _boss.GetActorLocation();
		if (Landing(_shadow, from, &_boss, _spot)) { return FVector::Dist2D(_arrival, _spot) >= swapGap; }

		for (int side = 0; side < 8; ++side)
		{
			const FVector dir = FVector::ForwardVector.RotateAngleAxis(side * 45.f, FVector::UpVector);
			if (!Landing(_shadow, from + dir * swapGap, &_boss, _spot)) { continue; }
			if (FVector::Dist2D(_arrival, _spot) > swapGap) { return true; }
		}
		return false;
	}

	//攻撃、抜刀、退避、段差移動を途中で止めない。
	bool CanStartStep(ABossEnemyCharacter& _boss)
	{
		if (!_boss.IsCombatActive() || _boss.IsActing()) { return false; }
		if (!_boss.m_approach->HasDrawn() || _boss.m_approach->IsDrawing()) { return false; }
		if (_boss.m_spacing->IsSpacing() || _boss.m_defense->IsBusy()) { return false; }
		if (_boss.m_traversal->IsTraversing()) { return false; }
		return _boss.GetCharacterMovement()->IsMovingOnGround();
	}

	//見失った相手を探している時も追跡として扱う。
	bool IsChasing(ABossEnemyCharacter& _boss, AActor* _target)
	{
		auto* ai = Cast<AAIController>(_boss.GetController());
		if (!ai) { return false; }
		if (ai->GetMoveStatus() == EPathFollowingStatus::Moving) { return true; }
		const FVector acceleration = _boss.GetCharacterMovement()->GetCurrentAcceleration();
		if (acceleration.SizeSquared2D() > 1.f) { return true; }
		return _boss.CanDecide() && !ai->LineOfSightTo(_target);
	}

	//両方が移れた時だけ交代を成立させる。
	bool SwapPlaces(ABossEnemyCharacter& _boss, ABossShadow& _shadow, const FVector& _arrival, const FVector& _returnSpot)
	{
		const FVector old = _shadow.GetActorLocation();
		const auto teleport = ETeleportType::TeleportPhysics;
		StopWalk(_boss);
		StopWalk(_shadow);
		if (!_shadow.SetActorLocation(_returnSpot, false, nullptr, teleport)) { return false; }
		if (_boss.SetActorLocation(_arrival, false, nullptr, teleport)) { return true; }

		//本体が移れなかった場合は影だけ元に戻す。
		_shadow.SetActorLocation(old, false, nullptr, teleport);
		return false;
	}

	//交代後も自分の攻撃相手を向く。
	void FaceTarget(AActor& _who, const AActor* _target)
	{
		if (!IsValid(_target)) { return; }
		const FVector direction = _target->GetActorLocation() - _who.GetActorLocation();
		_who.SetActorRotation(direction.GetSafeNormal2D().Rotation());
	}
}

//交代の動作中か、安全な距離で休んでいるか調べる。
bool UBossShadowSummon::IsChanging() const
{
	const auto* boss = Cast<ABossEnemyCharacter>(GetOwner());
	if (!boss) { return false; }
	if (m_changeMove.IsValid() && boss->GetCurrentMontage() == m_changeMove.Get()) { return true; }
	if (GetWorld()->GetTimeSeconds() >= m_restEnd) { return false; }
	const auto* target = boss->m_combatTarget.Get();
	return IsValid(target) && boss->GetDistanceTo(target) > boss->GetMeleeDistance();
}

//交代する二人だけ、影の出現を待つ。
bool UBossShadowSummon::IsWaitingForSwap(const AActor* _who) const
{
	if (!m_reserveShade.IsValid() || GetWorld()->GetTimeSeconds() >= m_rescueEnd) { return false; }
	return _who == GetOwner() || _who == m_reserveShade.Get();
}

//本体の攻撃に続けて、影の剣が同じ相手に届くか調べる。
bool UBossShadowSummon::CanCover(const ABossShadow& _partner) const
{
	const auto* boss = Cast<ABossEnemyCharacter>(GetOwner());
	if (!boss || !boss->IsActing()) { return false; }
	if (GetWorld()->GetTimeSeconds() < m_nextSwap) { return false; }

	//本体に次の一撃があり、二人が同じ相手を狙っている時だけ連携する。
	const auto* target = boss->m_combatTarget.Get();
	if (!target || target != _partner.m_combatTarget) { return false; }
	const auto* move = boss->GetActiveAction();
	if (!move || move->m_damage <= 0.f) { return false; }
	if (move->m_hitNextId.IsNone() && move->m_hitChoices.IsEmpty()) { return false; }
	if (_partner.GetDistanceTo(target) > boss->GetMeleeDistance()) { return false; }
	return _partner.GetDistanceTo(boss) >= swapGap;
}

//攻撃後に影の追撃や位置の交代を予約する。
void UBossShadowSummon::PlanAfterAttack()
{
	auto* boss = Cast<ABossEnemyCharacter>(GetOwner());
	if (!boss || !boss->HasAuthority()) { return; }
	const double now = GetWorld()->GetTimeSeconds();

	//影がいる時だけ連携を予約し、体勢を戻す判断は影がいなくても行う。
	if (m_shadow.IsValid() && !boss->GetPendingComboId().IsNone())
	{
		m_relayEnd = now + PrimaryComponentTick.TickInterval;
	}
	TryStep(*boss, now);
}

//安全な影の位置と、本体が立っていた位置を一度に交換する。
bool UBossShadowSummon::TryStep(ABossEnemyCharacter& _boss, double _now)
{
	auto* target = _boss.m_combatTarget.Get();
	if (!IsValid(target) || IsChanging() || !CanStartStep(_boss))
	{
		m_blockedSince = 0.;
		return false;
	}

	//追跡中に進めなかった時間を数える。
	const FVector from = _boss.GetActorLocation();
	const FVector targetPos = target->GetActorLocation();
	const float distance = FVector::Dist2D(from, targetPos);
	const bool chasing = distance > m_drawRange && IsChasing(_boss, target);
	if (!chasing || FVector::Dist2D(from, m_lastPlace) > stuckTravel || m_blockedSince == 0.)
	{
		m_lastPlace = from;
		m_blockedSince = chasing ? _now : 0.;
	}

	//足止め、退避、追撃のどれが必要か調べる。
	const bool rescue = _now < m_rescueEnd;
	const bool chaseBlocked = chasing && _now - m_blockedSince >= m_stuckTime;
	const bool stuck = chaseBlocked || (rescue && !m_rescuePanic);
	const bool lowPosture = _boss.m_posture->m_enabled && _boss.m_posture->GetRatio() <= m_lowPosture;
	const bool lowHP = _boss.GetHealth() <= _boss.GetMaxHealth() * m_panicHP;
	const bool threatened = lowHP && _boss.m_defense->IsTargetAttacking(target);
	const bool needsRest = lowPosture || threatened || (rescue && m_rescuePanic);
	const bool panic = distance < m_drawRange && needsRest;
	const bool relay = m_relayEnd > _now && !_boss.GetPendingComboId().IsNone();
	if (_now < m_nextSwap || (!stuck && !panic && !relay)) { return false; }

	//追撃では届く影を、退避では間合いを広げられる影を探す。
	const bool followOnly = relay && !panic && !stuck;
	const float retreatDistance = FMath::Max(distance + m_retreatGain, m_spawnRange);
	ABossShadow* partner = nullptr;
	FVector arrival, returnSpot;
	float bestFit = -TNumericLimits<float>::Max();
	for (auto* shadow : { m_shadow.Get(), m_secondShadow.Get() })
	{
		if (!IsValid(shadow) || !shadow->IsCombatActive() || shadow->IsActing()) { continue; }
		if (!shadow->GetCharacterMovement()->IsMovingOnGround()) { continue; }
		if (!IsWaitingForSwap(shadow))
		{
			if (relay && !shadow->CanRelay()) { continue; }
			if (!relay && shadow->IsRepositioning()) { continue; }
		}
		if (followOnly && shadow->m_combatTarget != target) { continue; }

		//本体と影の両方に立つ場所があり、相手まで戻れる影だけを選ぶ。
		FVector nextPlace, nextReturn;
		if (!Landing(_boss, shadow->GetActorLocation(), shadow, nextPlace)) { continue; }
		if (!FindReturn(_boss, *shadow, nextPlace, nextReturn)) { continue; }
		const float next = FVector::Dist2D(nextPlace, targetPos);
		if (panic && !stuck && next < retreatDistance) { continue; }
		if (followOnly && next > _boss.GetMeleeDistance()) { continue; }
		if (!HasPath(_boss, nextPlace, targetPos)) { continue; }

		//危険なら遠い影を、連携なら相手に近い影を選ぶ。
		const float fit = panic ? next : -next;
		if (fit <= bestFit) { continue; }
		bestFit = fit;
		partner = shadow;
		arrival = nextPlace;
		returnSpot = nextReturn;
	}
	if (!partner)
	{
		if (!stuck && !panic) { return false; }
		if (rescue || m_secondShadow.IsValid()) { return false; }
		if (!m_shadowClass || _now < m_nextTime) { return false; }
		return SpawnEscapeShadow(_boss, *target, panic, stuck, _now);
	}

	//両者の移動が成功した後にだけ、BPで選んだ煙と動作を再生する。
	if (!SwapPlaces(_boss, *partner, arrival, returnSpot)) { return false; }
	FaceTarget(_boss, target);
	FaceTarget(*partner, partner->m_combatTarget.Get());
	partner->PlaySmoke();
	partner->PlaySmokeAt(arrival);
	if (m_swapMove && _boss.PlayAnimMontage(m_swapMove) > 0.f) { m_changeMove = m_swapMove; }
	if (panic) { m_restEnd = _now + m_restTime; }
	m_nextSwap = _now + FMath::Max(1.f, m_swapDelay);
	m_reserveShade.Reset();

	//予約を使い切り、同じ相手を狙っていれば影に追撃を任せる。
	m_rescueEnd = 0.;
	m_relayEnd = 0.;
	m_blockedSince = 0.;
	const bool follow = relay && partner->m_combatTarget == target;
	if (follow) { partner->RelayStrike(m_relayAction); }
	return true;
}

//交代相手がいない時、安全な床に影を一体呼ぶ。
bool UBossShadowSummon::SpawnEscapeShadow(ABossEnemyCharacter& _boss, AActor& _target, bool _panic, bool _stuck, double _now)
{
	const FVector from = _boss.GetActorLocation();
	const FVector targetPos = _target.GetActorLocation();
	const FVector forward = (targetPos - from).GetSafeNormal2D();
	const float distance = FVector::Dist2D(from, targetPos);
	const float retreat = FMath::Max(distance + m_retreatGain, m_spawnRange);

	//横が壁なら斜めや後方を試す。退避なら遠ざかる向きを先に試す。
	for (const float turn : {90.f, -90.f, 135.f, -135.f, 180.f, 45.f, -45.f, 0.f})
	{
		const float angle = _panic ? turn + 90.f : turn;
		const FVector aim = forward.RotateAngleAxis(angle, FVector::UpVector);
		const FVector near = from + aim * m_spawnRange;
		FVector place;
		if (!Landing(_boss, near, nullptr, place)) { continue; }
		if (FVector::Dist2D(from, place) < swapGap) { continue; }
		if (_panic && FVector::Dist2D(place, targetPos) < retreat) { continue; }
		if (!HasPath(_boss, place, targetPos)) { continue; }

		//BPで指定した影に相手を渡してから、出現させる。
		const FRotator facing = (targetPos - place).GetSafeNormal2D().Rotation();
		const FTransform pose(facing, place);
		const auto collision = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
		auto* shade = GetWorld()->SpawnActorDeferred<ABossShadow>(m_shadowClass, pose, &_boss, nullptr, collision);
		if (!shade) { continue; }
		shade->Setup(&_target, FMath::Max(m_lifeMin, 1.f));
		UGameplayStatics::FinishSpawningActor(shade, pose);
		if (!IsValid(shade)) { continue; }

		//次の判断で同じ影と交代できるよう、相手と期限を覚える。
		if (m_shadow.IsValid()) { m_secondShadow = shade; }
		else { m_shadow = shade; }
		StopWalk(_boss);
		m_reserveShade = shade;
		m_rescuePanic = _panic && !_stuck;
		m_rescueEnd = _now + shade->m_riseTime + 2.;
		m_nextTime = _now + m_cooldown;
		return true;
	}

	//空きがなければ少し待って探し直す。
	m_nextSwap = _now + 1.;
	return false;
}
