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

    //ジャンプ段数設定
    JumpMaxCount = 2;

    //コントローラーの回転でキャラ自体が回らないように
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;
    //移動方向にキャラクターが自動でスッと向くようにする
    GetActionMovementComponent()->bOrientRotationToMovement = true;

    //キャラクターの移動入力がある方向へ、自動的に滑らかに向きを変える設定
    if (UActionMovementComponent* MoveComp = GetActionMovementComponent())
    {
        MoveComp->bOrientRotationToMovement = true;
        MoveComp->RotationRate = FRotator(0.f, 1000.f, 0.f);  //旋回速度
    }

    CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));

    //剣を右手のソケットに付ける
    WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
    WeaponMesh->SetupAttachment(GetMesh(), TEXT("WeaponSocket"));
    WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    LockOnComponent = CreateDefaultSubobject<ULockOnComponent>(TEXT("LockOnComponent"));

    //カメラの設定
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->bUsePawnControlRotation = true;

    CameraBoom->TargetArmLength = 400.f;
    //壁の衝突テスト
    CameraBoom->bDoCollisionTest = true;
    //カメラが反応するチャンネルををWorldStaticのみにする（変えたい）
    CameraBoom->ProbeChannel = ECC_Camera;
    //判定用の球の大きさ
    CameraBoom->ProbeSize = 15.f;
    //右肩越しオフセット
    CameraBoom->SocketOffset = FVector(0.f, 60.f, 50.f);
    //カメラスムージング
    CameraBoom->bEnableCameraLag = true;
    CameraBoom->CameraLagSpeed = 10.f;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    //Ghostシステム
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

    //最大エネルギーは「1本分 × 最大本数」で決める
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
            PC->PlayerCameraManager->ViewPitchMin = -60.f;   //見下ろしの限界
            PC->PlayerCameraManager->ViewPitchMax = 35.f;    //見上げの限界（地面に潜りにくくなる）
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

    //ロックオン中はカメラをターゲットへ追従させる
    if (LockOnComponent && LockOnComponent->IsLockedOn())
    {
        UpdateLockOnCamera(DeltaTime);
    }

    //戦闘中かどうかを更新
    UpdateCombatState(DeltaTime);

    //キャラクターの現在の平行移動速度を取得
    const float Speed = GetVelocity().Size2D();

    //戦闘中でなく、一定以上の速度で走り続けたらダッシュ
    if (Speed > 10.f && !m_bInCombat)
    {
        m_CurrentRunTime += DeltaTime;
        if (m_CurrentRunTime >= m_TimeToSprint) StartSprint();
    }
    else
    {
        //止まった、または戦闘に入ったらダッシュ解除
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

    //回避中は無敵
    if (m_bDodgeInvincible && CombatComponent && CombatComponent->IsDodging())
    {
        if (m_bDebugHP) GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Cyan, TEXT("Dodge! No Damage"));
        return 0.f;
    }

    //被弾直後の無敵時間
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
            FString::Printf(TEXT("Damage %.0f from %s"), Actual, *GetNameSafe(DamageCauser)));
    }

    if (m_CurrentHP <= 0.f) Die();
    return Actual;
}

void AActionCharacter::PlayHitReaction(AActor* DamageCauser)
{
    //攻撃中ならコンボを中断
    if (CombatComponent) CombatComponent->CancelAttack();

    //攻撃してきた相手の方を向く（正面から食らうモーションなので）
    if (DamageCauser && DamageCauser != this)
    {
        FVector ToCauser = DamageCauser->GetActorLocation() - GetActorLocation();
        ToCauser.Z = 0.f;
        if (!ToCauser.IsNearlyZero())
        {
            SetActorRotation(ToCauser.Rotation());
        }
    }

    //勢いを止めてモーション再生
    GetCharacterMovement()->StopMovementImmediately();
    if (m_HitReactMontage)
    {
        PlayAnimMontage(m_HitReactMontage, m_HitReactPlayRate);
    }

    //のけぞり開始
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
        //キーを固定すると同じ行が上書きされ続ける（画面左上に常に表示）
        //GEngine->AddOnScreenDebugMessage(1001, 9999.f, FColor::Green,FString::Printf(TEXT("HP: %.0f / %.0f"), m_CurrentHP, m_MaxHP));
    }
}

