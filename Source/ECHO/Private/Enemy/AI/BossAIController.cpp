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

void ABossAIController::OnPossess(APawn* _pawn)
{
	Super::OnPossess(_pawn);

	if (AEnemyCharacterBase* enemy = Cast<AEnemyCharacterBase>(_pawn))
	{
		if (enemy->m_behaviorTree)
		{
			RunBehaviorTree(enemy->m_behaviorTree);
		}
	}
}
