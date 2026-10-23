#include "Enemy/Minion/MinionFormation.h"
#include "Enemy/Minion/MinionCombatDirector.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Enemy/Combat/Defense.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
//
UMinionFormation::UMinionFormation() 
{ 
    PrimaryComponentTick.bCanEverTick=true; 
    PrimaryComponentTick.TickInterval=.2f; 
}

AEnemyCharacterBase* UMinionFormation::Enemy() const 
{ 
    return Cast<AEnemyCharacterBase>(GetOwner()); 
}

void UMinionFormation::BeginPlay()
{
    Super::BeginPlay();
    if (GetOwner()->HasAuthority())
    {
        GetWorld()->GetSubsystem<UMinionCombatDirector>()->Register(this);

    }
}

void UMinionFormation::EndPlay(const EEndPlayReason::Type reason)
{
    SetOrbit(false);
    if (auto* director = GetWorld()->GetSubsystem<UMinionCombatDirector>())
    {
        director->Unregister(this);
    }
    Super::EndPlay(reason);
}
void UMinionFormation::ReleaseTurn()
{
    //
    auto* enemy=Enemy();
    if (m_holdDuel && m_hasTurn && enemy && enemy->IsCombatActive() && m_target.IsValid() && enemy->m_combatTarget == m_target.Get()) { return; }
    //
    if (m_hasTurn)
    {
        GetWorld()->GetSubsystem<UMinionCombatDirector>()->ClearPlayerGuard(this);
        GetWorld()->GetSubsystem<UMinionCombatDirector>()->PauseTarget(m_target.Get(), m_groupRest);
    }
    m_hasTurn=false; m_gapSince=-1.; m_ready=GetWorld()->GetTimeSeconds()+FMath::Max(0.f,m_restTime);
    if (IsProvoked()) m_ready=FMath::Min(m_ready,m_counterReady);
}
//
void UMinionFormation::Provoke(AActor* attacker)
{
    auto* enemy = Enemy();
    if (!enemy || !enemy->HasAuthority() || !enemy->IsCombatActive() || !IsValid(attacker) || attacker == enemy) return;
    m_counterTarget = attacker;
    if (!enemy->IsActing() || enemy->m_combatTarget == attacker)
    {
        if (enemy->m_combatTarget != attacker) ReleaseTurn();
        enemy->m_combatTarget = attacker;
        m_target = attacker;
    }
    const double now = GetWorld()->GetTimeSeconds();
    if (m_counterUntil <= now) m_counterReady = now + FMath::Max(.2f, m_counterDelay);
    m_counterUntil = now + FMath::Max(1.f, m_counterTime);
    m_ready = FMath::Min(m_ready, m_counterReady);
}
bool UMinionFormation::WantsCounter() const
{
    const double now = GetWorld()->GetTimeSeconds();
    return now >= m_counterReady && now < m_counterUntil;
}
bool UMinionFormation::IsProvoked() const
{
    return GetWorld()->GetTimeSeconds() < m_counterUntil;
}
//
//
void UMinionFormation::SetOrbit(bool enabled)
{
    auto* enemy=Enemy(); 
    if (!enemy) { return; }

    auto* move=enemy->GetCharacterMovement(); auto* ai=Cast<AAIController>(enemy->GetController());
    if(enabled && !m_orbit)
    {
        m_oldOrient=move->bOrientRotationToMovement; m_oldDesired=move->bUseControllerDesiredRotation;
        m_oldYaw=enemy->bUseControllerRotationYaw; m_oldSpeed=move->MaxWalkSpeed;
        move->bOrientRotationToMovement=false; move->bUseControllerDesiredRotation=true;
        enemy->bUseControllerRotationYaw=false; move->MaxWalkSpeed=FMath::Max(50.f,m_orbitSpeed);
    }
    else if(!enabled && m_orbit)
    {
        move->bOrientRotationToMovement=m_oldOrient; move->bUseControllerDesiredRotation=m_oldDesired;
        enemy->bUseControllerRotationYaw=m_oldYaw; move->MaxWalkSpeed=m_oldSpeed;
        if(ai && ai->GetFocusActor()==m_target.Get()) ai->ClearFocus(EAIFocusPriority::Gameplay);
    }
    m_orbit=enabled;
    if (enabled && ai)
    {
        ai->SetFocus(m_target.Get());
    }
}
void UMinionFormation::TickComponent(float delta,ELevelTick type,FActorComponentTickFunction* fn)
{
    Super::TickComponent(delta,type,fn);

    auto* enemy=Enemy(); 
    if (!enemy || !enemy->HasAuthority()) { return; }
    if(!m_enabled) 
    { 
        SetOrbit(false); 
        return; 
    }
    if (m_target.Get()!=enemy->m_combatTarget) 
    { 
        SetOrbit(false); 
        m_hasTurn=false;
        m_gapSince=-1.; 
        m_target=enemy->m_combatTarget; 
        m_ready=GetWorld()->GetTimeSeconds()+FMath::Max(0.f,m_observeTime); 
    }

    GetWorld()->GetSubsystem<UMinionCombatDirector>()->Update();
    auto* ai=Cast<AAIController>(enemy->GetController());
    if (!ai || !enemy->IsCombatActive() || !m_target.IsValid()) 
    { 
        SetOrbit(false); return; 
    }
    if (m_hasTurn || enemy->IsActing() || enemy->m_defense->IsBusy())
    { 
        SetOrbit(false); 
        return; 
    }
    SetOrbit(true);
    if (enemy->m_defense->ShouldStartGuard()) 
    { 
        SetOrbit(false); 
        if (enemy->RequestAction()) { return; } 
        SetOrbit(true);
    }
    if (enemy->m_stationary || !enemy->GetCharacterMovement()->IsMovingOnGround()) { return; }
    
    const double now=GetWorld()->GetTimeSeconds(); 
    if (now < m_nextMove) { return; }  
    m_nextMove=now+.7;
    
    auto* nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()); 
    FNavLocation projected;
    const FVector center=m_target->GetActorLocation();
    const FVector radial=enemy->GetActorLocation()-center;
    const float angle=FMath::RadiansToDegrees(FMath::Atan2(radial.Y,radial.X));
    const FVector goalDir=m_ringGoal-center;
    const float desired=FMath::RadiansToDegrees(FMath::Atan2(goalDir.Y,goalDir.X));
    const float step=angle+FMath::Clamp(FMath::FindDeltaAngleDegrees(angle,desired),-25.f,25.f);
    const float radius=FMath::FInterpConstantTo(radial.Size2D(),m_radius,.7f,180.f);
    const FVector waypoint=center+FRotator(0,step,0).Vector()*radius;
    if(!nav || !nav->ProjectPointToNavigation(waypoint,projected,FVector(100,100,250))) 
    { 
        ai->StopMovement(); 
        return; 
    }
    
    auto* path=UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(),enemy->GetActorLocation(),projected.Location,enemy);
    if (!path || !path->IsValid() || path->IsPartial()) { return; }
    for (int32 i = 1;i < path->PathPoints.Num();++i)
    {
        if (FMath::PointDistToSegment(m_target->GetActorLocation(), path->PathPoints[i - 1], path->PathPoints[i]) < enemy->GetMeleeDistance() * .7f)
        {
            return;
        }
    }
    ai->MoveToLocation(projected.Location,35.f,false,true,true,true,nullptr,false);
}
//
