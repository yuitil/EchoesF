// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "EchoesGameInstance.generated.h"

/**
 * 
 */

class UUserWidget;

UCLASS()
class ECHO_API UEchoesGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
    virtual void Init() override;
    virtual void Shutdown() override;
protected:
    UPROPERTY(EditDefaultsOnly, Category = "Loading")
    TSubclassOf<UUserWidget> m_LoadingWidgetClass;

    UPROPERTY()
    TObjectPtr<UUserWidget> m_LoadingWidget;

    void OnPreLoadMap(const FString& MapName);
    void OnPostLoadMap(UWorld* LoadedWorld);
};
