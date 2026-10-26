// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelSequenceSubSystem/LevelSequenceSubSystem.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

bool ULevelSequenceSubSystem::PlaySequence(ALevelSequenceActor* _sequenceActor)
{
	if (!IsValid(_sequenceActor)) return false;

	SequenceActor = _sequenceActor;
	SequencePlayer = _sequenceActor->GetSequencePlayer();

	if (!IsValid(SequencePlayer)) return false;

	SequencePlayer->OnFinished.RemoveAll(this);

	SequencePlayer->OnFinished.AddDynamic(
		this,
		&ULevelSequenceSubSystem::HandleSequenceFinished
	);

	SequencePlayer->Play();

	return true;
}

bool ULevelSequenceSubSystem::PlaySequence(ALevelSequenceActor* _sequenceActor, bool _isEnabled)
{
	// プレイヤー入力無効化
	if (!PlaySequence(_sequenceActor))
	{
		return false;
	}

	SetPlayerInputEnabled(_isEnabled);

	return true;
}

void ULevelSequenceSubSystem::StopSequence()
{
	if (SequencePlayer)
	{
		SequencePlayer->Stop();
	}
}

void ULevelSequenceSubSystem::SkipSequence()
{
	if (SequencePlayer)
	{
		//通常の終了通知が重複しないよう解除
		SequencePlayer->OnFinished.RemoveAll(this);
		SequencePlayer->Stop();
	}

	// プレイヤー入力有効化
	SetPlayerInputEnabled(true);
	
	SequencePlayer = nullptr;
	SequenceActor = nullptr;

	//Sequenceが存在しない場合も終了扱いで通知
	OnSequenceFinished.Broadcast();
}

bool ULevelSequenceSubSystem::IsSequencePlaying() const
{
	return SequencePlayer && SequencePlayer->IsPlaying();
}

void ULevelSequenceSubSystem::HandleSequenceFinished()
{
	if (SequencePlayer)
	{
		SequencePlayer->OnFinished.RemoveAll(this);
	}

	SequencePlayer = nullptr;
	SequenceActor = nullptr;

	// プレイヤー入力有効化
	SetPlayerInputEnabled(true);

	OnSequenceFinished.Broadcast();
}

void ULevelSequenceSubSystem::SetPlayerInputEnabled(bool _isEnabled)
{
	UWorld* World = GetWorld();
	if (!World) return;

	// ローカルプレイヤー全員の入力を制御
	for (FConstPlayerControllerIterator It =
		     World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();

		if (!IsValid(PC)) continue;

		PC->SetIgnoreMoveInput(!_isEnabled);
		PC->SetIgnoreLookInput(!_isEnabled);
	}
}
