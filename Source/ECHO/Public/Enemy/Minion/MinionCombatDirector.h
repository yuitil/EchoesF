#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MinionCombatDirector.generated.h"
class UMinionFormation;

//同じ相手を狙う敵の攻撃人数を決める。
UCLASS()
class ECHO_API UMinionCombatDirector : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	//集団戦を管理する対象に敵を登録する。
	void Register(UMinionFormation* _member);

	//集団戦を管理する対象から敵を外す。
	void Unregister(UMinionFormation* _member);

	//集団の攻撃役と待機役を更新する。
	void Update();

	//プレイヤーと戦う担当の敵を返す。
	AActor* GetPlayerGuard(AActor* _player);

	//プレイヤーと戦う担当を解除する。
	void ClearPlayerGuard(UMinionFormation* _member);

	//同じ相手へ攻撃が重ならない待ち時間を設ける。
	void PauseTarget(AActor* _target,float _duration);

private:
	//プレイヤーとの戦闘を担当する敵。
	TWeakObjectPtr<UMinionFormation> m_playerGuard;

	//戦闘担当を次に確認する時刻。
	double m_guardCheck = 0.;

	//集団戦に参加している敵。
	TArray<TWeakObjectPtr<UMinionFormation>> m_members;

	//次に集団の役割を更新する時刻。
	double m_nextUpdate = 0.;

	//相手ごとの攻撃待ち時間。
	TMap<TWeakObjectPtr<AActor>,double> m_pauses;

	//相手ごとの攻撃担当。
	TMap<TWeakObjectPtr<AActor>,TWeakObjectPtr<UMinionFormation>> m_duelists;
};
