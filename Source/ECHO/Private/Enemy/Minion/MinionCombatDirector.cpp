#include "Enemy/Minion/MinionCombatDirector.h"
#include "Enemy/Minion/MinionFormation.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Enemy/Combat/Defense.h"
#include "AIController.h"
#include "Enemy/AI/EnemyTargeting.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Engine/World.h"

//判定に使う幅や待ち時間。
namespace MinionCombatDirectorTuning
{
    //状態を更新する間隔。
    constexpr auto updateDelay = .2;
    
    //一回転の角度。
    constexpr auto fullTurn = 360.f;
    
    //待ち時間による優先度の上限。
    constexpr auto maxWaitBonus = 150.;
    
    //待っていた敵を優先する値。
    constexpr auto waitBonus = 10.;
    
    //反撃を優先する値。
    constexpr auto counterBonus = 100000.f;
    
    //隙の確認時間の下限。
    constexpr auto minGapTime = .1f;
    
    //経路を確認する間隔の下限。
    constexpr auto minPathDelay = .2f;
}

//集団戦を管理する対象に敵を登録する。
void UMinionCombatDirector::Register(UMinionFormation* _member) 
{ 
    m_members.AddUnique(_member);
}

//集団戦を管理する対象から敵を外す。
void UMinionCombatDirector::Unregister(UMinionFormation* _member) 
{
    m_members.Remove(_member); 
}

//同じ相手へ攻撃が重ならない待ち時間を設ける。
void UMinionCombatDirector::PauseTarget(AActor* _target,float _duration)
{
    if(IsValid(_target)) 
    { 
        auto& until=m_pauses.FindOrAdd(_target); 
        until=FMath::Max(until,GetWorld()->GetTimeSeconds()+FMath::Max(MinionCombatDirectorTuning::minGapTime,_duration));
    }
}

