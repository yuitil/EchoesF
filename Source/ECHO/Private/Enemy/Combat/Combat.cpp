#include "Enemy/Combat/Combat.h"
#include "Enemy/Combat/EnemyHitEffect.h"
#include "GameFramework/DamageType.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/MeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Player/CombatComponent.h"

//判定に使う幅や待ち時間。
namespace CombatTuning
{
    //打ち上げ状態を続ける時間の上限。
    constexpr auto maxLiftTime = 3.f;
    //剣の軌道を調べる分割数の上限。
    constexpr auto maxSweepSteps = 24;
    //確認用の表示サイズ。
    constexpr auto debugSize = 5.0f;
    //打ち上げ判定に使う最小値。
    constexpr auto minLiftValue = .1f;
}

//初期設定と必要な部品を用意する。
UCombat::UCombat()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

//BPで指定した剣の判定を接続する。
void UCombat::BeginPlay()
{
    Super::BeginPlay();
    BindWeapon();
}

//攻撃前にも剣の接続を確かめる。
void UCombat::BindWeapon()
{
    if(!GetOwner() || m_weaponTag.IsNone()) { return; }

    for(auto* part : GetOwner()->GetComponentsByTag(UMeshComponent::StaticClass(), m_weaponTag))
    {
        if(SetWeaponMesh(Cast<UMeshComponent>(part), m_baseSocket, m_tipSocket)) { return; }
    }
}

//攻撃判定に使う武器を設定する。
bool UCombat::SetWeaponMesh(UMeshComponent* _mesh, FName _base, FName _tip)
{
	EndHitWindow();
	m_weapon = nullptr;
	m_useWeapon = true;
	if(!IsValid(_mesh) || _mesh->GetOwner() != GetOwner() || !_mesh->DoesSocketExist(_base) || !_mesh->DoesSocketExist(_tip)) { return false; }

	m_weapon = _mesh;
	m_baseSocket = _base;
	m_tipSocket = _tip;
	return true;
}

//命中した相手を打ち上げる。
void UCombat::BeginLift(ACharacter* _target, const FEnemyActionData& _action)
{
	//処理の対象となる敵。
	auto* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	if(!enemy || !enemy->CanLiftTarget(_target)) { return; }
	EndLift();
	m_liftTarget = _target;
	//元の重力を記録してから、空中コンボ用に変更する。
	m_targetGravity = _target->GetCharacterMovement()->GravityScale;
	m_selfGravity = enemy->GetCharacterMovement()->GravityScale;
	m_liftGravity = FMath::Max(CombatTuning::minLiftValue, _action.m_liftGravity);
	m_liftEnd = GetWorld()->GetTimeSeconds() + FMath::Clamp(_action.m_liftTime, CombatTuning::minLiftValue, CombatTuning::maxLiftTime);
	_target->GetCharacterMovement()->GravityScale = m_liftGravity;
	enemy->GetCharacterMovement()->GravityScale = m_liftGravity;
	//相手と自分を打ち上げて空中で攻撃をつなぐ。
	_target->LaunchCharacter(FVector(0, 0, _action.m_liftSpeed), true, true);
	enemy->LaunchCharacter(FVector(0, 0, _action.m_liftSpeed), true, true);
}

//打ち上げ後の重力を元に戻す。
void UCombat::EndLift()
{
	if(m_liftEnd <= 0.) { return; }
	if (auto* target = m_liftTarget.Get())
	{
		if (FMath::IsNearlyEqual(target->GetCharacterMovement()->GravityScale, m_liftGravity))
		{
			target->GetCharacterMovement()->GravityScale = m_targetGravity;
		}
	}
	if (auto* enemy = Cast<ACharacter>(GetOwner()))
	{
		if (FMath::IsNearlyEqual(enemy->GetCharacterMovement()->GravityScale, m_liftGravity))
		{
			enemy->GetCharacterMovement()->GravityScale = m_selfGravity;
		}
	}
	m_liftTarget.Reset();
	m_liftEnd = 0.;
}

//終了時にタイマーや登録を解除する。
void UCombat::EndPlay(const EEndPlayReason::Type _reason)
{
	EndLift();
	Super::EndPlay(_reason);
}

//剣の攻撃判定を有効にする。
void UCombat::BeginHitWindow(float _damageScale)
{
    BindWeapon();
	m_hitScale = FMath::Max(0.0f, _damageScale);
	//前の命中記録を消して、次の判定に備える。
	m_hitActors.Empty();
	m_hasPreviousHit = false;
	m_bHitWindow = true;

	if (m_bDebug) { UE_LOG(LogTemp, Display, TEXT("BossCombat WindowOpen %s"), *GetNameSafe(GetOwner())); }

}

