//SkillComponent.cpp

#include "Player/SkillComponent.h"
#include "Player/SkillDataAsset.h"
#include "Player/ActionCharacter.h"
#include "Player/ActionMovementComponent.h"
#include "Player/CombatComponent.h"
#include "Player/LockOnComponent.h"
#include "Combat/EnemyQuery.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Character.h"
#include "Animation/AnimInstance.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "GameFramework/RootMotionSource.h"
#include "Animation/AnimMontage.h"
#include "Engine/OverlapResult.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

static const FName SkillDashRootMotionName(TEXT("SkillDash"));

// =======================================================================
// コンポーネント
// =======================================================================
USkillComponent::USkillComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;   //ため中だけTickする
}

void USkillComponent::BeginPlay()
{
	Super::BeginPlay();

	//すり抜けを戻すときのために、元のPawnへの応答を覚えておく
	if (const AActionCharacter* Owner = GetOwnerCharacter())
	{
		m_SavedPawnResponse = Owner->GetCapsuleComponent()->GetCollisionResponseToChannel(ECC_Pawn);
	}
}

AActionCharacter* USkillComponent::GetOwnerCharacter() const
{
	return Cast<AActionCharacter>(GetOwner());
}

bool USkillComponent::CanStart() const
{
	if (!SkillData)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("SkillData is NULL"));
		return false;
	}
	return !IsBusy() && GetOwnerCharacter() != nullptr;
}

bool USkillComponent::TryConsume(int32 Cost)
{
	AActionCharacter* Owner = GetOwnerCharacter();
	if (!Owner || !Owner->TryConsumeEnergyStock(Cost))
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Orange,
			FString::Printf(TEXT("Not enough stock (need %d)"), Cost));
		return false;
	}
	return true;
}

//技の開始：攻撃を中断し、空中なら重力を止める
void USkillComponent::BeginSkill(ESkillState NewState)
{
	AActionCharacter* Owner = GetOwnerCharacter();
	m_State = NewState;

	if (UCombatComponent* Combat = Owner->FindComponentByClass<UCombatComponent>())
	{
		Combat->CancelAttack();
	}

	UActionMovementComponent* Move = Cast<UActionMovementComponent>(Owner->GetCharacterMovement());
	if (Move && Move->IsFalling() && NewState != ESkillState::Charging)
	{
		Move->SetAirHover(true);
		m_bHovering = true;
	}
	else if (Move)
	{
		Move->StopMovementImmediately();
	}
}

void USkillComponent::FinishSkill(float RecoveryTime)
{
	GetWorld()->GetTimerManager().SetTimer(m_EndTimer, this, &USkillComponent::EndSkill,
		FMath::Max(RecoveryTime, 0.01f), false);
}

void USkillComponent::EndSkill()
{
	GetWorld()->GetTimerManager().ClearTimer(m_StepTimer);
	GetWorld()->GetTimerManager().ClearTimer(m_EndTimer);
	SetComponentTickEnabled(false);
	SetPassThroughEnemies(false);
	SetMoveSpeedRate(1.f);
	FreezeCamera(false);
	ClearChargeEffect();

	if (m_bHovering)
	{
		if (AActionCharacter* Owner = GetOwnerCharacter())
		{
			if (UActionMovementComponent* Move = Cast<UActionMovementComponent>(Owner->GetCharacterMovement()))
			{
				Move->SetAirHover(false);
			}
		}
		m_bHovering = false;
	}

	if (m_bDebugDraw && m_State != ESkillState::None)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Green, TEXT("Skill End"));
	}
	m_State = ESkillState::None;
	m_ChargeLevel = 0;
	m_PendingTargets.Reset();
}

// -----------------------------------------------------------------------
// 共通の小道具
// -----------------------------------------------------------------------