//集団の攻撃役と待機役を更新する。
void UMinionCombatDirector::Update()
{
    //現在の時刻。
    const double now = GetWorld()->GetTimeSeconds();
    if(now < m_nextUpdate) { return; }
    m_nextUpdate = now + MinionCombatDirectorTuning::updateDelay;
    
    //期限が切れた待ち時間を片付ける。
    for(auto it=m_pauses.CreateIterator();it;++it) 
    {
        if (!it.Key().IsValid() || it.Value() <= now)
        {
            it.RemoveCurrent();
        }
    }
    m_members.RemoveAll([](const auto& item) { return !item.IsValid(); });
    
    //相手ごとに分けた敵の集まり。
    TMap<AActor*, TArray<UMinionFormation*>> groups;
    
    //生きている敵を、狙っている相手ごとに分ける。
    for (auto& item : m_members)
    {
        //集団戦に参加している敵の情報。
        auto* member = item.Get(); auto* enemy = member->Enemy();
        if (!member->m_enabled || !enemy || !enemy->IsCombatActive() || !IsValid(enemy->m_combatTarget))
        { 
            member->m_hasTurn = false; 
            continue; 
        }
        if (!member->m_holdDuel && member->m_hasTurn && !enemy->IsActing() && now-member->m_granted > member->m_turnTimeout) 
        {
            member->ReleaseTurn();
        }
        groups.FindOrAdd(enemy->m_combatTarget).Add(member);
    }

    //戦えなくなった担当を外して次の敵に備える。
    for(auto it=m_duelists.CreateIterator();it;++it)
    {
        //集団戦に参加している敵の情報。
        auto* member=it.Value().Get();
       
        //処理の対象となる敵。
        auto* enemy=member ? member->Enemy() : nullptr;
        if(!it.Key().IsValid() || !member || !m_members.Contains(member) || !member->m_enabled || !member->m_holdDuel || !enemy || !enemy->IsCombatActive() || enemy->m_combatTarget!=it.Key().Get())
        {
            it.RemoveCurrent();
        }
    }

    //相手ごとに攻撃役と周囲の待機位置を決める。
    for (auto& pair : groups)
    {
        //狙っている相手。
        AActor* target = pair.Key; auto& group = pair.Value;
        
        //攻撃に参加している敵。
        TArray<UMinionFormation*> active;

        //一対一の担当を維持する設定か。
        const bool duel=group.ContainsByPredicate([](const auto* member){return member->m_holdDuel;});
        
        //今の戦闘担当。
        auto* holder=m_duelists.Find(target);
        
        //一対一の戦闘を担当する部品。
        UMinionFormation* owner=holder ? holder->Get() : nullptr;
        
        //一対一を維持する設定では、現在の担当だけに順番を与える。
        if(duel)
        {
            for(auto* member:group) member->m_hasTurn=(member==owner);
        }

        //処理に使う上限。
        int32 limit = 2;
        for (int32 i=0; i<group.Num(); ++i)
        {
            //集団戦に参加している敵の情報。
            auto* member = group[i];
            limit = FMath::Min(limit,FMath::Clamp(member->m_attackers,1,2));
            if (member->m_hasTurn) { active.Add(member); }
            
            //方向を決める角度。
            const float angle = FMath::DegreesToRadians(i * MinionCombatDirectorTuning::fullTurn / group.Num() + now * member->m_orbitRate);
            member->m_ringGoal = target->GetActorLocation() + FVector(FMath::Cos(angle),FMath::Sin(angle),0) * member->m_radius;
        }
        //攻撃を待つ時間。
        const double* pause=m_pauses.Find(target);

        if (duel && owner) { continue; }
       
        //一対一を維持する設定では、現在の担当だけに順番を与える。
        if (duel) { limit = 1; }

        //反撃の順番を待っているか。
        const bool counterWaiting = group.ContainsByPredicate([](const auto* member)
            {
                return !member->m_hasTurn && member->WantsCounter();
            });
        
        //攻撃人数や待ち時間を守り、一斉攻撃を防ぐ。
        if (active.Num() >= limit || (!counterWaiting && ((pause && now<*pause) || (!active.IsEmpty() && (now-active[0]->m_granted < active[0]->m_secondDelay || !active[0]->Enemy()->IsActing())))))
        { 
            for(auto* member:group)
            {
                member->m_gapSince=-1.; 
                continue;
            }
        }
        //今までで最も適した候補。
        UMinionFormation* best = nullptr; float score = TNumericLimits<float>::Max();
        for (auto* member : group)
        {
            //処理の対象となる敵。
            auto* enemy = member->Enemy();
            
            //攻撃された相手への反撃か。
            const bool counter = member->WantsCounter();
            
            //状態が始まった時刻。
            const double since=member->m_gapSince; member->m_gapSince=-1.;
            if (member->m_hasTurn || now < member->m_ready || !enemy->CanDecide()) { continue; }
            
            //基準位置からのずらし量。
            const FVector offset = enemy->GetActorLocation()-target->GetActorLocation();
           
            //相手までの距離。
            const float distance = offset.Size2D();
            
            //敵の行動を操作するAI。
            auto* ai = Cast<AAIController>(enemy->GetController());
            if (!ai || !ai->LineOfSightTo(target) || distance > member->m_joinRange) { continue; }
            if(enemy->m_stationary && (!enemy->m_definition || !enemy->m_definition->m_actions.ContainsByPredicate([distance](const auto& action) 
                { 
                    return action.m_weight>0.f && distance>=action.m_minRange && distance<=action.m_maxRange;
                }))) 
            { 
                continue;
            }
            if (!active.IsEmpty() && !counter && !member->m_allowSupport) { continue; }
            
            //加勢する場合は、先の敵と違う方向に隙があるか調べる。
            if (!active.IsEmpty() && !counter)
            {
                //最初に選んだ候補。
                const FVector first = (active[0]->Enemy()->GetActorLocation()-target->GetActorLocation()).GetSafeNormal2D();
                if (FVector::DotProduct(first,offset.GetSafeNormal2D()) > FMath::Cos(FMath::DegreesToRadians(member->m_gapAngle))) { continue; }
                if (FVector::DotProduct(target->GetActorForwardVector().GetSafeNormal2D(),offset.GetSafeNormal2D()) > FMath::Cos(FMath::DegreesToRadians(member->m_openAngle)))
                { 
                    continue; 
                }
            }
            //移動する敵は相手までの道がある時だけ選ぶ。
            if (!enemy->m_stationary)
            {
                //目的地までの移動経路。
                auto* path = UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(),enemy->GetActorLocation(),target->GetActorLocation(),enemy);
                if (!path || !path->IsValid() || path->IsPartial()) 
                { 
                    continue;
                }
            }
            //隙を見つけてすぐ飛び込まず、続いているか待って確かめる。
            member->m_gapSince=since<0. ? now : since;
            if(!counter && now-member->m_gapSince<FMath::Max(MinionCombatDirectorTuning::minGapTime,member->m_gapTime)) { continue; }

            //現在調べている候補。
            const float candidate = (member->m_nearestFirst ? distance : distance - FMath::Min(MinionCombatDirectorTuning::maxWaitBonus,FMath::Max(0.,now-member->m_ready)*MinionCombatDirectorTuning::waitBonus)) - (counter ? MinionCombatDirectorTuning::counterBonus : 0.f);

            if (candidate < score) 
            { 
                score=candidate; best=member;
            }
        }
        //最も適した敵に攻撃の順番を渡す。
        if (best) 
        { 
            best->PrepareAttack(); 
            best->ConsumeCounter(); 
            best->m_hasTurn=true; best->m_granted=now; 
            if(duel)
            {
                m_duelists.Add(target,best); 
                for (auto* member : group)
                {
                    member->m_gapSince = -1.;
                }
            }
        }
    }
}

