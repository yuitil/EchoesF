//EnemyQuery.cpp

#include "Combat/EnemyQuery.h"
#include "Enemy/EnemyChara.h"
#include "Enemy/Core/EnemyCharacterBase.h"

namespace EnemyQuery
{
	//敵として扱う相手か調べる。
	bool IsEnemy(const AActor* Actor)
	{
		return Cast<AEnemyChara>(Actor) || Cast<AEnemyCharacterBase>(Actor);
	}

	//生きていて攻撃できる敵か調べる。
	bool IsAliveEnemy(const AActor* Actor)
	{
		//無効・非表示（プールに戻っている敵）は対象外
		if (!IsValid(Actor) || Actor->IsHidden()) return false;

		//旧敵クラス
		if (const AEnemyChara* OldEnemy = Cast<AEnemyChara>(Actor))
		{
			return OldEnemy->GetCurrentState() != FEEnemyState::Dead;
		}

		//新敵クラス（ボスなど）
		if (const AEnemyCharacterBase* NewEnemy = Cast<AEnemyCharacterBase>(Actor))
		{
			return NewEnemy->m_state != EEnemyState::Dead;
		}

		return false;
	}

	//敵が誰かを狙っているか調べる。
	bool IsEngaged(const AActor* Actor)
	{
		if (!IsAliveEnemy(Actor)) return false;

		if (const AEnemyChara* OldEnemy = Cast<AEnemyChara>(Actor))
		{
			return OldEnemy->GetCurrentTarget() != nullptr;
		}
		if (const AEnemyCharacterBase* NewEnemy = Cast<AEnemyCharacterBase>(Actor))
		{
			return NewEnemy->m_combatTarget != nullptr;
		}
		return false;
	}
}