//狙う敵：ロックオン中ならその敵、いなければ範囲内で「向きたい方向」に近い敵
AActor* USkillComponent::FindTarget(float Radius, const FVector& PreferDir) const
{
	AActionCharacter* Owner = GetOwnerCharacter();
	if (Owner->LockOnComponent && Owner->LockOnComponent->IsLockedOn())
	{
		AActor* Locked = Owner->LockOnComponent->GetTarget();
		if (EnemyQuery::IsAliveEnemy(Locked)) return Locked;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SkillFindTarget), false, Owner);
	GetWorld()->OverlapMultiByObjectType(Overlaps, Owner->GetActorLocation(), FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(Radius), Params);

	AActor* Best = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	for (const FOverlapResult& R : Overlaps)
	{
		AActor* Enemy = R.GetActor();
		if (!EnemyQuery::IsAliveEnemy(Enemy)) continue;

		FVector To = Enemy->GetActorLocation() - Owner->GetActorLocation();
		To.Z = 0.f;
		const float Dist = To.Size();
		const float Dot = Dist > 1.f ? FVector::DotProduct(PreferDir, To / Dist) : 1.f;
		//正面に近いほど・近いほど優先
		const float Score = (1.f - Dot) * 2.f + Dist / Radius;
		if (Score < BestScore)
		{
			BestScore = Score;
			Best = Enemy;
		}
	}
	return Best;
}

//スティック入力の方向（なければキャラの正面）
FVector USkillComponent::GetInputOrForward() const
{
	const AActionCharacter* Owner = GetOwnerCharacter();
	const FVector Input = Owner->GetLastMovementInputVector().GetSafeNormal2D();
	return Input.IsNearlyZero() ? Owner->GetActorForwardVector().GetSafeNormal2D() : Input;
}

//指定位置へ一定時間で移動（攻撃の踏み込みと同じ仕組み）
void USkillComponent::DashTo(const FVector& Location, float Duration)
{
	AActionCharacter* Owner = GetOwnerCharacter();
	UCharacterMovementComponent* Move = Owner->GetCharacterMovement();

	TSharedPtr<FRootMotionSource_MoveToForce> Dash = MakeShared<FRootMotionSource_MoveToForce>();
	Dash->InstanceName = SkillDashRootMotionName;
	Dash->AccumulateMode = ERootMotionAccumulateMode::Override;
	Dash->Priority = 10;
	Dash->StartLocation = Owner->GetActorLocation();
	Dash->TargetLocation = Location;
	Dash->Duration = Duration;
	Dash->bRestrictSpeedToExpected = true;
	Dash->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
	Dash->FinishVelocityParams.SetVelocity = FVector::ZeroVector;

	Move->RemoveRootMotionSource(SkillDashRootMotionName);
	Move->ApplyRootMotionSource(Dash);
}

//敵（Pawn）をすり抜けるかどうか
void USkillComponent::SetPassThroughEnemies(bool bEnable)
{
	if (AActionCharacter* Owner = GetOwnerCharacter())
	{
		Owner->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn,
			bEnable ? ECR_Ignore : m_SavedPawnResponse.GetValue());
	}
}

void USkillComponent::SetMoveSpeedRate(float Rate)
{
	if (AActionCharacter* Owner = GetOwnerCharacter())
	{
		if (UActionMovementComponent* Move = Cast<UActionMovementComponent>(Owner->GetCharacterMovement()))
		{
			Move->SetSpeedMultiplier(Rate);
		}
	}
}

//カメラ（SpringArm）をその場に止める／元に戻す
void USkillComponent::FreezeCamera(bool bFreeze)
{
	AActionCharacter* Owner = GetOwnerCharacter();
	if (!Owner) return;

	if (bFreeze)
	{
		USpringArmComponent* Boom = Owner->FindComponentByClass<USpringArmComponent>();
		if (!Boom || m_FrozenBoom.IsValid()) return;

		m_FrozenBoom = Boom;
		m_BoomRelativeLocation = Boom->GetRelativeLocation();
		const FVector WorldLocation = Boom->GetComponentLocation();
		//親（キャラ）について行かないようにして、今のワールド位置に置く
		//（SetWorldLocation だと移動量が0とみなされて位置が更新されず、原点に飛ぶので直接入れる）
		Boom->SetUsingAbsoluteLocation(true);
		Boom->SetRelativeLocation_Direct(WorldLocation);
		Boom->UpdateComponentToWorld();
	}
	else if (USpringArmComponent* Boom = m_FrozenBoom.Get())
	{
		Boom->SetUsingAbsoluteLocation(false);
		Boom->SetRelativeLocation_Direct(m_BoomRelativeLocation);
		Boom->UpdateComponentToWorld();
		m_FrozenBoom.Reset();
	}
}