void AActionCharacter::Die()
{
    if (m_bIsDead) return;
    m_bIsDead = true;

    EndHeal();

    //とりあえず操作不能にするだけ（演出は後で）
    if (APlayerController* PC = Cast<APlayerController>(GetController())) DisableInput(PC);
    GetCharacterMovement()->DisableMovement();

    if (m_bDebugHP) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PLAYER DEAD"));
}

void AActionCharacter::DebugDamage(float Amount)
{
    UGameplayStatics::ApplyDamage(this, Amount, GetController(), this, nullptr);
}

void AActionCharacter::UpdateCombatState(float DeltaTime)
{
    //毎フレームは不要なので0.25秒ごとに判定する
    m_CombatCheckTimer -= DeltaTime;
    if (m_CombatCheckTimer > 0.f) return;
    m_CombatCheckTimer = 0.25f;

    const float Now = GetWorld()->GetTimeSeconds();

    //ロックオン中、または最近攻撃を当てていれば戦闘中
    bool bCombat = (LockOnComponent && LockOnComponent->IsLockedOn())
        || (Now - m_LastCombatTime < m_CombatLingerTime);

    //それ以外は、近くに「誰かを狙っている敵」がいるかで判定
    if (!bCombat)
    {
        TArray<FOverlapResult> Overlaps;
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(this);
        GetWorld()->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity,
            FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(m_CombatCheckRadius), Params);

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
        FString::Printf(TEXT("Energy: %.1f / %.1f (Stock %d)"), m_CurrentEnergy, m_MaxEnergy, GetEnergyStock()));
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

    //ロックオン中は右スティックをターゲット切り替えに使う
    if (LockOnComponent && LockOnComponent->IsLockedOn())
    {
        LockOnComponent->TrySwitchTarget(LookAxisVector.X);
        return;
    }

    //左右のカメラ回転
    AddControllerYawInput(LookAxisVector.X);
    //上下のカメラ回転
    AddControllerPitchInput(LookAxisVector.Y);
}

void AActionCharacter::LockOn()
{
    if (LockOnComponent)
    {
        LockOnComponent->ToggleLockOn();
    }
}

