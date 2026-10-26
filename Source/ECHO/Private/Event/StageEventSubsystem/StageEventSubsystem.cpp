// Fill out your copyright notice in the Description page of Project Settings.


#include "Event/StageEventSubsystem/StageEventSubsystem.h"

#include "Event/Events/EventBase.h"
#include "WorldSettings/EchoesWorldSettings.h"
#include "Event/EventTriggerVolume/EventTriggerVolume.h"
#include "StageGimmick/Gate/Gate.h"

bool UStageEventSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer)) return false;

	if (UWorld* WorldOuter = Cast<UWorld>(Outer))
	{
		if (AEchoesWorldSettings* MyWorldSettings = Cast<AEchoesWorldSettings>(WorldOuter->GetWorldSettings()))
		{
			return MyWorldSettings->bUseEventSystem;
		}
	}

	return false;
}

void UStageEventSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 60.0f, FColor::Blue, TEXT("UStageEventSubsystem"));
	}
}

void UStageEventSubsystem::Deinitialize()
{
	Super::Deinitialize();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 60.0f, FColor::Blue, TEXT("UNpcManager::Deinitialize!!!"));
	}
}

void UStageEventSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!m_CurrentEvent)return;

	m_CurrentEvent->Update(DeltaTime);
}

void UStageEventSubsystem::EventStart(AEventTriggerVolume* _eventTrigger)
{
	if (m_CurrentEvent)return;

	// イベントクラスを取得
	TSubclassOf<UEventBase> EventClass = _eventTrigger->GetEventClass();

	if (!EventClass) return;

	// イベントを生成
	m_CurrentEvent = NewObject<UEventBase>(this, EventClass);

	if (!IsValid(m_CurrentEvent)) return;

	// イベント情報を渡して開始
	m_CurrentEvent->Enter(_eventTrigger->GetEventData());

	SetGateOpen(false, m_CurrentEvent->GetEventData());

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Blue, TEXT("Event Start"));
	}
}

void UStageEventSubsystem::EventFinish()
{
	if (!m_CurrentEvent)return;

	m_CurrentEvent->Finish();

	SetGateOpen(true, m_CurrentEvent->GetEventData());

	m_CurrentEvent = nullptr;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Blue, TEXT("Event Finish"));
	}
}

void UStageEventSubsystem::SetGateOpen(bool _isOpen, FEventData& _eventData)
{
	TArray<TObjectPtr<AGate>> Gates = _eventData.m_Gates;

	if (_isOpen)
	{
		for (TObjectPtr<AGate> Gate : Gates)
		{
			Gate->OpenGate();
		}
	}
	else
	{
		for (TObjectPtr<AGate> Gate : Gates)
		{
			Gate->CloseGate();
		}
	}
}

void UStageEventSubsystem::NotifyFinalEventFinished()
{
	OnFinalEventFinished.Broadcast();
}
