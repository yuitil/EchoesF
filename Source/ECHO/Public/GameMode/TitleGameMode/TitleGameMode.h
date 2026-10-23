// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Widget/TitleWidget/TitleWidget.h"
#include "TitleGameMode.generated.h"

/**
 * 
 */

class UTitleWidget;

UCLASS()
class ECHO_API ATitleGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnTitleMenuSelected(ETitleMenuType _menuType);

protected:
	void StartGamePlay();
	void OpenSettings();
	void ExitGame();


	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UTitleWidget> m_TitleWidgetClass = nullptr;

	UPROPERTY()
	TObjectPtr<UTitleWidget> m_TitleWidget = nullptr;

	UPROPERTY(EditAnywhere, Category="Level")
	TSoftObjectPtr<UWorld> m_NextLevel = nullptr;
};
