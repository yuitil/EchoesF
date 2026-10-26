#include "Event/Events/EnemyEliminationEvent/EnemyEliminationEvent.h"

#include "Enemy/Core/EnemyCharacterBase.h"
#include "Enemy/EnemyManager/EnemyManager.h"
#include "Event/EventPoint/EventPoint.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"

void UEnemyEliminationEvent::Enter(const FEventData& _data)
{
	Super::Enter(_data);

	m_CurrentEnemyCount = 0;
	m_TotalSpawnedCount = 0;
	m_EnemyIndex = 0;
	m_PointIndex = 0;

	m_SpawnedEnemies.Empty();

	UWorld* World = GetWorld();
	if (!World) return;

	UEnemyManager* EnemyManager = World->GetSubsystem<UEnemyManager>();

	if (!EnemyManager)
	{
		EndEvent();
		return;
	}

	// 死亡通知にバインド
	EnemyManager->OnEventEnemyDefeated.AddUObject(this, &UEnemyEliminationEvent::OnEventEnemyDead);

	// 初期スポーン
	FillEnemies();

	CheckEventClear();
}

void UEnemyEliminationEvent::Update(float _deltaTime)
{
	Super::Update(_deltaTime);
}

void UEnemyEliminationEvent::Finish()
{
	EventReset();

	Super::Finish();
}

void UEnemyEliminationEvent::EventReset()
{
	Super::EventReset();

	if (UWorld* World = GetWorld())
	{
		if (UEnemyManager* EnemyManager =
			World->GetSubsystem<UEnemyManager>())
		{
			EnemyManager->OnEventEnemyDefeated.RemoveAll(this);
		}
	}

	DestroyAllSpawnedEnemies();
}

void UEnemyEliminationEvent::StartClearSlowMotion()
{
	UWorld* World = GetWorld();
	if (!World) return;

	UGameplayStatics::SetGlobalTimeDilation(World, m_ClearTimeDilation);

	// Timerはゲーム内時間基準なので倍率を考慮
	const float TimerDuration = m_ClearSlowDuration * m_ClearTimeDilation;

	World->GetTimerManager().SetTimer(
		m_ClearSlowTimerHandle,
		this,
		&UEnemyEliminationEvent::EndClearSlowMotion,
		TimerDuration,
		false
	);
}

void UEnemyEliminationEvent::EndClearSlowMotion()
{
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(World, 1.0f);
	}
}

void UEnemyEliminationEvent::SpawnNextEnemy()
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (m_EventEnemys.IsEmpty()) return;
	if (m_EventData.m_EventPoints.IsEmpty()) return;

	if (m_TotalSpawnedCount >= m_EnemySpawnCount) return;

	UEnemyManager* EnemyManager = World->GetSubsystem<UEnemyManager>();

	if (!EnemyManager) return;

	// 敵クラスを登録順に循環して取得
	const int32 EnemyIndex = m_EnemyIndex % m_EventEnemys.Num();

	TSubclassOf<AEnemyCharacterBase> EnemyClass = m_EventEnemys[EnemyIndex];

	if (!EnemyClass) return;

	// イベントポイントも登録順に循環
	const int32 PointIndex = m_PointIndex % m_EventData.m_EventPoints.Num();

	AEventPoint* EventPoint = m_EventData.m_EventPoints[PointIndex];

	if (!IsValid(EventPoint)) return;

	// ポイントのTransformをそのまま使用
	const FTransform SpawnTransform = EventPoint->GetActorTransform();

	// EnemyManager経由で生成
	AEnemyCharacterBase* Enemy =
		EnemyManager->CreateEnemy(
			EnemyClass,
			SpawnTransform,
			true
		);

	if (!IsValid(Enemy)) return;

	// 生成した敵を配列に登録
	m_SpawnedEnemies.Add(Enemy);

	++m_CurrentEnemyCount;
	++m_TotalSpawnedCount;

	// 次のインデックスへ進める
	m_EnemyIndex =
		(m_EnemyIndex + 1) % m_EventEnemys.Num();

	m_PointIndex =
		(m_PointIndex + 1) % m_EventData.m_EventPoints.Num();
}

void UEnemyEliminationEvent::FillEnemies()
{
	if (m_EventEnemys.IsEmpty() || m_EventData.m_EventPoints.IsEmpty()) return;

	// 同時出現数の上限まで補充
	while (m_CurrentEnemyCount < m_ActiveEnemyCount && m_TotalSpawnedCount < m_EnemySpawnCount)
	{
		const int32 PreviousCount = m_TotalSpawnedCount;

		SpawnNextEnemy();

		// 生成失敗時は無限ループを防ぐ
		if (PreviousCount == m_TotalSpawnedCount) break;
	}
}

void UEnemyEliminationEvent::OnEventEnemyDead(AEnemyCharacterBase* _enemy)
{
	if (!IsValid(_enemy)) return;

	// このイベントで生成した敵か確認
	if (m_SpawnedEnemies.Remove(_enemy) == 0) return;

	if (m_CurrentEnemyCount <= 0) return;

	--m_CurrentEnemyCount;

	FillEnemies();

	CheckEventClear();
}

void UEnemyEliminationEvent::ShowClearWidget()
{
	if (!m_ClearWidgetClass) return;

	UWorld* World = GetWorld();
	if (!World) return;

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC) return;

	UUserWidget* Widget = CreateWidget<UUserWidget>(PC, m_ClearWidgetClass);

	if (!Widget) return;

	Widget->AddToViewport(10);
}

void UEnemyEliminationEvent::CheckEventClear()
{
	// まだ生成予定の敵が残っている
	if (m_TotalSpawnedCount < m_EnemySpawnCount) return;

	// 場に敵が残っている
	if (m_CurrentEnemyCount > 0) return;

	StartClearSlowMotion();
	ShowClearWidget();
	EndEvent();
}

void UEnemyEliminationEvent::DestroyAllSpawnedEnemies()
{
	for (AEnemyCharacterBase* Enemy : m_SpawnedEnemies)
	{
		if (!IsValid(Enemy)) continue;

		Enemy->Destroy();
	}

	m_SpawnedEnemies.Empty();
}
