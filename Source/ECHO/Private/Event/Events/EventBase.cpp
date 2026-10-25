// Fill out your copyright notice in the Description page of Project Settings.


#include "Event/Events/EventBase.h"

#include "Event/StageEventSubsystem/StageEventSubsystem.h"

void UEventBase::Enter(const FEventData& _data)
{
	m_EventData = _data;
}

void UEventBase::Update(float _deltaTime)
{
}

void UEventBase::Finish()
{
}

void UEventBase::EndEvent()
{
	UStageEventSubsystem* EventSubsystem = GetWorld()->GetSubsystem<UStageEventSubsystem>();

	if (!EventSubsystem)return;

	EventSubsystem->EventFinish();
}
