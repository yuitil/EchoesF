// Fill out your copyright notice in the Description page of Project Settings.


#include "EchoesGameInstance/EchoesGameInstance.h"

#include "MoviePlayer.h"
#include "Blueprint/UserWidget.h"

void UEchoesGameInstance::Init()
{
    Super::Init();

    FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UEchoesGameInstance::OnPreLoadMap);
    FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UEchoesGameInstance::OnPostLoadMap);
}

void UEchoesGameInstance::Shutdown()
{
    FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);
    FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

    Super::Shutdown();
}

void UEchoesGameInstance::OnPreLoadMap(const FString& MapName)
{
    if (IsRunningDedicatedServer()) return;

    FLoadingScreenAttributes Attr;
    Attr.bAutoCompleteWhenLoadingCompletes = true;
    Attr.MinimumLoadingScreenDisplayTime = 1.0f; // チラつき防止（任意）

    if (m_LoadingWidgetClass)
    {
        m_LoadingWidget = CreateWidget<UUserWidget>(this, m_LoadingWidgetClass);
        Attr.WidgetLoadingScreen = m_LoadingWidget->TakeWidget();
    }
    else
    {
        Attr.WidgetLoadingScreen = FLoadingScreenAttributes::NewTestLoadingScreenWidget();
    }

    GetMoviePlayer()->SetupLoadingScreen(Attr);
}

void UEchoesGameInstance::OnPostLoadMap(UWorld* LoadedWorld)
{
    m_LoadingWidget = nullptr;
}