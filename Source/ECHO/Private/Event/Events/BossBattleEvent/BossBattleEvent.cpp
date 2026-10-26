#include "Event/Events/BossBattleEvent/BossBattleEvent.h"

#include "Enemy/Core/EnemyCharacterBase.h"
#include "Enemy/EnemyManager/EnemyManager.h"
#include "Event/EventPoint/EventPoint.h"

#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"

#include "LevelSequenceSubSystem/LevelSequenceSubSystem.h"
#include "LevelSequenceActor.h"


void UBossBattleEvent::Enter(const FEventData& _data)
{
	Super::Enter(_data);

	// 状態初期化
	m_SpawnedBoss = nullptr;
	m_IsBossDead = false;
	m_IsCleared = false;
	m_IsBossIntroFinished = false;
	m_IsBossFinishFinished = false;

	UWorld* World = GetWorld();
	if (!World) return;

	UEnemyManager* EnemyManager =
		World->GetSubsystem<UEnemyManager>();

	if (!EnemyManager)
	{
		EndEvent();
		return;
	}

	// 敵死亡通知にバインド
	EnemyManager->OnEventEnemyDefeated.RemoveAll(this);

	EnemyManager->OnEventEnemyDefeated.AddUObject(
		this,
		&UBossBattleEvent::OnEventEnemyDead
	);

	// ボス生成
	SpawnBoss();
}

void UBossBattleEvent::Update(float _deltaTime)
{
	Super::Update(_deltaTime);
}

void UBossBattleEvent::Finish()
{
	EventReset();

	Super::Finish();
}

void UBossBattleEvent::EventReset()
{
	Super::EventReset();

	UWorld* World = GetWorld();

	if (World)
	{
		// 敵死亡通知を解除
		if (UEnemyManager* EnemyManager =
			World->GetSubsystem<UEnemyManager>())
		{
			EnemyManager->OnEventEnemyDefeated.RemoveAll(this);
		}

		// シーケンス終了通知を解除
		if (ULevelSequenceSubSystem* SequenceSystem =
			World->GetSubsystem<ULevelSequenceSubSystem>())
		{
			SequenceSystem->OnSequenceFinished.RemoveDynamic(
				this,
				&UBossBattleEvent::OnBossIntroSequenceFinished
			);

			SequenceSystem->OnSequenceFinished.RemoveDynamic(
				this,
				&UBossBattleEvent::OnBossFinishSequenceFinished
			);

			// 再生中のシーケンスを停止
			if (SequenceSystem->IsSequencePlaying())
			{
				SequenceSystem->StopSequence();
			}
		}

		// スローモーションのタイマーを解除
		World->GetTimerManager().ClearTimer(
			m_ClearSlowTimerHandle
		);

		// 時間倍率を元に戻す
		UGameplayStatics::SetGlobalTimeDilation(
			World,
			1.0f
		);
	}

	// 生成したボスを破棄
	if (IsValid(m_SpawnedBoss))
	{
		m_SpawnedBoss->Destroy();
	}

	m_SpawnedBoss = nullptr;

	// 状態初期化
	m_IsBossDead = false;
	m_IsCleared = false;
	m_IsBossIntroFinished = false;
	m_IsBossFinishFinished = false;
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

	// ボス登場シーケンスを再生
	PlayBossIntroSequence();
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
	// ボスが死亡していない
	if (!m_IsBossDead) return;

	// 既にクリア処理済み
	if (m_IsCleared) return;

	m_IsCleared = true;

	// クリア演出
	StartClearSlowMotion();
	ShowClearWidget();

	// ボス撃破シーケンス再生
	PlayBossFinishSequence();
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

void UBossBattleEvent::PlayBossIntroSequence()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		OnBossIntroSequenceFinished();
		return;
	}

	ULevelSequenceSubSystem* SequenceSystem =
		World->GetSubsystem<ULevelSequenceSubSystem>();

	// シーケンスが設定されていない場合は終了扱い
	if (!SequenceSystem ||
		!IsValid(m_EventData.m_EventIntroSequence))
	{
		OnBossIntroSequenceFinished();
		return;
	}

	// 終了通知の重複登録防止
	SequenceSystem->OnSequenceFinished.RemoveDynamic(
		this,
		&UBossBattleEvent::OnBossIntroSequenceFinished
	);

	// 終了通知にバインド
	SequenceSystem->OnSequenceFinished.AddDynamic(
		this,
		&UBossBattleEvent::OnBossIntroSequenceFinished
	);

	// ボス登場シーケンスを再生
	if (!SequenceSystem->PlaySequence(
		m_EventData.m_EventIntroSequence), false)
	{
		SequenceSystem->OnSequenceFinished.RemoveDynamic(
			this,
			&UBossBattleEvent::OnBossIntroSequenceFinished
		);

		OnBossIntroSequenceFinished();
	}
}

void UBossBattleEvent::OnBossIntroSequenceFinished()
{
	// 二重処理防止
	if (m_IsBossIntroFinished) return;

	m_IsBossIntroFinished = true;

	UWorld* World = GetWorld();
	if (!World) return;

	ULevelSequenceSubSystem* SequenceSystem =
		World->GetSubsystem<ULevelSequenceSubSystem>();

	if (SequenceSystem)
	{
		// 終了通知を解除
		SequenceSystem->OnSequenceFinished.RemoveDynamic(
			this,
			&UBossBattleEvent::OnBossIntroSequenceFinished
		);
	}

	// シーケンス終了後の処理
	// 現時点では何もしない
}

void UBossBattleEvent::PlayBossFinishSequence()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		OnBossFinishSequenceFinished();
		return;
	}

	ULevelSequenceSubSystem* SequenceSystem =
		World->GetSubsystem<ULevelSequenceSubSystem>();

	// シーケンスが設定されていなければ終了扱い
	if (!SequenceSystem ||
		!IsValid(m_EventData.m_EventFinishSequence))
	{
		OnBossFinishSequenceFinished();
		return;
	}

	// 重複登録防止
	SequenceSystem->OnSequenceFinished.RemoveDynamic(
		this,
		&UBossBattleEvent::OnBossFinishSequenceFinished
	);

	// 終了通知登録
	SequenceSystem->OnSequenceFinished.AddDynamic(
		this,
		&UBossBattleEvent::OnBossFinishSequenceFinished
	);

	// Finishシーケンス再生
	if (!SequenceSystem->PlaySequence(
		m_EventData.m_EventFinishSequence))
	{
		SequenceSystem->OnSequenceFinished.RemoveDynamic(
			this,
			&UBossBattleEvent::OnBossFinishSequenceFinished
		);

		OnBossFinishSequenceFinished();
	}
}

void UBossBattleEvent::OnBossFinishSequenceFinished()
{
	// 二重処理防止
	if (m_IsBossFinishFinished) return;

	m_IsBossFinishFinished = true;

	UWorld* World = GetWorld();

	if (World)
	{
		ULevelSequenceSubSystem* SequenceSystem =
			World->GetSubsystem<ULevelSequenceSubSystem>();

		if (SequenceSystem)
		{
			SequenceSystem->OnSequenceFinished.RemoveDynamic(
				this,
				&UBossBattleEvent::OnBossFinishSequenceFinished
			);
		}
	}

	// イベント終了
	EndEvent();
}