void USkillComponent::PlayMontage(UAnimMontage* Montage)
{
	if (Montage)
	{
		GetOwnerCharacter()->PlayAnimMontage(Montage);
	}
}

//範囲内の敵全員にダメージ。当たった数を返す
int32 USkillComponent::DamageInSphere(const FVector& Center, float Radius, float Damage)
{
	AActionCharacter* Owner = GetOwnerCharacter();

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SkillDamage), false, Owner);
	GetWorld()->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(Radius), Params);

	TSet<AActor*> Done;
	for (const FOverlapResult& R : Overlaps)
	{
		AActor* Enemy = R.GetActor();
		if (!EnemyQuery::IsAliveEnemy(Enemy) || Done.Contains(Enemy)) continue;
		Done.Add(Enemy);
		UGameplayStatics::ApplyDamage(Enemy, Damage, Owner->GetController(), Owner, nullptr);
	}

	if (m_bDebugDraw)
	{
		DrawDebugSphere(GetWorld(), Center, Radius, 16, Done.Num() > 0 ? FColor::Red : FColor::Yellow, false, 0.6f);
	}
	return Done.Num();
}

//通り道にいる敵を集める（ダメージはまだ与えない）
void USkillComponent::CollectAlongPath(const FVector& Start, const FVector& End, float Radius)
{
	AActionCharacter* Owner = GetOwnerCharacter();

	TArray<FHitResult> Hits;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SkillDashPath), false, Owner);
	GetWorld()->SweepMultiByObjectType(Hits, Start, End, FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(Radius), Params);

	m_PendingTargets.Reset();
	for (const FHitResult& Hit : Hits)
	{
		AActor* Enemy = Hit.GetActor();
		if (EnemyQuery::IsAliveEnemy(Enemy) && !m_PendingTargets.Contains(Enemy))
		{
			m_PendingTargets.Add(Enemy);
		}
	}

	if (m_bDebugDraw)
	{
		const FVector Center = (Start + End) * 0.5f;
		const float Half = FVector::Dist(Start, End) * 0.5f + Radius;
		const FQuat Rot = FRotationMatrix::MakeFromZ(End - Start).ToQuat();
		DrawDebugCapsule(GetWorld(), Center, Half, Radius, Rot, FColor::Orange, false, 1.f);
	}
}

// =======================================================================
// 1ゲージ技：一閃
// 構え → 一瞬で駆け抜ける（敵をすり抜ける）→ 少し遅れて通り道の敵が斬れる
// =======================================================================
bool USkillComponent::UseDashSlash()
{
	if (!CanStart()) return false;
	const FDashSlashData& D = SkillData->DashSlash;
	if (!TryConsume(D.m_Cost)) return false;

	BeginSkill(ESkillState::DashSlash);

	//構えの間に狙う方向を向いておく
	AActionCharacter* Owner = GetOwnerCharacter();
	const FVector Dir = GetInputOrForward();
	if (AActor* Target = FindTarget(D.m_SearchRadius, Dir))
	{
		FVector To = Target->GetActorLocation() - Owner->GetActorLocation();
		To.Z = 0.f;
		if (!To.IsNearlyZero()) Owner->SetActorRotation(To.Rotation());
	}
	else
	{
		Owner->SetActorRotation(Dir.Rotation());
	}
	PlayMontage(D.Montage);

	GetWorld()->GetTimerManager().SetTimer(m_StepTimer, this, &USkillComponent::DashSlash_Go,
		FMath::Max(D.m_WindupTime, 0.01f), false);
	return true;
}