//ロックオンシステム改良中
void AActionCharacter::OnLockOnChanged(AActor* NewTarget)
{
    if (NewTarget)
    {
        //ロックON：プレイヤーをコントローラーのYawに従わせる
        bUseControllerRotationYaw = true;
        GetActionMovementComponent()->bOrientRotationToMovement = false;

        //カメラを横にずらしてプレイヤーとターゲット両方映す
        //CameraBoom->SocketOffset = FVector(
        //	DefaultSocketOffset.X,
        //	DefaultSocketOffset.Y + LockOnCameraOffsetY,
        //	DefaultSocketOffset.Z
        //);

       // GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, FString::Printf(TEXT("ロックオン： %s"), *NewTarget->GetName()));
    }
    else
    {
        //ロックオンOFF：通常の移動方向向きに戻す
        bUseControllerRotationYaw = false;
        GetActionMovementComponent()->bOrientRotationToMovement = true;

        //ソケットオフセットを元に戻す
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

    //左右オフセット
    FVector ToTarget = (TargetLocation - PlayerLocation).GetSafeNormal();
    float RightDot = FVector::DotProduct(FollowCamera->GetRightVector(), ToTarget);

    //中央付近を0付近に収束させる。SideBlendRange を大きくするほど緩やかになる
    float SideRatio = FMath::Clamp(RightDot / m_SideBlendRange, -1.f, 1.f);
    float DesiredOffsetY = DefaultSocketOffset.Y + m_LockOnCameraOffsetY * SideRatio;

    //急にオフセットが変わるとき滑らかに移動
    FVector NewOffset = CameraBoom->SocketOffset;
    NewOffset.Y = FMath::FInterpTo(NewOffset.Y, DesiredOffsetY, DeltaTime, 6.f);
    CameraBoom->SocketOffset = NewOffset;

    //カメラ自身の位置ではなく SpringArm の根元を基準に注視点を設定
    FVector Pivot = CameraBoom->GetComponentLocation();
    FVector FocusPoint = FMath::Lerp(PlayerLocation, TargetLocation, m_TargetFocusBias);

    const FVector ToFocus = FocusPoint - Pivot;
    //水平距離がほぼ0だとYawが定義できず暴れる
    if (ToFocus.SizeSquared2D() < FMath::Square(20.f)) return;

    FRotator TargetRot = ToFocus.Rotation();

    //距離に応じて補完速度を変える
    float Distance = FVector::Dist(PlayerLocation, TargetLocation);
    float InterpSpeed = FMath::GetMappedRangeValueClamped(
        FVector2D(200.f, 1500.f),  //距離レンジ
        FVector2D(m_LockOnInterpSpeedNear, m_LockOnCameraInterpSpeed),  //対応する補完速度レンジ
        Distance
    );

    //極端な見上げ・見下ろしを防ぐ
    FRotator NewRot = FMath::RInterpTo(Controller->GetControlRotation(), TargetRot, DeltaTime, InterpSpeed);
    NewRot.Pitch = FMath::Clamp(NewRot.Pitch, m_LockOnPitchMin, m_LockOnPitchMax);
    NewRot.Roll = 0.f;
    Controller->SetControlRotation(NewRot);

    //プレイヤーの向きを敵の方向に固定
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
    //入力値を2Dベクトルとして取得
    const FVector2D MovementVector = Value.Get<FVector2D>();

    if (!Controller) return;

    const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
    const FVector Forward = FRotationMatrix(Yaw).GetUnitAxis(EAxis::X);
    const FVector Right = FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y);

    //攻撃の向き補正用に、入力方向は常に記録しておく
    if (CombatComponent)
    {
        CombatComponent->SetDesiredDirection(Forward * MovementVector.Y + Right * MovementVector.X);
    }

    //攻撃中は移動しない
    if (CombatComponent && CombatComponent->IsPlayingAttackMontage()) return;

    //のけぞり中は移動しない
    if (m_bIsHitStunned) return;

    //回避中は移動しない（既存の処理）
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
        if(!MC->IsSprinting()) MC->SetSprinting(true);
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

    //現在のジャンプ回数を確認
    //JumpCurrentCount はACharacterに標準で用意されている「現在何回目のジャンプか」を持つ変数
    if (JumpCurrentCount == 1)
    {
        //【1段目のジャンプ時の処理】
        MoveComp->AirControl = MoveComp->m_AirControlFirstJump;


        //地面を蹴る土煙のエフェクト(Niagara)を足元に出す
        //「ハッ！」という通常ジャンプのボイスやSEを再生する
    }
    else if (JumpCurrentCount == 2)
    {
        //【2段目のジャンプ（エアハイク）時の処理】
        MoveComp->AirControl = MoveComp->m_AirControlSecondJump;

        FVector Vel = MoveComp->Velocity;
        Vel.Z = MoveComp->m_SecondJumpZVelocity;
        MoveComp->Velocity = Vel;

        //入力方向に一度だけ方向転換
        FVector InputVector = MoveComp->GetLastInputVector();
        if (!InputVector.IsNearlyZero())
        {
            FVector InputDir = InputVector.GetSafeNormal2D();
            SetActorRotation(InputDir.Rotation());

            float HorizontalSpeed = FVector(Vel.X, Vel.Y, 0.f).Size();
            MoveComp->Velocity.X = InputDir.X * HorizontalSpeed;
            MoveComp->Velocity.Y = InputDir.Y * HorizontalSpeed;
        }

        //空中に魔法陣や衝撃波のエフェクトを出す
        //キャラクターが空中でクルッと回転するような専用のモンタージュを再生する
        //Z軸（上方向）に少し追加の初速を与えて、滞空時間を伸ばす
    }
}

