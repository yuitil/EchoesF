#include "Enemy/Animation/BossExcaliburHit.h"
#include "Combat/EBDefenseFeedback.h"
#include "Enemy/AI/EnemyTargeting.h"
#include "Enemy/Combat/EnemyHitEffect.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Enemy/Core/EnemyData.h"
#include "Components/CapsuleComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

//斬撃と衝撃波が重なる場合も、一度の被弾で両方のダメージを入れる。
void UBossExcaliburHit::Notify(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim,
	const FAnimNotifyEventReference& _event)
{
	Super::Notify(_mesh, _anim, _event);

	//武器や残像用のメッシュから同じ判定を出さない。
	auto* boss = _mesh ? Cast<AEnemyCharacterBase>(_mesh->GetOwner()) : nullptr;
	if (!boss || !boss->HasAuthority() || _mesh != boss->GetMesh()) { return; }
	const auto* action = boss->GetActiveAction();
	if (!action || !boss->IsCombatActive() || boss->GetMoveMontage(*action) != _anim) { return; }

	//足元の波紋と、腰の高さを通る斬撃の基準。
	const FVector center = boss->GetActorLocation();
	const FVector floor = boss->GetNavAgentLocation();
	const FVector forward = boss->GetActorForwardVector().GetSafeNormal2D();
	const float frontLimit = FMath::Cos(FMath::DegreesToRadians(m_bladeAngle));
	const float range = FMath::Max(m_bladeRange, m_waveRange);
	const float height = boss->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float queryHeight = FMath::Max(m_bladeHeight, height + m_waveHeight);
	const auto shape = FCollisionShape::MakeBox(FVector(range, range, queryHeight));
	const auto objects = FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllDynamicObjects);
	FCollisionQueryParams query(SCENE_QUERY_STAT(BossExcaliburHit), false, boss);

	//プレイヤーや分身を拾い、装備まで重なっても一人につき一回だけ調べる。
	TArray<FOverlapResult> hits;
	boss->GetWorld()->OverlapMultiByObjectType(hits, center, FQuat::Identity, objects, shape, query);
	TSet<AActor*> checked;
	for (const auto& hit : hits)
	{
		auto* target = Cast<ACharacter>(hit.GetActor());
		if (!IsValid(target) || checked.Contains(target) || !boss->m_targeting->IsCandidate(target)) { continue; }
		checked.Add(target);

		//カプセルの端がエフェクトに触れた時も当たりにする。
		const FVector offset = target->GetActorLocation() - center;
		const float radius = target->GetCapsuleComponent()->GetScaledCapsuleRadius();
		const float halfHeight = target->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		const float distance = FMath::Max(0., offset.Size2D() - radius);
		const bool inFront = distance <= 0.f || FVector::DotProduct(forward, offset.GetSafeNormal2D()) >= frontLimit;
		const bool blade = inFront && distance <= m_bladeRange && FMath::Abs(offset.Z) <= m_bladeHeight + halfHeight;

		//床の波は段違いの足元やジャンプ中の相手には当てない。
		const float feet = target->GetActorLocation().Z - halfHeight;
		const bool wave = distance <= m_waveRange && FMath::Abs(feet - floor.Z) <= m_waveHeight;
		const float scale = (blade ? m_bladeDamage : 0.f) + (wave ? m_waveDamage : 0.f);
		if (scale <= 0.f) { continue; }

		//石壁などで遮られた相手には当てない。
		FHitResult wall;
		const bool blocked = boss->GetWorld()->LineTraceSingleByChannel(
			wall, center, target->GetActorLocation(), ECC_Visibility, query);
		if (blocked && wall.GetActor() != target) { continue; }

		//回避やスキルの無敵の処理は相手の通常の被弾処理に任せる。
		const float damage = action->m_damage * scale;
		const auto type = UEBHeavyUnblockableDamageType::StaticClass();
		const float dealt = UGameplayStatics::ApplyDamage(target, damage, boss->GetController(), boss, type);
		if (dealt <= 0.f) { continue; }
		if (target == boss->m_combatTarget) { boss->RecordAttackHit(); }

		//命中した体の側に、従来の火花と打撃音を出す。
		if (boss->m_hitFx)
		{
			const FVector normal = -offset.GetSafeNormal2D();
			const FVector contact = target->GetActorLocation() + normal * radius;
			boss->m_hitFx->ShowStrike(contact, normal);
		}
	}
}
