// ActionCharacter.cpp

#include "Player/ActionCharacter.h"
#include "Player/ActionMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Player/CombatComponent.h"
#include "Player/GhostRecorderComponent.h"
#include "Ghost/GhostCharacter/GhostCharacter.h"
#include "Ghost/GhostEnemy/GhostEnemyComponent.h"
#include "Ghost/GhostCharacter/GhostManagerComponent.h"
#include "Ghost/Data/GhostTypes.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Combat/EnemyQuery.h"
#include "Engine/OverlapResult.h"
#include "Engine/DamageEvents.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

AActionCharacter::AActionCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UActionMovementComponent>(
		  ACharacter::CharacterMovementComponentName)),
	  m_TimeToSprint(1.5f),
	  m_CurrentRunTime(0.f),
	  m_GhostSummonCost(30.f),
	  m_MaxEnergy(100.f),
	  m_EnergyGainPerHit(15.f),
	  m_CurrentEnergy(0.f),
	  m_iActiveGhostCount(0),
	  m_iMaxGhostCount(5),
	  m_LockOnCameraInterpSpeed(4.f),
	  m_LockOnCameraOffsetY(60.f),
	  m_LockOnPitchMin(-45.f),
	  m_LockOnPitchMax(20.f),
	  m_SideBlendRange(0.5f),
	  m_TargetFocusBias(0.5f),
	  m_LockOnInterpSpeedNear(8.f),
	  m_JumpPressedTime(0.f),
	  m_JumpHoldThreshold(0.2f),
	  m_JumpZVelocityShort(1500.f),
	  m_JumpZVelocityLong(800.f)
{
	PrimaryActorTick.bCanEverTick = true;

	//�W�����v�i���ݒ�
	JumpMaxCount = 2;

	//�R���g���[���[�̉�]�ŃL�������̂����Ȃ��悤��
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	//�ړ������ɃL�����N�^�[�������ŃX�b�ƌ����悤�ɂ���
	GetActionMovementComponent()->bOrientRotationToMovement = true;

	//�L�����N�^�[�̈ړ����͂���������ցA�����I�Ɋ��炩�Ɍ�����ς���ݒ�
	if (UActionMovementComponent* MoveComp = GetActionMovementComponent())
	{
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->RotationRate = FRotator(0.f, 1000.f, 0.f); //���񑬓x
	}

	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));

	//�����E��̃\�P�b�g�ɕt����
	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(GetMesh(), TEXT("WeaponSocket"));
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	LockOnComponent = CreateDefaultSubobject<ULockOnComponent>(TEXT("LockOnComponent"));

	//�J�����̐ݒ�
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->bUsePawnControlRotation = true;

	CameraBoom->TargetArmLength = 400.f;
	//�ǂ̏Փ˃e�X�g
	CameraBoom->bDoCollisionTest = true;
	//�J��������������`�����l������WorldStatic�݂̂ɂ���i�ς������j
	CameraBoom->ProbeChannel = ECC_Camera;
	//����p�̋��̑傫��
	CameraBoom->ProbeSize = 15.f;
	//�E���z���I�t�Z�b�g
	CameraBoom->SocketOffset = FVector(0.f, 60.f, 50.f);
	//�J�����X���[�W���O
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 10.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	//Ghost�V�X�e��
	GhostRecorder = CreateDefaultSubobject<UGhostRecorderComponent>(TEXT("GhostRecorder"));
	GhostManager = CreateDefaultSubobject<UGhostManagerComponent>(TEXT("GhostManager"));
}

UActionMovementComponent* AActionCharacter::GetActionMovementComponent() const
{
	return Cast<UActionMovementComponent>(GetCharacterMovement());
}

void AActionCharacter::BeginPlay()
{
	Super::BeginPlay();

	//�ő�G�l���M�[�́u1�{�� �~ �ő�{���v�Ō��߂�
	m_MaxEnergy = m_EnergyPerStock * m_iMaxEnergyStock;
	SetEnergy(0.f);
	SetHP(m_MaxHP);
	RefillHeals();

	if (CombatComponent)
	{
		CombatComponent->OnHitEnemy.AddUObject(this, &AActionCharacter::OnHitEnemy);
	}

	DefaultSocketOffset = CameraBoom->SocketOffset;

	if (LockOnComponent)
	{
		LockOnComponent->OnLockOnChanged.AddUObject(this, &AActionCharacter::OnLockOnChanged);
	}

	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Sub =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Sub->AddMappingContext(DefaultMappingContext, 0);
		}

		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->ViewPitchMin = -60.f; //�����낵�̌��E
			PC->PlayerCameraManager->ViewPitchMax = 35.f; //���グ�̌��E�i�n�ʂɐ���ɂ����Ȃ�j
		}
	}

	//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::White,
	//    FString::Printf(TEXT("GhostClass:%s  Recorder:%s"),
	//    GhostCharacterClass ? *GhostCharacterClass->GetName() : TEXT("NULL"),
	//    GhostRecorder ? TEXT("OK") : TEXT("NULL")));
}

void AActionCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//���b�N�I�����̓J�������^�[�Q�b�g�֒Ǐ]������
	if (LockOnComponent && LockOnComponent->IsLockedOn())
	{
		UpdateLockOnCamera(DeltaTime);
	}

	//�퓬�����ǂ������X�V
	UpdateCombatState(DeltaTime);

	//�L�����N�^�[�̌��݂̕��s�ړ����x���擾
	const float Speed = GetVelocity().Size2D();

	//�퓬���łȂ��A���ȏ�̑��x�ő��葱������_�b�V��
	if (Speed > 10.f && !m_bInCombat)
	{
		m_CurrentRunTime += DeltaTime;
		if (m_CurrentRunTime >= m_TimeToSprint) StartSprint();
	}
	else
	{
		//�~�܂����A�܂��͐퓬�ɓ�������_�b�V������
		m_CurrentRunTime = 0.f;
		StopSprint();
	}
}

void AActionCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EIC->BindAction(AttackAction, ETriggerEvent::Started, this, &AActionCharacter::Attack);
		EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AActionCharacter::Move);
		EIC->BindAction(MoveAction, ETriggerEvent::Completed, this, &AActionCharacter::MoveCompleted);
		EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AActionCharacter::Look);
		EIC->BindAction(LockOnAction, ETriggerEvent::Started, this, &AActionCharacter::LockOn);
		EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		EIC->BindAction(DodgeAction, ETriggerEvent::Started, this, &AActionCharacter::Dodge);
		EIC->BindAction(SummonAction, ETriggerEvent::Started, this, &AActionCharacter::SummonGhost);
		EIC->BindAction(HealAction, ETriggerEvent::Started, this, &AActionCharacter::Heal);
	}
}

void AActionCharacter::Attack()
{
	if (IsHealing()) return;
	if (m_bIsHitStunned) return;
	if (CombatComponent) CombatComponent->ExecuteAttack();
}

float AActionCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
                                   AController* EventInstigator, AActor* DamageCauser)
{
	if (m_bIsDead || DamageAmount <= 0.f) return 0.f;

	//��𒆂͖��G
	if (m_bDodgeInvincible && CombatComponent && CombatComponent->IsDodging())
	{
		if (m_bDebugHP) GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Cyan, TEXT("Dodge! No Damage"));
		return 0.f;
	}

	//��e����̖��G����
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - m_LastDamageTime < m_HitInvincibleTime) return 0.f;

	const float Actual = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (Actual <= 0.f) return 0.f;

	m_LastDamageTime = Now;
	SetHP(m_CurrentHP - Actual);
	InterruptHeal();

	if (m_CurrentHP > 0.f)
	{
		PlayHitReaction(DamageCauser);
	}

	if (m_bDebugHP)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red,
		                                 FString::Printf(
			                                 TEXT("Damage %.0f from %s"), Actual, *GetNameSafe(DamageCauser)));
	}

	if (m_CurrentHP <= 0.f) Die();
	return Actual;
}

void AActionCharacter::PlayHitReaction(AActor* DamageCauser)
{
	//�U�����Ȃ�R���{�𒆒f
	if (CombatComponent) CombatComponent->CancelAttack();

	//�U�����Ă�������̕��������i���ʂ���H�炤���[�V�����Ȃ̂Łj
	if (DamageCauser && DamageCauser != this)
	{
		FVector ToCauser = DamageCauser->GetActorLocation() - GetActorLocation();
		ToCauser.Z = 0.f;
		if (!ToCauser.IsNearlyZero())
		{
			SetActorRotation(ToCauser.Rotation());
		}
	}

	//�������~�߂ă��[�V�����Đ�
	GetCharacterMovement()->StopMovementImmediately();
	if (m_HitReactMontage)
	{
		PlayAnimMontage(m_HitReactMontage, m_HitReactPlayRate);
	}

	//�̂�����J�n
	m_bIsHitStunned = true;
	GetWorldTimerManager().SetTimer(m_HitStunTimer, this, &AActionCharacter::EndHitStun, m_HitStunTime, false);
}

