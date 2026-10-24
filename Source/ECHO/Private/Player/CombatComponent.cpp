// CombatComponent.cpp

#include "Player/CombatComponent.h"
#include "Player/ActionMovementComponent.h"
#include "Enemy/EnemyChara.h"
#include "Player/LockOnComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "DrawDebugHelpers.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "Engine/OverlapResult.h"
#include "Combat/EnemyQuery.h"
#include "Player/ComboDataAsset.h"

static const FName LungeRootMotionName(TEXT("AttackLunge"));

UCombatComponent::UCombatComponent() :
	m_AttackTurnDuration(0.1f),
	m_TurnElapsed(0.f),
	m_DodgeForce(2200.f),
	m_DodgeDuration(0.1f),
	m_DodgeCooldown(0.7f),
	m_iCurrentComboIndex(0),
	m_bIsAttacking(false),
	m_bComboWindowOpen(false),
	m_bComboInputBuffered(false),
	m_bCanDodge(true),
	m_bHasAirDodged(false),
	m_bIsDodging(false)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	//データアセットが設定されていれば、その内容をコンボとして使う
	if (ComboData)
	{
		ComboSteps = ComboData->Steps;
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	if (!Owner) { SetComponentTickEnabled(false); return; }

	m_TurnElapsed += DeltaTime;
	float Alpha = FMath::Clamp(m_TurnElapsed / m_AttackTurnDuration, 0.f, 1.f);
	//最初に速く回って、最後にゆるやかに止まる
	Alpha = FMath::InterpEaseOut(0.f, 1.f, Alpha, 2.f);

	//Slerpで、遠回りせず近い方向へ回す
	const FQuat NewQuat = FQuat::Slerp(m_TurnStartRot.Quaternion(), m_TurnTargetRot.Quaternion(), Alpha);
	Owner->SetActorRotation(NewQuat.Rotator());

	if (Alpha >= 1.f) SetComponentTickEnabled(false);
}

void UCombatComponent::ExecuteAttack()
{

	//モーションが終わっていれば、リセット待ちの途中でも新しい攻撃として扱う
	if (m_bIsAttacking && !IsPlayingAttackMontage())
	{
		OnComboResetTimeout();
	}

	if (m_bIsAttacking)
	{
		//コンボ受付が開いていたら次の段数へ移行
		if (m_bComboWindowOpen)
		{
			int32 NextIndex = m_iCurrentComboIndex + 1;
			ExecuteComboStep(ComboSteps.IsValidIndex(NextIndex) ? NextIndex : 0);
		}
		else
		{
			//受付窓口が開く前にボタンが押されたら先行入力をONにする
			m_bComboInputBuffered = true;
		}
		return;
	}

	ExecuteComboStep(0);
}

FVector UCombatComponent::ResolveAttackDirection(ACharacter* Owner, AActor*& OutTarget) const
{
	OutTarget = nullptr;

	//ロックオン中はロックオン対象を最優先
	if (const ULockOnComponent* LockOn = Owner->FindComponentByClass<ULockOnComponent>())
	{
		if (LockOn->IsLockedOn()) OutTarget = LockOn->GetTarget();
	}

	//基準方向：入力があれば入力方向、なければ今の正面
	const FVector BaseDir = !m_DesiredDirection.IsNearlyZero()
		? m_DesiredDirection.GetSafeNormal2D()
		: Owner->GetActorForwardVector().GetSafeNormal2D();

	//ロックオンしていなければ、基準方向の近くにいる敵を探す
	if (!OutTarget) OutTarget = FindAssistTarget(Owner, BaseDir);

	if (OutTarget)
	{
		const FVector ToTarget = (OutTarget->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal2D();
		if (!ToTarget.IsNearlyZero()) return ToTarget;
	}
	return BaseDir;
}

AActor* UCombatComponent::FindAssistTarget(ACharacter* Owner, const FVector& BaseDir) const
{
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Owner);

	GetWorld()->OverlapMultiByObjectType(Overlaps, Owner->GetActorLocation(), FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(m_AssistRange), Params);

	const float CosLimit = FMath::Cos(FMath::DegreesToRadians(m_AssistHalfAngle));
	AActor* Best = nullptr;
	float BestScore = TNumericLimits<float>::Max();

	for (const FOverlapResult& Result : Overlaps)
	{
		AActor* Enemy = Result.GetActor();
		//生きている敵だけを対象にする（旧敵・ボスの両方に対応）
		if (!EnemyQuery::IsAliveEnemy(Enemy)) continue;

		FVector ToEnemy = Enemy->GetActorLocation() - Owner->GetActorLocation();
		ToEnemy.Z = 0.f;
		const float Dist = ToEnemy.Size();
		if (Dist < KINDA_SMALL_NUMBER) continue;

		//角度の範囲外は除外
		const float Dot = FVector::DotProduct(BaseDir, ToEnemy / Dist);
		if (Dot < CosLimit) continue;

		//正面に近いほど、距離が近いほど優先
		const float Score = (1.f - Dot) * 10.f + Dist / m_AssistRange;
		if (Score < BestScore)
		{
			BestScore = Score;
			Best = Enemy;
		}
	}
	return Best;
}

