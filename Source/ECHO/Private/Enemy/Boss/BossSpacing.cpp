#include "Enemy/Boss/BossSpacing.h"
#include "Enemy/Boss/BossShadowSummon.h"
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
{ PrimaryComponentTick.bCanEverTick = true; }

//相手のダウン状態を調べる標準処理。
bool UBossSpacing::IsTargetDown_Implementation(AActor* _target) const
{
    if(!IsValid(_target)) { return false; }
    if(!m_downTag.IsNone() && _target->ActorHasTag(m_downTag)) { return true; }

    //対象のキャラクター。
    const auto* character = Cast<ACharacter>(_target);

    //アニメーションを再生する部品。
    const auto* anim =
        character && character->GetMesh() ? character->GetMesh()->GetAnimInstance() : nullptr;
    if(anim)
    {
        for(const auto& montage : m_downMontages)
        {
            if(montage && anim->Montage_IsPlaying(montage)) { return true; }
        }
    }
    return false;
}

//攻撃後に間合いを取る行動を予約する。
void UBossSpacing::Queue()
{

    //処理の対象となる敵。
    auto* enemy = Cast<AEnemyCharacterBase>(GetOwner());

    const double now = GetWorld()->GetTimeSeconds();
    if(m_active || now < m_next || !CanSpace(enemy)) { return; }

    //相手が立っていて刀が届く距離なら、周回の代わりに退避する。
    auto* target = enemy->m_combatTarget.Get();
    const bool down = IsTargetDown(target);
    if(m_onlyDown && !down && !m_watching) { return; }
    const FVector offset = target->GetActorLocation() - enemy->GetActorLocation();
    if(!down && offset.Size2D() < OrbitRadius(*enemy) * m_retreatRatio)
    {
        if(auto* ai = Cast<AAIController>(enemy->GetController())) { ai->StopMovement(); }
        const bool retreat = Retreat(*enemy, offset);
        const float cooldown = FMath::Max(BossSpacingTuning::minDuration, m_cooldown);
        m_next = now + (retreat ? cooldown : BossSpacingTuning::retryDelay);

        //様子見中の退避は一回で終え、次は攻撃を選ぶ。
        if(m_watching && retreat) { m_press = true; }
        return;
    }
    m_active = true;
    m_wasDown = down;
    m_start = GetWorld()->GetTimeSeconds();
    m_repath = m_start + m_startDelay;
    m_side = FMath::RandBool() ? 1.f : -1.f;
}

