#include "Enemy/Boss/BossObservation.h"
#include "Enemy/Boss/BossData.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"

//動作を確かめるための幅や待ち時間。
namespace BossObservationTuning
{
    //近距離として記録する範囲。
    constexpr auto closeRange = 250;
    
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
{
	PrimaryComponentTick.bCanEverTick = false;
}

//相手の行動を記録する。
void UBossObservation::Observe(AActor* _target, const UBossData* _data)
{
	if(m_bIsFrozen || !IsValid(_target) || !_data) { return; }

	if (m_lastTarget.Get() != _target)
	{
		m_lastTarget = _target;
		m_lastMontage.Reset();
		m_lastAnimPos = -1.0f;
		m_lastAttack = -1.0;
	}

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
	UAnimInstance* animInstance = character && character->GetMesh() ? character->GetMesh()->GetAnimInstance() : nullptr;
	
	//対象のモンタージュ。
	UAnimMontage* montage = animInstance ? animInstance->GetCurrentActiveMontage() : nullptr;
	
	//現在の位置や再生位置。
	float position = montage ? animInstance->Montage_GetPosition(montage) : -1;
	
	//処理を開始できたか。
	const bool started = montage && (m_lastMontage.Get() != montage ||  position + BossObservationTuning::restartMargin < m_lastAnimPos);

	if (started)
	{
		//現在の時刻。
		const double now = GetWorld()->GetTimeSeconds();
		if (_data->m_attackMontages.Contains(montage))
		{
			++m_pattern.m_attacks;
			if (m_lastAttack >= 0.0)
			{
				m_intervalSum += now - m_lastAttack;
				++m_intervalCount;
			}

			if (now - m_lastBossEnd >= 0.0 && now - m_lastBossEnd < 1.0)
			{
				++m_punishCount;
			}
		}

		if (_data->m_dodgeMontages.Contains(montage))
		{
			++m_pattern.m_dodges;
		}
	}
	m_lastMontage = montage;
	m_lastAnimPos = position;
}

//相手から攻撃を受けた回数を記録する。
void UBossObservation::RecordHit()
{
	if (!m_bIsFrozen)
	{
		++m_pattern.m_hitsOnBoss;
	}
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
	if(!m_bIsFrozen || m_frozenPattern.m_samples < m_minimumSamples) { return 1.0f; }

	//対象の行動の種類。
	const EEnemyAction kind = static_cast<EEnemyAction>(_actionKind);

	if(kind == EEnemyAction::Heavy) { return 1.0f + m_frozenPattern.m_closeRatio; }
	if(kind == EEnemyAction::Chase) { return 1.0f + m_frozenPattern.m_retreatRatio; }
	if(kind == EEnemyAction::Counter) { return 1.0f + m_frozenPattern.m_punishRatio; }
	if (kind == EEnemyAction::Slash && m_frozenPattern.m_dodges > m_frozenPattern.m_attacks / 2 && m_frozenPattern.m_dodges >= BossObservationTuning::minDodges)
	{
		return 1.0f + m_frozenPattern.m_leftRatio + m_frozenPattern.m_rightRatio;
	}

	return 1.0f;
}

//次の戦闘に向けて状態を初期化する。
void UBossObservation::Reset()
{
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
