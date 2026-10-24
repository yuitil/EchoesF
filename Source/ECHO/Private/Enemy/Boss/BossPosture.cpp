#include "Enemy/Boss/BossPosture.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Enemy/Combat/Defense.h"
#include "Engine/World.h"

//毎フレームの回復を有効にする。
UBossPosture::UBossPosture()
{
    PrimaryComponentTick.bCanEverTick = true;
}

//ゲーム開始時に最大値へ戻す。
void UBossPosture::BeginPlay()
{
    Super::BeginPlay();
    Reset();
}

//フェーズ開始時に体勢を戻す。
void UBossPosture::Reset()
{
    m_value = FMath::Max(1.f, m_max);
    m_lastHit = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.;
    m_wasBroken = false;
}

//バーに表示する残りの割合。
float UBossPosture::GetRatio() const
{
    return FMath::Clamp(m_value / FMath::Max(1.f, m_max), 0.f, 1.f);
}

//受けた攻撃に合わせて体勢を減らす。
void UBossPosture::Hit(float _damage)
{
    //動けるボスの有効な被弾だけを数える。
    auto* enemy = Cast<AEnemyCharacterBase>(GetOwner());
    if(!m_enabled || !enemy || !enemy->HasAuthority() || !enemy->IsCombatActive() || !enemy->m_defense || !FMath::IsFinite(_damage) || _damage <= 0.f) { return; }

    m_lastHit = GetWorld()->GetTimeSeconds();
    if(enemy->m_defense->m_bGuardBroken) { return; }

    m_value = FMath::Max(0.f, m_value - FMath::Max(0.f, m_hitCost) - _damage * FMath::Max(0.f, m_damageScale));
    if(m_value > 0.f) { return; }

    //既存のガード崩れに任せ、HPは減らしたままにする。
    enemy->m_defense->BreakFor(m_breakTime);
    m_wasBroken = enemy->m_defense->m_bGuardBroken;
}

//被弾していない間の回復を進める。
void UBossPosture::TickComponent(float _delta, ELevelTick _type, FActorComponentTickFunction* _tick)
{
    Super::TickComponent(_delta, _type, _tick);
    //体勢が崩れている間は回復せず、終了後に少しずつ戻す。
    auto* enemy = Cast<AEnemyCharacterBase>(GetOwner());
    if(!m_enabled || !enemy || !enemy->HasAuthority() || !enemy->IsCombatActive() || !enemy->m_defense) { return; }

    if(enemy->m_defense->m_bGuardBroken) { m_value = 0.f; m_wasBroken = true; return; }
    if(m_wasBroken)
    {
        m_wasBroken = false;
        m_lastHit = GetWorld()->GetTimeSeconds() - FMath::Max(0.f, m_healDelay);
    }

    //最後の被弾から待ち時間を過ぎた分だけ回復する。
    const double now = GetWorld()->GetTimeSeconds();
    const double healStart = m_lastHit + FMath::Max(0.f, m_healDelay);
    const float healDelta = FMath::Clamp(float(now - healStart), 0.f, _delta);
    m_value = FMath::Min(FMath::Max(1.f, m_max), m_value + FMath::Max(0.f, m_healSpeed) * healDelta);
}
