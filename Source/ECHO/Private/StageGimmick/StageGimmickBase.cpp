// Fill out your copyright notice in the Description page of Project Settings.


#include "StageGimmick/StageGimmickBase.h"

// Sets default values
AStageGimmickBase::AStageGimmickBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AStageGimmickBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AStageGimmickBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

