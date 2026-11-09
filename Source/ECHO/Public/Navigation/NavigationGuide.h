#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NavigationGuide.generated.h"

class USceneComponent;
class UNiagaraComponent;
class UNiagaraSystem;

UCLASS()
class ECHO_API ANavigationGuide : public AActor
{
	GENERATED_BODY()

public:
	ANavigationGuide();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(
		const EEndPlayReason::Type EndPlayReason) override;

	// 目的地を配列に追加
	UFUNCTION(BlueprintCallable, Category = "Navigation")
	void AddDestination(AActor* NewDestination);

	// 目的地をすべて削除
	UFUNCTION(BlueprintCallable, Category = "Navigation")
	void ClearDestination();

	// 現在の目的地を取得
	UFUNCTION(BlueprintPure, Category = "Navigation")
	AActor* GetCurrentDestination() const;

	// 経路更新
	UFUNCTION(BlueprintCallable, Category = "Navigation")
	void UpdateNavigation();

protected:
	UPROPERTY(VisibleAnywhere, Category = "Navigation")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Navigation")
	TObjectPtr<AActor> TargetPlayer;

	// 目的地の配列
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Navigation")
	TArray<TObjectPtr<AActor>> Destinations;

	// 各地点に配置するNiagara
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Navigation|Visual")
	TObjectPtr<UNiagaraSystem> NavigationEffect;

	// プレイヤーから何cm先まで表示するか
	UPROPERTY(EditAnywhere, Category = "Navigation|Param", meta = (ClampMin = "100.0"))
	float MaxDisplayDistance = 1500.0f;

	// エフェクトの配置間隔
	UPROPERTY(EditAnywhere, Category = "Navigation|Param", meta = (ClampMin = "10.0"))
	float ParticleSpacing = 100.0f;

	// 地面からの高さ
	UPROPERTY(EditAnywhere, Category = "Navigation|Param")
	float HeightOffset = 10.0f;

	// 経路更新間隔
	UPROPERTY(EditAnywhere, Category = "Navigation|Param", meta = (ClampMin = "0.05"))
	float UpdateInterval = 0.3f;

	// 到達判定
	UPROPERTY(EditAnywhere, Category = "Navigation|Param", meta = (ClampMin = "0.0"))
	float ArrivalDistance = 150.0f;

private:
	// イベント開始通知
	UFUNCTION()
	void OnStageEventStarted();

	// イベント終了通知
	UFUNCTION()
	void OnStageEventEnded();

	// 次の目的地へ進む
	void AdvanceDestination();

	// プレイヤーの取得
	void ResolveTargetPlayer();

	// 経路上にエフェクトを配置する座標を生成
	void BuildParticlePositions(
		const TArray<FVector>& PathPoints,
		TArray<FVector>& OutPositions) const;

	// エフェクトの配置・更新
	void UpdateParticleEffects(const TArray<FVector>& Positions);

	// エフェクトを非表示
	void HideParticleEffects();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UNiagaraComponent>> ParticleEffects;

	// 現在の目的地インデックス
	UPROPERTY(Transient)
	int32 m_CurrentDestinationIndex = 0;

	float UpdateTimer = 0.0f;

	// イベント実行中かどうか
	bool m_IsEventActive = false;

	// 目的地に到着したかどうか
	bool m_HasArrived = false;
};