void AActionCharacter::EndHitStun()
{
	m_bIsHitStunned = false;
}

void AActionCharacter::SetHP(float NewHP)
{
	m_CurrentHP = FMath::Clamp(NewHP, 0.f, m_MaxHP);
	OnHPChanged.Broadcast(m_CurrentHP, m_MaxHP);

	if (m_bDebugHP && GEngine)
	{
		//�L�[���Œ肷��Ɠ����s���㏑�����ꑱ����i��ʍ���ɏ�ɕ\���j
		//GEngine->AddOnScreenDebugMessage(1001, 9999.f, FColor::Green,FString::Printf(TEXT("HP: %.0f / %.0f"), m_CurrentHP, m_MaxHP));
	}
}

void AActionCharacter::Die()
{
	if (m_bIsDead) return;
	m_bIsDead = true;

	EndHeal();

	//�Ƃ肠��������s�\�ɂ��邾���i���o�͌�Łj
	if (APlayerController* PC = Cast<APlayerController>(GetController())) DisableInput(PC);
	GetCharacterMovement()->DisableMovement();

	if (m_bDebugHP) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PLAYER DEAD"));
}

void AActionCharacter::DebugDamage(float Amount)
{
	UGameplayStatics::ApplyDamage(this, Amount, GetController(), this, nullptr);
}

void AActionCharacter::AddHp(float _hpAmount)
{
	if (m_CurrentHP == m_MaxHP)return;

	m_CurrentHP += _hpAmount;

	//最大HPを超えないように
	if (m_CurrentHP > m_MaxHP)
	{
		m_CurrentHP = m_MaxHP;
	}

	OnHPChanged.Broadcast(m_CurrentHP, m_MaxHP);
}

void AActionCharacter::OnRespawn()
{
}

void AActionCharacter::UpdateCombatState(float DeltaTime)
{
	//���t���[���͕s�v�Ȃ̂�0.25�b���Ƃɔ��肷��
	m_CombatCheckTimer -= DeltaTime;
	if (m_CombatCheckTimer > 0.f) return;
	m_CombatCheckTimer = 0.25f;

	const float Now = GetWorld()->GetTimeSeconds();

	//���b�N�I�����A�܂��͍ŋߍU���𓖂ĂĂ���ΐ퓬��
	bool bCombat = (LockOnComponent && LockOnComponent->IsLockedOn())
		|| (Now - m_LastCombatTime < m_CombatLingerTime);

	//����ȊO�́A�߂��Ɂu�N����_���Ă���G�v�����邩�Ŕ���
	if (!bCombat)
	{
		TArray<FOverlapResult> Overlaps;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);
		GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity,
		                                     FCollisionObjectQueryParams(ECC_Pawn),
		                                     FCollisionShape::MakeSphere(m_CombatCheckRadius), Params);

		for (const FOverlapResult& Result : Overlaps)
		{
			if (EnemyQuery::IsEngaged(Result.GetActor()))
			{
				bCombat = true;
				break;
			}
		}
	}

	m_bInCombat = bCombat;
}

void AActionCharacter::OnHitEnemy(float EnergyGain)
{
	m_LastCombatTime = GetWorld()->GetTimeSeconds();

	SetEnergy(m_CurrentEnergy + EnergyGain);

	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan,
	                                 FString::Printf(
		                                 TEXT("Energy: %.1f / %.1f (Stock %d)"), m_CurrentEnergy, m_MaxEnergy,
		                                 GetEnergyStock()));
}

void AActionCharacter::Dodge()
{
	if (m_bIsHitStunned) return;
	InterruptHeal();
	if (CombatComponent) CombatComponent->ExecuteDodge();
}

void AActionCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();
	if (!Controller) return;

	//���b�N�I�����͉E�X�e�B�b�N���^�[�Q�b�g�؂�ւ��Ɏg��
	if (LockOnComponent && LockOnComponent->IsLockedOn())
	{
		LockOnComponent->TrySwitchTarget(LookAxisVector.X);
		return;
	}

	//���E�̃J������]
	AddControllerYawInput(LookAxisVector.X);
	//�㉺�̃J������]
	AddControllerPitchInput(LookAxisVector.Y);
}