void USkillComponent::DashSlash_Go()
{
	const FDashSlashData& D = SkillData->DashSlash;
	AActionCharacter* Owner = GetOwnerCharacter();
	const FVector Start = Owner->GetActorLocation();
	const FVector Dir = Owner->GetActorForwardVector().GetSafeNormal2D();

	//敵がいれば敵を通り越すまで、いなければ決まった距離
	float Distance = D.m_Distance;
	if (AActor* Target = FindTarget(D.m_SearchRadius, Dir))
	{
		Distance = FVector::Dist2D(Start, Target->GetActorLocation()) + D.m_Overshoot;
	}
	Distance = FMath::Min(Distance, D.m_MaxDistance);
	FVector End = Start + Dir * Distance;

	//壁にめり込まないよう、地形の手前で止める（敵は無視）
	const UCapsuleComponent* Capsule = Owner->GetCapsuleComponent();
	FHitResult Wall;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SkillDashWall), false, Owner);
	if (GetWorld()->SweepSingleByObjectType(Wall, Start, End, FQuat::Identity,
		FCollisionObjectQueryParams(ECC_WorldStatic),
		FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight() * 0.8f),
		Params))
	{
		End = Wall.Location;
	}

	CollectAlongPath(Start, End, D.m_HitRadius);
	SetPassThroughEnemies(true);
	DashTo(End, D.m_DashTime);

	GetWorld()->GetTimerManager().SetTimer(m_StepTimer, this, &USkillComponent::DashSlash_ApplyDamage,
		D.m_DashTime + D.m_DamageDelay, false);
}

void USkillComponent::DashSlash_ApplyDamage()
{
	const FDashSlashData& D = SkillData->DashSlash;
	AActionCharacter* Owner = GetOwnerCharacter();
	SetPassThroughEnemies(false);

	for (const TWeakObjectPtr<AActor>& Enemy : m_PendingTargets)
	{
		if (EnemyQuery::IsAliveEnemy(Enemy.Get()))
		{
			UGameplayStatics::ApplyDamage(Enemy.Get(), D.m_Damage, Owner->GetController(), Owner, nullptr);
		}
	}
	if (m_bDebugDraw)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow,
			FString::Printf(TEXT("DashSlash hit %d"), m_PendingTargets.Num()));
	}

	FinishSkill(D.m_RecoveryTime);
}

// =======================================================================
// 3ゲージ技：テンペスト
// 敵のまわりを、中心をまたぐように何度も飛び交いながら斬る → 最後に締めの一撃
// =======================================================================
bool USkillComponent::UseTempest()
{
	if (!CanStart()) return false;
	const FTempestData& D = SkillData->Tempest;
	if (!TryConsume(D.m_Cost)) return false;

	BeginSkill(ESkillState::Tempest);

	AActionCharacter* Owner = GetOwnerCharacter();
	const FVector Dir = GetInputOrForward();
	//中心：敵を狙う設定なら近くの敵、そうでなければ発動した場所
	AActor* Target = D.m_bCenterOnTarget ? FindTarget(D.m_SearchRadius, Dir) : nullptr;
	m_TempestTarget = Target;
	m_TempestCenter = Target ? Target->GetActorLocation() : Owner->GetActorLocation();

	if (D.m_bFreezeCamera) FreezeCamera(true);

	//今いる位置の角度から始める
	const FVector FromCenter = Owner->GetActorLocation() - m_TempestCenter;
	m_TempestAngle = FMath::RadiansToDegrees(FMath::Atan2(FromCenter.Y, FromCenter.X));
	m_TempestStep = 0;

	SetPassThroughEnemies(true);
	Tempest_Step();
	return true;
}

