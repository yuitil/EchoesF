#include "Enemy/Combat/Combat.h"
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

UCombat::UCombat()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

bool UCombat::SetWeaponMesh(UMeshComponent* _mesh, FName _base, FName _tip)
{
	EndHitWindow();
	m_weapon = nullptr;
	m_useWeapon = true;
	if (!IsValid(_mesh) || _mesh->GetOwner() != GetOwner() || !_mesh->DoesSocketExist(_base) || !_mesh->DoesSocketExist(_tip))
	{
		return false;
	}

	m_weapon = _mesh;
	m_baseSocket = _base;
	m_tipSocket = _tip;
	return true;
}


void UCombat::BeginLift(ACharacter* _target, const FEnemyActionData& _action)
{
	auto* enemy = Cast<AEnemyCharacterBase>(GetOwner());
	if (!enemy || !enemy->CanLiftTarget(_target)) { return; }
	EndLift();
	m_liftTarget = _target;
	m_targetGravity = _target->GetCharacterMovement()->GravityScale;
	m_selfGravity = enemy->GetCharacterMovement()->GravityScale;
	m_liftGravity = FMath::Max(.1f, _action.m_liftGravity);
	m_liftEnd = GetWorld()->GetTimeSeconds() + FMath::Clamp(_action.m_liftTime, .1f, 3.f);
	_target->GetCharacterMovement()->GravityScale = m_liftGravity;
	enemy->GetCharacterMovement()->GravityScale = m_liftGravity;
	_target->LaunchCharacter(FVector(0, 0, _action.m_liftSpeed), true, true);
	enemy->LaunchCharacter(FVector(0, 0, _action.m_liftSpeed), true, true);
}

