#include "Enemy/Minion/MinionFormation.h"
#include "Enemy/Minion/MinionCombatDirector.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Enemy/Combat/Defense.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"

//判定に使う幅や待ち時間。
namespace MinionFormationTuning
{
    //周回速度の下限。
    constexpr auto minOrbitSpeed = 50.f;
   
    //移動先を更新する間隔。
    constexpr auto moveDelay = .7;
    
    //一度に周回する角度の上限。
    constexpr auto maxOrbitStep = 25.f;
    
    //周回の基本速度。
    constexpr auto ringSpeed = 180.f;
   
    //歩ける場所を探す横幅。
    constexpr auto navWidth = 100;
    
    //歩ける場所を探す高さ。
    constexpr auto navHeight = 250;
    
    //移動を止める距離。
    constexpr auto stopRange = 35.f;
    
    //更新間隔の下限。
    constexpr auto minUpdateDelay = .2f;
    
    //移動先を求めるための時間幅。
    constexpr auto moveStep = .7f;
    
    //移動先の補正に使う余裕。
    constexpr auto pathMargin = .7f;
}

//初期設定と必要な部品を用意する。
UMinionFormation::UMinionFormation()
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.TickInterval=MinionFormationTuning::minUpdateDelay;
}

//この部品を持つ敵を返す。
AEnemyCharacterBase* UMinionFormation::Enemy() const
{
    return Cast<AEnemyCharacterBase>(GetOwner());
}

//ゲーム開始時に必要な状態を準備する。
void UMinionFormation::BeginPlay()
{
    Super::BeginPlay();
    
    //敵の役割決めはサーバー側で行う。
    if (GetOwner()->HasAuthority())
    {
        GetWorld()->GetSubsystem<UMinionCombatDirector>()->Register(this);

    }
}

//終了時にタイマーや登録を解除する。
void UMinionFormation::EndPlay(const EEndPlayReason::Type _reason)
{
    SetOrbit(false);
    if (auto* director = GetWorld()->GetSubsystem<UMinionCombatDirector>())
    {
        director->Unregister(this);
    }
    Super::EndPlay(_reason);
}

//攻撃する役目を終えて待機へ戻る。
void UMinionFormation::ReleaseTurn()
{

    //処理の対象となる敵。
    auto* enemy=Enemy();
    if(m_holdDuel && m_hasTurn && enemy && enemy->IsCombatActive() && m_target.IsValid() && enemy->m_combatTarget == m_target.Get()) { return; }

    //役目を終えた相手に、次の攻撃までの間を作る。
    if (m_hasTurn)
    {
        GetWorld()->GetSubsystem<UMinionCombatDirector>()->ClearPlayerGuard(this);
        GetWorld()->GetSubsystem<UMinionCombatDirector>()->PauseTarget(m_target.Get(), m_groupRest);
    }
    m_hasTurn=false; m_gapSince=-1.; m_ready=GetWorld()->GetTimeSeconds()+FMath::Max(0.f,m_restTime);
    if (IsProvoked()) m_ready=FMath::Min(m_ready,m_counterReady);
}

//攻撃してきた相手への反撃を予約する。
void UMinionFormation::Provoke(AActor* _attacker)
{
    //処理の対象となる敵。
    auto* enemy = Enemy();
    if(!enemy || !enemy->HasAuthority() || !enemy->IsCombatActive() || !IsValid(_attacker) || _attacker == enemy) { return; }
    m_counterTarget = _attacker;
    
    //enemy->m_combatTarget == _attacker)|攻撃の途中で別の相手へ急に向かないようにする。
    if (!enemy->IsActing() || enemy->m_combatTarget == _attacker)
    {
        if (enemy->m_combatTarget != _attacker) { ReleaseTurn(); } 
        enemy->m_combatTarget = _attacker;
        m_target = _attacker;
    }

    //現在の時刻。
    const double now = GetWorld()->GetTimeSeconds();
    if (m_counterUntil <= now) m_counterReady = now + FMath::Max(MinionFormationTuning::minUpdateDelay, m_counterDelay);
    m_counterUntil = now + FMath::Max(1.f, m_counterTime);
    m_ready = FMath::Min(m_ready, m_counterReady);
}

//反撃を開始する時間か調べる。
bool UMinionFormation::WantsCounter() const
{
    //現在の時刻。
    const double now = GetWorld()->GetTimeSeconds();
    return now >= m_counterReady && now < m_counterUntil;
}

//攻撃してきた相手への反撃待ちか調べる。
bool UMinionFormation::IsProvoked() const
{
    return GetWorld()->GetTimeSeconds() < m_counterUntil;
}

