// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include  "Event/Events/EventData.h"
#include "StageEventSubsystem.generated.h"

/**
 * 
 */

class UEventBase;
class AEventTriggerVolume;

// ステージ最終ボスイベント終了通知
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFinalEventFinished);

UCLASS()
class ECHO_API UStageEventSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Tick
	virtual void Tick(float DeltaTime) override;

	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(UStageEventSubsystem, STATGROUP_Tickables);
	}

	void EventStart(AEventTriggerVolume* _eventTrigger);

	void EventFinish();

	void SetGateOpen(bool _isOpen,FEventData& _eventData);
	
	// ステージ最終ボスイベント終了
	void NotifyFinalEventFinished();
	
private:
	UPROPERTY()
	TObjectPtr<UEventBase> m_CurrentEvent;
	
public:
	// ステージ最終ボスイベント終了通知
	UPROPERTY(BlueprintAssignable, Category = "Event")
	FOnFinalEventFinished OnFinalEventFinished;
};
