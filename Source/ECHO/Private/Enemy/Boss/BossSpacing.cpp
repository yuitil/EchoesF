#include "Enemy/Boss/BossSpacing.h"
#include "Enemy/Boss/BossApproach.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Enemy/Core/EnemyTraversal.h"
#include "Enemy/Combat/Defense.h"
#include "Combat/GAS/CombatAttributeSet.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"

#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"

//判定に使う幅や待ち時間。
namespace BossSpacingTuning
{
    //距離の判定に持たせる余裕。
    constexpr auto rangeMargin = 60.f;
    
    //再び試すまでの時間。
    constexpr auto retryDelay = .25f;
    
    //行動を始めるまでの時間。
    constexpr auto startDelay = .3;
    
    //向きを変える速さ。
    constexpr auto turnSpeed = 5.f;
   
    //経路を更新する間隔。
    constexpr auto pathDelay = .5;
   
    //近づきすぎと判断する距離の割合。
    constexpr auto innerRing = .85f;
   
    //歩ける場所を探す横幅。
    constexpr auto navWidth = 100;
   
    //歩ける場所を探す高さ。
    constexpr auto navHeight = 200;
  
    //移動先の補正に使う余裕。
    constexpr auto pathMargin = 10.f;
 
    //移動を止める距離。
    constexpr auto stopRange = 30.f;
  
    //回復に必要な距離の割合。
    constexpr auto healRangeRatio = .8f;
   
    //行動時間の下限。
    constexpr auto minDuration = .1f;
   
    //様子見と判断する時間の下限。
    constexpr auto minIdleTime = .2f;
}

//初期設定と必要な部品を用意する。
UBossSpacing::UBossSpacing()
{
    PrimaryComponentTick.bCanEverTick = true;
}

//相手のダウン状態を調べる標準処理。
bool UBossSpacing::IsTargetDown_Implementation(AActor* _target) const
{
    if(!IsValid(_target)) { return false; }
    if(!m_downTag.IsNone() && _target->ActorHasTag(m_downTag)) { return true; }
    
    //対象のキャラクター。
    const auto* character = Cast<ACharacter>(_target);
    
    //アニメーションを再生する部品。
    const auto* anim = character && character->GetMesh() ? character->GetMesh()->GetAnimInstance() : nullptr;
    if (anim)
    {
        for (const auto& montage : m_downMontages)
        {
            if (montage && anim->Montage_IsPlaying(montage))
            {
                return true;
            }

        }
    }
    return false;
}

//攻撃後に間合いを取る行動を予約する。
void UBossSpacing::Queue()
{
    //処理の対象となる敵。
    auto* enemy = Cast<AEnemyCharacterBase>(GetOwner());
    
    //抜刀と接近を管理する部品。
    if(!CanSpace(enemy) || m_active || GetWorld()->GetTimeSeconds() < m_next) { return; }

    if(m_onlyDown && !IsTargetDown(enemy->m_combatTarget) && !m_watching) { return; }

    if (IsValid(enemy->m_combatTarget) && !IsTargetDown(enemy->m_combatTarget)
        && FVector::Dist2D(enemy->GetActorLocation(),enemy->m_combatTarget->GetActorLocation()) < FMath::Max(enemy->GetMeleeDistance()+BossSpacingTuning::rangeMargin,m_distance)*m_retreatRatio)
    {
        if (auto* ai=Cast<AAIController>(enemy->GetController())) ai->StopMovement();
        enemy->SetActorRotation((enemy->m_combatTarget->GetActorLocation()-enemy->GetActorLocation()).GetSafeNormal2D().Rotation());
        
        //退避に使う行動。
        const bool retreat=!m_retreatAction.IsNone() && enemy->RequestAction(m_retreatAction);
        m_next=GetWorld()->GetTimeSeconds()+(retreat ? FMath::Max(BossSpacingTuning::minDuration,m_cooldown) : BossSpacingTuning::retryDelay);
        //様子見中の退避は一回で終え、次は攻撃を選ぶ。
        if(m_watching) { m_press = true; }
        return;
    }
    m_active = true;
    m_start = GetWorld()->GetTimeSeconds();
    m_repath = m_start + BossSpacingTuning::startDelay;
    m_side = FMath::RandBool() ? 1.f : -1.f;
}

//間合い調整を止めて移動設定を戻す。
void UBossSpacing::Stop()
{
    if(!m_active) { return; }
    m_active = false;
    m_next = GetWorld()->GetTimeSeconds() + FMath::Max(0.f, m_cooldown);
    if (auto* enemy = Cast<AEnemyCharacterBase>(GetOwner()))
    {
        if (m_moving)
        {
            if (auto* ai = Cast<AAIController>(enemy->GetController())) ai->StopMovement();
            enemy->GetCharacterMovement()->MaxWalkSpeed = m_oldSpeed;
            enemy->GetCharacterMovement()->bOrientRotationToMovement = m_oldTurn;
        }
    }
    m_moving = false;
}

