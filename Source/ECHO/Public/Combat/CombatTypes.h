//Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CombatTypes.generated.h"

//攻撃を防いだ方法と結果。
UENUM(BlueprintType)
enum class EDefenseResult : uint8
{
	//防御なし。
	None,
	//通常のガード。
	Guard,
	//攻撃を弾く。
	Deflect,
	//攻撃を受け流す。
	Parry,
	//ガードが崩れた状態。
	GuardBreak,
	//ジャスト回避。
	PerfectDodge,
};
