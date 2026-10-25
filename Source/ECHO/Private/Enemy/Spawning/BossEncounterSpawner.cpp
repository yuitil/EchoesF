#include "Enemy/Spawning/BossEncounterSpawner.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Controller.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Components/CapsuleComponent.h"
#include "TimerManager.h"

//動作を確かめるための幅や待ち時間。
namespace BossEncounterSpawnerTuning
{
    //範囲の確認を細かくしすぎないための下限。
    constexpr float minCheckDelay = 0.1f;

    //開始範囲の半分の大きさ。
    constexpr auto triggerHalfSize = 150.f;
   
	//開始範囲の横幅。
    constexpr auto triggerWidth = 200.f;
   
	//生成位置までの距離。
    constexpr auto spawnDistance = 600.f;
   
	//生成時に持ち上げる高さ。
    constexpr auto spawnHeight = 100.f;
   
	//歩ける場所を探す横幅。
    constexpr auto navWidth = 200;
    
	//歩ける場所を探す高さ。
    constexpr auto navHeight = 500;
   
	//地面との距離に持たせる余裕。
    constexpr auto floorMargin = 5.f;
}

//初期設定と必要な部品を用意する。
ABossEncounterSpawner::ABossEncounterSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	m_trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("StartTrigger"));
	SetRootComponent(m_trigger);
	m_trigger->SetBoxExtent(FVector(BossEncounterSpawnerTuning::triggerHalfSize, BossEncounterSpawnerTuning::triggerWidth, BossEncounterSpawnerTuning::triggerHalfSize));
	m_trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	m_trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	m_trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	m_trigger->SetGenerateOverlapEvents(true);

	m_spawnAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnAnchor"));
	m_spawnAnchor->SetupAttachment(m_trigger);
	m_spawnAnchor->SetRelativeLocation(FVector(BossEncounterSpawnerTuning::spawnDistance, 0.f, BossEncounterSpawnerTuning::spawnHeight));
}

//ゲーム開始時に必要な状態を準備する。
void ABossEncounterSpawner::BeginPlay()
{
	Super::BeginPlay();
	if(!HasAuthority()) { return; }
	m_trigger->OnComponentBeginOverlap.AddDynamic(this, &ABossEncounterSpawner::PlayerEntered);
	//Overlapが登録より先に発生した場合や、プレイヤーの準備が遅れた場合も確認する。
	GetWorldTimerManager().SetTimer(m_entryTimer, this, &ABossEncounterSpawner::CheckEntry,
		FMath::Max(BossEncounterSpawnerTuning::minCheckDelay, m_checkDelay), true);

	if (m_bAutoStart)
	{
		StartEncounter();
	}
}

//開始範囲内にいるかを確認する。プレイヤー側の設定変更は不要。
void ABossEncounterSpawner::CheckEntry()
{
	if(!HasAuthority() || m_bBusy || m_bEnding || IsValid(m_boss)) { return; }
	if(m_state != EBossEncounterState::Idle && !(m_state == EBossEncounterState::Failed && m_retrySpawn)) { return; }
	if(GetWorld()->GetTimeSeconds() < m_retryTime) { return; }
	if(m_bAutoStart) { StartEncounter(); return; }
	if(!m_bStartOnEnter) { return; }

	const auto* player = UGameplayStatics::GetPlayerPawn(this, 0);
	if(!IsValid(player) || !player->IsPlayerControlled()) { return; }
	//衝突チャンネルやOverlap通知に依存せず、BPで決めた箱の中か確認する。
	const FVector local = m_trigger->GetComponentTransform().InverseTransformPosition(player->GetActorLocation());
	const FVector extent = m_trigger->GetUnscaledBoxExtent();
	const bool inside = FMath::Abs(local.X) <= extent.X && FMath::Abs(local.Y) <= extent.Y && FMath::Abs(local.Z) <= extent.Z;
	if(inside || m_trigger->IsOverlappingActor(player)) { StartEncounter(); }
}