//次の戦闘に向けて状態を初期化する。
void UBossSpacing::Reset()
{
    Stop();
    m_healed = 0.f;
    m_next = 0.;
    m_idleSince = -1.;
    m_watching = false;
    m_press = false;
    m_watched.Reset();
}

//周回と、攻撃・抜刀・ガードが同時に動かないようにする。
bool UBossSpacing::CanSpace(AEnemyCharacterBase* _enemy) const
{
    if(!m_enabled || !_enemy || !_enemy->HasAuthority() || !_enemy->IsCombatActive()) { return false; }
    if(!IsValid(_enemy->m_combatTarget)) { return false; }
    if(m_press && !IsTargetDown(_enemy->m_combatTarget)) { return false; }
    if(_enemy->IsActing() || !_enemy->GetPendingComboId().IsNone()) { return false; }
    if(_enemy->m_defense->IsBusy() || _enemy->m_traversal->IsTraversing()) { return false; }
    if(!_enemy->GetCharacterMovement()->IsMovingOnGround()) { return false; }

    //抜刀中は接近する処理に任せる。
    const auto* approach = _enemy->FindComponentByClass<UBossApproach>();
    return !approach || !approach->IsDrawing();
}

//プレイヤーが攻撃しなくても、一定時間で攻撃する側へ戻る。
void UBossSpacing::UpdateWatch(AEnemyCharacterBase* _enemy, double _now)
{
    //今回観察する相手。
    AActor* target = _enemy ? _enemy->m_combatTarget.Get() : nullptr;
    if(m_watched.Get() != target)
    {
        m_watched = target;
        m_idleSince = -1.;
        m_press = false;
    }
    m_watching = false;
    if(!_enemy || !IsValid(target) || !_enemy->IsCombatActive()) { m_idleSince = -1.; return; }

    //攻撃を始めたら、次の様子見を新しく数える。
    if(_enemy->IsActing())
    {
        const auto* action = _enemy->GetActiveAction();
        if(action && action->m_kind != EEnemyAction::Guard && action->m_kind != EEnemyAction::Dodge)
        {
            m_press = false;
        }
        m_idleSince = -1.;
        return;
    }

    //動いている相手や攻撃中の相手を、静止とみなさない。
    const bool quiet = m_watchIdle && !_enemy->m_defense->IsTargetAttacking(target)
        && target->GetVelocity().Size2D() <= m_idleSpeed
        && FVector::Dist2D(_enemy->GetActorLocation(), target->GetActorLocation()) <= m_watchRange;
    if(!quiet) { m_idleSince = -1.; return; }
    if(m_idleSince < 0.) { m_idleSince = _now; }

    //様子見には上限を設ける。再び周回するには一度攻撃する必要がある。
    const double wait = FMath::Max(BossSpacingTuning::minIdleTime, m_idleDelay);
    const double elapsed = _now - m_idleSince;
    if(elapsed >= wait + FMath::Max(BossSpacingTuning::minDuration, m_duration)) { m_press = true; }
    m_watching = !m_press && elapsed >= wait;
}

