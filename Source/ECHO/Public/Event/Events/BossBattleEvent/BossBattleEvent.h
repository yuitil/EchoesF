// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Event/Events/EventBase.h"
#include "BossBattleEvent.generated.h"

/**
 * 
 */
class AEnemyCharacterBase;
class ALevelSequenceActor;

UCLASS()
class ECHO_API UBossBattleEvent : public UEventBase
{
	GENERATED_BODY()
	
public:
	virtual void Enter(const FEventData& _data) override;
	virtual void Update(float _deltaTime) override;
	virtual void Finish() override;

	UFUNCTION()
	void OnEventEnemyDead(AEnemyCharacterBase* _enemy);

private:
	// ボスを生成
	void SpawnBoss();

	// イベントクリア判定
	void CheckEventClear();

	// クリアWidgetを表示
	void ShowClearWidget();

	// クリア時のスローモーション開始
	void StartClearSlowMotion();

	// スローモーション終了
	UFUNCTION()
	void EndClearSlowMotion();

	// ボス登場シーケンスを再生
	void PlayBossIntroSequence();

	// ボス登場シーケンス終了
	UFUNCTION()
	void OnBossIntroSequenceFinished();
	
	// ボス撃破シーケンスを再生
	void PlayBossFinishSequence();

	// ボス撃破シーケンス終了
	UFUNCTION()
	void OnBossFinishSequenceFinished();

	// ボス撃破シーケンスが終了したか
	bool m_IsBossFinishFinished = false;
protected:
	// 生成するボスクラス
	UPROPERTY(EditAnywhere, Category = "Param")
	TSubclassOf<AEnemyCharacterBase> m_BossClass;

private:
	// 生成したボス
	UPROPERTY()
	TObjectPtr<AEnemyCharacterBase> m_SpawnedBoss = nullptr;

	// ボスが死亡したか
	bool m_IsBossDead = false;

	// イベントクリア処理済みか
	bool m_IsCleared = false;

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
	
	// ボス登場シーケンスが終了したか
	bool m_IsBossIntroFinished = false;
};
