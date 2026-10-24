#include "Enemy/Spawning/MinionSpawner.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"
#include "NavigationSystem.h"
#include "Engine/World.h"

//動作を確かめるための幅や待ち時間。
namespace MinionSpawnerTuning
{
    //生成数の上限。
    constexpr auto maxSpawnCount = 100;
    //生成位置を探す回数の上限。
    constexpr auto maxTries = 8;
}

//初期設定と必要な部品を用意する。
AMinionSpawner::AMinionSpawner()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

//ゲーム開始時に必要な状態を準備する。
void AMinionSpawner::BeginPlay()
{
    Super::BeginPlay();
    if (m_autoStart) Spawn();
}

//設定した数と位置で雑魚敵を生成する。
void AMinionSpawner::Spawn()
{
    if(!HasAuthority() || m_started || !m_enemyClass) { return; }
    m_started = true;
    //歩ける場所を調べる機能。
    auto* nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    //生成するクラスの初期設定。
    const auto* defaults = m_enemyClass->GetDefaultObject<AEnemyCharacterBase>();
    //高さの計算に使う値。
    const float height = defaults->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    for (int32 i=0; i<FMath::Clamp(m_count,1,MinionSpawnerTuning::maxSpawnCount); ++i)
    {
        //基準となる位置。
        AActor* point = m_points.IsEmpty() ? nullptr : m_points[i % m_points.Num()].Get();
        //位置を決める基準点。
        const FVector origin = IsValid(point) ? point->GetActorLocation() : GetActorLocation();
        for (int32 attempt=0; attempt<MinionSpawnerTuning::maxTries; ++attempt)
        {
            //対象の位置。
            FVector location = origin;
            //地面に当たった情報。
            FNavLocation floor;
            if (nav && ((point && attempt==0) ? nav->ProjectPointToNavigation(origin, floor) :
                nav->GetRandomReachablePointInRadius(origin, FMath::Max(0.f,m_radius), floor)))
                location = floor.Location + FVector(0,0,height+2.f);
            else if (m_requireNav) continue;
            //生成や当たり判定に使う設定。
            FActorSpawnParameters params;
            params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
            //処理の対象となる敵。
            auto* enemy = GetWorld()->SpawnActor<AEnemyCharacterBase>(m_enemyClass, location, IsValid(point) ? point->GetActorRotation() : GetActorRotation(), params);
            if (!enemy) continue;
            if (!enemy->GetController()) enemy->SpawnDefaultController();
            m_enemies.Add(enemy);
            ++m_spawned;
            break;
        }
    }
    if (m_spawned != FMath::Clamp(m_count,1,MinionSpawnerTuning::maxSpawnCount))
        UE_LOG(LogTemp, Warning, TEXT("%s: Spawned %d/%d. Check NavMesh, points and collision."), *GetName(), m_spawned, m_count);
    if (m_spawned == 0) m_started = false;
}

//次の戦闘に向けて状態を初期化する。
void AMinionSpawner::Reset()
{
    if(!HasAuthority()) { return; }
    for (const auto& enemy : m_enemies) if (enemy.IsValid()) { enemy->DetachFromControllerPendingDestroy(); enemy->Destroy(); }
    m_enemies.Reset();
    m_spawned = 0;
    m_started = false;
}

//終了時にタイマーや登録を解除する。
void AMinionSpawner::EndPlay(const EEndPlayReason::Type _reason)
{
    Reset();
    Super::EndPlay(_reason);
}