void AActionCharacter::LockOn()
{
	if (LockOnComponent)
	{
		LockOnComponent->ToggleLockOn();
	}
}

//���b�N�I���V�X�e�����ǒ�
void AActionCharacter::OnLockOnChanged(AActor* NewTarget)
{
	if (NewTarget)
	{
		//���b�NON�F�v���C���[���R���g���[���[��Yaw�ɏ]�킹��
		bUseControllerRotationYaw = true;
		GetActionMovementComponent()->bOrientRotationToMovement = false;

		//�J���������ɂ��炵�ăv���C���[�ƃ^�[�Q�b�g�����f��
		//CameraBoom->SocketOffset = FVector(
		//	DefaultSocketOffset.X,
		//	DefaultSocketOffset.Y + LockOnCameraOffsetY,
		//	DefaultSocketOffset.Z
		//);

		// GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, FString::Printf(TEXT("���b�N�I���F %s"), *NewTarget->GetName()));
	}
	else
	{
		//���b�N�I��OFF�F�ʏ�̈ړ����������ɖ߂�
		bUseControllerRotationYaw = false;
		GetActionMovementComponent()->bOrientRotationToMovement = true;

		//�\�P�b�g�I�t�Z�b�g�����ɖ߂�
		CameraBoom->SocketOffset = DefaultSocketOffset;

		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Purple, TEXT("FinithLockOn"));
	}
}

void AActionCharacter::UpdateLockOnCamera(float DeltaTime)
{
	AActor* Target = LockOnComponent->GetTarget();
	if (!Target || !Controller) return;

	FVector PlayerLocation = GetActorLocation();
	FVector TargetLocation = Target->GetActorLocation();

	//���E�I�t�Z�b�g
	FVector ToTarget = (TargetLocation - PlayerLocation).GetSafeNormal();
	float RightDot = FVector::DotProduct(FollowCamera->GetRightVector(), ToTarget);

	//�����t�߂�0�t�߂Ɏ���������BSideBlendRange ��傫������قǊɂ₩�ɂȂ�
	float SideRatio = FMath::Clamp(RightDot / m_SideBlendRange, -1.f, 1.f);
	float DesiredOffsetY = DefaultSocketOffset.Y + m_LockOnCameraOffsetY * SideRatio;

	//�}�ɃI�t�Z�b�g���ς��Ƃ����炩�Ɉړ�
	FVector NewOffset = CameraBoom->SocketOffset;
	NewOffset.Y = FMath::FInterpTo(NewOffset.Y, DesiredOffsetY, DeltaTime, 6.f);
	CameraBoom->SocketOffset = NewOffset;

	//�J�������g�̈ʒu�ł͂Ȃ� SpringArm �̍�������ɒ����_��ݒ�
	FVector Pivot = CameraBoom->GetComponentLocation();
	FVector FocusPoint = FMath::Lerp(PlayerLocation, TargetLocation, m_TargetFocusBias);

	const FVector ToFocus = FocusPoint - Pivot;
	//�����������ق�0����Yaw����`�ł����\���
	if (ToFocus.SizeSquared2D() < FMath::Square(20.f)) return;

	FRotator TargetRot = ToFocus.Rotation();

	//�����ɉ����ĕ⊮���x��ς���
	float Distance = FVector::Dist(PlayerLocation, TargetLocation);
	float InterpSpeed = FMath::GetMappedRangeValueClamped(
		FVector2D(200.f, 1500.f), //���������W
		FVector2D(m_LockOnInterpSpeedNear, m_LockOnCameraInterpSpeed), //�Ή�����⊮���x�����W
		Distance
	);

	//�ɒ[�Ȍ��グ�E�����낵��h��
	FRotator NewRot = FMath::RInterpTo(Controller->GetControlRotation(), TargetRot, DeltaTime, InterpSpeed);
	NewRot.Pitch = FMath::Clamp(NewRot.Pitch, m_LockOnPitchMin, m_LockOnPitchMax);
	NewRot.Roll = 0.f;
	Controller->SetControlRotation(NewRot);

	//�v���C���[�̌�����G�̕����ɌŒ�
	FVector ToTargetFlat = TargetLocation - PlayerLocation;
	ToTargetFlat.Z = 0.f;
	if (!ToTargetFlat.IsNearlyZero())
	{
		SetActorRotation(FMath::RInterpTo(
			GetActorRotation(), ToTargetFlat.GetSafeNormal().Rotation(), DeltaTime, 10.f));
	}
}