//相手を向いた周回移動を切り替える。
void UMinionFormation::SetOrbit(bool _enabled)
{
    //処理の対象となる敵。
    auto* enemy=Enemy();
    if(!enemy) { return; }

    //移動を管理する部品。
    auto* move=enemy->GetCharacterMovement(); auto* ai=Cast<AAIController>(enemy->GetController());
    if(_enabled && !m_orbit)
    {
        m_oldOrient=move->bOrientRotationToMovement; m_oldDesired=move->bUseControllerDesiredRotation;
        m_oldYaw=enemy->bUseControllerRotationYaw; m_oldSpeed=move->MaxWalkSpeed;
        move->bOrientRotationToMovement=false; move->bUseControllerDesiredRotation=true;
        enemy->bUseControllerRotationYaw=false; move->MaxWalkSpeed=FMath::Max(MinionFormationTuning::minOrbitSpeed,m_orbitSpeed);
    }
    else if(!_enabled && m_orbit)
    {
        move->bOrientRotationToMovement=m_oldOrient; move->bUseControllerDesiredRotation=m_oldDesired;
        enemy->bUseControllerRotationYaw=m_oldYaw; move->MaxWalkSpeed=m_oldSpeed;
        if(ai && ai->GetFocusActor()==m_target.Get()) ai->ClearFocus(EAIFocusPriority::Gameplay);
    }
    m_orbit=_enabled;
    if (_enabled && ai)
    {
        ai->SetFocus(m_target.Get());
    }
}

//経過時間に合わせて敵の状態を更新する。
void UMinionFormation::TickComponent(float _delta,ELevelTick _type,FActorComponentTickFunction* _fn)
{
    Super::TickComponent(_delta,_type,_fn);

    //処理の対象となる敵。
    auto* enemy=Enemy();
    if(!enemy || !enemy->HasAuthority()) { return; }
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
    
    //敵の行動を操作するAI。
    auto* ai=Cast<AAIController>(enemy->GetController());
    if (!ai || !enemy->IsCombatActive() || !m_target.IsValid())
    {
        SetOrbit(false); 
        return;
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
        if(enemy->RequestAction()) { return; }
        SetOrbit(true);
    }
    if(enemy->m_stationary || !enemy->GetCharacterMovement()->IsMovingOnGround()) { return; }

    //現在の時刻。
    const double now=GetWorld()->GetTimeSeconds();
    if(now < m_nextMove) { return; }
    m_nextMove=now+MinionFormationTuning::moveDelay;

    //歩ける場所を調べる機能。
    auto* nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    
    //歩ける場所へ補正した位置。
    FNavLocation projected;
   
    //相手を囲む中心位置。
    const FVector center=m_target->GetActorLocation();
    
    //中心から外へ向かう方向。
    const FVector radial=enemy->GetActorLocation()-center;
    
    //方向を決める角度。
    const float angle=FMath::RadiansToDegrees(FMath::Atan2(radial.Y,radial.X));
    
    //目的地へ向かう方向。
    const FVector goalDir=m_ringGoal-center;
    
    //移動したい位置。
    const float desired=FMath::RadiansToDegrees(FMath::Atan2(goalDir.Y,goalDir.X));
    
    //今回進める幅。
    const float step=angle+FMath::Clamp(FMath::FindDeltaAngleDegrees(angle,desired),-MinionFormationTuning::maxOrbitStep,MinionFormationTuning::maxOrbitStep);
   
    //判定に使う半径。
    const float radius=FMath::FInterpConstantTo(radial.Size2D(),m_radius,MinionFormationTuning::moveStep,MinionFormationTuning::ringSpeed);
    
    //経路上の次の位置。
    const FVector waypoint=center+FRotator(0,step,0).Vector()*radius;
    if(!nav || !nav->ProjectPointToNavigation(waypoint,projected,FVector(MinionFormationTuning::navWidth,MinionFormationTuning::navWidth,MinionFormationTuning::navHeight)))
    {
        ai->StopMovement();
        return;
    }

    //目的地までの移動経路。
    auto* path=UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(),enemy->GetActorLocation(),projected.Location,enemy);
    if(!path || !path->IsValid() || path->IsPartial()) { return; }
    for (int32 i = 1;i < path->PathPoints.Num();++i)
    {
        if(FMath::PointDistToSegment(m_target->GetActorLocation(), path->PathPoints[i - 1], path->PathPoints[i]) < enemy->GetMeleeDistance() * MinionFormationTuning::pathMargin) 
        { 
            return;
        }
    }
    ai->MoveToLocation(projected.Location,MinionFormationTuning::stopRange,false,true,true,true,nullptr,false);
}
