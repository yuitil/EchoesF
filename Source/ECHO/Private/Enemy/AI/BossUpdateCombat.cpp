
#include "Enemy/AI/BossUpdateCombat.h"

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
	
	//生きていて戦闘できるか。
	bool alive = enemy->IsCombatActive();

	blackboard->SetValueAsObject(TEXT("TargetActor"), target);
	
	//攻撃に参加できるか。
	const bool turn=true;
	blackboard->SetValueAsBool(TEXT("CanDecide"), turn && enemy->CanDecide());
	blackboard->SetValueAsBool(TEXT("CanMove"), turn && !enemy->m_stationary && enemy->CanDecide() && !enemy->m_traversal->IsTraversing() && !enemy->IsActing() && !enemy->m_defense->IsBusy() && enemy->GetPendingComboId().IsNone() && IsValid(target) && FVector::Dist2D(enemy->GetActorLocation(), target->GetActorLocation()) > enemy->GetMeleeDistance() + BossUpdateCombatTuning::rangeMargin);
	blackboard->SetValueAsFloat(TEXT("Distance"), target ? FVector::Dist2D(enemy->GetActorLocation(), target->GetActorLocation()) : BossUpdateCombatTuning::noTargetRange);

	if (alive && target && aiController->LineOfSightTo(target))
	{
		enemy->ObserveTarget(target);
	}
}
