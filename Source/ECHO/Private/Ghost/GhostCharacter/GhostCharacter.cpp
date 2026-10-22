// GhostCharacter.cpp

#include "Ghost/GhostCharacter/GhostCharacter.h"
#include "Ghost/GhostCharacter/GhostPlaybackComponent.h"
#include "Ghost/GhostEnemy/GhostEnemyComponent.h"
#include "Ghost/AbilitySystem/GhostAbilitySystemComponent.h"
#include "Ghost/AutonomousGhost/AutonomousGhostAIController.h"
#include "Player/GhostRecorderComponent.h"
#include "Player/ActionMovementComponent.h"
#include "Player/CombatComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"

AGhostCharacter::AGhostCharacter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UActionMovementComponent>(
        ACharacter::CharacterMovementComponentName)),
    m_bGASInitialized(false)
{
    // -----------------------------------------------------------------------
    // GAS
    // -----------------------------------------------------------------------
    GhostASC = CreateDefaultSubobject<UGhostAbilitySystemComponent>(TEXT("GhostASC"));
    GhostASC->SetIsReplicated(false);

    // -----------------------------------------------------------------------
    // コンポーネント
    // -----------------------------------------------------------------------
    PlaybackComponent = CreateDefaultSubobject<UGhostPlaybackComponent>(TEXT("PlaybackComponent"));
    CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent_New"));
    EnemyComponent = CreateDefaultSubobject<UGhostEnemyComponent>(TEXT("EnemyComponent"));

    // -----------------------------------------------------------------------
    // CharacterMovement 設定
    // AddMovementInput が効くためにプレイヤーと同じ設定にする
    // -----------------------------------------------------------------------
    if (UCharacterMovementComponent* CMC = GetCharacterMovement())
    {
        // 移動方向に自動で向く（これがないと Ghost が正面を向いたまま横移動する）
        CMC->bOrientRotationToMovement = true;
        CMC->RotationRate = FRotator(0.f, 1000.f, 0.f);
    }

    // コントローラー回転でキャラが回らないように
    bUseControllerRotationYaw = false;
    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;

    // EnemyMaterial は BP で設定するため null 初期化
    EnemyMaterial = nullptr;

    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    AIControllerClass = AAIController::StaticClass();

    //剣を右手のソケットに付ける
    WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
    WeaponMesh->SetupAttachment(GetMesh(), TEXT("WeaponSocket"));
    WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AGhostCharacter::BeginPlay()
{
    Super::BeginPlay();

    // GAS 初期化（二重呼び出し防止）
    if (!m_bGASInitialized && GhostASC)
    {
        GhostASC->InitAbilityActorInfo(this, this);
        GrantDefaultAbilities();
        m_bGASInitialized = true;
    }
}

// -----------------------------------------------------------------------
// IAbilitySystemInterface
// -----------------------------------------------------------------------
UAbilitySystemComponent* AGhostCharacter::GetAbilitySystemComponent() const
{
    return GhostASC;
}

// -----------------------------------------------------------------------
// GrantDefaultAbilities
// -----------------------------------------------------------------------
void AGhostCharacter::GrantDefaultAbilities()
{
    if (!GhostASC) return;
    if (AttackAbilityClass) GhostASC->GiveAbilityIfNotExists(AttackAbilityClass);
    if (DodgeAbilityClass)  GhostASC->GiveAbilityIfNotExists(DodgeAbilityClass);
}

// -----------------------------------------------------------------------
// InitializeGhost
// GhostManagerComponent or ActionCharacter から召喚後に呼ばれる
// -----------------------------------------------------------------------
void AGhostCharacter::InitializeGhost(
    const TArray<FGhostActionData>& Snapshot,
    UGhostRecorderComponent* Recorder)
{
    // SpawnActor 直後・BeginPlay より前に呼ばれる場合の安全対策
    if (!m_bGASInitialized && GhostASC)
    {
        GhostASC->InitAbilityActorInfo(this, this);
        GrantDefaultAbilities();
        m_bGASInitialized = true;
    }

    if (AAutonomousGhostAIController* AutoAI = Cast<AAutonomousGhostAIController>(GetController()))
    {
        //自律型：記録データからタイプを決めてBTを起動
        AutoAI->StartFromSnapshot(Snapshot);
    }
    else if (PlaybackComponent)
    {
        //再生型：記録をそのまま再生
        PlaybackComponent->Initialize(Snapshot, Recorder);
    }

    if (EnemyComponent)
    {
        EnemyComponent->StartLifetimeTimer();
    }
}

// -----------------------------------------------------------------------
// ActivateEnemyAI
// GhostEnemyComponent::Corrupt() から呼ばれる
// ① メッシュマテリアルを敵色に変更
// ② AIController をスポーンして Possess
// -----------------------------------------------------------------------
void AGhostCharacter::ActivateEnemyAI()
{
    // ① マテリアル変更（BP で EnemyMaterial が設定されていれば全スロットに適用）
    if (EnemyMaterial && GetMesh())
    {
        const int32 NumMaterials = GetMesh()->GetNumMaterials();
        for (int32 i = 0; i < NumMaterials; i++)
        {
            GetMesh()->SetMaterial(i, EnemyMaterial);
        }
   /*     UE_LOG(LogTemp, Log, TEXT("[GhostCharacter] マテリアルを敵色に変更しました。"));*/
    }
    else
    {
       /* UE_LOG(LogTemp, Log,
            TEXT("[GhostCharacter] EnemyMaterial 未設定。BP_GhostCharacter の Ghost|Visual で設定してください。"));*/
    }

    // ② AIController スポーン
    if (!GhostAIControllerClass)
    {
   /*     UE_LOG(LogTemp, Warning,
            TEXT("[GhostCharacter] GhostAIControllerClass 未設定。敵化後に動きません。"));*/
        return;
    }

    if (Controller)
    {
        Controller->UnPossess();
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AAIController* NewAI = GetWorld()->SpawnActor<AAIController>(
        GhostAIControllerClass, GetActorTransform(), Params);

    if (NewAI)
    {
        NewAI->Possess(this);
        UE_LOG(LogTemp, Log, TEXT("[GhostCharacter] 敵化完了 : AIController をアタッチしました。"));
    }
}