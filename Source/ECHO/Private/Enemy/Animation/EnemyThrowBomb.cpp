#include "Enemy/Animation/EnemyThrowBomb.h"
#include "Enemy/Combat/EnemyBomb.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Engine/World.h"

//動作を確かめるための幅や待ち時間。
namespace EnemyThrowBombTuning
{
    //一度に生成できる弾の上限。
    constexpr auto maxBombs = 3;
}

//アニメーションの指定位置で処理を呼ぶ。
void UEnemyThrowBomb::Notify(USkeletalMeshComponent* _mesh,UAnimSequenceBase* _anim,const FAnimNotifyEventReference& _reference)
{
    Super::Notify(_mesh,_anim,_reference);
    //処理の対象となる敵。
    auto* enemy=_mesh ? Cast<AEnemyCharacterBase>(_mesh->GetOwner()) : nullptr;
    //使用する行動の設定。
    const auto* action=enemy ? enemy->GetActiveAction() : nullptr;
    if(!enemy || !enemy->HasAuthority() || !enemy->IsCombatActive() || !enemy->IsActing() || !action || !m_bomb || !IsValid(enemy->m_combatTarget)) { return; }
    //狙っている相手。
    auto* target=enemy->m_combatTarget.Get();
    //調べ始める位置。
    const FVector start=(_mesh->DoesSocketExist(m_socket) ? _mesh->GetSocketLocation(m_socket) : enemy->GetActorLocation()) + enemy->GetActorRotation().RotateVector(m_offset);
    //移動の目的地。
    const FVector goal=target->GetActorLocation() + target->GetVelocity().GetClampedToMaxSize(m_maxLeadSpeed)*FMath::Clamp(m_lead,0.f,m_maxLead);
    //生成や当たり判定に使う設定。
    FActorSpawnParameters params;params.Owner=enemy;params.Instigator=enemy;params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    //対象の数。
    const int32 count=FMath::Clamp(m_count,1,EnemyThrowBombTuning::maxBombs);
    //生成した敵や弾。
    TArray<AEnemyBomb*> spawned;
    for(int32 i=0;i<count;++i)
    {
        //生成した呪石弾。
        auto* bomb=enemy->GetWorld()->SpawnActor<AEnemyBomb>(m_bomb,start,FRotator::ZeroRotator,params);
        if(!bomb) continue;
        for(auto* other:spawned)
        {
            bomb->m_collision->IgnoreActorWhenMoving(other,true);
            other->m_collision->IgnoreActorWhenMoving(bomb,true);
        }
        spawned.Add(bomb);
        //基準となる位置。
        FVector point=goal+enemy->GetActorRightVector()*(i-(count-1)*.5f)*FMath::Max(0.f,m_spread);
        //体の中心ではなく足元を狙い、距離に合わせて飛ぶ時間を変える。
        FHitResult floorHit;
        //着地点の地面を調べる設定。
        FCollisionQueryParams floorQuery(SCENE_QUERY_STAT(BombFloor), false, enemy);
        floorQuery.AddIgnoredActor(target);
        for(auto* stone : spawned) { floorQuery.AddIgnoredActor(stone); }
        if(enemy->GetWorld()->LineTraceSingleByChannel(floorHit, point + FVector::UpVector * m_floorRise,
            point - FVector::UpVector * m_floorDepth, ECC_Visibility, floorQuery))
        {
            point = floorHit.ImpactPoint + floorHit.ImpactNormal * bomb->m_collision->GetScaledSphereRadius();
        }
        //弾が届くまでの時間。
        const float flight=FMath::Clamp(FVector::Dist2D(start,point)/FMath::Max(1.f,m_speed),m_minFlight,FMath::Max(m_minFlight,m_flight));
        //現在の移動速度。
        FVector velocity=(point-start)/flight;
        velocity.Z-=.5f*enemy->GetWorld()->GetGravityZ()*bomb->m_move->ProjectileGravityScale*flight;
        bomb->Launch(enemy,target,velocity,action->m_damage,action->m_unblockable);
    }
}
