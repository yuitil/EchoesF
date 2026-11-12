#include "Enemy/Boss/BossCrowd.h"
#include "Enemy/Audio/EnemyAudio.h"
#include "Enemy/Boss/BossEnemyCharacter.h"
#include "Enemy/Boss/BossShadow.h"
#include "Ghost/GhostCharacter/GhostCharacter.h"
#include "Ghost/GhostEnemy/GhostEnemyComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

namespace BossCrowdTuning
{
	//飛距離と空中時間の調整範囲。
	constexpr float minDistance = 300.f;
	constexpr float maxDistance = 500.f;
	constexpr float minAirTime = .2f;
	constexpr float maxAirTime = 1.5f;

	//一定の加速で進む距離を計算する係数。
	constexpr float half = .5f;
}

//近さと高さを調べ、壁越しの相手を除く。
bool UBossCrowd::IsNear(ACharacter* _target) const
{
	if (!IsValid(_target) || _target == GetOwner() || _target->IsHidden() || !_target->CanBeDamaged())
	{
		return false;
	}
	const FVector start = GetOwner()->GetActorLocation();
	const FVector end = _target->GetActorLocation();
	if (FVector::DistSquared2D(start, end) > FMath::Square(m_radius) || FMath::Abs(start.Z - end.Z) > m_height)
	{
		return false;
	}
	FCollisionQueryParams query;
	query.AddIgnoredActor(GetOwner());
	query.AddIgnoredActor(_target);
	FHitResult hit;
	return !GetWorld()->LineTraceSingleByObjectType(hit, start, end, FCollisionObjectQueryParams(ECC_WorldStatic), query);
}

//味方状態の分身とプレイヤーだけを対象にする。
void UBossCrowd::GetTargets(TArray<ACharacter*>& _targets, int32& _clones) const
{
	_targets.Reset();
	_clones = 0;
	for (TActorIterator<AGhostCharacter> it(GetWorld()); it; ++it)
	{
		auto* ghost = *it;
		if (!ghost->EnemyComponent || !IsNear(ghost)) { continue; }
		const auto state = ghost->EnemyComponent->GetLifecycleState();
		if (state != EGhostLifecycleState::Friendly && state != EGhostLifecycleState::Warning)
		{
			continue;
		}
		_targets.AddUnique(ghost);
		++_clones;
	}
	auto* player = UGameplayStatics::GetPlayerCharacter(this, 0);
	if (IsNear(player)) { _targets.AddUnique(player); }
}

//気絶中や空中では新しい技を始めない。
bool UBossCrowd::CanPush() const
{
	const auto* boss = Cast<ABossEnemyCharacter>(GetOwner());
	if (!m_enabled || !boss || boss->IsA<ABossShadow>() || !boss->IsCombatActive() || boss->GetCharacterMovement()->IsFalling())
	{
		return false;
	}
	if (GetWorld()->GetTimeSeconds() < m_nextTime) { return false; }
	TArray<ACharacter*> targets;
	int32 clones = 0;
	GetTargets(targets, clones);
	const bool crowded = clones >= FMath::Max(1, m_count);
	const bool waited = m_blockSince >= 0. && GetWorld()->GetTimeSeconds() - m_blockSince >= m_blockWait;
	const bool blocked = waited && clones >= FMath::Max(1, m_blockCount);
	return crowded || blocked;
}

//攻撃や気絶ではなく、追跡が足止めされた時間だけ数える。
void UBossCrowd::UpdateBlock()
{
	//相手へ移動中かを調べる。
	auto* boss = Cast<ABossEnemyCharacter>(GetOwner());
	auto* ai = boss ? Cast<AAIController>(boss->GetController()) : nullptr;
	const bool moving = ai && ai->GetMoveStatus() == EPathFollowingStatus::Moving;
	if (!boss || !moving || !boss->CanDecide())
	{
		m_blockSince = -1.;
		return;
	}

	//少しずつ進んでいる時は、足止めとみなさない。
	const FVector place = boss->GetActorLocation();
	const float travel = FVector::Dist2D(place, m_lastPlace);
	if (m_blockSince < 0. || travel >= m_blockTravel)
	{
		m_lastPlace = place;
		m_blockSince = GetWorld()->GetTimeSeconds();
	}
}

