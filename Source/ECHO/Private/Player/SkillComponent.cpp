//SkillComponent.cpp

#include "Player/SkillComponent.h"
#include "Player/SkillDataAsset.h"
#include "Player/ActionCharacter.h"
#include "Player/ActionMovementComponent.h"
#include "Player/CombatComponent.h"
#include "Player/LockOnComponent.h"
#include "Combat/EnemyQuery.h"
#include "Components/CapsuleComponent.h"
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
	AActor* Target = FindTarget(D.m_SearchRadius, Dir);
	m_TempestTarget = Target;
	m_TempestCenter = Target ? Target->GetActorLocation() : Owner->GetActorLocation() + Dir * D.m_OrbitRadius;

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

	//締め：中心を向いて大きく斬る
	const FVector ToCenter = (m_TempestCenter - Owner->GetActorLocation()).GetSafeNormal2D();
	if (!ToCenter.IsNearlyZero()) Owner->SetActorRotation(ToCenter.Rotation());
	PlayMontage(D.FinishMontage);
	DamageInSphere(m_TempestCenter, D.m_FinishRadius, D.m_FinishDamage);

	SetPassThroughEnemies(false);
	FinishSkill(D.m_RecoveryTime);
}

// =======================================================================
// 最終奥義：ため斬り
// 押している間ためる（段階 = ためた時間）→ 離すと段階ぶんゲージを消費して放つ
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
		OnChargeLevelChanged.Broadcast(m_ChargeLevel);
		GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Cyan,
			FString::Printf(TEXT("Charge Lv %d"), m_ChargeLevel));
	}
}

void USkillComponent::ReleaseCharge()
{
	if (!IsCharging()) return;

	AActionCharacter* Owner = GetOwnerCharacter();
	SetComponentTickEnabled(false);
	SetMoveSpeedRate(1.f);
	if (SkillData->Ultimate.ChargeMontage) Owner->StopAnimMontage(SkillData->Ultimate.ChargeMontage);

	//1段もたまっていなければ何もせず終わり（ゲージは減らない）
	if (m_ChargeLevel <= 0 || !TryConsume(m_ChargeLevel))
	{
		EndSkill();
		return;
	}

	const FUltimateLevelData& L = SkillData->Ultimate.Levels[m_ChargeLevel - 1];
	m_UltLevel = m_ChargeLevel;
	m_UltHitsLeft = L.m_HitCount;
	m_State = ESkillState::Ultimate;
	Owner->GetCharacterMovement()->StopMovementImmediately();

	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Magenta,
		FString::Printf(TEXT("ULTIMATE Lv %d"), m_UltLevel));

	PlayMontage(L.Montage);
	Ultimate_Hit();
}

void USkillComponent::Ultimate_Hit()
{
	const FUltimateLevelData& L = SkillData->Ultimate.Levels[m_UltLevel - 1];
	DamageInSphere(GetOwnerCharacter()->GetActorLocation(), L.m_Radius, L.m_Damage);

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
