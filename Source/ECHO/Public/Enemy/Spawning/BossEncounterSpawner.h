#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Enemy/Core/EnemyData.h"
#include "BossEncounterSpawner.generated.h"

class AEnemyCharacterBase;
class UBoxComponent;
class USceneComponent;

UENUM(BlueprintType)
enum class EBossEncounterState : uint8
{
	Idle,
	Running,
	Completed,
	Failed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBossSpawnEvent, AEnemyCharacterBase*, boss);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBossEncounterEvent, EBossEncounterState, state, FString, message);

UCLASS()
class ECHO_API ABossEncounterSpawner : public AActor
{
	GENERATED_BODY()

public:
	ABossEncounterSpawner();

	UFUNCTION(BlueprintCallable, Category = "Boss | Spawn")
	bool StartEncounter();

	UFUNCTION(BlueprintCallable, Category = "Boss | Spawn")
	void ResetEncounter();

	UFUNCTION(BlueprintPure, Category = "Boss | Spawn")
	AEnemyCharacterBase* GetBoss() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type _reason) override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss | Spawn", meta = (DisplayName = "Start Trigger"))
	TObjectPtr<UBoxComponent> m_trigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss | Spawn", meta = (DisplayName = "Spawn Anchor"))
	TObjectPtr<USceneComponent> m_spawnAnchor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss | Spawn", meta = (DisplayName = "Boss Class"))
	TSubclassOf<AEnemyCharacterBase> m_bossClass;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Boss | Spawn", meta = (DisplayName = "Spawn Point"))
	TObjectPtr<AActor> m_spawnPoint;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss | Spawn", meta = (DisplayName = "Start On Begin Play"))
	bool m_bAutoStart = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss | Spawn", meta = (DisplayName = "Require Nav Mesh"))
	bool m_navSpawn = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss | Spawn", meta = (DisplayName = "Player Clearance", ClampMin = "0"))
	float m_clearance = 250.f;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss | Spawn", meta = (DisplayName = "Start On Enter"))
	bool m_bStartOnEnter = true;

	UPROPERTY(BlueprintReadOnly, Category = "Boss | Spawn", meta = (DisplayName = "State"))
	EBossEncounterState m_state = EBossEncounterState::Idle;

	UPROPERTY(BlueprintAssignable, Category = "Boss | Spawn", meta = (DisplayName = "On Boss Spawned"))
	FBossSpawnEvent m_onBossSpawned;

	UPROPERTY(BlueprintAssignable, Category = "Boss | Spawn", meta = (DisplayName = "On Encounter Changed"))
	FBossEncounterEvent m_onEncounterChanged;

private:
	UPROPERTY(Transient)
	TObjectPtr<AEnemyCharacterBase> m_boss;

	bool m_bBusy = false;
	bool m_bEnding = false;

	void ChangeState(EBossEncounterState _state, const FString& _message);
	void ClearBoss();

	UFUNCTION()
	void BossStateChanged(EEnemyState _state);

	UFUNCTION()
	void BossDestroyed(AActor* _actor);

	UFUNCTION()
	void PlayerEntered(UPrimitiveComponent* _component, AActor* _actor, UPrimitiveComponent* _otherComp, int32 _bodyIndex, bool _fromSweep, const FHitResult& _hit);
};
