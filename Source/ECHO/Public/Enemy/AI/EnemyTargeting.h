#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyTargeting.generated.h"

//プレイヤーや分身から、狙う相手を選ぶ。
UCLASS(ClassGroup = (Enemy), meta = (BlueprintSpawnableComponent))
class ECHO_API UEnemyTargeting : public UActorComponent
{
	GENERATED_BODY()

public:

	//狙う相手を選ぶ。
	UFUNCTION(BlueprintCallable, Category = "Enemy|Target")
	AActor* SelectTarget();

	//戦闘の役割を考えて雑魚敵の相手を選ぶ。
	UFUNCTION(BlueprintCallable, Category = "Enemy|Target")
	AActor* SelectMinionTarget();

public:

	//戦闘開始後、壁の向こうのプレイヤーも追う。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Target")
	bool m_trackPlayer = false;

	//本体が見える時は、分身より本体を優先する。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Target")
	bool m_preferPlayer = false;

	//一体の分身を担当する敵の上限。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Target", meta = (ClampMin = "1", ClampMax = "2"))
	int32 m_cloneLimit = 1;

	//分身に向かわせる敵の割合。本体への圧力を残す。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Target", meta = (ClampMin = "0", ClampMax = "0.75"))
	float m_cloneRatio = .5f;

	//指定された分身だけを狙う。消えた後にプレイヤーへ切り替えない。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Target")
	bool m_lockTarget = false;

	//影が担当する分身。消滅後は参照を残さない。
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Enemy|Target")
	TWeakObjectPtr<AActor> m_fixedTarget;

	//分身を優先して狙うか。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Target")
	bool m_preferClones = true;

	//プレイヤーと戦う担当を維持するか。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Target")
	bool m_keepPlayerDuel = true;

	//分身も狙うか。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Target")
	bool m_targetClones = true;

	//相手を探す距離。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Target", meta = (ClampMin = "0"))
	float m_range = 2500.f;

	//相手を見失ったと判断する距離。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Target", meta = (ClampMin = "0"))
	float m_giveUpRange = 3500.f;

	//移動経路を確認する間隔。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Target", meta = (ClampMin = "0.2"))
	float m_pathCheck = 1.f;

	//今の相手を狙い続ける選びやすさ。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Target", meta = (ClampMin = "0", ClampMax = "1"))
	float m_keepWeight = .75f;

	//遠ざかった相手を追うか。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Target")
	bool m_followTarget = false;

	//追跡を続けられる距離。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Target", meta = (ClampMin = "0"))
	float m_chaseRange = 20000.f;

public:

	//攻撃対象にできる相手か調べる。
	bool IsCandidate(AActor* _actor) const;
};