void USkillComponent::Tempest_Step()
{
	const FTempestData& D = SkillData->Tempest;
	AActionCharacter* Owner = GetOwnerCharacter();

	//中心の敵が動いたらついていく
	if (m_TempestTarget.IsValid())
	{
		m_TempestCenter = m_TempestTarget->GetActorLocation();
	}
	m_TempestCenter.Z = Owner->GetActorLocation().Z;

	if (m_TempestStep < D.m_HitCount)
	{
		//反対側の少しずれた位置へ（中心をまたいで斬り抜ける軌道になる）
		m_TempestAngle += 180.f - 40.f + FMath::FRandRange(-25.f, 25.f);
		const float Rad = FMath::DegreesToRadians(m_TempestAngle);
		const FVector Next = m_TempestCenter + FVector(FMath::Cos(Rad), FMath::Sin(Rad), 0.f) * D.m_OrbitRadius;

		//進む方向を向いて飛ぶ
		const FVector Move = (Next - Owner->GetActorLocation()).GetSafeNormal2D();
		if (!Move.IsNearlyZero()) Owner->SetActorRotation(Move.Rotation());
		DashTo(Next, D.m_DashTime);

		if (D.SlashMontages.Num() > 0)
		{
			PlayMontage(D.SlashMontages[m_TempestStep % D.SlashMontages.Num()]);
		}
		DamageInSphere(m_TempestCenter, D.m_HitRadius, D.m_Damage);

		++m_TempestStep;
		GetWorld()->GetTimerManager().SetTimer(m_StepTimer, this, &USkillComponent::Tempest_Step,
			D.m_Interval, false);
		return;
	}

	//締め：発動した場所が中心なら、その場所に戻ってから大きく斬る
	//（カメラ固定を解いたときに画面が飛ばないように）
	if (!m_TempestTarget.IsValid())
	{
		DashTo(m_TempestCenter, D.m_DashTime);
	}
	const FVector ToCenter = (m_TempestCenter - Owner->GetActorLocation()).GetSafeNormal2D();
	if (!ToCenter.IsNearlyZero()) Owner->SetActorRotation(ToCenter.Rotation());
	PlayMontage(D.FinishMontage);
	DamageInSphere(m_TempestCenter, D.m_FinishRadius, D.m_FinishDamage);

	SetPassThroughEnemies(false);
	FinishSkill(D.m_RecoveryTime);
}

// =======================================================================
// 最終奥義：ため斬り（エクスカリバー型）
// 押している間ためる（段階 = ためた時間）→ 離すと段階ぶんゲージを消費し、
// 少し遅れて（振り下ろしの瞬間に）向いている方向へV字の光の斬撃を放つ
// =======================================================================
bool USkillComponent::StartCharge()
{
	if (!CanStart()) return false;
	AActionCharacter* Owner = GetOwnerCharacter();
	if (Owner->GetEnergyStock() < 1)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Orange, TEXT("Not enough stock (need 1)"));
		return false;
	}

	BeginSkill(ESkillState::Charging);
	m_ChargeTime = 0.f;
	m_ChargeLevel = 0;
	SetMoveSpeedRate(SkillData->Ultimate.m_ChargeMoveSpeedRate);
	PlayMontage(SkillData->Ultimate.ChargeMontage);
	SetComponentTickEnabled(true);
	return true;
}

void USkillComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!IsCharging()) return;

	const FUltimateData& U = SkillData->Ultimate;
	m_ChargeTime += DeltaTime;

	//上限は「段階データの数」と「今持っているストック」の小さい方
	//（ため中に分身がエネルギーを稼げば、さらに上までためられる）
	const int32 MaxLevel = FMath::Min(U.Levels.Num(), GetOwnerCharacter()->GetEnergyStock());
	const int32 NewLevel = FMath::Min(FMath::FloorToInt(m_ChargeTime / U.m_ChargeStepTime), MaxLevel);

	if (NewLevel != m_ChargeLevel)
	{
		m_ChargeLevel = NewLevel;
		UpdateChargeEffect();
		OnChargeLevelChanged.Broadcast(m_ChargeLevel);
		GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Cyan,
			FString::Printf(TEXT("Charge Lv %d"), m_ChargeLevel));
	}
}

