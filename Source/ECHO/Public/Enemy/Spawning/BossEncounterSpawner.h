#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Enemy/Core/EnemyData.h"
#include "BossEncounterSpawner.generated.h"

class AEnemyCharacterBase;
class UBoxComponent;
class USceneComponent;

//ボスの生成から戦闘終了までの状態。
UENUM(BlueprintType)
enum class EBossEncounterState : uint8
{
	//待機中。
	Idle,
	//戦闘中。
	Running,
	//戦闘完了。
	Completed,
	//生成に失敗。
	Failed
};

//状態の変化をBPや他の処理へ知らせる通知を用意する。
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBossSpawnEvent, AEnemyCharacterBase*, boss);
//状態の変化をBPや他の処理へ知らせる通知を用意する。
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBossEncounterEvent, EBossEncounterState, state, FString, message);

//入口に来たプレイヤーを検知し、ボスを一体出す。
UCLASS()
class ECHO_API ABossEncounterSpawner : public AActor
{
	GENERATED_BODY()

public:
	//入口に来たプレイヤーを検知し、ボスを一体出す。
	ABossEncounterSpawner();

public:
	//ボスを生成して戦闘を開始する。
	UFUNCTION(BlueprintCallable, Category = "Boss | Spawn")
	bool StartEncounter();

	//次の戦闘に向けて状態を初期化する。
	UFUNCTION(BlueprintCallable, Category = "Boss | Spawn")
	void ResetEncounter();

	//生成したボスを返す。
	UFUNCTION(BlueprintPure, Category = "Boss | Spawn")
	AEnemyCharacterBase* GetBoss() const;

public:
	//ボス戦を開始する範囲。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss | Spawn", meta = (DisplayName = "Start Trigger"))
	TObjectPtr<UBoxComponent> m_trigger;

	//生成位置の基準にするアクター。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss | Spawn", meta = (DisplayName = "Spawn Anchor"))
	TObjectPtr<USceneComponent> m_spawnAnchor;

	//生成するボスのBPクラス。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss | Spawn", meta = (DisplayName = "Boss Class"))
	TSubclassOf<AEnemyCharacterBase> m_bossClass;

	//ボスを生成する位置。
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Boss | Spawn", meta = (DisplayName = "Spawn Point"))
	TObjectPtr<AActor> m_spawnPoint;

	//ゲーム開始時に自動生成するか。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss | Spawn", meta = (DisplayName = "Start On Begin Play"))
	bool m_bAutoStart = false;

	//歩ける場所へ生成位置を補正するか。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss | Spawn", meta = (DisplayName = "Require Nav Mesh"))
	bool m_navSpawn = false;

	//段差移動に必要な空間の余裕。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss | Spawn", meta = (DisplayName = "Player Clearance", ClampMin = "0"))
	float m_clearance = 250.f;

	//プレイヤーが入ったら生成するか。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss | Spawn", meta = (DisplayName = "Start On Enter"))
	bool m_bStartOnEnter = true;

	//現在の状態。
	UPROPERTY(BlueprintReadOnly, Category = "Boss | Spawn", meta = (DisplayName = "State"))
	EBossEncounterState m_state = EBossEncounterState::Idle;

	//ボスを生成した時の通知。
	UPROPERTY(BlueprintAssignable, Category = "Boss | Spawn", meta = (DisplayName = "On Boss Spawned"))
	FBossSpawnEvent m_onBossSpawned;

	//戦闘の進行状態が変わった時の通知。
	UPROPERTY(BlueprintAssignable, Category = "Boss | Spawn", meta = (DisplayName = "On Encounter Changed"))
	FBossEncounterEvent m_onEncounterChanged;

protected:
	//ゲーム開始時に必要な状態を準備する。
	virtual void BeginPlay() override;

	//終了時にタイマーや登録を解除する。
	virtual void EndPlay(const EEndPlayReason::Type _reason) override;

private:
	//ボス戦の進行状態を切り替える。
	void ChangeState(EBossEncounterState _state, const FString& _message);

	//生成したボスの登録と通知を解除する。
	void ClearBoss();

private:
	//ボスの状態から戦闘完了を確認する。
	UFUNCTION()
	void BossStateChanged(EEnemyState _state);

	//ボスが消えた後の登録を解除する。
	UFUNCTION()
	void BossDestroyed(AActor* _actor);

	//プレイヤーの侵入を受けて生成を開始する。
	UFUNCTION()
	void PlayerEntered(UPrimitiveComponent* _component, AActor* _actor, UPrimitiveComponent* _otherComp, int32 _bodyIndex, bool _fromSweep, const FHitResult& _hit);

private:
	//ボスの生成やリセットの処理中か。
	bool m_bBusy = false;

	//生成処理を終了中か。
	bool m_bEnding = false;

private:
	//対象のボス。
	UPROPERTY(Transient)
	TObjectPtr<AEnemyCharacterBase> m_boss;
};
