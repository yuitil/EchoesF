#include "Enemy/AI/EnemyPlayerHistory.h"
#include "Player/CombatComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"

//集計の間隔と記憶の長さ。
namespace EnemyHistoryTuning
{
	constexpr float sampleTime = .2f;
	constexpr float newWeight = .2f;
	constexpr float restartMargin = .02f;
	constexpr float maxGap = 10.f;
	constexpr int32 maxCount = 64;
}

//マップ開始時に軽い定期観察を始める。
void UEnemyPlayerHistory::OnWorldBeginPlay(UWorld& _world)
{
	Super::OnWorldBeginPlay(_world);
	if (_world.GetNetMode() == NM_Client) { return; }
	_world.GetTimerManager().SetTimer(m_timer, this, &UEnemyPlayerHistory::Sample, EnemyHistoryTuning::sampleTime, true);
}

//マップを離れる時に観察を止める。
void UEnemyPlayerHistory::Deinitialize()
{
	if (GetWorld()) { GetWorld()->GetTimerManager().ClearTimer(m_timer); }
	Super::Deinitialize();
}

//今の攻撃を確認して、前の攻撃との違いを覚える。
void UEnemyPlayerHistory::Sample()
{

	//分身ではなく、操作中の本体だけを見る。
	auto* player = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (!IsValid(player)) { return; }
	if (m_player.Get() != player)
	{
		m_player = player;
		m_attacks = 0;
		m_repeatRate = m_attackGap = 0.f;
		m_attack.Reset();
		m_playing.Reset();
		m_lastAttack = -1.;
	}

	//既存の攻撃状態を読むだけで、プレイヤーの処理は変更しない。
	auto* combat = player->FindComponentByClass<UCombatComponent>();
	auto* anim = player->GetMesh() ? player->GetMesh()->GetAnimInstance() : nullptr;
	auto* montage = combat && combat->IsPlayingAttackMontage() && anim ? anim->GetCurrentActiveMontage() : nullptr;
	const float position = montage ? anim->Montage_GetPosition(montage) : -1.f;
	const bool started = montage && (montage != m_playing.Get() || position + EnemyHistoryTuning::restartMargin < m_position);
	if (started)
	{
		const double now = GetWorld()->GetTimeSeconds();
		if (m_attacks > 0)
		{
			const float repeat = montage == m_attack.Get() ? 1.f : 0.f;
			const float gap = FMath::Clamp(float(now - m_lastAttack), 0.f, EnemyHistoryTuning::maxGap);
			m_repeatRate = FMath::Lerp(m_repeatRate, repeat, EnemyHistoryTuning::newWeight);
			m_attackGap = m_attacks == 1 ? gap : FMath::Lerp(m_attackGap, gap, EnemyHistoryTuning::newWeight);
		}
		m_attacks = FMath::Min(m_attacks + 1, EnemyHistoryTuning::maxCount);
		m_lastAttack = now;
		m_attack = montage;
	}
	m_playing = montage;
	m_position = position;
}