void AActionCharacter::Move(const FInputActionValue& Value)
{
	//���͒l��2D�x�N�g���Ƃ��Ď擾
	const FVector2D MovementVector = Value.Get<FVector2D>();

	if (!Controller) return;

	const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
	const FVector Forward = FRotationMatrix(Yaw).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y);

	//�U���̌����␳�p�ɁA���͕����͏�ɋL�^���Ă���
	if (CombatComponent)
	{
		CombatComponent->SetDesiredDirection(Forward * MovementVector.Y + Right * MovementVector.X);
	}

	//�U�����͈ړ����Ȃ�
	if (CombatComponent && CombatComponent->IsPlayingAttackMontage()) return;

	//�̂����蒆�͈ړ����Ȃ�
	if (m_bIsHitStunned) return;

	//��𒆂͈ړ����Ȃ��i�����̏����j
	if (UActionMovementComponent* MoveComp = GetActionMovementComponent())
	{
		if (MoveComp->IsDodging()) return;
	}

	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), MovementVector.Y);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), MovementVector.X);
}

void AActionCharacter::MoveCompleted(const FInputActionValue& Value)
{
	if (CombatComponent) CombatComponent->SetDesiredDirection(FVector::ZeroVector);
}

void AActionCharacter::StartSprint()
{
	if (UActionMovementComponent* MC = GetActionMovementComponent())
		if (!MC->IsSprinting()) MC->SetSprinting(true);
}

void AActionCharacter::StopSprint()
{
	if (UActionMovementComponent* MC = GetActionMovementComponent())
		if (MC->IsSprinting()) MC->SetSprinting(false);
}


void AActionCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();

	UActionMovementComponent* MoveComp = GetActionMovementComponent();
	if (!MoveComp) return;

	//���݂̃W�����v�񐔂��m�F
	//JumpCurrentCount ��ACharacter�ɕW���ŗp�ӂ���Ă���u���݉���ڂ̃W�����v���v�����ϐ�
	if (JumpCurrentCount == 1)
	{
		//�y1�i�ڂ̃W�����v���̏����z
		MoveComp->AirControl = MoveComp->m_AirControlFirstJump;


		//�n�ʂ��R��y���̃G�t�F�N�g(Niagara)�𑫌��ɏo��
		//�u�n�b�I�v�Ƃ����ʏ�W�����v�̃{�C�X��SE���Đ�����
	}
	else if (JumpCurrentCount == 2)
	{
		//�y2�i�ڂ̃W�����v�i�G�A�n�C�N�j���̏����z
		MoveComp->AirControl = MoveComp->m_AirControlSecondJump;

		FVector Vel = MoveComp->Velocity;
		Vel.Z = MoveComp->m_SecondJumpZVelocity;
		MoveComp->Velocity = Vel;

		//��i�W�����v�ŋ󒆍U����������x�g����悤�ɂ���
		if (CombatComponent) CombatComponent->ResetAirAttack();

		//���͕����Ɉ�x���������]��
		FVector InputVector = MoveComp->GetLastInputVector();
		if (!InputVector.IsNearlyZero())
		{
			FVector InputDir = InputVector.GetSafeNormal2D();
			SetActorRotation(InputDir.Rotation());

			float HorizontalSpeed = FVector(Vel.X, Vel.Y, 0.f).Size();
			MoveComp->Velocity.X = InputDir.X * HorizontalSpeed;
			MoveComp->Velocity.Y = InputDir.Y * HorizontalSpeed;
		}

		if (m_SecondJumpMontage)
		{
			PlayAnimMontage(m_SecondJumpMontage, m_SecondJumpPlayRate);
		}
	}
}

bool AActionCharacter::CanJumpInternal_Implementation() const
{
	//�U�����[�V�������̓W�����v�s��
	if (CombatComponent && CombatComponent->IsPlayingAttackMontage()) return false;
	//�񕜒��̓W�����v�s��
	if (IsHealing()) return false;

	if (m_bIsHitStunned) return false;

	return Super::CanJumpInternal_Implementation();
}

void AActionCharacter::OnJumpPressed()
{
	m_JumpPressedTime = GetWorld()->GetTimeSeconds();
	Jump();
}

