#pragma once
#include "CoreMinimal.h"
#include "Enemy/Minion/MinionEnemyCharacter.h"
#include "FlyingMinion.generated.h"

//地面から高さを保ち、障害物を避けながら飛ぶ雑魚敵。
UCLASS()
class ECHO_API AFlyingMinion : public AMinionEnemyCharacter
{
	GENERATED_BODY()

public:
	//飛行中の位置を更新する。
	virtual void Tick(float _delta) override;

public:
	//出現時に再生する動作。素材はBPで選ぶ。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flight")
	TObjectPtr<UAnimMontage> m_spawnMove;

	//地面から体の中心までの高さ。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "50"))
	float m_hoverHeight = 160.f;

	//旋回中に上昇する幅。低い位置では剣が届く高さに戻る。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "0", Units = "cm"))
	float m_heightTravel = 190.f;

	//上昇して元の高さに戻るまでの秒数。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "1", Units = "s"))
	float m_flightPeriod = 5.f;

	//射撃するときに保つ距離。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "100"))
	float m_shotRange = 650.f;

	//距離調整を始めるまでの余裕。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "10"))
	float m_rangeMargin = 120.f;

	//相手の横へ動く速さ。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "0", ClampMax = "1"))
	float m_sideSpeed = .25f;

	//地面を探す距離。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "100"))
	float m_floorDepth = 1000.f;

	//壁の手前を調べる距離。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "20"))
	float m_wallDistance = 180.f;

protected:
	//出現動作の間だけ飛行を待つ。攻撃後の待ち時間とは分ける。
	double m_spawnUntil = 0.;

	//出現動作が終わってから戦闘を始める。
	virtual void BeginPlay() override;

	//死亡後は飛行を止める。
	virtual void HandleHealthDepleted() override;
};
