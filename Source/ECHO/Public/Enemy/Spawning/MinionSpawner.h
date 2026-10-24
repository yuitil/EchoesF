#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MinionSpawner.generated.h"

class AEnemyCharacterBase;

//設定した場所に雑魚敵を出し、倒された敵を片付ける。
UCLASS()
class ECHO_API AMinionSpawner : public AActor
{
	GENERATED_BODY()

public:
	//設定した場所に雑魚敵を出し、倒された敵を片付ける。
	AMinionSpawner();

public:
	//設定した数と位置で雑魚敵を生成する。
	UFUNCTION(BlueprintCallable, Category="Spawn")
	void Spawn();

	//次の戦闘に向けて状態を初期化する。
	UFUNCTION(BlueprintCallable, Category="Spawn")
	void Reset();

public:
	//生成する雑魚敵のBPクラス。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawn", meta=(DisplayName="Enemy BP"))
	TSubclassOf<AEnemyCharacterBase> m_enemyClass;

	//生成する数。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawn", meta=(ClampMin="1", ClampMax="100", DisplayName="Count"))
	int32 m_count = 3;

	//判定に使う半径。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawn", meta=(ClampMin="0", DisplayName="Radius"))
	float m_radius = 350.f;

	//雑魚敵を生成する位置の一覧。
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Spawn", meta=(DisplayName="Spawn Points"))
	TArray<TObjectPtr<AActor>> m_points;

	//ゲーム開始時に自動生成するか。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawn", meta=(DisplayName="Auto Start"))
	bool m_autoStart = true;

	//歩ける場所だけに生成するか。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spawn", meta=(DisplayName="Require NavMesh"))
	bool m_requireNav = true;

	//生成を終えているか。
	UPROPERTY(BlueprintReadOnly, Category="Spawn")
	int32 m_spawned = 0;

protected:
	//ゲーム開始時に必要な状態を準備する。
	virtual void BeginPlay() override;

	//終了時にタイマーや登録を解除する。
	virtual void EndPlay(const EEndPlayReason::Type _reason) override;

private:
	//生成を開始済みか。
	bool m_started = false;

private:
	//このSpawnerが生成した敵。
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AEnemyCharacterBase>> m_enemies;
};
