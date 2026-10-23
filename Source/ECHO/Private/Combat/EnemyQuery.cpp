//EnemyQuery.cpp

#include "Combat/EnemyQuery.h"
#include "Enemy/EnemyChara.h"
#include "Enemy/Core/EnemyCharacterBase.h"

namespace EnemyQuery
{
	bool IsEnemy(const AActor* Actor)
	{
		return Cast<AEnemyChara>(Actor) || Cast<AEnemyCharacterBase>(Actor);
	}

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