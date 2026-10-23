//EnemyQuery.h
//「敵かどうか」の判定をまとめたヘルパー
//旧敵クラス(AEnemyChara)と新敵クラス(AEnemyCharacterBase)の両方に対応する

#pragma once

#include "CoreMinimal.h"

class AActor;

namespace EnemyQuery
{
	//敵として扱うアクターか（死んでいても true）
	bool IsEnemy(const AActor* Actor);

	//生きていて、狙ったり攻撃したりできる敵か
	bool IsAliveEnemy(const AActor* Actor);

	//生きていて、誰か（プレイヤーや分身）を狙っている敵か
	bool IsEngaged(const AActor* Actor);
}