//間合い調整を止めて移動設定を戻す。
void UBossSpacing::Stop()
{
    if(!m_active) { return; }
    m_active = false;
    m_next = GetWorld()->GetTimeSeconds() + FMath::Max(0.f, m_cooldown);
    if(auto* enemy = Cast<AEnemyCharacterBase>(GetOwner()))
    {
        if(m_moving)
        {
            if(auto* ai = Cast<AAIController>(enemy->GetController())) ai->StopMovement();
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
    if(!m_enabled || !_enemy || !_enemy->HasAuthority() || !_enemy->IsCombatActive())
    {
        return false;
    }
    if(!IsValid(_enemy->m_combatTarget)) { return false; }
    const auto* summon = _enemy->FindComponentByClass<UBossShadowSummon>();
    if(summon && (summon->IsWaitingForSwap(_enemy) || summon->IsChanging())) { return false; }
    if(!m_active && m_press && !IsTargetDown(_enemy->m_combatTarget)) { return false; }
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
    if(!_enemy || !IsValid(target) || !_enemy->IsCombatActive())
    {
        m_idleSince = -1.;
        return;
    }

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
    const float distance = FVector::Dist2D(_enemy->GetActorLocation(), target->GetActorLocation());
    const bool moving = target->GetVelocity().Size2D() > m_idleSpeed;
    const bool canWatch = m_watchIdle && distance <= m_watchRange && !moving;
    if(!canWatch || _enemy->m_defense->IsTargetAttacking(target))
    {
        m_idleSince = -1.;
        return;
    }
    if(m_idleSince < 0.) { m_idleSince = _now; }

    //様子見には上限を設ける。再び周回するには一度攻撃する必要がある。
    const double wait = FMath::Max(BossSpacingTuning::minIdleTime, m_idleDelay);
    const double elapsed = _now - m_idleSince;
    if(elapsed >= wait + FMath::Max(BossSpacingTuning::minDuration, m_duration))
    {
        m_press = true;
    }
    m_watching = !m_press && elapsed >= wait;
}

//経過時間に合わせて敵の状態を更新する。
void UBossSpacing::TickComponent(float _delta, ELevelTick _type, FActorComponentTickFunction* _tick)
{
    Super::TickComponent(_delta, _type, _tick);

    //相手を観察し、周回する条件が整うまで待つ。
    auto* enemy = Cast<AEnemyCharacterBase>(GetOwner());
    if(!enemy || !enemy->HasAuthority()) { return; }
    const double now = GetWorld()->GetTimeSeconds();
    UpdateWatch(enemy, now);
    if(!m_active && m_onlyDown) { Queue(); }
    if(!m_active) { return; }

    auto* target = enemy->m_combatTarget.Get();
    auto* ai = Cast<AAIController>(enemy->GetController());
    if(!CanSpace(enemy) || !ai)
    {
        Stop();
        return;
    }

    //ダウン中以外の周回は、設定時間で必ず終える。
    const bool down = IsTargetDown(target);
    const bool stoodUp = m_wasDown && !down;
    m_wasDown = down;
    const FVector toward = target->GetActorLocation() - enemy->GetActorLocation();
    const float distance = toward.Size2D();

    //相手が目の前で起きた時は、周回を終える前に退避先を確かめる。
    const bool close = distance < OrbitRadius(*enemy) * m_retreatRatio;
    if(!down && close && (stoodUp || m_watching || !m_onlyDown))
    {
        Stop();
        const bool retreated = Retreat(*enemy, toward);
        if(retreated) { m_press = true; }
        return;
    }
    if(!down && m_press) { Stop(); return; }
    const bool timedOut = now - m_start >= FMath::Max(BossSpacingTuning::minDuration, m_duration);
    if(!down && timedOut)
    {
        m_press = true;
        Stop();
        return;
    }
    if(m_onlyDown && !down && !m_watching)
    {
        Stop();
        return;
    }

    //攻撃を再生中か。
    const bool attacking = enemy->m_defense->IsTargetAttacking(target);
    if(attacking)
    {
        Stop();
        return;
    }
    if(now < m_start + m_startDelay) { return; }
    if(!m_moving)
    {
        m_oldSpeed = enemy->GetCharacterMovement()->MaxWalkSpeed;
        m_oldTurn = enemy->GetCharacterMovement()->bOrientRotationToMovement;
        enemy->GetCharacterMovement()->MaxWalkSpeed = FMath::Max(1.f, m_speed);
        enemy->GetCharacterMovement()->bOrientRotationToMovement = false;
        m_moving = true;
    }

    //相手から離れる方向。
    const FVector away = (enemy->GetActorLocation() - target->GetActorLocation()).GetSafeNormal2D();
    enemy->SetActorRotation(FMath::RInterpTo(enemy->GetActorRotation(), (-away).Rotation(), _delta, m_turnSpeed));
    if(now >= m_repath)
    {
        m_repath = now + BossSpacingTuning::pathDelay;

        if(!MoveAround(*enemy, *ai, target->GetActorLocation(), away, distance))
        {
            Stop();
            return;
        }
    }
    Recover(*enemy, distance, now, _delta);
}

//近接攻撃が届く位置よりも外側に、周回の目安を置く。
float UBossSpacing::OrbitRadius(const AEnemyCharacterBase& _enemy) const
{
    const float reach = _enemy.GetMeleeDistance() + BossSpacingTuning::rangeMargin;
    return FMath::Max(reach, m_distance);
}

//相手を向いたまま、BPで選んだ退避技を使う。
bool UBossSpacing::Retreat(AEnemyCharacterBase& _enemy, const FVector& _toward)
{
    _enemy.SetActorRotation(_toward.GetSafeNormal2D().Rotation());
    if(m_retreatAction.IsNone() || !_enemy.RequestAction(m_retreatAction)) { return false; }
    return true;
}

//歩ける床を選び、相手を横切らない経路で周回する。
bool UBossSpacing::MoveAround(AEnemyCharacterBase& _enemy, AAIController& _ai, const FVector& _targetPos, const FVector& _away, float _distance)
{
    //歩ける場所を調べる機能。
    auto* nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

    //判定に使う半径。
    const float radius = OrbitRadius(_enemy);

    //方向を決める角度。
    const float angle = _distance < radius * BossSpacingTuning::innerRing ? 0.f : FMath::Clamp(m_orbitAngle, 0.f, BossSpacingTuning::rangeMargin) * m_side;

    //移動したい位置。
    const FVector desired = _targetPos + _away.RotateAngleAxis(angle, FVector::UpVector) * radius;

    //移動の目的地。
    FNavLocation goal;

    //目的地の周りで床を探す範囲。
    const float width = BossSpacingTuning::navWidth;
    const FVector searchArea(width, width, BossSpacingTuning::navHeight);
    if(!nav || !nav->ProjectPointToNavigation(desired, goal, searchArea))
    {
        return false;
    }

    //目的地までの移動経路。
    auto* path = UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(), _enemy.GetNavAgentLocation(), goal.Location, &_enemy);
    if(!path || !path->IsValid() || path->IsPartial())
    {
        return false;
    }
    const float minGap = FMath::Min(_distance - BossSpacingTuning::pathMargin, _enemy.GetMeleeDistance());
    for(int32 i = 1; i < path->PathPoints.Num(); ++i)
    {

        //近くの有効な位置。
        const FVector closest = FMath::ClosestPointOnSegment(_targetPos, path->PathPoints[i - 1], path->PathPoints[i]);
        if(FVector::Dist2D(closest, _targetPos) < minGap)
        {
            return false;
        }
    }
    const auto result = _ai.MoveToLocation(goal.Location, BossSpacingTuning::stopRange, false, true, true, true, nullptr, false);
    return result != EPathFollowingRequestResult::Failed;
}

//安全な距離で休めた分だけ、戦闘中の回復枠を使う。
void UBossSpacing::Recover(AEnemyCharacterBase& _enemy, float _distance, double _now, float _delta)
{
    if(!_enemy.m_attributeSet || _now - m_start < FMath::Max(0.f, m_healDelay)) { return; }
    const float reach = _enemy.GetMeleeDistance() + BossSpacingTuning::rangeMargin;
    const float safeRange = FMath::Max(reach, m_distance * BossSpacingTuning::healRangeRatio);
    if(_distance < safeRange) { return; }

    //毎秒の回復量、残りの回復枠、失ったHPのうち最も小さい量を使う。
    const float health = _enemy.GetHealth();
    const float maxHealth = _enemy.GetMaxHealth();
    const float budget = maxHealth * FMath::Clamp(m_healLimit, 0.f, 1.f) - m_healed;
    const float tickHeal = FMath::Max(0.f, m_healRate) * _delta;
    const float amount = FMath::Max(0.f, FMath::Min3(tickHeal, budget, maxHealth - health));
    _enemy.m_attributeSet->SetHealth(health + amount);
    m_healed += amount;
}

//終了時にタイマーや登録を解除する。
void UBossSpacing::EndPlay(const EEndPlayReason::Type _reason)
{
    Stop();
    Super::EndPlay(_reason);
}
