// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameMode/EchoesGameMode.h"

#include "Player/ActionCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

AEchoesGameMode::AEchoesGameMode()
{
	// stub
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
}
