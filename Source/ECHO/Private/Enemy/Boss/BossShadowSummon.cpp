#include "Enemy/Boss/BossShadowSummon.h"
#include "Enemy/Boss/BossShadow.h"
#include "Ghost/GhostCharacter/GhostCharacter.h"
#include "Ghost/GhostEnemy/GhostEnemyComponent.h"
#include "NavigationSystem.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Enemy/Combat/Defense.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace BossShadowTuning
{
	//分身を探す間隔。
	constexpr float checkDelay = .5f;

	//生成する床を探す横幅。
	constexpr float floorWidth = 100.f;

	//床との接触で生成に失敗しないための余白。
	constexpr float floorGap = 3.f;

	//左右がふさがっていても、周りの安全な床を探す。
	bool FindFloor(ABossEnemyCharacter& _boss, const ABossShadow& _shade, float _range, FVector& _place)
	{
		auto* world = _boss.GetWorld();
		auto* nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(world);
		if (!nav) { return false; }

		const auto* body = _shade.GetCapsuleComponent();
		const float height = body->GetScaledCapsuleHalfHeight();
		const FVector extent(floorWidth, floorWidth, height + floorWidth);
		FCollisionQueryParams query(SCENE_QUERY_STAT(ShadowSpawn), false, &_boss);

		for (const float angle : { 90.f, -90.f, 135.f, -135.f, 45.f, -45.f, 180.f, 0.f })
		{
			const FVector direction = _boss.GetActorForwardVector().RotateAngleAxis(angle, FVector::UpVector);
			const FVector point = _boss.GetNavAgentLocation() + direction * _range;
			FNavLocation floor;
			if (!nav->ProjectPointToNavigation(point, floor, extent)) { continue; }

			const FVector up = FVector::UpVector * height;
			FHitResult hit;
			if (!world->LineTraceSingleByChannel(hit, floor.Location + up, floor.Location - up, ECC_WorldStatic, query)) { continue; }
			if (!_shade.GetCharacterMovement()->IsWalkable(hit)) { continue; }

			_place = hit.ImpactPoint + FVector::UpVector * (height + floorGap);
			const auto shape = body->GetCollisionShape();
			const auto profile = body->GetCollisionProfileName();
			if (!world->OverlapBlockingTestByProfile(_place, FQuat::Identity, profile, shape, query)) { return true; }
		}
		return false;
	}
}

//分身の確認は毎フレームではなく一定間隔で行う。
UBossShadowSummon::UBossShadowSummon()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = BossShadowTuning::checkDelay;
}

//本体の戦闘が終われば影も消す。
void UBossShadowSummon::Reset()
{
	for (auto* shadow : { m_shadow.Get(), m_secondShadow.Get() })
	{
		if (IsValid(shadow)) { shadow->FadeOut(); }
	}
	m_restEnd = m_rescueEnd = m_nextSwap = m_blockedSince = m_relayEnd = 0.;
	m_lastPlace = FVector::ZeroVector;
	m_changeMove.Reset();
	m_reserveShade.Reset();
	m_nextTime = 0.;
	m_nextJoinTime = 0.;
	m_lastJoin = INDEX_NONE;
	m_joinMisses = 0;
	m_joinReady = false;
	m_cloneCount = 0;
}

//削除時は影のAIも残さず片付ける。
void UBossShadowSummon::EndPlay(const EEndPlayReason::Type _reason)
{
	for (auto* shadow : { m_shadow.Get(), m_secondShadow.Get() })
	{
		if (!IsValid(shadow)) { continue; }
		if (auto* ai = shadow->GetController())
		{
			ai->UnPossess();
			ai->Destroy();
		}
		shadow->Destroy();
	}
	Super::EndPlay(_reason);
}

