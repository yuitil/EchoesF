// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EnemyManager.generated.h"

class AEnemyCharacterBase;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnEventEnemyDefeated, AEnemyCharacterBase*);

/**
 * 
 */

UCLASS()
class ECHO_API UEnemyManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// イベント対象の敵が死亡した際に呼び出す
	void NotifyEventEnemyDefeated(AEnemyCharacterBase* _enemy);

	AEnemyCharacterBase* CreateEnemy(TSubclassOf<AEnemyCharacterBase> _enemyClass, FTransform _transform);
	AEnemyCharacterBase* CreateEnemy(TSubclassOf<AEnemyCharacterBase> _enemyClass, FTransform _transform,bool _eventflag);
	
	// 死亡通知Delegate
	FOnEventEnemyDefeated OnEventEnemyDefeated;
};
