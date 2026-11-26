#include "Enemy/Minion/FlyingMinion.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"

//BPで選んだ出現動作を再生する。
void AFlyingMinion::BeginPlay()
{
	Super::BeginPlay();
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);

	//動作の終了までは攻撃を選ばない。
	if (!m_spawnMove) { return; }
	const float length = PlayAnimMontage(m_spawnMove);
	m_readyTime = GetWorld()->GetTimeSeconds() + length;
	m_spawnUntil = m_readyTime;
}

//攻撃の選択はBTに任せ、ここでは安全な移動だけを行う。
void AFlyingMinion::Tick(float _delta)
{
	Super::Tick(_delta);
	if (!HasAuthority() || !IsCombatActive()) { return; }

	//出現中だけ止め、射撃後の待ち時間も飛行を続ける。
	if (GetWorld()->GetTimeSeconds() < m_spawnUntil)
	{
		GetCharacterMovement()->StopMovementImmediately();
		return;
	}

	//地面が見つかる範囲だけを飛ぶ。
	const FVector here = GetActorLocation();
	const FVector below = here - FVector(0.f, 0.f, m_floorDepth);
	FCollisionQueryParams query;
	query.AddIgnoredActor(this);

	//キャラクターの頭を地面として扱わない。
	const FCollisionObjectQueryParams ground(ECC_WorldStatic);
	FHitResult floor;
	const bool hasFloor = GetWorld()->LineTraceSingleByObjectType(floor, here, below, ground, query);
	if (!hasFloor) { GetCharacterMovement()->StopMovementImmediately(); return; }

	//相手に近すぎると離れ、遠すぎると近づく。
	FVector direction = FVector::ZeroVector;
	const float elapsed = GetWorld()->GetTimeSeconds() - m_spawnUntil;
	const float phase = elapsed * UE_TWO_PI / FMath::Max(1.f, m_flightPeriod);
	if (IsValid(m_combatTarget))
	{
		const FVector offset = m_combatTarget->GetActorLocation() - here;
		const FVector forward = offset.GetSafeNormal2D();
		const float range = offset.Size2D();
		if (range > m_shotRange + m_rangeMargin) { direction = forward; }
		else if (range < m_shotRange - m_rangeMargin) { direction = -forward; }

		//近づく時も離れる時も、BPで決めた横移動を加える。
		const FVector side = FVector::CrossProduct(FVector::UpVector, forward);
		direction += side * (m_sideSpeed * FMath::Cos(phase * .5f));
	}

	//高さのずれを緩やかに直す。
	const float rise = m_heightTravel * (.5f - .5f * FMath::Cos(phase));
	const float height = floor.ImpactPoint.Z + m_hoverHeight + rise - here.Z;
	direction.Z = FMath::Clamp(height / FMath::Max(1.f, m_hoverHeight), -1.f, 1.f);

	//壁は横方向だけ調べ、下降先の床を壁として扱わない。
	const FVector wallDirection = direction.GetSafeNormal2D();
	const FVector ahead = here + wallDirection * m_wallDistance;
	const float radius = GetCapsuleComponent()->GetScaledCapsuleRadius();
	const FCollisionShape shape = FCollisionShape::MakeSphere(radius);
	FHitResult wall;
	const bool blocked = GetWorld()->SweepSingleByChannel(wall, here, ahead, FQuat::Identity, ECC_WorldStatic, shape, query);
	if (blocked) { direction = FVector::VectorPlaneProject(direction, wall.Normal); }
	AddMovementInput(direction.GetSafeNormal(), FMath::Min(direction.Size(), 1.f));
}

//死亡した飛行敵をその場に浮かせたままにしない。
void AFlyingMinion::HandleHealthDepleted()
{
	Super::HandleHealthDepleted();

	//倒れた後も床との当たり判定を残し、AIが外れても落下を続ける。
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	GetCharacterMovement()->bRunPhysicsWithNoController = true;
	GetCharacterMovement()->SetMovementMode(MOVE_Falling);
}
