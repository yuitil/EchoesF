#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyTraversal.generated.h"

//通れる段差を調べて、登る動きや降りる動きにつなぐ。
UCLASS(ClassGroup=(Enemy), meta=(BlueprintSpawnableComponent))
class ECHO_API UEnemyTraversal : public UActorComponent
{
	GENERATED_BODY()

public:
	//通れる段差を調べて、登る動きや降りる動きにつなぐ。
	UEnemyTraversal();

public:
	//段差を移動中か返す。
	UFUNCTION(BlueprintPure, Category="Traversal")
	bool IsTraversing() const { return m_running; }

	//越えられる段差を探して移動を開始する。
	UFUNCTION(BlueprintCallable, Category="Traversal")
	bool TryTraverse(AActor* target);

public:
	//この機能を使うか。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal")
	bool m_enabled = true;

	//登れる段差の高さ。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal", meta=(ClampMin="0", DisplayName="Max Climb Height"))
	float m_up = 140.f;

	//降りられる段差の高さ。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal", meta=(ClampMin="0", DisplayName="Max Drop Height"))
	float m_down = 200.f;

	//段差移動を試す相手までの距離。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal", meta=(ClampMin="50", DisplayName="Search Distance"))
	float m_range = 360.f;

	//段差を調べる前方の距離。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal", meta=(ClampMin="10", DisplayName="Obstacle Distance"))
	float m_probe = 130.f;

	//段差移動に必要な空間の余裕。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal", meta=(ClampMin="10", DisplayName="Arc Clearance"))
	float m_clearance = 65.f;

	//移動や再生に使う速さ。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal", meta=(ClampMin="100", DisplayName="Max Horizontal Speed"))
	float m_speed = 700.f;

	//段差を調べる間隔。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal", meta=(ClampMin="0.1", DisplayName="Retry Interval"))
	float m_interval = .4f;

protected:
	//経過時間に合わせて敵の状態を更新する。
	virtual void TickComponent(float _delta, ELevelTick _type, FActorComponentTickFunction* _function) override;

private:
	//段差を越える移動中か。
	bool m_running = false;

	//次に段差を調べる時刻。
	double m_next = 0.;

	//段差移動を始めた時刻。
	double m_started = 0.;
};
