//LockOnComponent.cpp
//ロックオンコンポーネントソース

#include "Player/LockOnComponent.h"
#include "Enemy/EnemyChara.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Combat/EnemyQuery.h"

#include "DrawDebugHelpers.h" //デバッグ用

ULockOnComponent::ULockOnComponent() :
	m_LockOnRange(1500.f),
	m_LockOnBreakRange(2000.f),
	m_SwitchThreshold(0.7f),
	m_SwitchCooldown(0.4f),
	m_SwitchCooldownTimer(0.f),
	m_bSwitchInputActive(false)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void ULockOnComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//切り替えクールダウンを更新
	if (m_SwitchCooldownTimer > 0.f)
	{
		m_SwitchCooldownTimer = FMath::Max(0.f, m_SwitchCooldownTimer - DeltaTime);
	}

	if (!CurrentTarget.IsValid()) return;

	//ターゲットが無効になったら自動解除
	if (!IsTargetValid(CurrentTarget.Get()))
	{
		UE_LOG(LogTemp, Log, TEXT("[LockOn] ターゲット無効により自動解除: %s"), *CurrentTarget->GetName());
		ClearLockOn();
	}

	//デバッグ用
	if (m_bDebugDraw && IsLockedOn()) DrawLockOnDebug();
}

void ULockOnComponent::ToggleLockOn()
{
	if (IsLockedOn())
	{
		UE_LOG(LogTemp, Log, TEXT("[LockOn] 手動解除"));
		ClearLockOn();
	}
	else
	{
		AActor* Best = FindBestTarget();
		if (Best)
		{
			CurrentTarget = Best;
			
			AEnemyChara* Enemy = Cast <AEnemyChara>(Best);
			if (Enemy)
			{
				Enemy->m_onEnemyEliminated.AddUniqueDynamic(this, &ULockOnComponent::OnEnemyEliminated);
			}

			UE_LOG(LogTemp, Log, TEXT("[LockOn] ロックオン開始: %s"), *Best->GetName());
			OnLockOnChanged.Broadcast(Best);
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("[LockOn] 対象なし、ロックオンできませんでした"));
		}
	}
}

void ULockOnComponent::TrySwitchTarget(float StickX)
{
	//ロックオン中であればなにもしない
	if (!IsLockedOn()) return;

	//クールダウン中は無視
	if (m_SwitchCooldownTimer > 0.f) return;

	//基準値を超えたら切り返し
	if (FMath::Abs(StickX) >= m_SwitchThreshold && !m_bSwitchInputActive)
	{
		m_bSwitchInputActive = true;

		AActor* Next = FindNextTarget(StickX);
		if (Next && Next != CurrentTarget.Get())
		{
			//旧ターゲットのデリゲートを解除
			AEnemyChara* OldEnemy = Cast<AEnemyChara>(CurrentTarget.Get());
			if (OldEnemy)
			{
				OldEnemy->m_onEnemyEliminated.RemoveDynamic(this, &ULockOnComponent::OnEnemyEliminated);
			}

			//新ターゲットのデリゲートを受け取る
			AEnemyChara* NewEnemy = Cast<AEnemyChara>(Next);
			if (NewEnemy)
			{
				NewEnemy->m_onEnemyEliminated.AddUniqueDynamic(this, &ULockOnComponent::OnEnemyEliminated);
			}

			CurrentTarget = Next;
			m_SwitchCooldownTimer = m_SwitchCooldown;

			UE_LOG(LogTemp, Log, TEXT("[LockOn] ターゲット切り替え: %s"), *Next->GetName());

			OnLockOnChanged.Broadcast(Next);
		}
	}

	//スティックを戻したらフラグをリセット
	if (FMath::Abs(StickX) < m_SwitchThreshold * 0.5f)
	{
		m_bSwitchInputActive = false;
	}
}

void ULockOnComponent::OnEnemyEliminated(AEnemyChara* EliminatedEnemy)
{
	if (!EliminatedEnemy) return;

	//倒された敵が現在のターゲットなら即解除
	if (CurrentTarget.Get() == EliminatedEnemy)
	{
		UE_LOG(LogTemp, Log, TEXT("[LockOn] ターゲット死亡により解除: %s"), *EliminatedEnemy->GetName());
		ClearLockOn();
	}
}

void ULockOnComponent::ClearLockOn()
{
	//デリゲートを解除してからクリア
	AEnemyChara* Enemy = Cast<AEnemyChara>(CurrentTarget.Get());
	if (Enemy)
	{
		Enemy->m_onEnemyEliminated.RemoveDynamic(this, &ULockOnComponent::OnEnemyEliminated);
	}

	CurrentTarget = nullptr;
	m_bSwitchInputActive = false;
	m_SwitchCooldownTimer = 0.f;
	OnLockOnChanged.Broadcast(nullptr);
}

