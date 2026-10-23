#include "Enemy/Spawning/MinionSpawner.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"
#include "NavigationSystem.h"
#include "Engine/World.h"
//
AMinionSpawner::AMinionSpawner()
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}
void AMinionSpawner::BeginPlay()
{
    Super::BeginPlay();
    if (m_autoStart) Spawn();
}
void AMinionSpawner::Spawn()
{
    if (!HasAuthority() || m_started || !m_enemyClass) return;
    m_started = true;
    auto* nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    const auto* defaults = m_enemyClass->GetDefaultObject<AEnemyCharacterBase>();
    const float height = defaults->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    for (int32 i=0; i<FMath::Clamp(m_count,1,100); ++i)
    {
        AActor* point = m_points.IsEmpty() ? nullptr : m_points[i % m_points.Num()].Get();
        const FVector origin = IsValid(point) ? point->GetActorLocation() : GetActorLocation();
        for (int32 attempt=0; attempt<8; ++attempt)
        {
            FVector location = origin;
            FNavLocation floor;
            if (nav && ((point && attempt==0) ? nav->ProjectPointToNavigation(origin, floor) :
                nav->GetRandomReachablePointInRadius(origin, FMath::Max(0.f,m_radius), floor)))
                location = floor.Location + FVector(0,0,height+2.f);
            else if (m_requireNav) continue;
            FActorSpawnParameters params;
            params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
            auto* enemy = GetWorld()->SpawnActor<AEnemyCharacterBase>(m_enemyClass, location, IsValid(point) ? point->GetActorRotation() : GetActorRotation(), params);
            if (!enemy) continue;
            if (!enemy->GetController()) enemy->SpawnDefaultController();
            m_enemies.Add(enemy);
            ++m_spawned;
            break;
        }
    }
    if (m_spawned != FMath::Clamp(m_count,1,100))
        UE_LOG(LogTemp, Warning, TEXT("%s: Spawned %d/%d. Check NavMesh, points and collision."), *GetName(), m_spawned, m_count);
    if (m_spawned == 0) m_started = false;
}
void AMinionSpawner::Reset()
{
    if (!HasAuthority()) return;
    for (const auto& enemy : m_enemies) if (enemy.IsValid()) { enemy->DetachFromControllerPendingDestroy(); enemy->Destroy(); }
    m_enemies.Reset();
    m_spawned = 0;
    m_started = false;
}
//
void AMinionSpawner::EndPlay(const EEndPlayReason::Type reason)
{
    Reset();
    Super::EndPlay(reason);
}
//
