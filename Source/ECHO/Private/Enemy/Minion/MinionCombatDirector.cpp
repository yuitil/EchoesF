#include "Enemy/Minion/MinionCombatDirector.h"
#include "Enemy/Minion/MinionFormation.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Enemy/Combat/Defense.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Engine/World.h"
//
void UMinionCombatDirector::Register(UMinionFormation* member) { m_members.AddUnique(member); }
void UMinionCombatDirector::Unregister(UMinionFormation* member) { m_members.Remove(member); }
void UMinionCombatDirector::PauseTarget(AActor* target,float duration)
{
    if(IsValid(target)) { auto& until=m_pauses.FindOrAdd(target); until=FMath::Max(until,GetWorld()->GetTimeSeconds()+FMath::Max(.1f,duration)); }
}
void UMinionCombatDirector::Update()
{
    const double now = GetWorld()->GetTimeSeconds();
    if (now < m_nextUpdate) return;
    m_nextUpdate = now + .2;
    for(auto it=m_pauses.CreateIterator();it;++it) if(!it.Key().IsValid() || it.Value()<=now) it.RemoveCurrent();
    m_members.RemoveAll([](const auto& item) { return !item.IsValid(); });
    TMap<AActor*, TArray<UMinionFormation*>> groups;
    for (auto& item : m_members)
    {
        auto* member = item.Get(); auto* enemy = member->Enemy();
        if (!member->m_enabled || !enemy || !enemy->IsCombatActive() || !IsValid(enemy->m_combatTarget))
        { member->m_hasTurn = false; continue; }
        if (!member->m_holdDuel && member->m_hasTurn && !enemy->IsActing() && now-member->m_granted > member->m_turnTimeout) member->ReleaseTurn();
        groups.FindOrAdd(enemy->m_combatTarget).Add(member);
    }
    //
    for(auto it=m_duelists.CreateIterator();it;++it)
    {
        auto* member=it.Value().Get();
        auto* enemy=member ? member->Enemy() : nullptr;
        if(!it.Key().IsValid() || !member || !m_members.Contains(member) || !member->m_enabled || !member->m_holdDuel || !enemy || !enemy->IsCombatActive() || enemy->m_combatTarget!=it.Key().Get()) it.RemoveCurrent();
    }
    //
    for (auto& pair : groups)
    {
        AActor* target = pair.Key; auto& group = pair.Value;
        TArray<UMinionFormation*> active;
        //
        const bool duel=group.ContainsByPredicate([](const auto* member){return member->m_holdDuel;});
        auto* holder=m_duelists.Find(target);
        UMinionFormation* owner=holder ? holder->Get() : nullptr;
        if(duel)
        {
            for(auto* member:group) member->m_hasTurn=(member==owner);
        }
        //
        int32 limit = 2;
        for (int32 i=0; i<group.Num(); ++i)
        {
            auto* member = group[i];
            limit = FMath::Min(limit,FMath::Clamp(member->m_attackers,1,2));
            if (member->m_hasTurn) active.Add(member);
            const float angle = FMath::DegreesToRadians(i * 360.f / group.Num() + now * member->m_orbitRate);
            member->m_ringGoal = target->GetActorLocation() + FVector(FMath::Cos(angle),FMath::Sin(angle),0) * member->m_radius;
        }
        const double* pause=m_pauses.Find(target);
        //
        if(duel && owner) continue;
        if(duel) limit=1;
        //
        if (active.Num() >= limit || (pause && now<*pause) || (!active.IsEmpty() && (now-active[0]->m_granted < active[0]->m_secondDelay || !active[0]->Enemy()->IsActing())))
        { for(auto* member:group) member->m_gapSince=-1.; continue; }
        UMinionFormation* best = nullptr; float score = TNumericLimits<float>::Max();
        for (auto* member : group)
        {
            auto* enemy = member->Enemy();
            const double since=member->m_gapSince; member->m_gapSince=-1.;
            if (member->m_hasTurn || now < member->m_ready || !enemy->CanDecide()) continue;
            const FVector offset = enemy->GetActorLocation()-target->GetActorLocation();
            const float distance = offset.Size2D();
            auto* ai = Cast<AAIController>(enemy->GetController());
            if (!ai || !ai->LineOfSightTo(target) || distance > member->m_joinRange || (enemy->m_stationary && distance > enemy->GetMeleeDistance()+20.f)) continue;
            if (!active.IsEmpty())
            {
                const FVector first = (active[0]->Enemy()->GetActorLocation()-target->GetActorLocation()).GetSafeNormal2D();
                if (FVector::DotProduct(first,offset.GetSafeNormal2D()) > FMath::Cos(FMath::DegreesToRadians(member->m_gapAngle))) continue;
                if (FVector::DotProduct(target->GetActorForwardVector().GetSafeNormal2D(),offset.GetSafeNormal2D()) > FMath::Cos(FMath::DegreesToRadians(member->m_openAngle))) continue;
            }
            if (!enemy->m_stationary)
            {
                auto* path = UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(),enemy->GetActorLocation(),target->GetActorLocation(),enemy);
                if (!path || !path->IsValid() || path->IsPartial()) continue;
            }
            member->m_gapSince=since<0. ? now : since;
            if(now-member->m_gapSince<FMath::Max(.1f,member->m_gapTime)) continue;
            const float candidate = distance - FMath::Min(150.,FMath::Max(0.,now-member->m_ready)*10.);
            if (candidate < score) { score=candidate; best=member; }
        }
        if (best) { best->PrepareAttack(); best->m_hasTurn=true; best->m_granted=now; if(duel) m_duelists.Add(target,best); for(auto* member:group) member->m_gapSince=-1.; }
    }
}
//
