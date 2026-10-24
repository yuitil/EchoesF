#pragma once
#include "CoreMinimal.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "MinionEnemyCharacter.generated.h"
class UMinionFormation;
class UMinionParryResponse;

//雑魚敵の攻撃、ひるみ、死亡を管理する。
UCLASS()
class ECHO_API AMinionEnemyCharacter : public AEnemyCharacterBase
{
	GENERATED_BODY()

public:
	//雑魚敵の攻撃、ひるみ、死亡を管理する。
	AMinionEnemyCharacter();

	//受け流された後の停止を解除する。
	void FinishParryReaction(UAnimMontage* _montage,float _duration);

	//攻撃を防がれた結果を受け取る。
	virtual void AttackDefended_Implementation(AActor* _defender, EDefenseResult _result) override;

public:
	//集団戦での役割を管理する部品。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy")
	TObjectPtr<UMinionFormation> m_formation;

	//受け流された後の隙を管理する部品。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy")
	TObjectPtr<UMinionParryResponse> m_parry;

	//死亡後に残す時間。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Death", meta=(ClampMin="0.1"))
	float m_corpseTime = 2.f;

	//ダメージを受けた時のモンタージュ。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Hit")
	TObjectPtr<UAnimMontage> m_hitMontage;

	//被弾の動作を再生する間隔。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Hit", meta=(ClampMin="0"))
	float m_hitCooldown = 1.f;

	//被弾で攻撃を中断するか。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Hit")
	bool m_interruptAttack = false;

protected:
	//現在の状態で選べる攻撃か調べる。
	virtual bool IsActionAllowed(const FEnemyActionData& _action) const override;

	//攻撃終了後の次の行動を準備する。
	virtual void OnAttackFinished() override;

	//行動終了後の状態を整える。
	virtual void OnActionEnded() override;

	//体力がなくなった時の処理を行う。
	virtual void HandleHealthDepleted() override;

	//ダメージを受けた後の反応を行う。
	virtual void OnAcceptedHit(AActor* _attacker) override;

private:
	//次に被弾アニメーションを再生できる時刻。
	double m_nextHit = 0.;
};
