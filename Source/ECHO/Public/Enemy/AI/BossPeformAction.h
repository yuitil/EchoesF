#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BossPeformAction.generated.h"

//攻撃が届く距離へ近づき、選んだ攻撃を始める。
UCLASS()
class ECHO_API UBossPeformAction : public UBTTaskNode
{
	GENERATED_BODY()

public:
	//攻撃が届く距離へ近づき、選んだ攻撃を始める。
	UBossPeformAction();

public:
	//突進に使う攻撃名。
	UPROPERTY(EditAnywhere, Category = "Rush", meta = (DisplayName = "Rush Action"))
	FName m_rushAction;

	//退避に使う行動名。
	UPROPERTY(EditAnywhere, Category = "Combat", meta = (DisplayName = "Retreat Action"))
	FName m_retreatAction;

	//突進に切り替える区間名。
	UPROPERTY(EditAnywhere, Category = "Rush", meta = (DisplayName = "Move Section"))
	FName m_moveSection;

	//突進を検討する最大距離。
	UPROPERTY(EditAnywhere, Category = "Rush", meta = (ClampMin = "0", DisplayName = "Rush Range"))
	float m_rushRange = 800.f;

	//突進中の移動速度。
	UPROPERTY(EditAnywhere, Category = "Rush", meta = (ClampMin = "1", DisplayName = "Rush Speed"))
	float m_rushSpeed = 1400.f;

	//突進の出だしの移動速度。
	UPROPERTY(EditAnywhere, Category = "Rush", meta = (ClampMin = "0", DisplayName = "Start Speed"))
	float m_startSpeed = 400.f;

	//近接攻撃へつなぐ距離。
	UPROPERTY(EditAnywhere, Category = "Rush", meta = (ClampMin = "1", DisplayName = "Attack Distance"))
	float m_attackDist = 140.f;

	//突進できる高さの差。
	UPROPERTY(EditAnywhere, Category = "Rush", meta = (ClampMin = "0", DisplayName = "Height Limit"))
	float m_heightLimit = 80.f;

	//突進を始める最小距離。
	UPROPERTY(EditAnywhere, Category = "Rush", meta = (ClampMin = "0", DisplayName = "Rush Min Distance"))
	float m_rushMin = 280.f;

	//追跡してから突進を判断する待ち時間。
	UPROPERTY(EditAnywhere, Category = "Rush", meta = (ClampMin = "0", DisplayName = "Chase Delay"))
	float m_chaseDelay = .25f;

	//突進で短縮したい時間。
	UPROPERTY(EditAnywhere, Category = "Rush", meta = (ClampMin = "0", DisplayName = "Minimum Time Gain"))
	float m_minGain = .15f;

	//突進できる正面の角度。
	UPROPERTY(EditAnywhere, Category = "Rush", meta = (ClampMin = "1", ClampMax = "90", DisplayName = "Rush Angle"))
	float m_rushAngle = 60.f;

protected:
	//BTから呼ばれた行動を開始する。
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& _ownerComp, uint8* _nodeMemory) override;

	//進行中の行動を確認して終了を判断する。
	virtual void TickTask(UBehaviorTreeComponent& _ownerComp, uint8* _nodeMemory, float _delta) override;

	//BTの行動を中断して状態を戻す。
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& _ownerComp, uint8* _nodeMemory) override;

private:
	//選んだ攻撃を開始する。
	bool StartAttack(UBehaviorTreeComponent& _ownerComp);

	//突進で相手に届くか調べて開始する。
	bool TryDash(UBehaviorTreeComponent& _ownerComp);

	//突進で相手に追いつけるか調べる。
	bool CanCatch(class AEnemyCharacterBase* _enemy, float& _hitEnd, float& _moveStart) const;

	//突進を止めて移動設定を戻す。
	void StopRush(UBehaviorTreeComponent& _ownerComp);

private:
	//突進の移動中か。
	bool m_rushing = false;

	//突進速度へ切り替えたか。
	bool m_fast = false;

	//変更前の移動速度。
	float m_oldSpeed = 0.f;

	//変更前の加速の強さ。
	float m_oldAccel = 0.f;

	//開始した行動を見分ける番号。
	int32 m_serial = INDEX_NONE;

	//行動を待つ制限時刻。
	double m_deadline = 0.;

	//突進攻撃の判定を終える時刻。
	double m_hitEnd = 0.;

	//突進の移動を始める時刻。
	double m_moveStart = 0.;

	//次に移動先を調べる時刻。
	double m_nextCheck = 0.;

	//追跡を始めた時刻。
	double m_chaseStart = 0.;
};
