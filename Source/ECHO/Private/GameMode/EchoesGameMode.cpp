// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameMode/EchoesGameMode.h"

#include "DSP/MelScale.h"
#include "Player/ActionCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Event/StageEventSubsystem/StageEventSubsystem.h"

AEchoesGameMode::AEchoesGameMode()
{
	// stub
}

void AEchoesGameMode::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!World) return;

	UStageEventSubsystem* StageEventSubsystem = World->GetSubsystem<UStageEventSubsystem>();

	if (!StageEventSubsystem) return;

	// 最終イベント終了通知にバインド
	StageEventSubsystem->OnFinalEventFinished.AddUniqueDynamic(this, &AEchoesGameMode::OnFinalEventFinish);
}

void AEchoesGameMode::RespawnPlayer(AActionCharacter* _player)
{
	if (!IsValid(_player)) return;

	// 移動速度をリセット
	if (UCharacterMovementComponent* Movement = _player->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}

	_player->SetActorTransform(m_RespawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
	_player->OnRespawn();

	//ステージの初期化処理呼んであげる
	UStageEventSubsystem* StageEventSubsystem = GetWorld()->GetSubsystem<UStageEventSubsystem>();

	if (!StageEventSubsystem) return;

	StageEventSubsystem->OnPlayerRespawn();
}

void AEchoesGameMode::OnFinalEventFinish()
{
	if (m_NextLevel.IsNull())return;

	UGameplayStatics::OpenLevelBySoftObjectPtr(this, m_NextLevel);
}
