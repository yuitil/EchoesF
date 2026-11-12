#include "Enemy/Animation/EnemySwing.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Enemy/Combat/Combat.h"
#include "Enemy/Audio/EnemyAudio.h"
#include "Components/SkeletalMeshComponent.h"
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

//命中判定が開いた手の軌道を、今の刃の位置から描く。
void UEnemySwing::StartHand(bool left)
{
	auto& hand = m_hands[left];
	CloseHand(hand);
	hand.m_left = left;
	hand.m_open = true;
	SetComponentTickEnabled(true);
	auto* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	FVector base, tip;
	if (!enemy || !enemy->m_combat || !enemy->m_combat->ReadPoints(left, base, tip)) { return; }
	hand.m_lastTip = enemy->GetActorTransform().InverseTransformPosition(tip);
	hand.m_hasTip = true;
	if (m_bladeArc) { hand.m_poses.Add({ base, tip, GetWorld()->GetTimeSeconds() }); }
}

//判定が閉じたフレームで、この手の表示を消す。
void UEnemySwing::EndHand(bool left)
{
	CloseHand(m_hands[left]);
	SetComponentTickEnabled(m_hands[0].m_open || m_hands[1].m_open);
}

//中断した線を次の攻撃へつながない。
void UEnemySwing::ClearTrail(FTrail& hand)
{
	if (hand.m_fx.IsValid()) { hand.m_fx->DestroyComponent(); }
	hand.m_fx = nullptr;
}

//通知終了や攻撃中断で必ず止める。
void UEnemySwing::Stop()
{
	for (auto& hand : m_hands) { CloseHand(hand); }
	SetComponentTickEnabled(false);
}

//もう一方の斬撃はそのまま続ける。
void UEnemySwing::CloseHand(FTrail& hand)
{
	ClearTrail(hand);
	hand = FTrail();
}

//移動そのものを剣の振りと間違えないようにする。
void UEnemySwing::TickComponent(float _delta, ELevelTick _type, FActorComponentTickFunction* _tick)
{
	Super::TickComponent(_delta, _type, _tick);
	for (auto& hand : m_hands)
	{
		if (hand.m_open) { UpdateHand(hand, _delta); }
	}
	SetComponentTickEnabled(m_hands[0].m_open || m_hands[1].m_open);
}

//通知中の手の動きに斬撃を合わせる。
void UEnemySwing::UpdateHand(FTrail& hand, float _delta)
{
	auto* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	if (!hand.m_open || !enemy || !enemy->IsActing() || !enemy->m_combat ||
		!enemy->m_combat->IsHandOpen(hand.m_left))
	{
		CloseHand(hand);
		return;
	}
	if (_delta <= SMALL_NUMBER) { return; }

	//攻撃判定と同じ剣の位置を使う。
	FVector base, tip;
	const bool found = enemy->m_combat->ReadPoints(hand.m_left, base, tip);
	if (!found)
	{
		ClearTrail(hand);
		hand.m_poses.Reset();
		hand.m_hasTip = false;
		return;
	}
	if (!hand.m_hasTip)
	{
		hand.m_lastTip = enemy->GetActorTransform().InverseTransformPosition(tip);
		hand.m_hasTip = true;
		hand.m_poses.Add({ base, tip, GetWorld()->GetTimeSeconds() });
		return;
	}
	const FVector localTip = enemy->GetActorTransform().InverseTransformPosition(tip);
	const FVector travel = hand.m_hasTip ? localTip - hand.m_lastTip : FVector::ZeroVector;
	const float step = travel.Size();
	hand.m_lastTip = localTip;
	hand.m_hasTip = true;
	//位置交換で刀が飛んだ時は、前の場所から斬撃を引かない。
	bool jumped = false;
	if (m_bladeArc && !hand.m_poses.IsEmpty())
	{
		const auto& last = hand.m_poses.Last();
		const float limit = FMath::Square(m_maxStep * enemy->GetActorScale3D().GetAbsMax());
		jumped = FVector::DistSquared(last.base, base) > limit || FVector::DistSquared(last.tip, tip) > limit;
	}
	if (jumped || step > m_maxStep)
	{
		ClearTrail(hand);
		hand.m_poses.Reset();
		if (m_bladeArc) { hand.m_poses.Add({ base, tip, GetWorld()->GetTimeSeconds() }); }
		return;
	}

	//剣が動き出した一回だけ音を出す。
	const FVector point = FMath::Lerp(base, tip, m_tipPart);

	if (!hand.m_played && step / _delta >= m_minSpeed)
	{
		if (enemy->m_audio) { enemy->m_audio->Play(EEnemySound::Swing, point, m_gain); }
		hand.m_played = true;
	}

	//素材は敵BPに設定されたものを使う。
	if (!m_trail || GetNetMode() == NM_DedicatedServer) { return; }
	if (m_bladeArc)
	{
		UpdateSweep(hand, base, tip);
		return;
	}
	if (!hand.m_fx.IsValid())
	{
		hand.m_fx = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, m_trail, point, FRotator::ZeroRotator, FVector::OneVector, true, true, ENCPoolMethod::None, false);
	}
	if (hand.m_fx.IsValid()) { hand.m_fx->SetWorldLocation(point); }
}

//直近の軌道を四つの断面に分けてNiagaraのマテリアルへ渡す。
void UEnemySwing::UpdateSweep(FTrail& hand, FVector _base, FVector _tip)
{
	const double now = GetWorld()->GetTimeSeconds();
	hand.m_poses.Add({ _base, _tip, now });
	const double oldest = m_fullSwing ? hand.m_poses[0].time : now - FMath::Clamp(m_sweepTime, .01f, .15f);
	while (hand.m_poses.Num() > 2 && hand.m_poses[1].time <= oldest)
	{
		hand.m_poses.RemoveAt(0, 1, EAllowShrinking::No);
	}
	if (hand.m_poses.Num() < 2) { return; }

	const FVector center = (_base + _tip) * .5f;
	if (!hand.m_fx.IsValid())
	{
		hand.m_fx = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, m_trail, center, FRotator::ZeroRotator, FVector::OneVector, true, false, ENCPoolMethod::None, false);
	}
	if (!hand.m_fx.IsValid()) { return; }
	hand.m_fx->SetWorldLocation(center);
	constexpr int32 sections = 4;

	//マテリアルの各点はXYZと余白の4値で並ぶ。
	constexpr int32 vectorSlots = 4;
	float points[sections * 2 * vectorSlots] = {};
	int32 next = 1;
	for (int32 i = 0; i < sections; ++i)
	{
		const double time = FMath::Lerp(FMath::Max(oldest, hand.m_poses[0].time), now, double(i) / (sections - 1));
		while (next < hand.m_poses.Num() - 1 && hand.m_poses[next].time < time)
		{
			++next;
		}
		const auto& a = hand.m_poses[next - 1];
		const auto& b = hand.m_poses[next];
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
	hand.m_fx->SetCustomPrimitiveDataFloatArray(0, MakeArrayView(points));
	if (!hand.m_fx->IsActive()) { hand.m_fx->Activate(); }
}

//死亡・解除・マップ終了でも残さない。
void UEnemySwing::EndPlay(const EEndPlayReason::Type _reason)
{
	Stop();
	Super::EndPlay(_reason);
}
