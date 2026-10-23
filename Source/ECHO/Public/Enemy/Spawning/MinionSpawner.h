#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MinionSpawner.generated.h"

class AEnemyCharacterBase;
//
UCLASS()
class ECHO_API AMinionSpawner : public AActor
{
    GENERATED_BODY()
public:
    AMinionSpawner();
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawn", meta=(DisplayName="Enemy BP"))
    TSubclassOf<AEnemyCharacterBase> m_enemyClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawn", meta=(ClampMin="1", ClampMax="100", DisplayName="Count"))
    int32 m_count = 3;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawn", meta=(ClampMin="0", DisplayName="Radius"))
    float m_radius = 350.f;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Spawn", meta=(DisplayName="Spawn Points"))
    TArray<TObjectPtr<AActor>> m_points;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawn", meta=(DisplayName="Auto Start"))
    bool m_autoStart = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawn", meta=(DisplayName="Require NavMesh"))
    bool m_requireNav = true;
    UPROPERTY(BlueprintReadOnly, Category="Spawn")
    int32 m_spawned = 0;
    UFUNCTION(BlueprintCallable, Category="Spawn")
    void Spawn();
    UFUNCTION(BlueprintCallable, Category="Spawn")
    void Reset();
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type reason) override;
private:
    bool m_started = false;
    UPROPERTY(Transient)
    TArray<TWeakObjectPtr<AEnemyCharacterBase>> m_enemies;
};
//