bool ULockOnComponent::IsTargetValid(AActor* Target) const
{
	if (!Target) return false;
	
	//距離チェック
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter) return false;

	float Distance = FVector::Dist(
		OwnerCharacter->GetActorLocation(),
		Target->GetActorLocation()
	);

	if (Distance > m_LockOnBreakRange) return false;

	//生きている敵か（旧敵・ボスの両方に対応。非表示のチェックも含む）
	if (!EnemyQuery::IsAliveEnemy(Target)) return false;

	return true;
}

AActor* ULockOnComponent::FindBestTarget() const
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter) return nullptr;

	//カメラを取得
	UCameraComponent* Camera = OwnerCharacter->FindComponentByClass<UCameraComponent>();
	if (!Camera) return nullptr;

	FVector CameraForward = Camera->GetForwardVector();
	FVector CameraLocation = Camera->GetComponentLocation();

	//全エネミーを取得
	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), Enemies);

	AActor* BestTarget = nullptr;
	float BestDot = -1.f;

	for (AActor* Enemy : Enemies)
	{
		if (!IsTargetValid(Enemy)) continue;

		//距離チェック
		float Distance = FVector::Dist(
			OwnerCharacter->GetActorLocation(),
			Enemy->GetActorLocation()
		);
		if (Distance > m_LockOnRange) continue;

		//カメラ前方との内積で画面中央に近いものを選ぶ
		FVector ToEnemy = (Enemy->GetActorLocation() - CameraLocation).GetSafeNormal();
		float Dot = FVector::DotProduct(CameraForward, ToEnemy);

		if (Dot > BestDot)
		{
			BestDot = Dot;
			BestTarget = Enemy;
		}
	}

	return BestTarget;
}

AActor* ULockOnComponent::FindNextTarget(float Direction) const
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter) return nullptr;

	UCameraComponent* Camera = OwnerCharacter->FindComponentByClass<UCameraComponent>();
	if (!Camera) return nullptr;

	//カメラの右ベクトルを使って敵の左右位置を判定
	FVector CameraRight = Camera->GetRightVector();
	FVector CurrentTargetLoc = CurrentTarget->GetActorLocation();

	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), Enemies);

	AActor* BestCandidate = nullptr;

	//現在のターゲットから距離が最も近い敵を選ぶ
	float BestScore = FLT_MAX;

	for (AActor* Enemy : Enemies)
	{
		if (!IsTargetValid(Enemy)) continue;
		if (Enemy == CurrentTarget.Get()) continue;

		float Distance = FVector::Dist(OwnerCharacter->GetActorLocation(), Enemy->GetActorLocation());
		if (Distance > m_LockOnRange) continue;

		//現在ターゲットから候補敵への方向を求め、カメラ右ベクトルとの内積で左右を判定
		FVector ToCandidate = (Enemy->GetActorLocation() - CurrentTargetLoc).GetSafeNormal();
		float RightDot = FVector::DotProduct(CameraRight, ToCandidate);

		//入力方向と同じ側にいる敵だけを対象にする
		bool bIsOnCorrectSide = (Direction > 0.f) ? (RightDot > 0.f) : (RightDot < 0.f);
		if (!bIsOnCorrectSide) continue;

		//距離が近いものを優先
		if (Distance < BestScore)
		{
			BestScore = Distance;
			BestCandidate = Enemy;
		}
	}

	return BestCandidate;
}

//デバッグ用
void ULockOnComponent::DrawLockOnDebug() const
{
#if ENABLE_DRAW_DEBUG
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	UCameraComponent* Camera = OwnerCharacter ? OwnerCharacter->FindComponentByClass<UCameraComponent>() : nullptr;
	if (!Camera) return;

	UWorld* World = GetWorld();
	const FVector CurLoc = CurrentTarget->GetActorLocation();
	const FVector CamRight = Camera->GetRightVector();

	//現在のターゲットと、カメラ右ベクトル
	DrawDebugSphere(World, CurLoc, 60.f, 16, FColor::Yellow, false, -1.f, 0, 2.f);
	const FVector ArrowStart = CurLoc + FVector(0.f, 0.f, 50.f);
	DrawDebugDirectionalArrow(World, ArrowStart, ArrowStart + CamRight * 300.f, 60.f, FColor::White, false, -1.f, 0, 4.f);

	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsOfClass(World, ACharacter::StaticClass(), Enemies);

	for (AActor* Enemy : Enemies)
	{
		if (Enemy == CurrentTarget.Get() || !IsTargetValid(Enemy)) continue;

		const FVector ToCandidate = (Enemy->GetActorLocation() - CurLoc).GetSafeNormal();
		const float   RightDot = FVector::DotProduct(CamRight, ToCandidate);

		//右側＝ピンク、左側＝紫
		const FColor Col = (RightDot > 0.f) ? FColor(255, 110, 160) : FColor(150, 140, 230);

		DrawDebugLine(World, CurLoc, Enemy->GetActorLocation(), Col, false, -1.f, 0, 3.f);
		DrawDebugString(World, Enemy->GetActorLocation() + FVector(0, 0, 120.f),
			FString::Printf(TEXT("%+.2f"), RightDot), nullptr, Col, 0.f, true, 2.f);
	}
#endif
}