//空いている場所へ影を呼び、同時に二体までにする。
void UBossShadowSummon::TickComponent(float _delta, ELevelTick _type, FActorComponentTickFunction* _tick)
{
	Super::TickComponent(_delta, _type, _tick);
	auto* boss = Cast<ABossEnemyCharacter>(GetOwner());
	if (!boss || !boss->HasAuthority() || boss->IsA<ABossShadow>()) { return; }
	if (!boss->IsCombatActive())
	{
		m_spawnState = TEXT("NotInCombat");
		for (auto* shadow : { m_shadow.Get(), m_secondShadow.Get() })
		{
			if (IsValid(shadow)) { shadow->FadeOut(); }
		}
		return;
	}
	const double now = GetWorld()->GetTimeSeconds();

	//一体目が消えたら、残った影を一体目として扱う。
	if (!m_shadow.IsValid() && m_secondShadow.IsValid())
	{
		m_shadow = m_secondShadow;
		m_secondShadow.Reset();
	}

	//味方として活動中の分身から、担当が重ならない相手を探す。
	AActor* target = nullptr;
	float best = FMath::Square(m_range);
	m_cloneCount = 0;
	for (TActorIterator<AGhostCharacter> it(GetWorld()); it; ++it)
	{
		auto* ghost = *it;
		if (!ghost->EnemyComponent || ghost->IsHidden() || !ghost->CanBeDamaged()) { continue; }
		const auto state = ghost->EnemyComponent->GetLifecycleState();
		if (state != EGhostLifecycleState::Friendly && state != EGhostLifecycleState::Warning)
		{
			continue;
		}
		const float distance = FVector::DistSquared2D(boss->GetActorLocation(), ghost->GetActorLocation());
		if (distance <= FMath::Square(m_range)) { ++m_cloneCount; }
		if (m_shadow.IsValid() && m_shadow->m_combatTarget == ghost) { continue; }
		if (distance < best)
		{
			best = distance;
			target = ghost;
		}
	}
	if (TryStep(*boss, now)) { m_spawnState = TEXT("Swapping"); return; }
	const bool second = m_shadow.IsValid();
	if (!m_shadowClass) { m_spawnState = TEXT("NoShadowClass"); return; }
	if (m_secondShadow.IsValid()) { m_spawnState = TEXT("TwoShadowsActive"); return; }
	if (boss->m_defense->IsBusy()) { m_spawnState = TEXT("Defending"); return; }
	if (second && (!target || m_cloneCount < FMath::Max(2, m_twoCloneCount)))
	{
		m_spawnState = TEXT("ShadowActive");
		return;
	}
	const bool firstPhase = boss->m_bossPhase == EBossPhase::PhaseOne;

	//フェーズ1で対応する分身の数はBPの設定で決める。
	if (firstPhase && target && m_cloneCount < m_cloneLimit) { m_spawnState = TEXT("CloneCount"); return; }

	//指定した本体の攻撃に合わせ、分身がいなければ連携する。
	const bool join = !target;

	//対分身と連携は、それぞれの待ち時間を使う。
	const double nextTime = join ? m_nextJoinTime : m_nextTime;
	if (now < nextTime) { m_spawnState = TEXT("Cooldown"); return; }
	if (join)
	{
		const auto* action = boss->GetActiveAction();
		auto* player = UGameplayStatics::GetPlayerPawn(this, 0);
        if(!m_enableJoin || !action || !m_joinMoves.Contains(action->m_actionId) || !IsValid(player) || !player->CanBeDamaged())
        {
			m_spawnState = TEXT("WaitingForJoinMove");
			return;
		}
        if(FVector::DistSquared2D(boss->GetActorLocation(), player->GetActorLocation()) > FMath::Square(m_joinRange))
        {
			m_spawnState = TEXT("OutsideJoinRange");
			return;
		}
		if (firstPhase && !m_phaseOneJoin) { m_spawnState = TEXT("PhaseOneJoinDisabled"); return; }
		//同じ攻撃では抽選を一度だけ行う。
		if (m_lastJoin != boss->m_actionSerial)
		{
			m_lastJoin = boss->m_actionSerial;
			const bool waited = m_joinLimit > 0 && m_joinMisses >= m_joinLimit - 1;
			const float chance = FMath::Clamp(m_joinChance, 0.f, 1.f);
			m_joinReady = waited || FMath::FRand() < chance;
			if (!m_joinReady) { ++m_joinMisses; }
		}
		if (!m_joinReady) { m_spawnState = TEXT("JoinRoll"); return; }
		target = player;
	}

	//壁や床に埋まる場所には生成しない。
	const auto* defaults = m_shadowClass->GetDefaultObject<ABossShadow>();
    FVector location;
    if (!BossShadowTuning::FindFloor(*boss, *defaults, m_spawnRange, location)) { m_spawnState = TEXT("NoSafeFloor"); return; }
    const FTransform pose((target->GetActorLocation() - location).Rotation(), location);
    auto *shadow = GetWorld()->SpawnActorDeferred<ABossShadow>(m_shadowClass, pose, boss, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding);
    if (!shadow)
	{
		m_spawnState = TEXT("SpawnFailed");
		m_nextTime = now + PrimaryComponentTick.TickInterval;
		return;
	}

	shadow->Setup(target, FMath::FRandRange(FMath::Max(1.f, m_lifeMin), FMath::Max(m_lifeMin, m_lifeMax)));
	if (join) { shadow->StartJoin(m_joinAction, m_joinDelay, m_range); }
	UGameplayStatics::FinishSpawningActor(shadow, pose);
	if (!IsValid(shadow)) { return; }
	m_spawnState = TEXT("Summoned");

    //実際に呼べた時だけ抽選待ちの記録を消す。
    m_joinMisses = 0;
    m_joinReady = false;

	if (second) { m_secondShadow = shadow; }
	else
	{
		m_shadow = shadow;
	}

	//五体以上なら、次の確認で二体目も呼べるようにする。
	const bool needSecond = !second && !join && m_cloneCount >= FMath::Max(2, m_twoCloneCount);
    if (join)
    {
        m_nextJoinTime = now + FMath::Max(0.f, m_joinCooldown);
    }
    else
    {
        const float delay = needSecond ? PrimaryComponentTick.TickInterval : m_cooldown;
        m_nextTime = now + FMath::Max(0.f, delay);
    }
}
