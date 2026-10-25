#include "Enemy/AI/EnemyTargeting.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Ghost/GhostCharacter/GhostCharacter.h"
#include "Ghost/GhostEnemy/GhostEnemyComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Enemy/Minion/MinionFormation.h"
#include "Enemy/Minion/MinionCombatDirector.h"

//攻撃対象にできる相手か調べる。
bool UEnemyTargeting::IsCandidate(AActor* _actor) const
{
    if(!IsValid(_actor) || _actor->IsHidden() || !_actor->CanBeDamaged()) { return false; }
    if (auto* ghost = Cast<AGhostCharacter>(_actor))
    {
        if(!m_targetClones || !ghost->EnemyComponent) { return false; }
        //現在の状態。
        const auto state = ghost->EnemyComponent->GetLifecycleState();
        return state == EGhostLifecycleState::Friendly || state == EGhostLifecycleState::Warning;
    }
    return _actor == UGameplayStatics::GetPlayerPawn(this,0);
}

//狙う相手を選ぶ。
AActor* UEnemyTargeting::SelectTarget()
{
    //影は担当の分身以外へ攻撃を向けない。
    if(m_lockTarget) { return IsCandidate(m_fixedTarget.Get()) ? m_fixedTarget.Get() : nullptr; }
    //処理の対象となる敵。
    auto* enemy = Cast<AEnemyCharacterBase>(GetOwner());
    if(!enemy) { return nullptr; }
    if(const auto* formation=enemy->FindComponentByClass<UMinionFormation>(); formation && formation->IsProvoked() && IsCandidate(enemy->m_combatTarget)
        && FVector::Dist2D(enemy->GetActorLocation(),enemy->m_combatTarget->GetActorLocation()) <= m_range) { return enemy->m_combatTarget; }
    if(enemy->IsActing()) { return IsCandidate(enemy->m_combatTarget) ? enemy->m_combatTarget.Get() : nullptr; }

    if(m_followTarget && IsCandidate(enemy->m_combatTarget)
        && (m_chaseRange <= 0.f || FVector::Dist2D(enemy->GetActorLocation(), enemy->m_combatTarget->GetActorLocation()) <= m_chaseRange)) { return enemy->m_combatTarget; }

    //敵の行動を操作するAI。
    auto* ai = Cast<AAIController>(enemy->GetController());
    
    //今までで最も適した候補。
    AActor* best = nullptr;
    
    //候補の選ばれやすさ。
    float score = TNumericLimits<float>::Max();
    
    //候補として調べる処理。
    auto consider = [&](AActor* actor)
    {
        if(!IsCandidate(actor)) { return; }
        
        //相手までの距離。
        float distance = FVector::Dist2D(actor->GetActorLocation(),enemy->GetActorLocation());
        if(distance > FMath::Max(0.f,m_range) || (ai && !ai->LineOfSightTo(actor))) { return; }
        if (actor == enemy->m_combatTarget) distance *= FMath::Clamp(m_keepWeight,0.f,1.f);
        if (distance < score) { score = distance; best = actor; }
    };
    consider(UGameplayStatics::GetPlayerPawn(this,0));
    if (m_targetClones) for (TActorIterator<AGhostCharacter> it(GetWorld()); it; ++it) consider(*it);
    if(!best && IsCandidate(enemy->m_combatTarget) && FVector::Dist2D(enemy->GetActorLocation(), enemy->m_combatTarget->GetActorLocation()) <= m_range) 
    { 
        return enemy->m_combatTarget;
    }
    return best;
}

//戦闘の役割を考えて雑魚敵の相手を選ぶ。
AActor* UEnemyTargeting::SelectMinionTarget()
{
    //専用の相手が指定されていれば集団戦の選択を使わない。
    if(m_lockTarget) { return SelectTarget(); }
    //処理の対象となる敵。
    auto* enemy = Cast<AEnemyCharacterBase>(GetOwner());
    if(!enemy) { return nullptr; }
   
    //集団戦での役割を管理する部品。
    auto* formation = enemy->FindComponentByClass<UMinionFormation>();
    if(enemy->IsActing()) { return IsCandidate(enemy->m_combatTarget) ? enemy->m_combatTarget.Get() : nullptr; }
    
    //今の相手を狙い続ける距離。
    const float keepRange = FMath::Max(m_range,m_giveUpRange);
    
    //現在の相手を維持するか。
    auto keep = [&](AActor* actor) 
        { 
            return IsCandidate(actor) && FVector::DistSquared2D(enemy->GetActorLocation(),actor->GetActorLocation()) <= FMath::Square(keepRange); 
        };

    if(formation && formation->IsProvoked() && keep(formation->GetCounterTarget())) { return formation->GetCounterTarget(); }
    if(formation && formation->HasTurn() && keep(enemy->m_combatTarget)) { return enemy->m_combatTarget; }
    if(!m_preferClones || !m_targetClones) { return SelectTarget(); }
    
    //敵の行動を操作するAI。
    auto* ai = Cast<AAIController>(enemy->GetController());
    
    //今までで最も適した候補。
    AActor* best = nullptr;
    
    //候補の選ばれやすさ。
    float score = TNumericLimits<float>::Max();
    for (TActorIterator<AGhostCharacter> it(GetWorld()); it; ++it)
    {
        //狙っている相手。
        auto* target = *it;
        if (!IsCandidate(target)) continue;
       
        //相手までの距離。
        float distance = FVector::Dist2D(enemy->GetActorLocation(), target->GetActorLocation());
        if (distance > m_range || (ai && !ai->LineOfSightTo(target))) continue;
        if (!enemy->m_stationary)
        {
            //目的地までの移動経路。
            auto* path = UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(), enemy->GetActorLocation(), target->GetActorLocation(), enemy);
            if (!path || !path->IsValid() || path->IsPartial()) continue;
        }
        if (target == enemy->m_combatTarget) distance *= FMath::Clamp(m_keepWeight, 0.f, 1.f);
        if (distance < score) { score = distance; best = target; }
    }

    //対象のプレイヤー。
    auto* player = UGameplayStatics::GetPlayerPawn(this,0);
    if(best && m_keepPlayerDuel && IsCandidate(player) && GetWorld()->GetSubsystem<UMinionCombatDirector>()->GetPlayerGuard(player) == enemy) { return player; }

    return best ? best : SelectTarget();
}
