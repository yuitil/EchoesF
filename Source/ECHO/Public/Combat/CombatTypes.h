// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CombatTypes.generated.h"

UENUM(BlueprintType)
enum class EDefenseResult : uint8
{
	None,
	Guard,
	Deflect,
	Parry,
	GuardBreak,
	PerfectDodge,
};