//剣にまとわせるエフェクトを、今の段階のものに差し替える
void USkillComponent::UpdateChargeEffect()
{
	ClearChargeEffect();
	if (m_ChargeLevel <= 0) return;

	const FUltimateData& U = SkillData->Ultimate;
	UNiagaraSystem* Effect = U.Levels[m_ChargeLevel - 1].ChargeEffect;
	UStaticMeshComponent* Weapon = GetOwnerCharacter()->GetWeaponMesh();
	if (!Effect || !Weapon) return;

	m_ChargeFX = UNiagaraFunctionLibrary::SpawnSystemAttached(Effect, Weapon, U.m_ChargeEffectSocket,
		FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true);
}

void USkillComponent::ClearChargeEffect()
{
	if (UNiagaraComponent* FX = m_ChargeFX.Get())
	{
		FX->DestroyComponent();
	}
	m_ChargeFX.Reset();
}

void USkillComponent::ReleaseCharge()
{
	if (!IsCharging()) return;

	const FUltimateData& U = SkillData->Ultimate;
	AActionCharacter* Owner = GetOwnerCharacter();
	SetComponentTickEnabled(false);
	SetMoveSpeedRate(1.f);
	ClearChargeEffect();

	//1段もたまっていなければ何もせず終わり（ゲージは減らない）
	if (m_ChargeLevel <= 0 || !TryConsume(m_ChargeLevel))
	{
		if (U.ChargeMontage) Owner->StopAnimMontage(U.ChargeMontage);
		EndSkill();
		return;
	}

	const FUltimateLevelData& L = U.Levels[m_ChargeLevel - 1];
	m_UltLevel = m_ChargeLevel;
	m_State = ESkillState::Ultimate;
	Owner->GetCharacterMovement()->StopMovementImmediately();

	//ロックオン中なら敵の方へ向き直ってから放つ
	if (Owner->LockOnComponent && Owner->LockOnComponent->IsLockedOn())
	{
		if (const AActor* Target = Owner->LockOnComponent->GetTarget())
		{
			FVector To = Target->GetActorLocation() - Owner->GetActorLocation();
			To.Z = 0.f;
			if (!To.IsNearlyZero()) Owner->SetActorRotation(To.Rotation());
		}
	}

	//モーション：段階専用があればそれ、なければため用モンタージュの振り下ろしへ
	UAnimInstance* Anim = Owner->GetMesh()->GetAnimInstance();
	if (L.Montage)
	{
		if (U.ChargeMontage) Owner->StopAnimMontage(U.ChargeMontage);
		PlayMontage(L.Montage);
	}
	else if (Anim && U.ChargeMontage && Anim->Montage_IsPlaying(U.ChargeMontage)
		&& U.ChargeMontage->GetSectionIndex(U.m_ReleaseSectionName) != INDEX_NONE)
	{
		Anim->Montage_JumpToSection(U.m_ReleaseSectionName, U.ChargeMontage);
	}
	else if (U.ChargeMontage)
	{
		Owner->StopAnimMontage(U.ChargeMontage);
	}

	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Magenta,
		FString::Printf(TEXT("ULTIMATE Lv %d"), m_UltLevel));

	//振り下ろしの瞬間に合わせて斬撃を出す
	GetWorld()->GetTimerManager().SetTimer(m_StepTimer, this, &USkillComponent::Ultimate_Fire,
		FMath::Max(U.m_ReleaseDelay, 0.01f), false);
}

void USkillComponent::Ultimate_Fire()
{
	const FUltimateLevelData& L = SkillData->Ultimate.Levels[m_UltLevel - 1];
	AActionCharacter* Owner = GetOwnerCharacter();

	//放った瞬間の位置と向きで固定（多段ヒットの間に向きが変わっても斬撃はまっすぐ）
	m_UltOrigin = Owner->GetActorLocation();
	m_UltDir = Owner->GetActorForwardVector().GetSafeNormal2D();

	if (L.BeamEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, L.BeamEffect, m_UltOrigin, m_UltDir.Rotation());
	}

	m_UltHitsLeft = L.m_HitCount;
	Ultimate_Hit();
}