void AActionCharacter::OnJumpReleased()
{
	StopJumping();

	// ��i�W�����v����Z���x��G��Ȃ�
	if (JumpCurrentCount >= 2) return;

	//�{�^���������Ă������Ԃ��Z�o
	float HoldDuration = GetWorld()->GetTimeSeconds() - m_JumpPressedTime;

	UActionMovementComponent* MoveComp = GetActionMovementComponent();
	if (!MoveComp) return;

	//�󒆂Ń{�^���������ɗ����ꂽ�ꍇ�A�㏸���x�̏����Ⴍ�J�b�g���邱�Ƃŏ��W�����v��\������
	if (MoveComp->IsFalling())
	{
		FVector Vel = MoveComp->Velocity;
		if (HoldDuration < m_JumpHoldThreshold)
		{
			Vel.Z = FMath::Min(Vel.Z, m_JumpZVelocityShort);
		}
		MoveComp->Velocity = Vel;
	}
}

void AActionCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	//���n���A����
	UActionMovementComponent* MoveComp = GetActionMovementComponent();
	if (MoveComp)
	{
		MoveComp->AirControl = 0.05f;
		MoveComp->StartLandingRecovery();
	}

	if (CombatComponent)
	{
		CombatComponent->ResetAirDodge();
		CombatComponent->ResetAirAttack();
	}

	if (m_SecondJumpMontage)
	{
		if (UAnimInstance* Anim = GetMesh()->GetAnimInstance())
		{
			if (Anim->Montage_IsPlaying(m_SecondJumpMontage))
			{
				StopAnimMontage(m_SecondJumpMontage);
			}
		}
	}
}

// -----------------------------------------------------------------------
// ���g�̏���
// -----------------------------------------------------------------------
void AActionCharacter::SummonGhost()
{
	//�X�g�b�N�s���`�F�b�N�i�����ɂ̓X�g�b�N1�{���K�v�j
	if (GetEnergyStock() < 1)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("Not enough energy stock"));
		return;
	}

	//�ő吔�`�F�b�N
	if (m_ActiveGhosts.Num() >= m_iMaxGhostCount)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange, TEXT("MaxGhostCount!"));
		return;
	}

	if (!GhostRecorder || !GhostCharacterClass)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("Recorder or GhostClass is NULL"));
		return;
	}

	//����20�b�̍s���L�^���擾
	const TArray<FGhostActionData> Snapshot = GhostRecorder->GetSnapshot(20.f);
	if (Snapshot.IsEmpty())
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("NO Data MoveMore"));
		return;
	}

	//�X�|�[���ʒu�i�v���C���[�̍����j
	const FVector SpawnLoc = GetActorLocation() + GetActorRightVector() * -100.f;
	const FRotator SpawnRot = GetActorRotation();

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AGhostCharacter* Ghost = GetWorld()->SpawnActor<AGhostCharacter>(
		GhostCharacterClass, SpawnLoc, SpawnRot, Params);
	if (!Ghost)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("SpawnActor failed"));
		return;
	}

	//�L�^�f�[�^��n���čs���J�n
	Ghost->InitializeGhost(Snapshot, GhostRecorder);

	//���g���������烊�X�g����O��
	Ghost->OnDestroyed.AddDynamic(this, &AActionCharacter::OnGhostDestroyed);

	//���g�̍U���ł��G�l���M�[�𓾂�
	if (Ghost->CombatComponent)
	{
		Ghost->CombatComponent->OnHitEnemy.AddUObject(this, &AActionCharacter::OnGhostHitEnemy);
	}

	//���g�̏�ԁi�����钼�O�Ȃǁj�̕ω���UI�ɓ`����
	if (Ghost->EnemyComponent)
	{
		Ghost->EnemyComponent->OnStateChanged.AddDynamic(this, &AActionCharacter::OnGhostStateChanged);
	}

	//�X�g�b�N��1�{����
	SetEnergy(m_CurrentEnergy - m_EnergyPerStock);

	m_ActiveGhosts.Add(Ghost);
	m_iActiveGhostCount = m_ActiveGhosts.Num();
	OnGhostsChanged.Broadcast();
}

void AActionCharacter::SetEnergy(float NewEnergy)
{
	m_CurrentEnergy = FMath::Clamp(NewEnergy, 0.f, m_MaxEnergy);
	OnEnergyChanged.Broadcast(m_CurrentEnergy, m_MaxEnergy);
}

