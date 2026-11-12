#include "Enemy/Core/EnemyTraversal.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Enemy/Boss/BossEnemyCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Engine/World.h"

//初期設定と必要な部品を用意する。
UEnemyTraversal::UEnemyTraversal()
{
	PrimaryComponentTick.bCanEverTick = true;
}

//越えられる段差を探して移動を開始する。
bool UEnemyTraversal::TryTraverse(AActor* target)
{

	//処理の対象となる敵。
	auto* enemy = Cast<AEnemyCharacterBase>(GetOwner());
    if(!m_enabled || m_running || !enemy || !enemy->HasAuthority() || !IsValid(target) || !enemy->CanDecide() || !enemy->GetPendingComboId().IsNone())
    {
		return false;
	}

	//移動を管理する部品。
	auto* move = enemy->GetCharacterMovement();
	if (!move->IsMovingOnGround()) { return false; }

	//調べ始める位置。
	const FVector start = enemy->GetActorLocation();

	//相手へ向かう方向。
	const FVector toward = (target->GetActorLocation() - start).GetSafeNormal2D();

	//相手までの距離。
	const float distance = FVector::Dist2D(start, target->GetActorLocation());
	if (toward.IsNearlyZero() || distance < enemy->GetMeleeDistance()) { return false; }

	//正面の弧が通らない時は、相手に近づける左右の方向も調べる。
	for (float angle : { 0.f, -m_sideAngle, m_sideAngle })
	{
		const FVector direction = toward.RotateAngleAxis(angle, FVector::UpVector);
		bool needsJump = false;
		if (TryDirection(*enemy, *target, direction, distance, needsJump)) { return true; }
		if (angle == 0.f && !needsJump) { return false; }
	}
	return false;
}

