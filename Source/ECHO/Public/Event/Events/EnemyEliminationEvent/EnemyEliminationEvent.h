#pragma once

#include "CoreMinimal.h"
#include "Event/Events/EventBase.h"
#include "EnemyEliminationEvent.generated.h"

class AEnemyCharacterBase;

UCLASS()
class ECHO_API UEnemyEliminationEvent : public UEventBase
{
	GENERATED_BODY()

public:
	virtual void Enter(const FEventData& _data) override;
	virtual void Update(float _deltaTime) override;
	virtual void Finish() override;

	UFUNCTION()
	void OnEventEnemyDead(AEnemyCharacterBase* _enemy);

private:
	// クリアWidgetを表示
	void ShowClearWidget();
	
	void StartClearSlowMotion();

	UFUNCTION()
	void EndClearSlowMotion();
	
	void SpawnNextEnemy();
	void FillEnemies();
	void CheckEventClear();
	
	void DestroyAllSpawnedEnemies();
protected:
	// イベント全体で生成する敵の数
	UPROPERTY(EditAnywhere, Category = "Param", meta = (ClampMin = "1"))
	int32 m_EnemySpawnCount = 1;

	// 同時に場に存在できる敵の最大数
	UPROPERTY(EditAnywhere, Category = "Param", meta = (ClampMin = "1"))
	int32 m_ActiveEnemyCount = 1;

	// 出現する敵のクラス一覧
	UPROPERTY(EditAnywhere, Category = "Param")
	TArray<TSubclassOf<AEnemyCharacterBase>> m_EventEnemys;

	// 次に使用するイベントポイント
	UPROPERTY(VisibleAnywhere, Category = "Param")
	int32 m_PointIndex = 0;

private:
	// 現在生存している敵の数
	int32 m_CurrentEnemyCount = 0;

	// これまでに生成した敵の総数
	int32 m_TotalSpawnedCount = 0;

	// 次に使用する敵クラスのインデックス
	int32 m_EnemyIndex = 0;
	

	// このイベントで生成した敵
	UPROPERTY()
	TArray<TObjectPtr<AEnemyCharacterBase>> m_SpawnedEnemies;

private:
    // クリア時のスロー倍率
    UPROPERTY(EditAnywhere, Category = "Param|SlowMotion",
        meta = (ClampMin = "0.01", ClampMax = "1.0"))
    float m_ClearTimeDilation = 0.3f;

    // スロー継続時間（現実時間）
    UPROPERTY(EditAnywhere, Category = "Param|SlowMotion",
        meta = (ClampMin = "0.0"))
    float m_ClearSlowDuration = 0.8f;

	// クリア時に表示するWidget
	UPROPERTY(EditAnywhere, Category = "Param|Clear")
	TSubclassOf<UUserWidget> m_ClearWidgetClass;
	
	FTimerHandle m_ClearSlowTimerHandle;
};