// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "EchoesGameMode.generated.h"

class AActionCharacter;

UCLASS(abstract)
class AEchoesGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AEchoesGameMode();

	virtual void BeginPlay() override;
	
	void RespawnPlayer(AActionCharacter* _player);

	// リスポーン位置を設定
	void SetRespawnTransform(const FTransform& _transform) { m_RespawnTransform = _transform; };

	UFUNCTION()
	void OnFinalEventFinish();
protected:
	//現在の復帰地点
	FTransform m_RespawnTransform = FTransform::Identity;
	
	UPROPERTY(EditAnywhere, Category="Level")
	TSoftObjectPtr<UWorld> m_NextLevel = nullptr;
};
