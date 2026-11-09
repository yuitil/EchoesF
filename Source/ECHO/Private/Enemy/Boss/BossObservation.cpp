#include "Enemy/Boss/BossObservation.h"
#include "Enemy/Boss/BossData.h"
#include "Enemy/AI/EnemyPlayerHistory.h"
#include "Kismet/GameplayStatics.h"
#include "Player/CombatComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"

//動作を確かめるための幅や待ち時間。
namespace BossObservationTuning
{

//近距離として記録する範囲。
constexpr auto closeRange = 250;

//予測が外れても、ほかの技を完全には消さない。
constexpr float minWeight = .2f;

//同じ予測だけで攻撃が偏らない上限。
constexpr float maxWeight = 3.f;

//間合いから外れる技を控える強さ。
constexpr float missPenalty = .6f;

//状況に合う技を少し増やす値。
constexpr float smallBonus = .5f;

//移動したと判断する基準。
constexpr auto moveThreshold = 100;

//アニメーションが再開したと判断する余裕。
constexpr auto restartMargin = 0.02f;

//回避の癖を判断する最低回数。
constexpr auto minDodges = 3;

//攻撃後の隙を記録する時間。
constexpr auto historyDelay = 10.0;
}

//初期設定と必要な部品を用意する。
UBossObservation::UBossObservation()
{ PrimaryComponentTick.bCanEverTick = false; }

//相手の行動を記録する。
void UBossObservation::Observe(AActor* _target, const UBossData* _data)
{
    if(!IsValid(_target) || _target != UGameplayStatics::GetPlayerPawn(this, 0) || !_data)
    {
        return;
    }

    if(m_lastTarget.Get() != _target)
    {
        m_lastTarget = _target;
        m_lastMontage.Reset();
        m_lastAnimPos = -1.0f;
        m_lastAttack = -1.0;
    }

    ReadMotion(_target);

    ++m_pattern.m_samples;

    //基準位置からのずらし量。
    FVector offSet = _target->GetActorLocation() - GetOwner()->GetActorLocation();
    offSet.Z = 0.0f;

    //移動や判定に使う方向。
    FVector direction = offSet.GetSafeNormal();

    m_closeCount += offSet.Size() < BossObservationTuning::closeRange;
    m_retreatCount += FVector::DotProduct(_target->GetVelocity(), direction) > BossObservationTuning::moveThreshold;

    //左右の向き。
    float side = FVector::DotProduct(_target->GetVelocity(), FVector::CrossProduct(FVector::UpVector, direction));
    m_leftCount += side < -BossObservationTuning::moveThreshold;
    m_rightCount += side > BossObservationTuning::moveThreshold;

    //対象のキャラクター。
    ACharacter* character = Cast<ACharacter>(_target);

    //アニメーションを再生する部品。
    UAnimInstance *animInstance = character && character->GetMesh() ? character->GetMesh()->GetAnimInstance() : nullptr;

    //対象のモンタージュ。
    UAnimMontage* montage = animInstance ? animInstance->GetCurrentActiveMontage() : nullptr;

    //BPで登録した独自の攻撃も、見えている動作として扱う。
    m_seenAttack |= montage && _data->m_attackMontages.Contains(montage);

    //現在の位置や再生位置。
    float position = montage ? animInstance->Montage_GetPosition(montage) : -1;

    //処理を開始できたか。
    const bool started = montage && (m_lastMontage.Get() != montage || position + BossObservationTuning::restartMargin < m_lastAnimPos);

    if(started)
    {
        //現在の時刻。
        const double now = GetWorld()->GetTimeSeconds();
        auto* combat = _target->FindComponentByClass<UCombatComponent>();
        if(_data->m_attackMontages.Contains(montage) || (combat && combat->IsPlayingAttackMontage()))
        {
            ++m_pattern.m_attacks;
            if(m_lastAttack >= 0.0)
            {
                m_intervalSum += now - m_lastAttack;
                ++m_intervalCount;
            }

            m_lastAttack = now;

            if(now - m_lastBossEnd >= 0.0 && now - m_lastBossEnd < 1.0) { ++m_punishCount; }
        }

        if(_data->m_dodgeMontages.Contains(montage)) { ++m_pattern.m_dodges; }
    }
    m_lastMontage = montage;
    m_lastAnimPos = position;
}

