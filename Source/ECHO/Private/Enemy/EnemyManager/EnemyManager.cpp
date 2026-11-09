//Fill out your copyright notice in the Description page of Project Settings.
#include "Enemy/EnemyManager/EnemyManager.h"

#include "WorldSettings/EchoesWorldSettings.h"
#include "Enemy/Core/EnemyCharacterBase.h"

//イベントを使うマップだけで敵を管理する。
bool UEnemyManager::ShouldCreateSubsystem(UObject* Outer) const
{
    if(!Super::ShouldCreateSubsystem(Outer)) return false;

    if(UWorld* WorldOuter = Cast<UWorld>(Outer))
    {
        if(AEchoesWorldSettings* WorldSettings = Cast<AEchoesWorldSettings>(WorldOuter->GetWorldSettings()))
        {
            return WorldSettings->bUseEventSystem;
        }
    }

    return false;
}

//マップ開始時に敵の管理を始める。
void UEnemyManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if(GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 60.0f, FColor::Blue, TEXT("UEnemyManager"));
    }
}

//マップ終了時に敵の管理を終える。
void UEnemyManager::Deinitialize()
{
    Super::Deinitialize();

    if(GEngine)
    {
        GEngine->AddOnScreenDebugMessage( -1, 60.0f, FColor::Blue, TEXT("UEnemyManager::Deinitialize!!!"));
    }
}

//イベント対象の敵が死亡した際に呼び出す
void UEnemyManager::NotifyEventEnemyDefeated(AEnemyCharacterBase* _enemy)
{
    if(!_enemy) return;

    OnEventEnemyDefeated.Broadcast(_enemy);
}

//指定の敵をマップに生成する。
AEnemyCharacterBase* UEnemyManager::CreateEnemy(
    TSubclassOf<AEnemyCharacterBase> _enemyClass, FTransform _transform)
{
    if(!_enemyClass) return nullptr;

    UWorld* World = GetWorld();
    if(!World) return nullptr;

    FActorSpawnParameters SpawnParams;

    SpawnParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AEnemyCharacterBase* Enemy = World->SpawnActor<AEnemyCharacterBase>(_enemyClass, _transform, SpawnParams);

    return Enemy;
}

//生成した敵にイベントの対象かどうかを設定する。
AEnemyCharacterBase *UEnemyManager::CreateEnemy(TSubclassOf<AEnemyCharacterBase> _enemyClass,
                                                FTransform _transform, bool _eventflag)
{
    AEnemyCharacterBase* Enemy = CreateEnemy(_enemyClass, _transform);

    if(!Enemy) return nullptr;

    Enemy->SetIsEventEnemy(_eventflag);

    return Enemy;
}
