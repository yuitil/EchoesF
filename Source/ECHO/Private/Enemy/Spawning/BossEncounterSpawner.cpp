#include "Enemy/Spawning/BossEncounterSpawner.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Controller.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Components/CapsuleComponent.h"

ABossEncounterSpawner::ABossEncounterSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	m_trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("StartTrigger"));
	SetRootComponent(m_trigger);
	m_trigger->SetBoxExtent(FVector(150.f, 200.f, 150.f));
	m_trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	m_trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	m_trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	m_trigger->SetGenerateOverlapEvents(true);

	m_spawnAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnAnchor"));
	m_spawnAnchor->SetupAttachment(m_trigger);
	m_spawnAnchor->SetRelativeLocation(FVector(600.f, 0.f, 100.f));
}

void ABossEncounterSpawner::BeginPlay()
{
	Super::BeginPlay();
	m_trigger->OnComponentBeginOverlap.AddDynamic(this, &ABossEncounterSpawner::PlayerEntered);

	if (m_bAutoStart)
	{
		StartEncounter();
	}
}

void ABossEncounterSpawner::PlayerEntered(UPrimitiveComponent* _component, AActor* _actor, UPrimitiveComponent* _otherComp, int32 _bodyIndex, bool _fromSweep, const FHitResult& _hit)
{
	APawn* player = Cast<APawn>(_actor);
	if (m_bStartOnEnter && player && player->IsPlayerControlled())
	{
		StartEncounter();
	}
}

AEnemyCharacterBase* ABossEncounterSpawner::GetBoss() const
{
	return IsValid(m_boss) ? m_boss.Get() : nullptr;
}

void ABossEncounterSpawner::ChangeState(EBossEncounterState _state, const FString& _message)
{
	m_state = _state;
	UE_LOG(LogTemp, Display, TEXT("%s: %s"), *GetName(), *_message);
	m_onEncounterChanged.Broadcast(m_state, _message);
}

bool ABossEncounterSpawner::StartEncounter()
{
	if (!HasAuthority() || m_bBusy || m_bEnding || m_state != EBossEncounterState::Idle || IsValid(m_boss))
	{
		return false;
	}

	if (!m_bossClass || m_bossClass->HasAnyClassFlags(CLASS_Abstract))
	{
		ChangeState(EBossEncounterState::Failed, TEXT("Set Boss Class to a valid enemy Blueprint."));
		return false;
	}

	m_bBusy = true;
	FTransform spawnTransform = IsValid(m_spawnPoint) ? m_spawnPoint->GetActorTransform() : m_spawnAnchor->GetComponentTransform();
	spawnTransform.SetScale3D(FVector::OneVector);
	if (m_navSpawn)
	{
		auto* nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
		FNavLocation point;
		if (!nav || !nav->ProjectPointToNavigation(spawnTransform.GetLocation(), point, FVector(200, 200, 500)))
		{
			m_bBusy = false;
			ChangeState(EBossEncounterState::Failed, TEXT("Spawn Anchor requires a built NavMesh. Reset Encounter after fixing placement."));
			return false;
		}
		const auto* defaults = m_bossClass->GetDefaultObject<AEnemyCharacterBase>();
		spawnTransform.SetLocation(point.Location + FVector(0, 0, defaults->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 5.f));
	}
	if (const auto* player = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		if (FVector::Dist2D(player->GetActorLocation(), spawnTransform.GetLocation()) < m_clearance)
		{
			m_bBusy = false;
			ChangeState(EBossEncounterState::Failed, TEXT("Spawn point is too close to the player. Reset Encounter after moving it."));
			return false;
		}
	}
	

	m_boss = GetWorld()->SpawnActorDeferred<AEnemyCharacterBase>(m_bossClass, spawnTransform, this, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding);
	if (!IsValid(m_boss))
	{
		m_bBusy = false;
		ChangeState(EBossEncounterState::Failed, TEXT("Boss spawn failed. Check the spawn position and collision."));
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

	if (m_bEnding)
	{
		return false;
	}
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

void ABossEncounterSpawner::BossStateChanged(EEnemyState _state)
{
	if (!HasAuthority() || m_bBusy || m_bEnding || m_state != EBossEncounterState::Running || _state != EEnemyState::Dead)
	{
		return;
	}
	ChangeState(EBossEncounterState::Completed, TEXT("Boss defeated."));
}

void ABossEncounterSpawner::BossDestroyed(AActor* _actor)
{
	if (_actor != m_boss.Get())
	{
		return;
	}

	m_boss = nullptr;
	if (!m_bBusy && !m_bEnding && m_state == EBossEncounterState::Running)
	{
		ChangeState(EBossEncounterState::Failed, TEXT("Boss was destroyed before final defeat."));
	}
}

void ABossEncounterSpawner::ClearBoss()
{
	AEnemyCharacterBase* boss = m_boss.Get();
	m_boss = nullptr;
	if (!IsValid(boss))
	{
		return;
	}

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

void ABossEncounterSpawner::ResetEncounter()
{
	if (!HasAuthority() || m_bBusy || m_bEnding)
	{
		return;
	}

	m_bBusy = true;
	ClearBoss();
	m_bBusy = false;
	ChangeState(EBossEncounterState::Idle, TEXT("Encounter reset. Call Start Encounter to retry."));
}

void ABossEncounterSpawner::EndPlay(const EEndPlayReason::Type _reason)
{
	m_bEnding = true;
	m_trigger->OnComponentBeginOverlap.RemoveDynamic(this, &ABossEncounterSpawner::PlayerEntered);
	if (HasAuthority())
	{
		ClearBoss();
	}
	Super::EndPlay(_reason);
}