//既存の攻撃管理を使うので、途中の攻撃を切らない。
bool UBossCrowd::TryPush()
{
	UpdateBlock();
	auto* boss = Cast<ABossEnemyCharacter>(GetOwner());
	if (!boss || !boss->HasAuthority() || boss->IsActing() || !CanPush()) { return false; }

	//遠くのプレイヤーとの距離で、足元の分身への反撃を止めない。
	TArray<ACharacter*> targets;
	int32 clones = 0;
	GetTargets(targets, clones);
	AActor* previous = boss->m_combatTarget.Get();
	if (targets.IsEmpty()) { return false; }
	if (!targets.Contains(Cast<ACharacter>(previous))) { boss->m_combatTarget = targets[0]; }
	if (!boss->RequestAction(m_action))
	{
		boss->m_combatTarget = previous;
		return false;
	}
	m_nextTime = GetWorld()->GetTimeSeconds() + m_cooldown;
	m_blockSince = -1.;
	return true;
}

//予備動作を終えた通知の位置で、外向きの力を加える。
void UBossCrowd::Release()
{
	auto* boss = Cast<ABossEnemyCharacter>(GetOwner());
	if (!boss || !boss->HasAuthority() || !boss->IsCombatActive() || !boss->IsActing())
	{
		return;
	}
	const auto* action = boss->GetActiveAction();
	if (!action || action->m_actionId != m_action || m_lastSerial == boss->m_actionSerial)
	{
		return;
	}
	m_lastSerial = boss->m_actionSerial;
	m_nextTime = GetWorld()->GetTimeSeconds() + m_cooldown;

	TArray<ACharacter*> targets;
	int32 clones = 0;
	GetTargets(targets, clones);
	const float airTime = FMath::Clamp(m_airTime, BossCrowdTuning::minAirTime, BossCrowdTuning::maxAirTime);
	for (auto* target : targets)
	{
		auto* move = target->GetCharacterMovement();
		if (!move || !move->IsActive() || move->MovementMode == MOVE_None) { continue; }
		FVector direction = (target->GetActorLocation() - boss->GetActorLocation()).GetSafeNormal2D();
		if (direction.IsNearlyZero()) { direction = boss->GetActorForwardVector(); }

		//相手の空中ブレーキで短くなる分を、最初の勢いに加える。
		const float distance = FMath::Clamp(m_distance, BossCrowdTuning::minDistance, BossCrowdTuning::maxDistance);
		const float brake = FMath::Max(0.f, move->BrakingDecelerationFalling);
		FVector speed = direction * (distance / airTime + brake * airTime * BossCrowdTuning::half);
		speed.Z = FMath::Abs(move->GetGravityZ()) * airTime * BossCrowdTuning::half;
		if (auto* ai = Cast<AAIController>(target->GetController())) { ai->StopMovement(); }
		target->LaunchCharacter(speed, true, true);
	}

	//見た目と音の素材はBPに保存したものを使う。
	if (GetNetMode() == NM_DedicatedServer) { return; }
	const FVector floor = boss->GetActorLocation() - FVector(0, 0, boss->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	if (m_fx) { UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, m_fx, floor); }
	if (m_sound && boss->m_audio)
	{
		UGameplayStatics::PlaySoundAtLocation(this, m_sound, floor, FRotator::ZeroRotator, 1.f, 1.f, 0.f, boss->m_audio->m_range, boss->m_audio->m_limit, boss);
	}
}

//再戦時に前の攻撃番号を持ち越さない。
void UBossCrowd::Reset()
{
	m_nextTime = 0.;
	m_lastSerial = INDEX_NONE;
	m_blockSince = -1.;
	m_lastPlace = FVector::ZeroVector;
}
