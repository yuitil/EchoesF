// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/TitleWidget/TitleWidget.h"

void UTitleWidget::MoveUp()
{
	--m_CurrentIndex;

	if (m_CurrentIndex < 0)
	{
		m_CurrentIndex = int32(ETitleMenuType::Num) - 1;
	}

	UpdateSelection(m_CurrentIndex);
	OnTitleMenuSelectionChanged.Broadcast(GetCurrentMenuType());
}

void UTitleWidget::MoveDown()
{
	++m_CurrentIndex;

	if (m_CurrentIndex > int32(ETitleMenuType::Num) - 1)
	{
		m_CurrentIndex = 0;
	}

	UpdateSelection(m_CurrentIndex);
	OnTitleMenuSelectionChanged.Broadcast(GetCurrentMenuType());
}

void UTitleWidget::Decision()
{
	OnTitleMenuSelected.Broadcast(GetCurrentMenuType());
}


ETitleMenuType UTitleWidget::GetCurrentMenuType() const
{
	switch (m_CurrentIndex)
	{
	case 0:
		return ETitleMenuType::Start;

	case 1:
		return ETitleMenuType::Settings;

	case 2:
		return ETitleMenuType::Exit;

	default:
		return ETitleMenuType::Start;
	}
}