void UCombatComponent::StartTurn(ACharacter* Owner, const FVector& Dir)
{
	m_TurnTargetRot = FRotator(0.f, Dir.Rotation().Yaw, 0.f);

	if (m_AttackTurnDuration <= 0.f)
	{
		Owner->SetActorRotation(m_TurnTargetRot);
		return;
	}
	m_TurnStartRot = Owner->GetActorRotation();
	m_TurnElapsed = 0.f;
	SetComponentTickEnabled(true);
}

void UCombatComponent::StartLunge(ACharacter* Owner, const FVector& Dir,
	const AActor* Target, const FComboStepData& Step)
{
	UCharacterMovementComponent* MoveComp = Owner->GetCharacterMovement();
	if (!MoveComp || Step.m_LungeDuration <= 0.f) return;

	const bool bInAir = !MoveComp->IsMovingOnGround();
	const float DistToTarget = Target
		? FVector::Dist2D(Owner->GetActorLocation(), Target->GetActorLocation())
		: 0.f;

	float Distance = Step.m_LungeDistance;
	if (bInAir)
	{
		//空中：ほんの少しだけ前へ。敵に近すぎるときは進まない
		Distance = Step.m_AirLungeDistance;
		if (Target) Distance = FMath::Min(Distance, FMath::Max(0.f, DistToTarget - m_LungeStopDistance));
	}
	else
	{
		//地上：敵がいれば手前まで詰め、いなければ少し前へ
		Distance = Target
			? FMath::Clamp(DistToTarget - m_LungeStopDistance, 0.f, Step.m_LungeMaxDistance)
			: Step.m_LungeDistance;
	}
	if (Distance <= 1.f) return;   //すでに十分近い

	const FVector Start = Owner->GetActorLocation();

	TSharedPtr<FRootMotionSource_MoveToForce> Lunge = MakeShared<FRootMotionSource_MoveToForce>();
	Lunge->InstanceName = LungeRootMotionName;
	Lunge->AccumulateMode = ERootMotionAccumulateMode::Override;
	Lunge->Priority = 5;
	Lunge->StartLocation = Start;
	Lunge->TargetLocation = Start + Dir * Distance;
	Lunge->Duration = Step.m_LungeDuration;
	Lunge->bRestrictSpeedToExpected = true;
	//踏み込み終了時はピタッと止める
	Lunge->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
	Lunge->FinishVelocityParams.SetVelocity = FVector::ZeroVector;

	MoveComp->RemoveRootMotionSource(LungeRootMotionName);   //前の段の踏み込みが残っていれば消す
	MoveComp->ApplyRootMotionSource(Lunge);
}

