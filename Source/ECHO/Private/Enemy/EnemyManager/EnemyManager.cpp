// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/EnemyManager/EnemyManager.h"

#include "WorldSettings/EchoesWorldSettings.h"
#include "Enemy/Core/EnemyCharacterBase.h"


bool UEnemyManager::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer)) return false;

	if (UWorld* WorldOuter = Cast<UWorld>(Outer))
	{
		if (AEchoesWorldSettings* MyWorldSettings = Cast<AEchoesWorldSettings>(WorldOuter->GetWorldSettings()))
		{
			return MyWorldSettings->bUseEventSystem;
		}
	}

	return false;
}

void UEnemyManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 60.0f, FColor::Blue, TEXT("UEnemyManager"));
	}
}

void UEnemyManager::Deinitialize()
{
	Super::Deinitialize();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 60.0f, FColor::Blue, TEXT("UNpcManager::Deinitialize!!!"));
	}
}

void UEnemyManager::NotifyEventEnemyDefeated(AEnemyCharacterBase* _enemy)
{
	if (!_enemy)return;

	OnEventEnemyDefeated.Broadcast(_enemy);
}

AEnemyCharacterBase* UEnemyManager::CreateEnemy(TSubclassOf<AEnemyCharacterBase> _enemyClass, FTransform _transform)
{
	if (!_enemyClass) return nullptr;

	UWorld* World = GetWorld();
	if (!World) return nullptr;

	FActorSpawnParameters SpawnParams;

	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AEnemyCharacterBase* Enemy =
		World->SpawnActor<AEnemyCharacterBase>(
			_enemyClass,
			_transform,
			SpawnParams
		);

	return Enemy;
}

AEnemyCharacterBase* UEnemyManager::CreateEnemy(TSubclassOf<AEnemyCharacterBase> _enemyClass, FTransform _transform,
                                                bool _eventflag)
{
	AEnemyCharacterBase* Enemy = CreateEnemy(_enemyClass, _transform);

	if (!Enemy)return nullptr;

	Enemy->SetIsEventEnemy(_eventflag);

	return Enemy;
}