void AActionCharacter::OnGhostStateChanged(EGhostLifecycleState NewState)
{
	OnGhostsChanged.Broadcast();
}

void AActionCharacter::GetGhostStates(TArray<EGhostLifecycleState>& OutStates) const
{
	OutStates.Reset();
	for (const TWeakObjectPtr<AGhostCharacter>& G : m_ActiveGhosts)
	{
		if (const AGhostCharacter* Ghost = G.Get())
		{
			OutStates.Add(Ghost->EnemyComponent
				              ? Ghost->EnemyComponent->GetLifecycleState()
				              : EGhostLifecycleState::Friendly);
		}
	}
}

void AActionCharacter::OnGhostHitEnemy(float EnergyGain)
{
	OnHitEnemy(EnergyGain * m_GhostEnergyRate);
}

void AActionCharacter::OnGhostDestroyed(AActor* DestroyedActor)
{
	m_ActiveGhosts.RemoveAll([DestroyedActor](const TWeakObjectPtr<AGhostCharacter>& G)
	{
		return !G.IsValid() || G.Get() == DestroyedActor;
	});
	m_iActiveGhostCount = m_ActiveGhosts.Num();
	OnGhostsChanged.Broadcast();
}

void AActionCharacter::Heal()
{
	if (m_bIsDead || IsHealing()) return;

	if (m_bIsHitStunned) return;

	if (m_iHealCount <= 0)
	{
		if (m_bDebugHP) GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Orange, TEXT("No Heal Left"));
		return;
	}
	if (m_CurrentHP >= m_MaxHP) return;
	if (GetCharacterMovement()->IsFalling()) return;
	if (CombatComponent && (CombatComponent->IsAttacking() || CombatComponent->IsDodging())) return;

	//�\���J�n�F�ړ����x�𗎂Ƃ�
	m_HealPhase = EHealPhase::Windup;
	if (UActionMovementComponent* Move = Cast<UActionMovementComponent>(GetCharacterMovement()))
	{
		Move->SetSpeedMultiplier(m_HealMoveSpeedRate);
	}
	//TODO�F�����ŉ񕜃��[�V�����i�r�����ށj���Đ�����

	GetWorldTimerManager().SetTimer(m_HealTimer, this, &AActionCharacter::ApplyHeal, m_HealWindupTime, false);

	if (m_bDebugHP) GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, TEXT("Heal Start..."));
}

void AActionCharacter::ApplyHeal()
{
	//�����ŏ��߂ĉ񐔂�����ĉ�
	--m_iHealCount;
	SetHP(m_CurrentHP + m_HealAmount);
	OnHealCountChanged.Broadcast(m_iHealCount, m_iMaxHealCount);

	if (m_bDebugHP)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
		                                 FString::Printf(
			                                 TEXT("Heal +%.0f  (Left %d / %d)"), m_HealAmount, m_iHealCount,
			                                 m_iMaxHealCount));
	}

	//�㌄�F���x�͗��Ƃ����܂�
	m_HealPhase = EHealPhase::Recovery;
	GetWorldTimerManager().SetTimer(m_HealTimer, this, &AActionCharacter::EndHeal, m_HealRecoveryTime, false);
}

void AActionCharacter::EndHeal()
{
	GetWorldTimerManager().ClearTimer(m_HealTimer);
	m_HealPhase = EHealPhase::None;

	if (UActionMovementComponent* Move = Cast<UActionMovementComponent>(GetCharacterMovement()))
	{
		Move->SetSpeedMultiplier(1.f);
	}
	//TODO�F�񕜃��[�V�������~�߂�i���f���j
}

void AActionCharacter::InterruptHeal()
{
	if (!IsHealing()) return;

	if (m_bDebugHP)
	{
		//�\�����Ȃ玸�s�A�㌄���Ȃ瓮�삾�����f�i�񕜂͍ς�ł���j
		const bool bFailed = (m_HealPhase == EHealPhase::Windup);
		GEngine->AddOnScreenDebugMessage(-1, 2.f, bFailed ? FColor::Red : FColor::Yellow,
		                                 bFailed
			                                 ? TEXT("Heal Failed (Not consumed)")
			                                 : TEXT("Heal Motion Interrupted"));
	}
	EndHeal();
}

void AActionCharacter::RefillHeals()
{
	m_iHealCount = m_iMaxHealCount;
	OnHealCountChanged.Broadcast(m_iHealCount, m_iMaxHealCount);
}