void UCombatComponent::ExecuteComboStep(int32 StepIndex)
{
	if (!ComboSteps.IsValidIndex(StepIndex)) return;

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter) return;

	const FComboStepData& Step = ComboSteps[StepIndex];

	//空中攻撃の判定
	UActionMovementComponent* MoveComp = Cast<UActionMovementComponent>(OwnerCharacter->GetCharacterMovement());
	const bool bInAir = MoveComp && MoveComp->IsFalling();
	if (bInAir)
	{
		if (StepIndex == 0)
		{
			//空中攻撃は1回の滞空につき1セットまで
			if (m_bHasAirAttacked) return;
			m_bHasAirAttacked = true;
			m_bAirAttack = true;
		}
		else if (!m_bAirAttack || !m_bAirComboAllowed)
		{
			//前の段が空振りなら、空中コンボは続かない
			return;
		}
		m_bAirComboAllowed = false;   //この段が当たったら再び true になる
		SetAirHover(true);
	}

	//ステートの更新
	m_iCurrentComboIndex = StepIndex;
	m_bIsAttacking = true;
	m_bComboWindowOpen = false;
	m_bComboInputBuffered = false;
	HitActorsThisAttack.Empty();

	//以前のコンボリセットタイマーをクリア、再セット
	GetWorld()->GetTimerManager().ClearTimer(ComboResetTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(
		ComboResetTimerHandle,
		this,
		&UCombatComponent::OnComboResetTimeout,
		Step.ComboResetTime,
		false
	);

	//ダッシュの勢いを消す（踏み込みより先に行う）
	OwnerCharacter->GetCharacterMovement()->StopMovementImmediately();

	//攻撃方向の決定 → 向き直り → 踏み込み
	AActor* AssistTarget = nullptr;
	const FVector AttackDir = ResolveAttackDirection(OwnerCharacter, AssistTarget);
	StartTurn(OwnerCharacter, AttackDir);
	StartLunge(OwnerCharacter, AttackDir, AssistTarget, Step);

	if (m_bDebugAssist && AssistTarget)
	{
		DrawDebugLine(GetWorld(), OwnerCharacter->GetActorLocation(),
			AssistTarget->GetActorLocation(), FColor::Cyan, false, 1.f, 0, 3.f);
	}

	if (Step.Montage)
	{
		if (UAnimInstance* Anim = OwnerCharacter->GetMesh()->GetAnimInstance())
		{
			Anim->Montage_Play(Step.Montage, Step.m_PlayRate,
				EMontagePlayReturnType::MontageLength, Step.m_StartTime);

			//モーション終了時に空中攻撃を終わらせる
			FOnMontageEnded EndDelegate;
			EndDelegate.BindUObject(this, &UCombatComponent::OnAttackMontageEnded);
			Anim->Montage_SetEndDelegate(EndDelegate, Step.Montage);
		}
	}
}

void UCombatComponent::OpenComboWindow()
{
	m_bComboWindowOpen = true;

	//ボタン連打されたら次のコンボを即座に発動させる
	if (m_bComboInputBuffered)
	{
		m_bComboInputBuffered = false;
		int32 NextIndex = m_iCurrentComboIndex + 1;
		if (NextIndex < ComboSteps.Num())
			ExecuteComboStep(NextIndex);
	}
}

void UCombatComponent::CloseComboWindow()
{
	m_bComboWindowOpen = false;
}

void UCombatComponent::OnComboResetTimeout()
{
	//猶予時間に入力がなければコンボステートを初期化
	m_iCurrentComboIndex = 0;
	m_bIsAttacking = false;
	m_bComboWindowOpen = false;
	m_bComboInputBuffered = false;
	HitActorsThisAttack.Empty();
	GetWorld()->GetTimerManager().ClearTimer(ComboResetTimerHandle);
	EndAirAttack();
}

void UCombatComponent::CheckHit()
{
	if (!ComboSteps.IsValidIndex(m_iCurrentComboIndex)) return;

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter) return;

	const FComboStepData& Step = ComboSteps[m_iCurrentComboIndex];

	//自身の位置からキャラクターの前方へリーチの分だけ伸ばした線分を作成
	FVector Start = OwnerCharacter->GetActorLocation();
	FVector End = Start + OwnerCharacter->GetActorForwardVector() * Step.HitRange;

	TArray<FHitResult> HitResults;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(Step.HitRadius);

	//デバッグ用（フラグがオンのときだけ表示）
	if (m_bDebugDrawHit)
	{
		DrawDebugSphere(GetWorld(), Start, Step.HitRadius, 12, FColor::Yellow, false, 1.f);
		DrawDebugSphere(GetWorld(), End, Step.HitRadius, 12, FColor::Red, false, 1.f);
		DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, 1.f);
	}

	//当たり判定用の球体を移動させる
	bool bHit = GetWorld()->SweepMultiByChannel(
		HitResults, Start, End,
		FQuat::Identity, ECC_Pawn, Sphere);

	bool bHitEnemy = false;

	for (const FHitResult& Hit : HitResults)
	{
		AActor* HitActor = Hit.GetActor();

		//自身を無視、他アクタに重複して当たらないようにする
		if (!HitActor || HitActor == OwnerCharacter) continue;
		if (HitActorsThisAttack.Contains(HitActor)) continue;
		//敵以外（プレイヤー・分身など）には当たらないようにする
		if (!EnemyQuery::IsEnemy(HitActor)) continue;
		HitActorsThisAttack.Add(HitActor);

		//UE標準の汎用ダメージシステムを適応
		UGameplayStatics::ApplyDamage(
			HitActor, Step.Damage,
			OwnerCharacter->GetController(),
			OwnerCharacter,
			UDamageType::StaticClass());

		//吹き飛ばし力が設定されている場合ノックバック処理
		if (Step.LaunchForce > 0.f)
		{
			if (ACharacter* HitCharacter = Cast<ACharacter>(HitActor))
			{
				//自分から敵への方向ベクトルわ算出
				FVector LaunchDir = (HitActor->GetActorLocation() - OwnerCharacter->GetActorLocation()).GetSafeNormal();
				LaunchDir.Z = 0.2f; //少し斜め上に打ち上げる
				//敵の移動速度をリセット
				HitCharacter->LaunchCharacter(LaunchDir * Step.LaunchForce, true, true);
			}
		}

		//敵に当たったらキャラクター側にエネルギーを送る（上で敵だけに絞っているので判定は不要）
		bHitEnemy = true;
		OnHitEnemy.Broadcast(Step.m_EnergyGain);

	}

	if (m_bAirAttack)
	{
		if (bHitEnemy)
		{
			m_bAirComboAllowed = true;
		}
		else if (!m_bAirComboAllowed)
		{
			SetAirHover(false);
		}
	}
}