//床が未準備、プレイヤーが近い、生成場所が塞がっている場合は再試行できるようにする。
void ABossEncounterSpawner::RetrySpawn(const FString& _message)
{
	m_bBusy = false;
	m_retrySpawn = true;
	m_retryTime = GetWorld()->GetTimeSeconds() + FMath::Max(BossEncounterSpawnerTuning::minCheckDelay, m_retryDelay);
	//待機中に同じ失敗ログを大量に出さない。
	if(m_state != EBossEncounterState::Failed) { ChangeState(EBossEncounterState::Failed, _message); }
}

//プレイヤーの侵入を受けて生成を開始する。
void ABossEncounterSpawner::PlayerEntered(UPrimitiveComponent* _component, AActor* _actor, UPrimitiveComponent* _otherComp, int32 _bodyIndex, bool _fromSweep, const FHitResult& _hit)
{
	//対象のプレイヤー。
	APawn* player = Cast<APawn>(_actor);
	if (m_bStartOnEnter && player && player->IsPlayerControlled())
	{
		StartEncounter();
	}
}

//生成したボスを返す。
AEnemyCharacterBase* ABossEncounterSpawner::GetBoss() const
{
	return IsValid(m_boss) ? m_boss.Get() : nullptr;
}

//ボス戦の進行状態を切り替える。
void ABossEncounterSpawner::ChangeState(EBossEncounterState _state, const FString& _message)
{
	m_state = _state;
	UE_LOG(LogTemp, Display, TEXT("%s: %s"), *GetName(), *_message);
	m_onEncounterChanged.Broadcast(m_state, _message);
}

//ボスを生成して戦闘を開始する。
bool ABossEncounterSpawner::StartEncounter()
{
	if(!HasAuthority() || m_bBusy || m_bEnding || (m_state != EBossEncounterState::Idle && !(m_state == EBossEncounterState::Failed && m_retrySpawn)) || IsValid(m_boss)) { return false; }

	m_retrySpawn = false;
	if (!m_bossClass || m_bossClass->HasAnyClassFlags(CLASS_Abstract))
	{
		ChangeState(EBossEncounterState::Failed, TEXT("Set Boss Class to a valid enemy Blueprint."));
		return false;
	}

	m_bBusy = true;
	//生成する位置と向き。
	FTransform spawnTransform = IsValid(m_spawnPoint) ? m_spawnPoint->GetActorTransform() : m_spawnAnchor->GetComponentTransform();
	spawnTransform.SetScale3D(FVector::OneVector);
	if (m_navSpawn)
	{
		//歩ける場所を調べる機能。
		auto* nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
		
		//基準となる位置。
		FNavLocation point;
		if (!nav || !nav->ProjectPointToNavigation(spawnTransform.GetLocation(), point, FVector(BossEncounterSpawnerTuning::navWidth, BossEncounterSpawnerTuning::navWidth, BossEncounterSpawnerTuning::navHeight)))
		{
			RetrySpawn(TEXT("Waiting for NavMesh at Spawn Anchor."));
			return false;
		}
		
		//生成するクラスの初期設定。
		const auto* defaults = m_bossClass->GetDefaultObject<AEnemyCharacterBase>();
		spawnTransform.SetLocation(point.Location + FVector(0, 0, defaults->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + BossEncounterSpawnerTuning::floorMargin));
	}
	if (const auto* player = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		if (FVector::Dist2D(player->GetActorLocation(), spawnTransform.GetLocation()) < m_clearance)
		{
			RetrySpawn(TEXT("Waiting for player clearance at spawn point."));
			return false;
		}
	}

	m_boss = GetWorld()->SpawnActorDeferred<AEnemyCharacterBase>(m_bossClass, spawnTransform, this, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding);
	if (!IsValid(m_boss))
	{
		RetrySpawn(TEXT("Waiting for space at boss spawn point."));
		return false;
	}

	m_boss->m_onStateChanged.AddDynamic(this, &ABossEncounterSpawner::BossStateChanged);
	m_boss->OnDestroyed.AddDynamic(this, &ABossEncounterSpawner::BossDestroyed);
	UGameplayStatics::FinishSpawningActor(m_boss, spawnTransform);

	if (!IsValid(m_boss) || m_boss->m_state == EEnemyState::Dead)
	{
		ClearBoss();
		m_bBusy = false;
		ChangeState(EBossEncounterState::Failed, TEXT("Boss became invalid during initialization."));
		return false;
	}

	if (!m_boss->GetController())
	{
		m_boss->SpawnDefaultController();
	}

	if (!IsValid(m_boss) || !m_boss->GetController())
	{
		ClearBoss();
		m_bBusy = false;
		ChangeState(EBossEncounterState::Failed, TEXT("Boss has no controller. Check AI Controller Class."));
		return false;
	}

	m_state = EBossEncounterState::Running;
	m_onBossSpawned.Broadcast(m_boss);
	m_bBusy = false;

	if(m_bEnding) { return false; }
	if (!IsValid(m_boss))
	{
		ChangeState(EBossEncounterState::Failed, TEXT("Boss was destroyed during the spawn event."));
		return false;
	}

	if (m_boss->m_state == EEnemyState::Dead)
	{
		BossStateChanged(EEnemyState::Dead);
	}
	else
	{
		ChangeState(EBossEncounterState::Running, TEXT("Boss encounter started."));
	}
	return m_state == EBossEncounterState::Running;
}

