#include "Enemy/Animation/EnemySwing.h"
#include "Enemy/Animation/BossBladeTrail.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Enemy/Combat/Combat.h"
#include "Enemy/Audio/EnemyAudio.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

//剣の更新後に、短い区間だけ動かす。
UEnemySwing::UEnemySwing()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

//前のフレームの剣を読まないようにする。
void UEnemySwing::BeginPlay()
{
	Super::BeginPlay();
	auto* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	if (enemy && enemy->m_combat) { AddTickPrerequisiteComponent(enemy->m_combat); }
	if (enemy && enemy->GetMesh()) { AddTickPrerequisiteComponent(enemy->GetMesh()); }
}

//一振りごとに音と位置を初期化する。
void UEnemySwing::Start()
{
	Stop();
	m_open = true;
	SetComponentTickEnabled(true);
	auto* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	FVector base, tip;
	if (!enemy || !enemy->m_combat || !enemy->m_combat->GetWeaponPoints(base, tip)) { return; }
	m_lastTip = enemy->GetActorTransform().InverseTransformPosition(tip);
	m_hasTip = true;
	if (m_bladeArc) { m_poses.Add({ base, tip, GetWorld()->GetTimeSeconds() }); }
}

//見た目専用の通知が付いたモンタージュか調べる。
bool UEnemySwing::UsesTrailNotify() const
{
	if (!m_useBladeNotify && !m_fullSwing) { return false; }
	const auto* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	const auto* action = enemy ? enemy->GetActiveAction() : nullptr;
	if (!action || !action->m_montage) { return false; }
	return action->m_montage->Notifies.ContainsByPredicate([](const FAnimNotifyEvent& event)
		{
			return event.NotifyStateClass && event.NotifyStateClass->IsA<UBossBladeTrail>();
		});
}

//左右の刃と、光を出す区間を受け取る。
void UEnemySwing::StartBlade(UAnimMontage* _move, bool _left, float _start, float _end)
{
	Stop();
	m_notifyBlade = m_open = true;
	m_left = _left;
	m_swingStart = _start;
	SetSwingEnd(_move, _end);
	SetComponentTickEnabled(true);
}

//中断した線を次の攻撃へつながない。
void UEnemySwing::ClearTrail()
{
	if (IsValid(m_fx)) { m_fx->DestroyComponent(); }
	m_fx = nullptr;
}

//通知に設定した終了時刻を保存する。
void UEnemySwing::SetSwingEnd(UAnimMontage* _move, float _end)
{
	m_swingMove = _move;
	m_swingEnd = _end;
}

//通知終了や攻撃中断で必ず止める。
void UEnemySwing::Stop()
{
	ClearTrail();
	m_poses.Reset();
	m_notifyBlade = false;
	m_swingStart = 0.f;
	m_swingMove.Reset();
	m_open = false;
	m_hasTip = false;
	m_played = false;
	SetComponentTickEnabled(false);
}