//剣の攻撃判定を終了する。
void UCombat::EndHitWindow()
{
	m_bHitWindow = false;
	m_hasPreviousHit = false;
}

//攻撃と防御の判定を初期化する。
void UCombat::ResetWindows()
{
	EndHitWindow();
	//前の命中記録を消して、次の判定に備える。
	m_hitActors.Empty();
	m_bInvulnerable = false;
	m_bGuarding = false;
}

//経過時間に合わせて敵の状態を更新する。
void UCombat::TickComponent(float _deltaTime, ELevelTick _type, FActorComponentTickFunction* _function)
{
	Super::TickComponent(_deltaTime, _type, _function);

	//処理の対象となる敵。
	AEnemyCharacterBase* enemy = GetOwner() ? Cast<AEnemyCharacterBase>(GetOwner()) : nullptr;
	if (m_liftEnd > 0. && (!enemy || !enemy->IsCombatActive() || !m_liftTarget.IsValid() || GetWorld()->GetTimeSeconds() >= m_liftEnd)) { EndLift(); }

	//使用する行動の設定。
	const FEnemyActionData* action = enemy ? enemy->GetActiveAction() : nullptr;
	//狙っている相手。
	AActor* target = enemy ? enemy->m_combatTarget.Get() : nullptr;
	//相手の攻撃や回避の状態を確認する部品。
	const auto* playerCombat = IsValid(target) ? target->FindComponentByClass<UCombatComponent>() : nullptr;
	//相手が回避中か。
	const bool dodging = playerCombat && playerCombat->IsDodging();
	//狙う相手が変わったら、回避の記録を切り替える。
	if (m_dodgeTarget.Get() != target)
	{
		m_dodgeTarget = target; m_wasDodging = false; m_dodgeStart = -100.;
		m_lastTargetPos = IsValid(target) ? target->GetActorLocation() : FVector::ZeroVector;
	}
	if (dodging && !m_wasDodging)
	{
		m_dodgeStart = GetWorld()->GetTimeSeconds(); m_dodgePos = m_lastTargetPos;
		//キャラクターの当たり判定。
		const auto* capsule = target->FindComponentByClass<UCapsuleComponent>();
		m_dodgeRadius = capsule ? capsule->GetScaledCapsuleRadius() : 0.f;
		m_dodgeHalf = capsule ? capsule->GetScaledCapsuleHalfHeight() : 0.f;
	}
	m_wasDodging = dodging;
	if (IsValid(target)) { m_lastTargetPos = target->GetActorLocation(); }

	if(!enemy || !enemy->HasAuthority() || !m_bHitWindow || !action || !enemy->IsCombatActive()) { return; }

    //相手の体の当たり判定。
    const bool body = !action->m_hitBase.IsNone() && !action->m_hitTip.IsNone();
    //剣の根元の位置。
    const FName base = body ? action->m_hitBase : m_baseSocket;
    //剣先の位置。
    const FName tip = body ? action->m_hitTip : m_tipSocket;
    //対象のモデル。
    UMeshComponent* mesh = body ? enemy->GetMesh() : (m_useWeapon ? m_weapon.Get() : enemy->GetMesh());

	if (!IsValid(mesh) || mesh->GetOwner() != enemy || !mesh->DoesSocketExist(base) || !mesh->DoesSocketExist(tip))
	{
		m_hasPreviousHit = false;
		return;
	}

	//調べ始める位置。
	FVector start = mesh->GetSocketLocation(base);
	//調べ終える位置。
	FVector end = mesh->GetSocketLocation(tip);
	//判定の対象とする種類。
	FCollisionObjectQueryParams objects;
	objects.AddObjectTypesToQuery(ECC_Pawn);
	//ターゲットのオブジェクトタイプも対象にする（Player プリセット対応）
	if (const UPrimitiveComponent* targetRoot = IsValid(target) ? Cast<UPrimitiveComponent>(target->GetRootComponent()) : nullptr)
	{
		objects.AddObjectTypesToQuery(targetRoot->GetCollisionObjectType());
	}

	//生成や当たり判定に使う設定。
	FCollisionQueryParams params(SCENE_QUERY_STAT(BossSword), false, enemy);

	if (enemy->m_definition && enemy->m_definition->m_perfectDodge && m_dodgeRadius > 0.f
		&& enemy->IsDefenseInputValid(EDefenseResult::PerfectDodge, GetWorld()->GetTimeSeconds() - m_dodgeStart) && !enemy->IsAttackSlowed())
	{
		//高さの計算に使う値。
		const FVector height(0, 0, FMath::Max(0.f, m_dodgeHalf - m_dodgeRadius));
		FVector bladePoint, bodyPoint;
		FMath::SegmentDistToSegmentSafe(start, end, m_dodgePos - height, m_dodgePos + height, bladePoint, bodyPoint);
		//進行方向の障害物の情報。
		FHitResult wall;
		//攻撃が防がれたか。
		const bool blocked = GetWorld()->LineTraceSingleByChannel(wall, start, bodyPoint, ECC_Visibility, params) && wall.GetActor() != target;
		if (!blocked && FVector::DistSquared(bladePoint, bodyPoint) <= FMath::Square(m_radius + m_dodgeRadius)) { enemy->ApplyPerfectDodge(target); }
	}

	//命中した場所の一覧。
	TArray<FHitResult> hits;
	GetWorld()->SweepMultiByObjectType(hits, start, end, FQuat::Identity, objects, FCollisionShape::MakeSphere(m_radius), params);
	if (m_hasPreviousHit)
	{
		//軌道を調べる分割数。
		const int32 samples = FMath::Clamp(FMath::CeilToInt(FVector::Dist(start, end) / FMath::Max(2.f, m_radius * 2.f)), 1, CombatTuning::maxSweepSteps);
		for (int32 i = 0; i <= samples; ++i)
		{
			//二つの値の間の割合。
			const float alpha = float(i) / samples;
			//移動した剣の軌道の命中結果。
			TArray<FHitResult> motionHits;
			GetWorld()->SweepMultiByObjectType(motionHits, FMath::Lerp(m_previousBase, m_previousTip, alpha), FMath::Lerp(start, end, alpha), FQuat::Identity, objects, FCollisionShape::MakeSphere(m_radius), params);
			hits.Append(motionHits);
		}

	}

	m_previousTip = end;
	m_previousBase = start;
	m_hasPreviousHit = true;

	if (m_bDebug)
	{
		DrawDebugLine(GetWorld(), start, end, FColor::Red, false, CombatTuning::debugSize, 0, CombatTuning::debugSize);
	}

	//今回与えるダメージ。
	const float hitDamage = action->m_damage * m_hitScale;
	//強攻撃として扱うか。
	const bool heavy = action->m_kind == EEnemyAction::Heavy;
	//ダメージの種類。
	const TSubclassOf<UDamageType> damageType = action->m_unblockable ? (heavy ? UEBHeavyUnblockableDamageType::StaticClass() : UEBUnblockableDamageType::StaticClass()) : (heavy ? UEBHeavyDamageType::StaticClass() : UEBCombatDamageType::StaticClass());

	for (const FHitResult& hit : hits)
	{
		//攻撃が当たった相手。
		AActor* hitActor = hit.GetActor();
		if (!IsValid(hitActor) || m_hitActors.Contains(hitActor) || hitActor != enemy->m_combatTarget)
		{
			continue;
		}

		//障害物との接触結果。
		FHitResult wallHit;
		if (GetWorld()->LineTraceSingleByChannel(wallHit, start, hit.ImpactPoint, ECC_Visibility, params) && wallHit.GetActor() != hitActor)
		{
			continue;
		}

		m_hitActors.Add(hitActor);
		if (dodging || enemy->IsAttackSlowed()) { continue; }
		//防御の結果。
		const EDefenseResult response = enemy->ResolveTargetDefense(hitActor);

        if (action->m_special && action->m_unblockable && response == EDefenseResult::Guard)
        {
            enemy->TargetGuardBroken(hitActor);
        }

		if (response == EDefenseResult::PerfectDodge || (!action->m_unblockable && (response == EDefenseResult::Guard || response == EDefenseResult::Parry || response == EDefenseResult::Deflect)))
		{
			//回避には火花を出さず、剣で防がれた時だけ出す。
			if(response != EDefenseResult::PerfectDodge) { enemy->m_hitFx->PlayClash(hitActor); }

			enemy->AttackDefended_Implementation(hitActor, response);
			if (m_bDebug) { UE_LOG(LogTemp, Display, TEXT("BossCombat Defense result=%d"), int32(response)); }
			break;
		}

		//実際に与えたダメージ。
		const float actualDamage = UGameplayStatics::ApplyDamage(hitActor, hitDamage, enemy->GetController(), enemy, damageType);

		if (m_bDebug) { UE_LOG(LogTemp, Display, TEXT("BossCombat Hit action=%s target=%s damage=%.2f distance=%.1f"), *action->m_actionId.ToString(), *GetNameSafe(hitActor), actualDamage, FVector::Dist2D(enemy->GetActorLocation(), hitActor->GetActorLocation())); }

		if (actualDamage > 0.0f)
		{
			enemy->RecordAttackHit();
			if (action->m_launch && enemy->IsActing() && !enemy->WasAttackBlocked()) { BeginLift(Cast<ACharacter>(hitActor), *action); }

		}

		if (!enemy->IsActing() || !m_bHitWindow)
		{
			break;
		}
	}
}