//ボスの状態から戦闘完了を確認する。
void ABossEncounterSpawner::BossStateChanged(EEnemyState _state)
{
	if(!HasAuthority() || m_bBusy || m_bEnding || m_state != EBossEncounterState::Running || _state != EEnemyState::Dead) { return; }
	ChangeState(EBossEncounterState::Completed, TEXT("Boss defeated."));
}

//ボスが消えた後の登録を解除する。
void ABossEncounterSpawner::BossDestroyed(AActor* _actor)
{
	if(_actor != m_boss.Get()) { return; }

	m_boss = nullptr;
	if (!m_bBusy && !m_bEnding && m_state == EBossEncounterState::Running)
	{
		ChangeState(EBossEncounterState::Failed, TEXT("Boss was destroyed before final defeat."));
	}
}

//生成したボスの登録と通知を解除する。
void ABossEncounterSpawner::ClearBoss()
{
	//対象のボス。
	AEnemyCharacterBase* boss = m_boss.Get();
	m_boss = nullptr;
	if(!IsValid(boss)) { return; }

	boss->m_onStateChanged.RemoveDynamic(this, &ABossEncounterSpawner::BossStateChanged);
	boss->OnDestroyed.RemoveDynamic(this, &ABossEncounterSpawner::BossDestroyed);
	boss->CancelAction();
	if (AController* controller = boss->GetController())
	{
		controller->UnPossess();
		controller->Destroy();
	}
	boss->Destroy();
}

//次の戦闘に向けて状態を初期化する。
void ABossEncounterSpawner::ResetEncounter()
{
	if(!HasAuthority() || m_bBusy || m_bEnding) { return; }

	m_bBusy = true;
	ClearBoss();
	m_bBusy = false;
	m_retrySpawn = false;
	m_retryTime = 0.;
	ChangeState(EBossEncounterState::Idle, TEXT("Encounter reset. Waiting for start conditions."));
}

//終了時にタイマーや登録を解除する。
void ABossEncounterSpawner::EndPlay(const EEndPlayReason::Type _reason)
{
	m_bEnding = true;
	GetWorldTimerManager().ClearTimer(m_entryTimer);
	m_trigger->OnComponentBeginOverlap.RemoveDynamic(this, &ABossEncounterSpawner::PlayerEntered);
	if (HasAuthority())
	{
		ClearBoss();
	}
	Super::EndPlay(_reason);
}
