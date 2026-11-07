#include "Enemy/Combat/EnemyHitEffect.h"
#include "Enemy/Audio/EnemyAudio.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

//初期設定と必要な部品を用意する。
UEnemyHitEffect::UEnemyHitEffect()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

//BPで表示している刀のうち、指定した位置に一番近い刃を探す。
void UEnemyHitEffect::FindBladePoint(AActor* _owner, const FVector& _near, FVector& _point) const
{
	//別アクターとして装備した刀と、BPの部品として置いた刀を調べる。
	TArray<AActor*> actors;
	_owner->GetAttachedActors(actors);
	actors.Add(_owner);

	//刃が見つかるまでは呼び出し元の位置を残す。
	float best = TNumericLimits<float>::Max();
	for (auto* actor : actors)
	{
		if (!IsValid(actor) || actor->IsHidden()) { continue; }

		//納刀中の刀は、火花や命中位置の候補から外す。
		TInlineComponentArray<UMeshComponent*> meshes(actor);
		for (const auto* mesh : meshes)
		{
			if (!mesh->IsVisible() || mesh->bHiddenInGame) { continue; }
			if (!mesh->DoesSocketExist(m_base) || !mesh->DoesSocketExist(m_tip)) { continue; }

			//刃元と剣先の間で、接触位置に最も近い点を選ぶ。
			const FVector base = mesh->GetSocketLocation(m_base);
			const FVector tip = mesh->GetSocketLocation(m_tip);
			const FVector point = FMath::ClosestPointOnSegment(_near, base, tip);
			const float distance = FVector::DistSquared(point, _near);
			if (distance >= best) { continue; }
			best = distance;
			_point = point;
		}
	}
}

//攻撃者の刀に近い体の表面を探す。
bool UEnemyHitEffect::EstimateContact(AActor* _attacker, FVector& _point, FVector& _normal) const
{
	const auto* victim = Cast<ACharacter>(GetOwner());
	if (!victim || !IsValid(_attacker)) { return false; }

	//素手の攻撃では、攻撃者の位置を基準にする。
	FVector source = _attacker->GetActorLocation();
	FindBladePoint(_attacker, victim->GetActorLocation(), source);

	//対象のボーン。
	FName bone;

	//相手までの距離。
	float distance = 0.f;
	if (victim->GetMesh()->K2_GetClosestPointOnPhysicsAsset(
		source, _point, _normal, bone, distance))
	{
		_normal = _normal.GetSafeNormal();
		if (_normal.IsNearlyZero())
		{
			_normal = (source - victim->GetActorLocation()).GetSafeNormal();
		}
		return !_point.ContainsNaN() && !_normal.ContainsNaN();
	}
	if (victim->GetCapsuleComponent()->GetClosestPointOnCollision(source, _point) < 0.f)
	{

		//物理の形が使えなくても、体の表面に命中演出を残す。
		const FVector direction = (source - victim->GetActorLocation()).GetSafeNormal2D();
		_point = victim->GetActorLocation() +
			direction * victim->GetCapsuleComponent()->GetScaledCapsuleRadius();
	}
	_normal = (source - _point).GetSafeNormal();
	if (_normal.IsNearlyZero()) { _normal = -victim->GetActorForwardVector(); }
	return !_point.ContainsNaN();
}

//体に当たった場所の演出を開始する。
void UEnemyHitEffect::PlayHit(AActor* _attacker, const FHitResult* _hit)
{
	if (!GetOwner()->HasAuthority()) { return; }
	FVector point, normal;
	if (_hit && _hit->GetActor() == GetOwner() && !_hit->bStartPenetrating && !_hit->ImpactPoint.ContainsNaN())
	{
		point = _hit->ImpactPoint;
		normal = _hit->ImpactNormal.GetSafeNormal();
	}
	else if (!EstimateContact(_attacker, point, normal)) { return; }
	ShowHit(point, normal);
}

//命中位置でエフェクトを再生する。
void UEnemyHitEffect::ShowHit_Implementation(FVector _point, FVector _normal)
{
	//鎧や体の命中音。
	if (auto* audio = GetOwner()->FindComponentByClass<UEnemyAudio>())
	{
		audio->Emit_Implementation(EEnemySound::Armor, _point, 1.f);
	}
	if (!m_effect || GetNetMode() == NM_DedicatedServer) { return; }
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, m_effect, _point + _normal * m_offset, _normal.Rotation(), m_scale, true, true, ENCPoolMethod::AutoRelease);
}

//相手に近い剣の位置を使う。剣がなければ体の接触位置を使う。
void UEnemyHitEffect::PlayClash(AActor* _attacker)
{
	if (!GetOwner()->HasAuthority() || !IsValid(_attacker)) { return; }

	FVector point, normal;
	if (!EstimateContact(_attacker, point, normal)) { return; }

	//体に当たるはずだった位置に一番近い、防御側の刀を使う。
	const FVector contact = point;
	FindBladePoint(GetOwner(), contact, point);
	ShowClash(point, normal);
}

//体の命中とは別の火花を出す。
void UEnemyHitEffect::ShowClash_Implementation(FVector _point, FVector _normal)
{

	//剣で受けた時の金属音。
	if (auto* audio = GetOwner()->FindComponentByClass<UEnemyAudio>())
	{
		audio->Emit_Implementation(EEnemySound::Clash, _point, 1.f);
	}
	if (!m_clashEffect || GetNetMode() == NM_DedicatedServer) { return; }

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, m_clashEffect, _point, _normal.Rotation(), m_clashScale, true, true, ENCPoolMethod::AutoRelease);
}

//ダメージを与えた時だけ、相手の命中位置で音と効果を出す。
void UEnemyHitEffect::ShowStrike_Implementation(FVector _point, FVector _normal)
{
	if (GetNetMode() == NM_DedicatedServer || _point.ContainsNaN()) { return; }
	if (auto* audio = GetOwner()->FindComponentByClass<UEnemyAudio>())
	{
		audio->Emit_Implementation(EEnemySound::Hit, _point, 1.f);
	}
	if (!m_strikeEffect) { return; }
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, m_strikeEffect, _point + _normal * m_offset, _normal.Rotation(), m_strikeScale, true, true, ENCPoolMethod::AutoRelease);
}