//プレイヤーと戦う担当を解除する。
void UMinionCombatDirector::ClearPlayerGuard(UMinionFormation* _member)
{
    if (m_playerGuard.Get() == _member) { m_playerGuard.Reset(); m_guardCheck=0.; }
}

//プレイヤーと戦う担当の敵を返す。
AActor* UMinionCombatDirector::GetPlayerGuard(AActor* _player)
{
    if(!IsValid(_player)) { return nullptr; }
    
    //攻撃に参加できる候補。
    auto eligible = [_player](UMinionFormation* member)
    {
        //処理の対象となる敵。
        auto* enemy = member ? member->Enemy() : nullptr;
        return member && member->m_enabled && enemy && enemy->IsCombatActive() && !enemy->m_stationary && enemy->m_targeting
            && FVector::Dist2D(enemy->GetActorLocation(),_player->GetActorLocation()) <= enemy->m_targeting->m_range;
    };

    if (eligible(m_playerGuard.Get()))
    {
        //処理の対象となる敵。
        auto* enemy = m_playerGuard->Enemy();
        if (enemy->m_combatTarget == _player || (!enemy->IsActing() && !m_playerGuard->HasTurn()))
        {
            //現在の時刻。
            const double now = GetWorld()->GetTimeSeconds();
            if(enemy->IsActing() || now < m_guardCheck) { return enemy; }
            m_guardCheck = now + FMath::Max(MinionCombatDirectorTuning::minPathDelay,enemy->m_targeting->m_pathCheck);
            
            //敵の行動を操作するAI。
            auto* ai = Cast<AAIController>(enemy->GetController());
            if (ai && ai->LineOfSightTo(_player))
            {
                //目的地までの移動経路。
                auto* path = UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(),enemy->GetActorLocation(),_player->GetActorLocation(),enemy);
                if(path && path->IsValid() && !path->IsPartial()) { return enemy; }
            }
            m_playerGuard->ReleaseTurn();
        }
    }
    m_playerGuard.Reset();
    m_guardCheck=0.;
    
    //候補の選ばれやすさ。
    float score = TNumericLimits<float>::Max();
    
    //生きている敵を、狙っている相手ごとに分ける。
    for (auto& item : m_members)
    {
        //集団戦に参加している敵の情報。
        auto* member = item.Get();
        if (!eligible(member)) { continue; }
        
        //処理の対象となる敵。
        auto* enemy = member->Enemy();
        if (enemy->m_combatTarget == _player && (member->HasTurn() || enemy->IsActing()))
        { 
            m_playerGuard = member; 
            return enemy; 
        }
        if (member->HasTurn() || enemy->IsActing() || member->IsProvoked()) { continue; }
       
        //敵の行動を操作するAI。
        auto* ai = Cast<AAIController>(enemy->GetController());
        if (!ai || !ai->LineOfSightTo(_player)) { continue; }
        
        //目的地までの移動経路。
        auto* path = UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(),enemy->GetActorLocation(),_player->GetActorLocation(),enemy);
        if (!path || !path->IsValid() || path->IsPartial()) { continue; }
        
        //相手までの距離。
        const float distance = FVector::Dist2D(enemy->GetActorLocation(),_player->GetActorLocation());
        if (distance < score) 
        { 
            m_playerGuard = member; 
            score = distance; 
        }
    }
    return m_playerGuard.IsValid() ? m_playerGuard->Enemy() : nullptr;
}