//経過時間に合わせて敵の状態を更新する。
void UBossSpacing::TickComponent(float _delta, ELevelTick _type, FActorComponentTickFunction* _tick)
{
    Super::TickComponent(_delta, _type, _tick);

    //相手の行動を観察する部品。
    auto* observer = Cast<AEnemyCharacterBase>(GetOwner());
    
    //現在の時刻。
    const double clock = GetWorld()->GetTimeSeconds();
    if(!observer || !observer->HasAuthority()) { return; }
    UpdateWatch(observer, clock);
    if (!m_active && m_onlyDown) { Queue(); }

    if(!m_active) { return; }
   
    //処理の対象となる敵。
    auto* enemy = Cast<AEnemyCharacterBase>(GetOwner());
   
    //狙っている相手。
    auto* target = enemy ? enemy->m_combatTarget.Get() : nullptr;
   
    //敵の行動を操作するAI。
    auto* ai = enemy ? Cast<AAIController>(enemy->GetController()) : nullptr;
   
    //現在の時刻。
    const double now = GetWorld()->GetTimeSeconds();
    if(!CanSpace(enemy) || !ai) { Stop(); return; }
    //ダウン中以外の周回は、設定時間で必ず終える。
    const bool down = IsTargetDown(target);
    const bool timedOut = now - m_start >= FMath::Max(BossSpacingTuning::minDuration, m_duration);
    if(!down && timedOut) { m_press = true; Stop(); return; }
    if(m_onlyDown && !down && !m_watching) { Stop(); return; }
    
    //相手までの距離。
    const float distance = FVector::Dist2D(enemy->GetActorLocation(), target->GetActorLocation());
    if (!IsTargetDown(target) && distance < FMath::Max(enemy->GetMeleeDistance()+BossSpacingTuning::rangeMargin,m_distance)*m_retreatRatio)
    {
        Stop();
        m_press = true;
        enemy->SetActorRotation((target->GetActorLocation()-enemy->GetActorLocation()).GetSafeNormal2D().Rotation());
        if (!m_retreatAction.IsNone()) enemy->RequestAction(m_retreatAction);
        return;
    }
    
    //攻撃を再生中か。
    const bool attacking = enemy->m_defense->IsTargetAttacking(target);
    if (attacking) 
    { 
        Stop();
        return; 
    }
    if(now < m_start + BossSpacingTuning::startDelay) { return; }
    if (!m_moving)
    {
        m_oldSpeed = enemy->GetCharacterMovement()->MaxWalkSpeed;
        m_oldTurn = enemy->GetCharacterMovement()->bOrientRotationToMovement;
        enemy->GetCharacterMovement()->MaxWalkSpeed = FMath::Max(1.f, m_speed);
        enemy->GetCharacterMovement()->bOrientRotationToMovement = false;
        m_moving = true;
    }
    
    //相手から離れる方向。
    const FVector away = (enemy->GetActorLocation() - target->GetActorLocation()).GetSafeNormal2D();
    enemy->SetActorRotation(FMath::RInterpTo(enemy->GetActorRotation(), (-away).Rotation(), _delta, BossSpacingTuning::turnSpeed));
    if (now >= m_repath)
    {
        m_repath = now + BossSpacingTuning::pathDelay;
        //歩ける場所を調べる機能。
        auto* nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
        
        //判定に使う半径。
        const float radius = FMath::Max(enemy->GetMeleeDistance() + BossSpacingTuning::rangeMargin, m_distance);
        
        //方向を決める角度。
        const float angle = distance < radius * BossSpacingTuning::innerRing ? 0.f : FMath::Clamp(m_orbitAngle, 0.f, BossSpacingTuning::rangeMargin) * m_side;
        
        //移動したい位置。
        const FVector desired = target->GetActorLocation() + away.RotateAngleAxis(angle, FVector::UpVector) * radius;
        
        //移動の目的地。
        FNavLocation goal;
        if (!nav || !nav->ProjectPointToNavigation(desired, goal, FVector(BossSpacingTuning::navWidth,BossSpacingTuning::navWidth,BossSpacingTuning::navHeight))) 
        { 
            Stop(); 
            return; 
        }
        
        //目的地までの移動経路。
        auto* path = UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(), enemy->GetNavAgentLocation(), goal.Location, enemy);
        if (!path || !path->IsValid() || path->IsPartial()) 
        { 
            Stop(); 
            return; 
        }
        for (int32 i=1; i<path->PathPoints.Num(); ++i)
        {
            //近くの有効な位置。
            const FVector closest = FMath::ClosestPointOnSegment(target->GetActorLocation(), path->PathPoints[i-1], path->PathPoints[i]);
            if (FVector::Dist2D(closest, target->GetActorLocation()) < FMath::Min(distance - BossSpacingTuning::pathMargin, enemy->GetMeleeDistance())) 
            { 
                Stop(); 
                return; 
            }
        }
        if (ai->MoveToLocation(goal.Location, BossSpacingTuning::stopRange, false, true, true, true, nullptr, false) == EPathFollowingRequestResult::Failed) 
        { 
            Stop(); 
            return; 
        }
    }
    if (!attacking && distance >= FMath::Max(m_distance * BossSpacingTuning::healRangeRatio, enemy->GetMeleeDistance() + BossSpacingTuning::rangeMargin)
        && now - m_start >= FMath::Max(0.f, m_healDelay) && enemy->m_attributeSet)
    {
        //今回反映する量。
        const float amount = FMath::Max(0.f, FMath::Min3(FMath::Max(0.f, m_healRate) * _delta,
            enemy->GetMaxHealth() * FMath::Clamp(m_healLimit, 0.f, 1.f) - m_healed, enemy->GetMaxHealth() - enemy->GetHealth()));
        enemy->m_attributeSet->SetHealth(enemy->GetHealth() + amount);
        m_healed += amount;
    }
}

//終了時にタイマーや登録を解除する。
void UBossSpacing::EndPlay(const EEndPlayReason::Type _reason)
{
    Stop();
    Super::EndPlay(_reason);
}
