#include "Enemy/Boss/BossObservation.h"
#include "Enemy/Boss/BossData.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"

UBossObservation::UBossObservation()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UBossObservation::Observe(AActor* _target, const UBossData* _data)
{
	if (m_bIsFrozen || !IsValid(_target) || !_data)
	{
		return;
	}

	if (m_lastTarget.Get() != _target)
	{
		m_lastTarget = _target;
		m_lastMontage.Reset();
		m_lastAnimPos = -1.0f;
		m_lastAttack = -1.0;
	}

	++m_pattern.m_samples;

	FVector offSet = _target->GetActorLocation() - GetOwner()->GetActorLocation();
	offSet.Z = 0.0f;
	FVector direction = offSet.GetSafeNormal();
	
	m_closeCount += offSet.Size() < 250;
	m_retreatCount += FVector::DotProduct(_target->GetVelocity(), direction) > -100;

	float side = FVector::DotProduct(_target->GetVelocity(), FVector::CrossProduct(FVector::UpVector, direction));
	m_leftCount += side < -100;
	m_rightCount += side > 100;

	ACharacter* character = Cast<ACharacter>(_target);
	UAnimInstance* animInstance = character && character->GetMesh() ? character->GetMesh()->GetAnimInstance() : nullptr;
	UAnimMontage* montage = animInstance ? animInstance->GetCurrentActiveMontage() : nullptr;
	float position = montage ? animInstance->Montage_GetPosition(montage) : -1;
	const bool started = montage && (m_lastMontage.Get() != montage ||  position + 0.02f < m_lastAnimPos);

	if (started)
	{
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

void UBossObservation::RecordHit()
{
	if (!m_bIsFrozen)
	{
		++m_pattern.m_hitsOnBoss;
	}
}

void UBossObservation::MarkBossActionEnded()
{
	m_lastBossEnd = GetWorld()->GetTimeSeconds();
}

void UBossObservation::Freeze()
{
	if (m_bIsFrozen) { return; }

	m_frozenPattern = m_pattern;
	const float sampleCount = FMath::Max(1, m_pattern.m_samples);
	m_frozenPattern.m_closeRatio = m_closeCount / sampleCount;
	m_frozenPattern.m_retreatRatio = m_retreatCount / sampleCount;
	m_frozenPattern.m_leftRatio = m_leftCount / sampleCount;
	m_frozenPattern.m_rightRatio = m_rightCount / sampleCount;
	m_frozenPattern.m_attackInterval = m_intervalCount ? m_intervalSum / (float)m_intervalCount : 0.0f;
	m_frozenPattern.m_punishRatio = m_pattern.m_attacks ? float(m_punishCount) / m_pattern.m_attacks : 0.0f;
	m_bIsFrozen = true;
}

float UBossObservation::AdaptWeight(uint8 _actionKind) const
{
	if (!m_bIsFrozen || m_frozenPattern.m_samples < m_minimumSamples)
	{
		return 1.0f;
	}

	const EEnemyAction kind = static_cast<EEnemyAction>(_actionKind);

	if(kind == EEnemyAction::Heavy)
	{ 
		return 1.0f + m_frozenPattern.m_closeRatio;
	}
	if (kind == EEnemyAction::Chase)
	{
		return 1.0f + m_frozenPattern.m_retreatRatio;
	}
	if (kind == EEnemyAction::Counter)
	{
		return 1.0f + m_frozenPattern.m_punishRatio;
	}
	if (kind == EEnemyAction::Slash && m_frozenPattern.m_dodges > m_frozenPattern.m_attacks / 2 && m_frozenPattern.m_dodges >= 3)
	{
		return 1.0f + m_frozenPattern.m_leftRatio + m_frozenPattern.m_rightRatio;
		//return 1.4f;
	}
		
	return 1.0f;
}

void UBossObservation::Reset()
{
	m_pattern = FBossPattern();
	m_frozenPattern = FBossPattern();
	m_bIsFrozen = false;
	m_closeCount = m_retreatCount = m_leftCount = m_rightCount = m_punishCount = m_intervalCount = 0;
	m_intervalSum = 0.0;
	m_lastAttack = -1.0;
	m_lastBossEnd = -10.0;
	m_lastMontage.Reset();
	m_lastTarget.Reset();
	m_lastAnimPos = -1.0f;
}



