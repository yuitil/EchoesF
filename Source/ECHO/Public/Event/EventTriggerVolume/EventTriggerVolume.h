// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EventTriggerVolume.generated.h"

class UBoxComponent;

UCLASS()
class ECHO_API AEventTriggerVolume : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AEventTriggerVolume();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
protected:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> m_TriggerBox;

	//イベント実行したかのフラグ
	bool m_IsEventExecution = false;
};