void UCombat::EndLift()
{
	if (m_liftEnd <= 0.) { return; }
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

void UCombat::EndPlay(const EEndPlayReason::Type _reason)
{
	EndLift();
	Super::EndPlay(_reason);
}

void UCombat::BeginHitWindow(float _damageScale)
{
	m_hitScale = FMath::Max(0.0f, _damageScale);
	m_hitActors.Empty();
	m_hasPreviousHit = false;
	m_bHitWindow = true;
	
	if (m_bDebug) { UE_LOG(LogTemp, Display, TEXT("BossCombat WindowOpen %s"), *GetNameSafe(GetOwner())); }
	
}

void UCombat::EndHitWindow()
{
	m_bHitWindow = false;
	m_hasPreviousHit = false;
}

void UCombat::ResetWindows()
{
	EndHitWindow();
	m_hitActors.Empty();
	m_bInvulnerable = false;
	m_bGuarding = false;
}

void UCombat::TickComponent(float _deltaTime, ELevelTick _type, FActorComponentTickFunction* _function)
{
	Super::TickComponent(_deltaTime, _type, _function);

	AEnemyCharacterBase* enemy = GetOwner() ? Cast<AEnemyCharacterBase>(GetOwner()) : nullptr;
	if (m_liftEnd > 0. && (!enemy || !enemy->IsCombatActive() || !m_liftTarget.IsValid() || GetWorld()->GetTimeSeconds() >= m_liftEnd)) { EndLift(); }
	
	const FEnemyActionData* action = enemy ? enemy->GetActiveAction() : nullptr;
	AActor* target = enemy ? enemy->m_combatTarget.Get() : nullptr;
	const auto* playerCombat = IsValid(target) ? target->FindComponentByClass<UCombatComponent>() : nullptr;
	const bool dodging = playerCombat && playerCombat->IsDodging();
	if (m_dodgeTarget.Get() != target)
	{
		m_dodgeTarget = target; m_wasDodging = false; m_dodgeStart = -100.;
		m_lastTargetPos = IsValid(target) ? target->GetActorLocation() : FVector::ZeroVector;
	}
	if (dodging && !m_wasDodging)
	{
		m_dodgeStart = GetWorld()->GetTimeSeconds(); m_dodgePos = m_lastTargetPos;
		const auto* capsule = target->FindComponentByClass<UCapsuleComponent>();
		m_dodgeRadius = capsule ? capsule->GetScaledCapsuleRadius() : 0.f;
		m_dodgeHalf = capsule ? capsule->GetScaledCapsuleHalfHeight() : 0.f;
	}
	m_wasDodging = dodging;
	if (IsValid(target)) { m_lastTargetPos = target->GetActorLocation(); }
	
	if (!enemy || !enemy->HasAuthority() || !m_bHitWindow || !action || !enemy->IsCombatActive())
	{
		return;
	}

	//
    const bool body = !action->m_hitBase.IsNone() && !action->m_hitTip.IsNone();
    const FName base = body ? action->m_hitBase : m_baseSocket;
    const FName tip = body ? action->m_hitTip : m_tipSocket;
    UMeshComponent* mesh = body ? enemy->GetMesh() : (m_useWeapon ? m_weapon.Get() : enemy->GetMesh());
    //
	if (!IsValid(mesh) || mesh->GetOwner() != enemy || !mesh->DoesSocketExist(base) || !mesh->DoesSocketExist(tip))
	{
		m_hasPreviousHit = false;
		return;
	}
	
	FVector start = mesh->GetSocketLocation(base);
	FVector end = mesh->GetSocketLocation(tip);
	FCollisionObjectQueryParams objects;
	objects.AddObjectTypesToQuery(ECC_Pawn);
	//ターゲットのオブジェクトタイプも対象にする（Player プリセット対応）
	if (const UPrimitiveComponent* targetRoot = IsValid(target) ? Cast<UPrimitiveComponent>(target->GetRootComponent()) : nullptr)
	{
		objects.AddObjectTypesToQuery(targetRoot->GetCollisionObjectType());
	}

	FCollisionQueryParams params(SCENE_QUERY_STAT(BossSword), false, enemy);
	
	if (enemy->m_definition && enemy->m_definition->m_perfectDodge && m_dodgeRadius > 0.f
		&& enemy->IsDefenseInputValid(EDefenseResult::PerfectDodge, GetWorld()->GetTimeSeconds() - m_dodgeStart) && !enemy->IsAttackSlowed())
	{
		const FVector height(0, 0, FMath::Max(0.f, m_dodgeHalf - m_dodgeRadius));
		FVector bladePoint, bodyPoint;
		FMath::SegmentDistToSegmentSafe(start, end, m_dodgePos - height, m_dodgePos + height, bladePoint, bodyPoint);
		FHitResult wall;
		const bool blocked = GetWorld()->LineTraceSingleByChannel(wall, start, bodyPoint, ECC_Visibility, params) && wall.GetActor() != target;
		if (!blocked && FVector::DistSquared(bladePoint, bodyPoint) <= FMath::Square(m_radius + m_dodgeRadius)) { enemy->ApplyPerfectDodge(target); }
	}
	
	TArray<FHitResult> hits;
	GetWorld()->SweepMultiByObjectType(hits, start, end, FQuat::Identity, objects, FCollisionShape::MakeSphere(m_radius), params);
	if (m_hasPreviousHit)
	{
		const int32 samples = FMath::Clamp(FMath::CeilToInt(FVector::Dist(start, end) / FMath::Max(2.f, m_radius * 2.f)), 1, 24);
		for (int32 i = 0; i <= samples; ++i)
		{
			const float alpha = float(i) / samples;
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
		DrawDebugLine(GetWorld(), start, end, FColor::Red, false, 5.0f, 0, 5.0f);
	}

	const float hitDamage = action->m_damage * m_hitScale;
	const bool heavy = action->m_kind == EEnemyAction::Heavy;
	const TSubclassOf<UDamageType> damageType = action->m_unblockable ? (heavy ? UEBHeavyUnblockableDamageType::StaticClass() : UEBUnblockableDamageType::StaticClass()) : (heavy ? UEBHeavyDamageType::StaticClass() : UEBCombatDamageType::StaticClass());

	for (const FHitResult& hit : hits)
	{
		AActor* hitActor = hit.GetActor();
		if (!IsValid(hitActor) || m_hitActors.Contains(hitActor) || hitActor != enemy->m_combatTarget)
		{
			continue;
		}

		FHitResult wallHit;
		if (GetWorld()->LineTraceSingleByChannel(wallHit, start, hit.ImpactPoint, ECC_Visibility, params) && wallHit.GetActor() != hitActor)
		{
			continue;
		}

		m_hitActors.Add(hitActor);
		if (dodging || enemy->IsAttackSlowed()) { continue; }
		const EDefenseResult response = enemy->ResolveTargetDefense(hitActor);
        //
        if (action->m_special && action->m_unblockable && response == EDefenseResult::Guard)
        {
            enemy->TargetGuardBroken(hitActor);
        }
        //
		if (response == EDefenseResult::PerfectDodge || (!action->m_unblockable && (response == EDefenseResult::Guard || response == EDefenseResult::Parry || response == EDefenseResult::Deflect)))
		{
			enemy->AttackDefended_Implementation(hitActor, response);
			if (m_bDebug) { UE_LOG(LogTemp, Display, TEXT("BossCombat Defense result=%d"), int32(response)); }
			break;
		}
		
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
