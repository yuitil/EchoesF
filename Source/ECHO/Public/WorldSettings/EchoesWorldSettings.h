// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/WorldSettings.h"
#include "EchoesWorldSettings.generated.h"

/**
 * 
 */
UCLASS()
class ECHO_API AEchoesWorldSettings : public AWorldSettings
{
	GENERATED_BODY()
	
public:   
    	UPROPERTY(EditAnywhere)
    	bool bUseEventSystem = true;
     
    	UPROPERTY(EditAnywhere)
    	bool bUseEnemyManager = true;
};
