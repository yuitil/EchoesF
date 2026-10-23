// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TitleWidget.generated.h"

/**
 * 
 */

//タイトルメニュー
UENUM(BlueprintType)
enum class ETitleMenuType : uint8
{
	Start UMETA(DisplayName = "Start"),
	Settings UMETA(DisplayName = "Settings"),
	Exit UMETA(DisplayName = "Exit"),
	
	Num UMETA(DisplayName = "Num")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTitleMenuSelected, ETitleMenuType, MenuType);

//メニュー選択変更通知
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTitleMenuSelectionChanged, ETitleMenuType, MenuType);

UCLASS()
class ECHO_API UTitleWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
	void MoveUp();

	UFUNCTION(BlueprintCallable)
	void MoveDown();

	UFUNCTION(BlueprintCallable)
	void Decision();
	
protected:
	UFUNCTION(BlueprintImplementableEvent)
	void UpdateSelection(int32 Index);
	
	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	int32 m_CurrentIndex = 0;
	
	//現在選択中のMenuTypeを取得
	ETitleMenuType GetCurrentMenuType() const;
	
public:
	FOnTitleMenuSelected OnTitleMenuSelected;
	
	FOnTitleMenuSelectionChanged OnTitleMenuSelectionChanged;
};
