#include "Enemy/Combat/EnemyBomb.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Player/CombatComponent.h"

#include "TimerManager.h"
#include "Engine/World.h"
//
AEnemyBomb::AEnemyBomb()
{
    bReplicates = true;
    SetReplicateMovement(true);
    m_collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    SetRootComponent(m_collision);
    m_collision->InitSphereRadius(14.f);
    m_collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    m_collision->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);
    m_mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Stone"));
    m_mesh->SetupAttachment(m_collision);
    m_mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    m_move = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Flight"));
    m_move->ProjectileGravityScale = 1.f;
    m_move->bRotationFollowsVelocity = true;
    m_move->bShouldBounce = true;
    m_move->Bounciness = .15f;
    m_move->Friction = .5f;
    m_move->BounceVelocityStopSimulatingThreshold = 0.f;
    m_move->bForceSubStepping = true;
    m_move->OnProjectileBounce.AddDynamic(this, &AEnemyBomb::Landed);
    InitialLifeSpan = 8.f;
}
void AEnemyBomb::BeginPlay()
{
    Super::BeginPlay();
    if (GetOwner()) m_collision->IgnoreActorWhenMoving(GetOwner(),true);

}
void AEnemyBomb::Launch(AEnemyCharacterBase* source,AActor* target,FVector velocity,float damage,bool unblockable)
{
    m_source=source; m_target=target; m_damage=FMath::Max(0.f,damage); m_unblockable=unblockable;
    m_move->Velocity=velocity;
}
void AEnemyBomb::Landed(const FHitResult& hit, const FVector& velocity)
{
    if (!HasAuthority() || m_armed || hit.bStartPenetrating || hit.ImpactNormal.Z < m_floorNormal) return;
    m_move->StopMovementImmediately();
    m_move->Deactivate();
    m_armed=true;
    m_collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    //
    m_groundPoint=hit.ImpactPoint+hit.ImpactNormal*2.f;
    m_groundNormal=hit.ImpactNormal;
    //
    Armed();
    GetWorldTimerManager().SetTimer(m_timer,this,&AEnemyBomb::Explode,FMath::Max(.3f,m_fuse),false);
}
void AEnemyBomb::Explode()
{
    if (!HasAuthority() || !m_armed) return;
    const FVector point=GetActorLocation();
    auto* target=m_target.Get(); auto* source=m_source.Get();
    if (IsValid(target) && target->CanBeDamaged() && FVector::DistSquared(point,target->GetActorLocation())<=FMath::Square(FMath::Max(1.f,m_radius)))
    {
        FHitResult wall;
        FCollisionQueryParams params(SCENE_QUERY_STAT(EnemyBomb),false,this);
        if(source) params.AddIgnoredActor(source);
        const bool blocked=GetWorld()->LineTraceSingleByChannel(wall,point,target->GetActorLocation(),ECC_Visibility,params) && wall.GetActor()!=target;
        const auto* combat=target->FindComponentByClass<UCombatComponent>();
        const bool dodge=combat && combat->IsDodging();
        const EDefenseResult defense=source ? source->ResolveTargetDefense(target) : EDefenseResult::None;
        const bool defended=defense==EDefenseResult::PerfectDodge || (!m_unblockable && defense!=EDefenseResult::None);
        if (!blocked && !dodge && !defended)
        {
            if(source && m_unblockable && defense==EDefenseResult::Guard) source->TargetGuardBroken(target);
            UGameplayStatics::ApplyDamage(target,m_damage,source ? source->GetController() : nullptr,source,
                m_unblockable ? UEBUnblockableDamageType::StaticClass() : UEBCombatDamageType::StaticClass());
        }
    }
    ShowExplosion(point);
    Destroy();
}
void AEnemyBomb::ShowExplosion_Implementation(FVector point)
{
    if (GetNetMode()!=NM_DedicatedServer) BurstAt(point);
}
void AEnemyBomb::EndPlay(const EEndPlayReason::Type reason)
{
    GetWorldTimerManager().ClearTimer(m_timer);
    Super::EndPlay(reason);
}
//
