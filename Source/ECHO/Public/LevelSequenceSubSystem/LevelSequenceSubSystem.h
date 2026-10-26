// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LevelSequenceSubSystem.generated.h"

/**
 * 
 */

class ALevelSequenceActor;
class ULevelSequencePlayer;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLevelSequenceFinished);

UCLASS()
class ECHO_API ULevelSequenceSubSystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	
	bool PlaySequence(ALevelSequenceActor* SequenceActor);
	
	UFUNCTION(BlueprintCallable)
	bool PlaySequence(ALevelSequenceActor* SequenceActor,bool _isEnabled);

	UFUNCTION(BlueprintCallable)
	void StopSequence();
    
	UFUNCTION(BlueprintCallable)
	void SkipSequence();
    
	UFUNCTION(BlueprintPure)
	bool IsSequencePlaying() const;

	UPROPERTY(BlueprintAssignable)
	FOnLevelSequenceFinished OnSequenceFinished;

private:

	UFUNCTION()
	void HandleSequenceFinished();

	// プレイヤー入力の制御
	void SetPlayerInputEnabled(bool _isEnabled);
private:

	UPROPERTY()
	TObjectPtr<ALevelSequenceActor> SequenceActor;

	UPROPERTY()
	TObjectPtr<ULevelSequencePlayer> SequencePlayer;

};
