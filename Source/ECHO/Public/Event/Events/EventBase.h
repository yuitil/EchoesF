// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EventData.h"
#include "UObject/Object.h"

#include "EventBase.generated.h"
/**
 * 
 */

FEventData;

UCLASS(BlueprintType, Blueprintable)
class ECHO_API UEventBase : public UObject
{
	GENERATED_BODY()

public:
	virtual void Enter(const FEventData& _data);
	virtual void Update(float _deltaTime);
	virtual void Finish();

	FEventData& GetEventData() { return m_EventData; }

protected:
	void EndEvent();

protected:
	UPROPERTY()
	FEventData m_EventData;
};