void USkillComponent::Ultimate_Hit()
{
	const FUltimateLevelData& L = SkillData->Ultimate.Levels[m_UltLevel - 1];
	DamageInWedge(m_UltOrigin, m_UltDir, L);

	if (--m_UltHitsLeft > 0)
	{
		GetWorld()->GetTimerManager().SetTimer(m_StepTimer, this, &USkillComponent::Ultimate_Hit,
			L.m_HitInterval, false);
	}
	else
	{
		FinishSkill(L.m_RecoveryTime);
	}
}

//前方にV字（先に行くほど広がる帯）の範囲にいる敵にダメージ
int32 USkillComponent::DamageInWedge(const FVector& Origin, const FVector& Dir, const FUltimateLevelData& L)
{
	AActionCharacter* Owner = GetOwnerCharacter();
	const float Tan = FMath::Tan(FMath::DegreesToRadians(L.m_HalfAngle));

	//まず届く範囲の敵をざっくり集める
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SkillWedge), false, Owner);
	GetWorld()->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(L.m_Length + 200.f), Params);

	TSet<AActor*> Done;
	for (const FOverlapResult& R : Overlaps)
	{
		AActor* Enemy = R.GetActor();
		if (!EnemyQuery::IsAliveEnemy(Enemy) || Done.Contains(Enemy)) continue;

		FVector To = Enemy->GetActorLocation() - Origin;
		if (FMath::Abs(To.Z) > L.m_HalfHeight) continue;
		To.Z = 0.f;

		//前にどれだけ進んだ位置か / 中心線から横にどれだけずれているか
		const float Forward = FVector::DotProduct(To, Dir);
		if (Forward < 0.f || Forward > L.m_Length) continue;
		const float Side = FMath::Abs(FVector::CrossProduct(Dir, To).Z);

		//その位置での帯の幅（敵の太さぶん少し甘くする）
		float EnemyRadius = 0.f;
		if (const ACharacter* EnemyChara = Cast<ACharacter>(Enemy))
		{
			EnemyRadius = EnemyChara->GetCapsuleComponent()->GetScaledCapsuleRadius();
		}
		if (Side > L.m_BaseWidth + Forward * Tan + EnemyRadius) continue;

		Done.Add(Enemy);
		UGameplayStatics::ApplyDamage(Enemy, L.m_Damage, Owner->GetController(), Owner, nullptr);
	}

	if (m_bDebugDraw)
	{
		const FVector Right = FVector::CrossProduct(FVector::UpVector, Dir);
		const float EndWidth = L.m_BaseWidth + L.m_Length * Tan;
		const FVector NearL = Origin - Right * L.m_BaseWidth;
		const FVector NearR = Origin + Right * L.m_BaseWidth;
		const FVector FarL = Origin + Dir * L.m_Length - Right * EndWidth;
		const FVector FarR = Origin + Dir * L.m_Length + Right * EndWidth;
		const FColor Color = Done.Num() > 0 ? FColor::Red : FColor::Yellow;
		DrawDebugLine(GetWorld(), NearL, FarL, Color, false, 1.f, 0, 4.f);
		DrawDebugLine(GetWorld(), NearR, FarR, Color, false, 1.f, 0, 4.f);
		DrawDebugLine(GetWorld(), FarL, FarR, Color, false, 1.f, 0, 4.f);
		DrawDebugLine(GetWorld(), NearL, NearR, Color, false, 1.f, 0, 4.f);
	}
	return Done.Num();
}

// -----------------------------------------------------------------------
// 被弾：ため中だけキャンセル（ほかの技の最中は無敵なのでここには来ない）
// -----------------------------------------------------------------------
void USkillComponent::Interrupt()
{
	if (!IsCharging()) return;

	if (SkillData && SkillData->Ultimate.ChargeMontage)
	{
		GetOwnerCharacter()->StopAnimMontage(SkillData->Ultimate.ChargeMontage);
	}
	GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Red, TEXT("Charge Interrupted"));
	EndSkill();
}
