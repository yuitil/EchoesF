
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
}

//初期設定と必要な部品を用意する。
UBossUpdateCombat::UBossUpdateCombat()
{
	NodeName = TEXT("Boss Update Combat");
	Interval = BossUpdateCombatTuning::updateDelay;
	RandomDeviation = 0.0f;
	bNotifyTick = true;
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
	if(!enemy || !blackboard) { return; }

	//狙っている相手。
	AActor* target = enemy->m_targeting->SelectTarget();

    if (!IsValid(target) && enemy->IsActing()) enemy->CancelAction();

	enemy->m_combatTarget = target;
	//囲まれていて行動できる時は、通常の抽選より先に押し返す。
	if(auto* boss = Cast<ABossEnemyCharacter>(enemy)) { boss->m_crowd->TryPush(); }
	
	//生きていて戦闘できるか。
	const bool alive = enemy->IsCombatActive();

	blackboard->SetValueAsObject(TEXT("TargetActor"), target);
	
	//行動の共通条件は敵本体で一度だけ調べる。
	const bool canDecide = enemy->CanDecide();
	//相手までの距離を、攻撃と追跡で共有する。
	const float distance = IsValid(target)
		? FVector::Dist2D(enemy->GetActorLocation(), target->GetActorLocation())
		: BossUpdateCombatTuning::noTargetRange;
	//コンボ中でなく、剣が届かない時は近づく。
	const bool canMove = canDecide && !enemy->m_stationary && enemy->GetPendingComboId().IsNone()
		&& distance > enemy->GetMeleeDistance() + BossUpdateCombatTuning::rangeMargin;
	blackboard->SetValueAsBool(TEXT("CanDecide"), canDecide);
	blackboard->SetValueAsBool(TEXT("CanMove"), canMove);
	blackboard->SetValueAsFloat(TEXT("Distance"), distance);

	if (alive && target && aiController->LineOfSightTo(target))
	{
		enemy->ObserveTarget(target);
	}
}
