#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BossUpdateCombat.generated.h"

//ボスの相手と距離を調べ、行動の判断へ渡す。
UCLASS()
class ECHO_API UBossUpdateCombat : public UBTService
{
	GENERATED_BODY()

public:

	//ボスの相手と距離を調べ、行動の判断へ渡す。
	UBossUpdateCombat();

protected:

	//BTが判断に使う敵と相手の状態を更新する。
	virtual void TickNode(UBehaviorTreeComponent& _ownerComp, uint8* _nodeMemory, float _delta) override;

	//見失った地点の周囲を探す広さ。
	UPROPERTY(EditAnywhere, Category = "Boss|Search", meta = (ClampMin = "100"))
	float m_searchRadius = 650.f;

private:

	//相手を最後に見た位置。
	TOptional<FVector> m_lastSeen;

	//見失った相手を探しているか。
	bool m_searching = false;

	//次に周りを見回す時刻。
	double m_nextLook = 0.;
};
