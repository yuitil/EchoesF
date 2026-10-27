// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/TitleGameMode/TitleGameMode.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Widget/TitleWidget/TitleWidget.h"
#include "Engine/AssetManager.h"

void ATitleGameMode::BeginPlay()
{
	Super::BeginPlay();

	m_TitleWidget = CreateWidget<UTitleWidget>(GetWorld(), m_TitleWidgetClass);

	if (!m_TitleWidget)return;

	m_TitleWidget->AddToViewport();

	m_TitleWidget->OnTitleMenuSelected.AddDynamic(this, &ATitleGameMode::OnTitleMenuSelected);
}

void ATitleGameMode::StartGamePlay()
{
	if (m_NextLevel.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("NextLevel is null"));
		return;
	}

	UGameplayStatics::OpenLevelBySoftObjectPtr(this, m_NextLevel);
}

void ATitleGameMode::OpenSettings()
{
}

void ATitleGameMode::ExitGame()
{
	UKismetSystemLibrary::QuitGame(this, GetWorld()->GetFirstPlayerController(), EQuitPreference::Quit, false);
}

void ATitleGameMode::OnTitleMenuSelected(ETitleMenuType _menuType)
{
	switch (_menuType)
	{
	case ETitleMenuType::Start:
		StartGamePlay();
		break;

	case ETitleMenuType::Settings:
		OpenSettings();
		break;

	case ETitleMenuType::Exit:
		ExitGame();
		break;

	default:
		break;
	}
}
