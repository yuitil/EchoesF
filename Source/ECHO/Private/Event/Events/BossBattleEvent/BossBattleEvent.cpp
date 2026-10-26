// Fill out your copyright notice in the Description page of Project Settings.


#include "Event/Events/BossBattleEvent/BossBattleEvent.h"


#include "Enemy/Core/EnemyCharacterBase.h"
#include "Enemy/EnemyManager/EnemyManager.h"
#include "Event/EventPoint/EventPoint.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"

void UBossBattleEvent::Enter(const FEventData& _data)
{
	Super::Enter(_data);

	m_SpawnedBoss = nullptr;
	m_IsBossDead = false;
	m_IsCleared = false;

	UWorld* World = GetWorld();
	if (!World) return;

	UEnemyManager* EnemyManager =
		World->GetSubsystem<UEnemyManager>();

	if (!EnemyManager)
	{
		EndEvent();
		return;
	}

	// 死亡通知にバインド
	EnemyManager->OnEventEnemyDefeated.AddUObject(
		this,
		&UBossBattleEvent::OnEventEnemyDead
	);

	// ボスを生成
	SpawnBoss();
}

void UBossBattleEvent::Update(float _deltaTime)
{
	Super::Update(_deltaTime);
}

void UBossBattleEvent::Finish()
{
	if (UWorld* World = GetWorld())
	{
		if (UEnemyManager* EnemyManager = World->GetSubsystem<UEnemyManager>())
		{
			EnemyManager->OnEventEnemyDefeated.RemoveAll(this);
		}
	}

	m_SpawnedBoss = nullptr;

	Super::Finish();
}

void UBossBattleEvent::SpawnBoss()
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (!m_BossClass) return;
	if (m_EventData.m_EventPoints.IsEmpty()) return;

	UEnemyManager* EnemyManager =
		World->GetSubsystem<UEnemyManager>();

	if (!EnemyManager) return;

	// 最初のイベントポイントを使用
	AEventPoint* EventPoint =
		m_EventData.m_EventPoints[0];

	if (!IsValid(EventPoint)) return;

	// ポイントのTransformを使用
	const FTransform SpawnTransform =
		EventPoint->GetActorTransform();

	// EnemyManager経由でボスを生成
	AEnemyCharacterBase* Boss =
		EnemyManager->CreateEnemy(
			m_BossClass,
			SpawnTransform,
			true
		);

	if (!IsValid(Boss)) return;

	m_SpawnedBoss = Boss;
}

void UBossBattleEvent::OnEventEnemyDead(
	AEnemyCharacterBase* _enemy)
{
	if (!IsValid(_enemy)) return;

	// このイベントで生成したボス以外は無視
	if (_enemy != m_SpawnedBoss) return;

	// 二重処理防止
	if (m_IsBossDead) return;

	m_IsBossDead = true;

	CheckEventClear();
}

void UBossBattleEvent::CheckEventClear()
{
	// ボスがまだ死亡していない
	if (!m_IsBossDead) return;

	// 既にクリア処理済み
	if (m_IsCleared) return;

	m_IsCleared = true;

	StartClearSlowMotion();
	ShowClearWidget();

	EndEvent();
}

void UBossBattleEvent::ShowClearWidget()
{
	if (!m_ClearWidgetClass) return;

	UWorld* World = GetWorld();
	if (!World) return;

	APlayerController* PC =
		World->GetFirstPlayerController();

	if (!PC) return;

	UUserWidget* Widget =
		CreateWidget<UUserWidget>(
			PC,
			m_ClearWidgetClass
		);

	if (!Widget) return;

	Widget->AddToViewport(10);
}

void UBossBattleEvent::StartClearSlowMotion()
{
	UWorld* World = GetWorld();
	if (!World) return;

	UGameplayStatics::SetGlobalTimeDilation(
		World,
		m_ClearTimeDilation
	);

	// Timerはゲーム内時間基準なので倍率を考慮
	const float TimerDuration =
		m_ClearSlowDuration * m_ClearTimeDilation;

	World->GetTimerManager().SetTimer(
		m_ClearSlowTimerHandle,
		this,
		&UBossBattleEvent::EndClearSlowMotion,
		TimerDuration,
		false
	);
}

void UBossBattleEvent::EndClearSlowMotion()
{
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(
			World,
			1.0f
		);
	}
}
