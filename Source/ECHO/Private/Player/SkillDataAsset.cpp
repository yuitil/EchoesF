//SkillDataAsset.cpp

#include "Player/SkillDataAsset.h"

// -----------------------------------------------------------------------
// 最終奥義の段階データが空なら、5段ぶんの初期値を入れておく
// -----------------------------------------------------------------------
USkillDataAsset::USkillDataAsset()
{
	if (Ultimate.Levels.Num() == 0)
	{
		//半径 ダメージ ヒット数 間隔 後隙
		const float Table[5][5] = {
			{ 300.f,  60.f,  1, 0.15f, 0.4f },
			{ 400.f,  50.f,  2, 0.15f, 0.5f },
			{ 500.f,  45.f,  3, 0.12f, 0.5f },
			{ 650.f,  45.f,  4, 0.12f, 0.6f },
			{ 1000.f, 60.f,  6, 0.10f, 0.8f },   //5段：とてつもない技
		};
		for (const auto& Row : Table)
		{
			FUltimateLevelData Level;
			Level.m_Radius = Row[0];
			Level.m_Damage = Row[1];
			Level.m_HitCount = static_cast<int32>(Row[2]);
			Level.m_HitInterval = Row[3];
			Level.m_RecoveryTime = Row[4];
			Ultimate.Levels.Add(Level);
		}
	}
}
