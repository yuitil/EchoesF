#include "Enemy/Animation/EnemyThrowBomb.h"
#include "Enemy/Combat/EnemyBomb.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Engine/World.h"
//
void UEnemyThrowBomb::Notify(USkeletalMeshComponent* mesh,UAnimSequenceBase* anim,const FAnimNotifyEventReference& reference)
{
    Super::Notify(mesh,anim,reference);
    auto* enemy=mesh ? Cast<AEnemyCharacterBase>(mesh->GetOwner()) : nullptr;
    const auto* action=enemy ? enemy->GetActiveAction() : nullptr;
    if(!enemy || !enemy->HasAuthority() || !enemy->IsCombatActive() || !enemy->IsActing() || !action || !m_bomb || !IsValid(enemy->m_combatTarget)) return;
    auto* target=enemy->m_combatTarget.Get();
    const FVector start=(mesh->DoesSocketExist(m_socket) ? mesh->GetSocketLocation(m_socket) : enemy->GetActorLocation()) + enemy->GetActorRotation().RotateVector(m_offset);
    const FVector goal=target->GetActorLocation() + target->GetVelocity().GetClampedToMaxSize(500.f)*FMath::Clamp(m_lead,0.f,.5f);
    FActorSpawnParameters params;params.Owner=enemy;params.Instigator=enemy;params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    const int32 count=FMath::Clamp(m_count,1,3);
    TArray<AEnemyBomb*> spawned;
    for(int32 i=0;i<count;++i)
    {
        auto* bomb=enemy->GetWorld()->SpawnActor<AEnemyBomb>(m_bomb,start,FRotator::ZeroRotator,params);
        if(!bomb) continue;
        for(auto* other:spawned)
        {
            bomb->m_collision->IgnoreActorWhenMoving(other,true);
            other->m_collision->IgnoreActorWhenMoving(bomb,true);
        }
        spawned.Add(bomb);
        const FVector point=goal+enemy->GetActorRightVector()*(i-(count-1)*.5f)*FMath::Max(0.f,m_spread);
        const float flight=FMath::Clamp(m_flight,.5f,3.f);
        FVector velocity=(point-start)/flight;
        velocity.Z-=.5f*enemy->GetWorld()->GetGravityZ()*bomb->m_move->ProjectileGravityScale*flight;
        bomb->Launch(enemy,target,velocity,action->m_damage,action->m_unblockable);
    }
}
//