//見えていると確認した相手の動きを、予測用に残す。
void UBossObservation::ReadMotion(AActor* _target)
{
    if(!IsValid(_target)) { return; }
    m_lastTarget = _target;

    //BTが見えている時だけ渡した情報を使う。見えない相手の位置は読まない。
    m_seenTime = GetWorld()->GetTimeSeconds();
    m_seenPos = _target->GetActorLocation();
    m_seenSpeed = _target->GetVelocity();
    m_seenSpeed.Z = 0.f;
    const FVector towardBoss = (GetOwner()->GetActorLocation() - m_seenPos).GetSafeNormal2D();
    m_seenFacing = FVector::DotProduct(_target->GetActorForwardVector().GetSafeNormal2D(), towardBoss) >= FMath::Cos(FMath::DegreesToRadians(m_faceAngle));
    auto* currentCombat = _target->FindComponentByClass<UCombatComponent>();
    m_seenAttack = currentCombat && currentCombat->IsPlayingAttackMontage();
}

//相手から攻撃を受けた回数を記録する。
void UBossObservation::RecordHit()
{ 
    ++m_pattern.m_hitsOnBoss; 
}

//ボスの攻撃が終わった時刻を記録する。
void UBossObservation::MarkBossActionEnded()
{
    m_lastBossEnd = GetWorld()->GetTimeSeconds();
}

//フェーズ1の観察結果を保存する。
void UBossObservation::Freeze()
{
    if(m_bIsFrozen) { return; }

    m_frozenPattern = m_pattern;

    //記録した数。
    const float sampleCount = FMath::Max(1, m_pattern.m_samples);
    m_frozenPattern.m_closeRatio = m_closeCount / sampleCount;
    m_frozenPattern.m_retreatRatio = m_retreatCount / sampleCount;
    m_frozenPattern.m_leftRatio = m_leftCount / sampleCount;
    m_frozenPattern.m_rightRatio = m_rightCount / sampleCount;
    m_frozenPattern.m_attackInterval = m_intervalCount ? m_intervalSum / (float)m_intervalCount : 0.0f;
    m_frozenPattern.m_punishRatio = m_pattern.m_attacks ? float(m_punishCount) / m_pattern.m_attacks : 0.0f;
    m_bIsFrozen = true;
}

//観察した癖に合わせて攻撃の選びやすさを変える。
float UBossObservation::AdaptWeight(uint8 _actionKind) const
{

    //経験がなくても通常の抽選を残す。
    const EEnemyAction kind = static_cast<EEnemyAction>(_actionKind);
    const auto* history = GetWorld()->GetSubsystem<UEnemyPlayerHistory>();
    float bonus = 0.f;
    if(m_pattern.m_samples >= m_minimumSamples)
    {
        const float count = FMath::Max(1, m_pattern.m_samples);
        if(kind == EEnemyAction::Heavy) { bonus += m_closeCount / count; }
        if(kind == EEnemyAction::Chase) { bonus += m_retreatCount / count; }
        if(kind == EEnemyAction::Counter && m_pattern.m_attacks > 0)
        {
            bonus += float(m_punishCount) / m_pattern.m_attacks;
        }
        if(kind == EEnemyAction::Slash && m_pattern.m_dodges >= BossObservationTuning::minDodges)
        {
            bonus += (m_leftCount + m_rightCount) / count;
        }
    }

    //雑魚戦を飛ばした場合は、この戦闘中の記録から育てる。
    if(m_useHistory && history && history->m_attacks >= m_minAttacks)
    {
        if(kind == EEnemyAction::Counter) { bonus += history->m_repeatRate; }
        if(kind == EEnemyAction::Heavy && history->m_attackGap > 0.f && history->m_attackGap < m_fastGap)
        {
            bonus += 1.f - history->m_attackGap / FMath::Max(.1f, m_fastGap);
        }
    }
    return 1.f + FMath::Clamp(bonus * m_adaptStrength, 0.f, m_maxBonus);
}

//少ない記録を断定せず、記録が増えるほど信用する。
float UBossObservation::GetConfidence() const
{
    const auto* history = GetWorld()->GetSubsystem<UEnemyPlayerHistory>();
    const int32 attacks = m_useHistory && history ? FMath::Max(history->m_attacks, m_pattern.m_attacks) : m_pattern.m_attacks;
    const float attackTrust = float(attacks) / FMath::Max(1, m_knownAttacks);
    const float moveTrust = float(m_pattern.m_samples) / FMath::Max(1, m_knownSamples);
    return FMath::Clamp(FMath::Max(attackTrust, moveTrust), 0.f, 1.f);
}

