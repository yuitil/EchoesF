#include "Integration/PlayerBridge.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"
#include "BrainComponent.h"

APlayerBridge::APlayerBridge()
{
	PrimaryActorTick.bCanEverTick = false;
}

void APlayerBridge::BeginPlay()
{
	Super::BeginPlay();
	m_health = FMath::Max(1.f, m_maxHealth);
	
	if (!m_bUseDemoHealth) { return; }
	m_observedPlayer = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (m_observedPlayer)
	{
		m_observedPlayer->OnTakeAnyDamage.AddDynamic(this, &APlayerBridge::Damaged);
	}
	m_onHealthChanged.Broadcast(m_health, m_maxHealth);
	
}

void APlayerBridge::EndPlay(const EEndPlayReason::Type _reason)
{
	if (m_observedPlayer)
	{
		m_observedPlayer->OnTakeAnyDamage.RemoveDynamic(this, &APlayerBridge::Damaged);
		Super::EndPlay(_reason);
	}
}

void APlayerBridge::Damaged(AActor * _actor, float _damage, const UDamageType * _damageType, AController * _instigator, AActor * _causer)
{
	if (!HasAuthority() || !m_bUseDemoHealth || !FMath::IsFinite(_damage) || _damage <= 0 || m_health <= 0 || _causer != m_boss) { return; }
	m_health = FMath::Clamp(m_health - _damage, 0.f, m_maxHealth);
	m_onHealthChanged.Broadcast(m_health, m_maxHealth);
	if (m_health <= 0)
	{
		if (m_boss)
		{
			m_boss->CancelAction();
			if (AAIController* controller = Cast<AAIController>(m_boss->GetController()))
			{
				controller->StopMovement();

				if (controller->GetBrainComponent())
				{
					controller->GetBrainComponent()->StopLogic(TEXT("Player Down"));
				}
			}
		}
		m_playerDown.Broadcast();
	}
}