bool UCombatComponent::IsPlayingAttackMontage() const
{
	const ACharacter* Owner = Cast<ACharacter>(GetOwner());
	if (!Owner || !ComboSteps.IsValidIndex(m_iCurrentComboIndex)) return false;

	const UAnimInstance* Anim = Owner->GetMesh() ? Owner->GetMesh()->GetAnimInstance() : nullptr;
	return Anim && Anim->Montage_IsPlaying(ComboSteps[m_iCurrentComboIndex].Montage);
}

// -----------------------------------------------------------------------
// 空中攻撃
// -----------------------------------------------------------------------
void UCombatComponent::SetAirHover(bool bEnable)
{
	if (ACharacter* Owner = Cast<ACharacter>(GetOwner()))
	{
		if (UActionMovementComponent* MoveComp = Cast<UActionMovementComponent>(Owner->GetCharacterMovement()))
		{
			MoveComp->SetAirHover(bEnable);
		}
	}
}

void UCombatComponent::EndAirAttack()
{
	if (!m_bAirAttack) return;
	m_bAirAttack = false;
	m_bAirComboAllowed = false;
	SetAirHover(false);
}

void UCombatComponent::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	//次の段に切り替わっただけなら攻撃継続中なので何もしない
	if (IsPlayingAttackMontage()) return;
	EndAirAttack();
}

void UCombatComponent::ResetAirAttack()
{
	m_bHasAirAttacked = false;
	EndAirAttack();
}

// -----------------------------------------------------------------------
// 回避
// -----------------------------------------------------------------------
void UCombatComponent::ExecuteDodge(const FVector& OverrideDirection)
{
	if (!m_bCanDodge || m_bIsDodging) return;

	//攻撃モーション中だけ回避不可
	if (IsPlayingAttackMontage()) return;

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter) return;

	UActionMovementComponent* MoveComp = Cast<UActionMovementComponent>(OwnerCharacter->GetCharacterMovement());
	if (!MoveComp) return;

	UWorld* World = GetWorld();
	if (!World) return;

	bool bInAir = MoveComp->IsFalling();
	if (bInAir && m_bHasAirDodged) return;

	if (bInAir) m_bHasAirDodged = true;
	m_bIsDodging = true;

	//入力方向を回避方向とする。入力がない場合は正面方向
	FVector DodgeDirection = !OverrideDirection.IsNearlyZero()
		? OverrideDirection
		: MoveComp->GetLastInputVector();
	if (DodgeDirection.IsNearlyZero())
	{
		DodgeDirection = OwnerCharacter->GetActorForwardVector();
	}

	//回避クールダウンタイマーの開始
	m_bCanDodge = false;
	GetWorld()->GetTimerManager().SetTimer(
		DodgeCoolDownTimerHandle, this,
		&UCombatComponent::ResetDodgeCooldown,
		m_DodgeCooldown, false);

	//攻撃キャンセル用
	MoveComp->RemoveRootMotionSource(LungeRootMotionName);

	MoveComp->StartDodge(DodgeDirection, m_DodgeForce);

	GetWorld()->GetTimerManager().SetTimer(
		DodgeTimerHandle, this, &UCombatComponent::EndDodge, m_DodgeDuration, false);
}

void UCombatComponent::EndDodge()
{
	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
	{
		if (UActionMovementComponent* MoveComp =
			Cast<UActionMovementComponent>(OwnerCharacter->GetCharacterMovement()))
		{
			MoveComp->EndDodge();
		}
	}
	m_bIsDodging = false;
}

void UCombatComponent::ResetDodgeCooldown()
{
	m_bCanDodge = true;
}

void UCombatComponent::ResetAirDodge()
{
	m_bHasAirDodged = false;
}