//今の移動が続いた場合の位置から、次の技を予測する。
float UBossObservation::PredictWeight(const FEnemyActionData& _action) const
{
    if(!m_usePrediction || !m_lastTarget.IsValid() || GetWorld()->GetTimeSeconds() - m_seenTime > m_sightMemory)
    {
        return 0.f;
    }
    const FVector offset = m_seenPos - GetOwner()->GetActorLocation();
    const FVector direction = offset.GetSafeNormal2D();
    const FVector lead = (m_seenSpeed * FMath::Clamp(m_lookAhead, 0.f, .8f)).GetClampedToMaxSize(FMath::Max(0.f, m_maxLead));
    const float distance = (offset + lead).Size2D();
    const float away = FVector::DotProduct(m_seenSpeed, direction);
    const float side = FMath::Abs(FVector::DotProduct(m_seenSpeed, FVector::CrossProduct(FVector::UpVector, direction)));
    const bool inRange = distance >= _action.m_minRange && distance <= _action.m_maxRange;
    float score = inRange ? 0.f : -BossObservationTuning::missPenalty;

    //遠ざかる相手には追跡、横へ動く相手には通常の斬撃を選びやすくする。
    if(_action.m_kind == EEnemyAction::Chase && away > BossObservationTuning::moveThreshold)
    {
        score += 1.f;
    }
    if(_action.m_kind == EEnemyAction::Slash && inRange && side > BossObservationTuning::moveThreshold)
    {
        score += BossObservationTuning::smallBonus;
    }

    //こちらへ向けた攻撃や接近が見えた時は、長い強攻撃より反撃を選ぶ。
    const bool threat = m_seenFacing && (m_seenAttack || away < -BossObservationTuning::moveThreshold);
    if(threat && inRange)
    {
        if(_action.m_kind == EEnemyAction::Counter || _action.m_kind == EEnemyAction::Dodge)
        {
            score += 1.f;
        }
        if(_action.m_kind == EEnemyAction::Heavy)
        {
            score -= BossObservationTuning::smallBonus;
        }
    }

    //近くで攻撃していない相手には、自分から通常攻撃や強攻撃を仕掛ける。
    else if(inRange && !m_seenAttack && away <= BossObservationTuning::moveThreshold)
    {
        if(_action.m_kind == EEnemyAction::Slash || _action.m_kind == EEnemyAction::Heavy)
        {
            score += BossObservationTuning::smallBonus;
        }
    }
    return score;
}

//未観察は予測中心、記録がある時は予測と癖の両方で判断する。
float UBossObservation::GetActionWeight(const FEnemyActionData& _action) const
{
    const float trust = GetConfidence();
    const float read = FMath::Lerp(FMath::Max(0.f, m_newRead), FMath::Max(0.f, m_knownRead), trust);
    const float learned = (AdaptWeight(uint8(_action.m_kind)) - 1.f) * trust;
    return FMath::Clamp(1.f + PredictWeight(_action) * read + learned, BossObservationTuning::minWeight, BossObservationTuning::maxWeight);
}

//外した技にも、後でもう一度試す余地を残す。
void UBossObservation::RecordMove(FName _move, bool _hit)
{
    constexpr float recentWeight = .35f;
    float& score = m_moveScores.FindOrAdd(_move);
    score = FMath::Lerp(score, _hit ? 1.f : -1.f, recentWeight);
}

//その技の戦績を、次に選ぶときの重みに変える。
float UBossObservation::MoveWeight(FName _move) const
{ 
    return 1.f + .5f * m_moveScores.FindRef(_move); 
}

//次の戦闘に向けて状態を初期化する。
void UBossObservation::Reset()
{
    m_moveScores.Reset();
    m_seenTime = -100.;
    m_seenAttack = m_seenFacing = false;
    m_seenSpeed = FVector::ZeroVector;
    m_pattern = FBossPattern();
    m_frozenPattern = FBossPattern();
    m_bIsFrozen = false;
    m_closeCount = m_retreatCount = m_leftCount = m_rightCount = m_punishCount = m_intervalCount = 0;
    m_intervalSum = 0.0;
    m_lastAttack = -1.0;
    m_lastBossEnd = -BossObservationTuning::historyDelay;
    m_lastMontage.Reset();
    m_lastTarget.Reset();
    m_lastAnimPos = -1.0f;
}