bool AActionCharacter::CanJumpInternal_Implementation() const
{
    //攻撃モーション中はジャンプ不可
    if (CombatComponent && CombatComponent->IsPlayingAttackMontage()) return false;
    //回復中はジャンプ不可
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

    // 二段ジャンプ中はZ速度を触らない
    if (JumpCurrentCount >= 2) return;

    //ボタンを押していた時間を算出
    float HoldDuration = GetWorld()->GetTimeSeconds() - m_JumpPressedTime;

    UActionMovementComponent* MoveComp = GetActionMovementComponent();
    if (!MoveComp) return;

    //空中でボタンが即座に離された場合、上昇速度の上限を低くカットすることで小ジャンプを表現する
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

    //着地復帰処理
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

    //【着地時の処理】
    //着地した瞬間に「ドスッ」という重いSEと、足元に砂埃エフェクトを出す
    //高い場所から落ちた場合（落下速度 Z が一定以上だった場合）、数フレームだけ移動入力を無視して「着地硬直」のアニメーションを入れる
}

// -----------------------------------------------------------------------
// 分身の召喚
// -----------------------------------------------------------------------
void AActionCharacter::SummonGhost()
{
    //ストック不足チェック（召喚にはストック1本が必要）
    if (GetEnergyStock() < 1)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("Not enough energy stock"));
        return;
    }

    //最大数チェック
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

    //直近20秒の行動記録を取得
    const TArray<FGhostActionData> Snapshot = GhostRecorder->GetSnapshot(20.f);
    if (Snapshot.IsEmpty())
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("NO Data MoveMore"));
        return;
    }

    //スポーン位置（プレイヤーの左横）
    const FVector  SpawnLoc = GetActorLocation() + GetActorRightVector() * -100.f;
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

    //記録データを渡して行動開始
    Ghost->InitializeGhost(Snapshot, GhostRecorder);

    //分身が消えたらリストから外す
    Ghost->OnDestroyed.AddDynamic(this, &AActionCharacter::OnGhostDestroyed);

    //分身の攻撃でもエネルギーを得る
    if (Ghost->CombatComponent)
    {
        Ghost->CombatComponent->OnHitEnemy.AddUObject(this, &AActionCharacter::OnGhostHitEnemy);
    }

    //分身の状態（消える直前など）の変化をUIに伝える
    if (Ghost->EnemyComponent)
    {
        Ghost->EnemyComponent->OnStateChanged.AddDynamic(this, &AActionCharacter::OnGhostStateChanged);
    }

    //ストックを1本消費
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

    //構え開始：移動速度を落とす
    m_HealPhase = EHealPhase::Windup;
    if (UActionMovementComponent* Move = Cast<UActionMovementComponent>(GetCharacterMovement()))
    {
        Move->SetSpeedMultiplier(m_HealMoveSpeedRate);
    }
    //TODO：ここで回復モーション（瓶を飲む）を再生する

    GetWorldTimerManager().SetTimer(m_HealTimer, this, &AActionCharacter::ApplyHeal, m_HealWindupTime, false);

    if (m_bDebugHP) GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Green, TEXT("Heal Start..."));
}

void AActionCharacter::ApplyHeal()
{
    //ここで初めて回数を消費して回復
    --m_iHealCount;
    SetHP(m_CurrentHP + m_HealAmount);
    OnHealCountChanged.Broadcast(m_iHealCount, m_iMaxHealCount);

    if (m_bDebugHP)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green,
            FString::Printf(TEXT("Heal +%.0f  (Left %d / %d)"), m_HealAmount, m_iHealCount, m_iMaxHealCount));
    }

    //後隙：速度は落としたまま
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
    //TODO：回復モーションを止める（中断時）
}

void AActionCharacter::InterruptHeal()
{
    if (!IsHealing()) return;

    if (m_bDebugHP)
    {
        //構え中なら失敗、後隙中なら動作だけ中断（回復は済んでいる）
        const bool bFailed = (m_HealPhase == EHealPhase::Windup);
        GEngine->AddOnScreenDebugMessage(-1, 2.f, bFailed ? FColor::Red : FColor::Yellow,
            bFailed ? TEXT("Heal Failed (Not consumed)") : TEXT("Heal Motion Interrupted"));
    }
    EndHeal();
}
void AActionCharacter::RefillHeals()
{
    m_iHealCount = m_iMaxHealCount;
    OnHealCountChanged.Broadcast(m_iHealCount, m_iMaxHealCount);
}
