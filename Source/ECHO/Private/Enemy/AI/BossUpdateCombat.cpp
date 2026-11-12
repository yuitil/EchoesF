
#include "Enemy/AI/BossUpdateCombat.h"

#include "Enemy/Core/EnemyCharacterBase.h"
#include "Enemy/Boss/BossEnemyCharacter.h"
#include "Enemy/Boss/BossCrowd.h"
#include "Enemy/Core/EnemyTraversal.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Enemy/Combat/Defense.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationPath.h"
#include "Enemy/Boss/BossShadow.h"

#include "Enemy/AI/EnemyTargeting.h"

//動作を確かめるための幅や待ち時間。
namespace BossUpdateCombatTuning
{
	//状態を更新する間隔。
	constexpr auto updateDelay = 0.2f;

	//距離の判定に持たせる余裕。
	constexpr auto rangeMargin = 20.f;

	//相手がいない時に使う距離。
	constexpr auto noTargetRange = 99999.0f;

	//入口まで道が続かない時も、その方向で一番先まで歩ける床を探す。
	bool ApproachClue(AAIController& ai, AEnemyCharacterBase& enemy, const FVector& clue)
	{
		const FVector start = enemy.GetNavAgentLocation();
		const auto& agent = enemy.GetCharacterMovement()->GetNavAgentPropertiesRef();
		auto* world = enemy.GetWorld();
		auto* nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(world);
		if (!nav || FVector::Dist2D(start, clue) <= enemy.GetMeleeDistance()) { return false; }

		const FVector extent(agent.AgentRadius * 2.f, agent.AgentRadius * 2.f, agent.AgentHeight);
		for (float part : { 1.f, .75f, .5f, .25f })
		{
			FNavLocation floor;
			if (!nav->ProjectPointToNavigation(FMath::Lerp(start, clue, part), floor, extent, &agent)) { continue; }
			if (FVector::Dist2D(start, floor.Location) <= agent.AgentRadius) { continue; }
			const auto* path = nav->FindPathToLocationSynchronously(world, start, floor.Location, &enemy);
			if (!path || !path->IsValid() || path->IsPartial()) { continue; }
			return ai.MoveToLocation(floor.Location, agent.AgentRadius, false) != EPathFollowingRequestResult::Failed;
		}
		return false;
	}
}

//初期設定と必要な部品を用意する。
UBossUpdateCombat::UBossUpdateCombat()
{
	NodeName = TEXT("Boss Update Combat");
	Interval = BossUpdateCombatTuning::updateDelay;
	RandomDeviation = 0.0f;
	bNotifyTick = true;
	bCreateNodeInstance = true;
}

//BTが判断に使う敵と相手の状態を更新する。
void UBossUpdateCombat::TickNode(UBehaviorTreeComponent& _ownerComp, uint8* _nodeMemory, float _delta)
{
	Super::TickNode(_ownerComp, _nodeMemory, _delta);

	//敵の行動を操作するAI。
	AAIController* aiController = _ownerComp.GetAIOwner();

	//処理の対象となる敵。
	AEnemyCharacterBase* enemy = aiController ? Cast<AEnemyCharacterBase>(aiController->GetPawn()) : nullptr;

	//BTが判断に使う値の保存先。
	UBlackboardComponent* blackboard = _ownerComp.GetBlackboardComponent();
	if (!enemy || !blackboard) { return; }

	//狙っている相手。
	AActor* target = enemy->m_targeting->SelectTarget();

	//初めて会う前は待機し、見失った後だけ最後に見た場所から探す。
	if (!enemy->IsA<ABossShadow>() && enemy->IsCombatActive())
	{
		//イベントで呼ばれたボスは、最初に入口の物音を調べる。
		if (!m_lastSeen.IsSet() && enemy->IsEventEnemy())
		{
			if (auto* player = UGameplayStatics::GetPlayerCharacter(enemy, 0))
			{
				m_lastSeen = player->GetNavAgentLocation();
			}
		}
		const bool visible = IsValid(target) && aiController->LineOfSightTo(target);
		if (!enemy->m_targeting->m_trackPlayer && IsValid(target) && !visible && !enemy->IsActing() && enemy->GetDistanceTo(target) > enemy->m_targeting->m_range)
		{
			target = nullptr;
		}
		if (visible) { m_lastSeen = target->GetActorLocation(); }
		if (IsValid(target) && m_searching)
		{
			aiController->StopMovement();
			m_searching = false;
		}
		if (!IsValid(target) && m_lastSeen.IsSet() && !enemy->IsActing() && !enemy->IsRepositioning() && !enemy->m_defense->IsBusy())
		{
			const double now = enemy->GetWorld()->GetTimeSeconds();
			if (now >= m_nextLook && (!m_searching || aiController->GetMoveStatus() == EPathFollowingStatus::Idle))
			{
				FNavLocation point;
				auto* nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(enemy->GetWorld());
				const bool approaching = BossUpdateCombatTuning::ApproachClue(*aiController, *enemy, m_lastSeen.GetValue());
				if (!approaching && nav && (nav->GetRandomReachablePointInRadius(m_lastSeen.GetValue(), m_searchRadius, point) || nav->GetRandomReachablePointInRadius(enemy->GetActorLocation(), m_searchRadius, point)))
				{
					aiController->MoveToLocation(point.Location);
				}
				m_searching = true;
				m_nextLook = now + 1.;
			}
		}
	}

	if (!IsValid(target) && enemy->IsActing()) enemy->CancelAction();

	enemy->m_combatTarget = target;
	enemy->CheckComboRange();

	//囲まれていて行動できる時は、通常の抽選より先に押し返す。
	if (auto* boss = Cast<ABossEnemyCharacter>(enemy); boss && boss->m_crowd->TryPush())
	{
		target = boss->m_combatTarget.Get();
	}

	//生きていて戦闘できるか。
	const bool alive = enemy->IsCombatActive();

	blackboard->SetValueAsObject(TEXT("TargetActor"), target);

	//行動の共通条件は敵本体で一度だけ調べる。
	const bool canDecide = enemy->CanDecide();

	//相手までの距離を、攻撃と追跡で共有する。
	const float distance = IsValid(target) ? FVector::Dist2D(enemy->GetActorLocation(), target->GetActorLocation()) : BossUpdateCombatTuning::noTargetRange;

	//コンボ中でなく、剣が届かない時は近づく。
	bool canMove = canDecide && !enemy->m_stationary && enemy->GetPendingComboId().IsNone() && distance > enemy->GetMeleeDistance() + BossUpdateCombatTuning::rangeMargin;

	//道がない間はBTの移動を繰り返さず、足元を確かめる移動に任せる。
	if (canMove && enemy->m_targeting->m_trackPlayer && IsValid(target))
	{
		const FVector start = enemy->GetNavAgentLocation();
		const FVector goal = target->GetActorLocation();
		auto* path = UNavigationSystemV1::FindPathToLocationSynchronously(
			enemy->GetWorld(), start, goal, enemy);
		canMove = path && path->IsValid();
	}
	blackboard->SetValueAsBool(TEXT("CanDecide"), canDecide);
	blackboard->SetValueAsBool(TEXT("CanMove"), canMove);
	blackboard->SetValueAsFloat(TEXT("Distance"), distance);

	if (alive && target && aiController->LineOfSightTo(target)) { enemy->ObserveTarget(target); }
}
