//SkillDataAsset.cpp

#include "Player/SkillDataAsset.h"

// -----------------------------------------------------------------------
// 最終奥義の段階データが空なら、5段ぶんの初期値を入れておく
// （新しくデータアセットを作ったときだけ入る）
// -----------------------------------------------------------------------
USkillDataAsset::USkillDataAsset()
{
	if (Ultimate.Levels.Num() == 0)
	{
		//          長さ    角度   根元幅  ダメージ ヒット数 間隔    後隙
		const float Table[5][7] = {
			{  800.f,  8.f, 100.f,  80.f, 1, 0.10f, 0.5f },
			{ 1100.f,  9.f, 120.f,  60.f, 2, 0.10f, 0.6f },
			{ 1400.f, 10.f, 140.f,  50.f, 3, 0.10f, 0.6f },
			{ 1800.f, 11.f, 170.f,  45.f, 4, 0.10f, 0.7f },
			{ 2500.f, 12.f, 220.f,  45.f, 8, 0.08f, 1.0f },   //5段：とてつもない技
		};
		for (const auto& Row : Table)
		{
			FUltimateLevelData Level;
			Level.m_Length = Row[0];
			Level.m_HalfAngle = Row[1];
			Level.m_BaseWidth = Row[2];
			Level.m_Damage = Row[3];
			Level.m_HitCount = static_cast<int32>(Row[4]);
			Level.m_HitInterval = Row[5];
			Level.m_RecoveryTime = Row[6];
			Ultimate.Levels.Add(Level);
		}
	}
}