//壁や床の切れ目の先に着地できるか調べる。
bool UEnemyTraversal::TryDirection(AEnemyCharacterBase& owner, AActor& target, const FVector& toward, float distance, bool& needsJump)
{
	auto* enemy = &owner;
	auto* move = enemy->GetCharacterMovement();
	const FVector start = enemy->GetActorLocation();

	//当たり判定の半分の高さ。
	const float half = enemy->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	//判定に使う半径。
	const float radius = enemy->GetCapsuleComponent()->GetScaledCapsuleRadius();

	//足元の位置。
	const FVector feet = start - FVector(0, 0, half);

	//生成や当たり判定に使う設定。
	FCollisionQueryParams params(SCENE_QUERY_STAT(EnemyTraversal), false, enemy);

	//前方の障害物の情報。
	FHitResult obstacle;

	//進行方向の障害物の情報。
	const FVector ahead = feet + toward * (m_probe + radius);
	const FVector lift = FVector::UpVector * (move->MaxStepHeight + 2.f);
	const bool wall = GetWorld()->LineTraceSingleByChannel(obstacle, feet + lift, ahead + lift, ECC_Pawn, params);
	if (wall && Cast<APawn>(obstacle.GetActor())) { return false; }

    //移動先の下にある地面。
	FHitResult support;
	const bool supported = GetWorld()->LineTraceSingleByChannel(support, ahead + lift, ahead - lift, ECC_Pawn, params);

	//歩ける場所を調べる機能。
	auto* nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!nav) { return false; }
	const auto& agent = move->GetNavAgentPropertiesRef();
	const FVector extent(25, 25, 25);
	FNavLocation edge;
	const bool missingFloor = !supported || !move->IsWalkable(support);
	const bool leavingNav = !missingFloor && !nav->ProjectPointToNavigation(support.ImpactPoint, edge, extent, &agent)
		&& nav->ProjectPointToNavigation(feet, edge, extent, &agent);
	const bool gap = missingFloor || leavingNav;
	if (!wall && !gap) { return false; }
	needsJump = true;

	//落下に使う重力。
	const float gravity = -move->GetGravityZ();
	if (gravity < 1.f) { return false; }

	//低い壁でも、その向こうの床だけを見て低く跳びすぎないようにする。
	float wallHeight = 0.f;
	if (wall)
	{
		const FVector atWall = obstacle.ImpactPoint + toward * 5.f;
		const FVector above = atWall + FVector::UpVector * m_up;
		FHitResult top;
		if (GetWorld()->LineTraceSingleByChannel(top, above, atWall, ECC_Pawn, params))
		{
			wallHeight = top.ImpactPoint.Z - feet.Z;
		}
	}
	float reach = FMath::Max(radius * 2.f + 20.f, wall ? obstacle.Distance + radius + 25.f : m_probe + radius);
    for(; reach <= FMath::Min(m_range, distance - 60.f); reach += 35.f)
    {
		//経路の途中の確認位置。
		const FVector sample = feet + toward * reach;

		//地面に当たった情報。
		FHitResult floor;

		//移動先の上から下へ線を引き、歩ける床を探す。
		const FVector top = sample + FVector(0, 0, m_up + 25.f);
		const FVector bottom = sample - FVector(0, 0, m_down + 10.f);
		const bool foundFloor = GetWorld()->LineTraceSingleByChannel(floor, top, bottom, ECC_Pawn, params);
		if (!foundFloor || Cast<APawn>(floor.GetActor()) || !move->IsWalkable(floor)) { continue; }

		//高さの計算に使う値。
		const float height = floor.ImpactPoint.Z - feet.Z;
        if(height > m_up || height < -m_down)
        {
			continue;
		}

		//着地する位置。
		FNavLocation landing;
		const bool onNav = nav->ProjectPointToNavigation(floor.ImpactPoint, landing, extent, &agent);
		if (onNav && FMath::Abs(landing.Location.Z - floor.ImpactPoint.Z) > 20.f) { continue; }
		if (!onNav)
		{
			if (!enemy->IsA<ABossEnemyCharacter>() || (!wall && !missingFloor)) { continue; }

			//道の外でも、前後左右すべてに歩ける床がある時だけ着地する。
			bool wideFloor = true;
			for (const FVector& side : { FVector::ForwardVector, -FVector::ForwardVector, FVector::RightVector, -FVector::RightVector })
			{
				const FVector foot = floor.ImpactPoint + side * radius;
				FHitResult ground;
				if (!GetWorld()->LineTraceSingleByChannel(ground, foot + lift, foot - lift, ECC_Pawn, params)
					|| !move->IsWalkable(ground) || Cast<APawn>(ground.GetActor()))
				{
					wideFloor = false;
					break;
				}
			}
			if (!wideFloor) { continue; }
		}

		//調べ終える位置。
		const FVector end = floor.ImpactPoint + FVector(0, 0, half + 3.f);
		if (FVector::Dist2D(end, target.GetActorLocation()) >= distance) { continue; }

		//キャラクターの当たり判定。
		const FCollisionShape capsule = FCollisionShape::MakeCapsule(radius, half);
		if (GetWorld()->OverlapBlockingTestByChannel(end, FQuat::Identity, ECC_Pawn, capsule, params))
		{
			continue;
		}

		//上下の位置の差。
		const float dz = end.Z - start.Z;

		//上下方向の速度。
		const float apex = FMath::Max3(0.f, dz, wallHeight) + m_clearance;
		const float vz = FMath::Sqrt(2.f * gravity * apex);

		//処理に使う時間。
		const float time = (vz + FMath::Sqrt(FMath::Max(0.f, vz * vz - 2.f * gravity * dz))) / gravity;

		//現在の移動速度。
		FVector velocity = (end - start) / time;
		velocity.Z = vz;
		if (velocity.Size2D() > m_speed) { continue; }

		//経路に障害物がないか。
		bool clear = true;
		FVector previous = start;
		for (int32 i = 1; i <= 32; ++i)
		{
			//経路上の進み具合。
			const float t = time * i / 32.f;

			//次に使う区間または候補。
			const FVector next = start + velocity * t - FVector(0, 0, .5f * gravity * t * t);

			//ぶつかった場所の情報。
			FHitResult hit;
			if (GetWorld()->SweepSingleByChannel(hit, previous, next, FQuat::Identity, ECC_Pawn, capsule, params))
			{
				clear = false;
				break;
			}
			previous = next;
		}
		if (!clear) { continue; }

		if (auto* ai = Cast<AAIController>(enemy->GetController())) { ai->StopMovement(); }

		move->StopMovementImmediately();
		enemy->SetActorRotation(toward.Rotation());
		m_running = true;
		m_started = GetWorld()->GetTimeSeconds();
		m_next = m_started + time + m_interval;
		enemy->LaunchCharacter(velocity, true, true);
		return true;
	}
	return false;
}

//経過時間に合わせて敵の状態を更新する。
void UEnemyTraversal::TickComponent(float delta, ELevelTick type, FActorComponentTickFunction* function)
{
	Super::TickComponent(delta, type, function);

	//処理の対象となる敵。
	auto* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	if (!enemy || !enemy->HasAuthority()) { return; }

	//現在の時刻。
	const double now = GetWorld()->GetTimeSeconds();
	if (m_running)
	{
        if(!enemy->IsCombatActive() || now - m_started > 3. ||
           (now - m_started > .1 && enemy->GetCharacterMovement()->IsMovingOnGround()))
        {
			m_running = false;
		}	
		return;
	}
	if (now < m_next) { return; }
	m_next = now + FMath::Max(.1f, m_interval);
	TryTraverse(enemy->m_combatTarget);
}