//移動そのものを剣の振りと間違えないようにする。
void UEnemySwing::TickComponent(float _delta, ELevelTick _type, FActorComponentTickFunction* _tick)
{
	Super::TickComponent(_delta, _type, _tick);
	auto* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	if (!m_open || !enemy || !enemy->IsActing() || !enemy->m_combat ||
		(!m_notifyBlade && !enemy->m_combat->IsHitWindowOpen()))
	{
		Stop();
		return;
	}

	//通知の終了が次の更新に回っても、斬撃は振り終わりで消す。
	if (auto* move = m_swingMove.Get())
	{
		auto* anim = enemy->GetMesh()->GetAnimInstance();
		if (!anim || !anim->Montage_IsPlaying(move) || anim->GetCurrentActiveMontage() != move)
		{
			Stop();
			return;
		}

		//剣が振られている区間を外れたら、軌跡を閉じる。
		const float position = anim->Montage_GetPosition(move);
		if (position < m_swingStart - KINDA_SMALL_NUMBER || position >= m_swingEnd)
		{
			Stop();
			return;
		}
	}
	if (_delta <= SMALL_NUMBER) { return; }

	//攻撃判定と同じ剣の位置を使う。
	FVector base, tip;
	const bool found = m_notifyBlade ? enemy->m_combat->GetBladePoints(m_left, base, tip) : enemy->m_combat->GetWeaponPoints(base, tip);
	if (!found)
	{
		ClearTrail();
		m_poses.Reset();
		m_hasTip = false;
		return;
	}
	if (m_notifyBlade && !m_hasTip)
	{
		m_lastTip = enemy->GetActorTransform().InverseTransformPosition(tip);
		m_hasTip = true;
		m_poses.Add({ base, tip, GetWorld()->GetTimeSeconds() });
		return;
	}
	const FVector localTip = enemy->GetActorTransform().InverseTransformPosition(tip);
	const FVector travel = m_hasTip ? localTip - m_lastTip : FVector::ZeroVector;
	const float step = travel.Size();
	m_lastTip = localTip;
	m_hasTip = true;
	if (step > m_maxStep || (!m_notifyBlade && (!m_fullSwing || !m_bladeArc || !m_played) && step / _delta < m_minSpeed))
	{
		ClearTrail();
		m_poses.Reset();
		if (m_bladeArc) { m_poses.Add({ base, tip, GetWorld()->GetTimeSeconds() }); }
		return;
	}

	//剣が動き出した一回だけ音を出す。
	const FVector point = FMath::Lerp(base, tip, m_tipPart);

	if (!m_played)
	{
		if (enemy->m_audio) { enemy->m_audio->Play(EEnemySound::Swing, point, m_gain); }
		m_played = true;
	}

	//素材は敵BPに設定されたものを使う。
	if (!m_trail || GetNetMode() == NM_DedicatedServer) { return; }
	if (m_bladeArc)
	{
		UpdateSweep(base, tip);
		return;
	}
	if (!IsValid(m_fx))
	{
		m_fx = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, m_trail, point, FRotator::ZeroRotator, FVector::OneVector, true, true, ENCPoolMethod::None, false);
	}
	if (m_fx) { m_fx->SetWorldLocation(point); }
}

//直近の軌道を四つの断面に分けてNiagaraのマテリアルへ渡す。
void UEnemySwing::UpdateSweep(FVector _base, FVector _tip)
{
	const double now = GetWorld()->GetTimeSeconds();
	m_poses.Add({ _base, _tip, now });
	const double oldest = m_fullSwing ? m_poses[0].time : now - FMath::Clamp(m_sweepTime, .01f, .15f);
	while (m_poses.Num() > 2 && m_poses[1].time <= oldest)
	{
		m_poses.RemoveAt(0, 1, EAllowShrinking::No);
	}
	if (m_poses.Num() < 2) { return; }

	const FVector center = (_base + _tip) * .5f;
	if (!IsValid(m_fx))
	{
		m_fx = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, m_trail, center, FRotator::ZeroRotator, FVector::OneVector, true, false, ENCPoolMethod::None, false);
	}
	if (!m_fx) { return; }
	m_fx->SetWorldLocation(center);
	constexpr int32 sections = 4;

	//マテリアルの各点はXYZと余白の4値で並ぶ。
	constexpr int32 vectorSlots = 4;
	float points[sections * 2 * vectorSlots] = {};
	int32 next = 1;
	for (int32 i = 0; i < sections; ++i)
	{
		const double time = FMath::Lerp(FMath::Max(oldest, m_poses[0].time), now, double(i) / (sections - 1));
		while (next < m_poses.Num() - 1 && m_poses[next].time < time)
		{
			++next;
		}
		const auto& a = m_poses[next - 1];
		const auto& b = m_poses[next];
		const double alpha = FMath::Clamp((time - a.time) / FMath::Max(UE_SMALL_NUMBER, b.time - a.time), 0., 1.);
		const FVector3f basePos(FMath::Lerp(a.base, b.base, alpha));
		const FVector3f tipPos(FMath::Lerp(a.tip, b.tip, alpha));
		const int32 offset = i * 2 * vectorSlots;
		for (int32 axis = 0; axis < 3; ++axis)
		{
			points[offset + axis] = basePos[axis];
			points[offset + vectorSlots + axis] = tipPos[axis];
		}
	}
	m_fx->SetCustomPrimitiveDataFloatArray(0, MakeArrayView(points));
	if (!m_fx->IsActive()) { m_fx->Activate(); }
}

//死亡・解除・マップ終了でも残さない。
void UEnemySwing::EndPlay(const EEndPlayReason::Type _reason)
{
	Stop();
	Super::EndPlay(_reason);
}
