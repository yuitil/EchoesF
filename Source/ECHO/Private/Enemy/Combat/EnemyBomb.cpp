#include "Enemy/Combat/EnemyBomb.h"
#include "Enemy/Audio/EnemyAudio.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Player/CombatComponent.h"

#include "TimerManager.h"
#include "Engine/World.h"

//動作を確かめるための幅や待ち時間。
namespace EnemyBombTuning
{
    //弾の当たり判定の半径。
    constexpr auto ballRadius = 14.f;
    
    //着地した弾の跳ね返りの強さ。
    constexpr auto bounceRatio = .15f;
    
    //生成した弾を残す時間。
    constexpr auto lifeTime = 8.f;
    
    //爆発までの待ち時間の下限。
    constexpr auto minFuse = .3f;
    
    //命中位置から演出をずらす距離。
    constexpr auto hitOffset = 2.f;
}

//初期設定と必要な部品を用意する。
AEnemyBomb::AEnemyBomb()
{
    bReplicates = true;
    SetReplicateMovement(true);
    m_collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    SetRootComponent(m_collision);
    m_collision->InitSphereRadius(EnemyBombTuning::ballRadius);
    m_collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    m_collision->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);
    m_mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Stone"));
    m_mesh->SetupAttachment(m_collision);
    m_mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    m_move = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Flight"));
    m_move->ProjectileGravityScale = 1.f;
    m_move->bRotationFollowsVelocity = true;
    m_move->bShouldBounce = true;
    m_move->Bounciness = EnemyBombTuning::bounceRatio;
    m_move->Friction = .5f;
    m_move->BounceVelocityStopSimulatingThreshold = 0.f;
    m_move->bForceSubStepping = true;
    m_move->OnProjectileBounce.AddDynamic(this, &AEnemyBomb::Landed);
    m_move->OnProjectileStop.AddDynamic(this, &AEnemyBomb::Stopped);
    InitialLifeSpan = EnemyBombTuning::lifeTime;
}

//ゲーム開始時に必要な状態を準備する。
void AEnemyBomb::BeginPlay()
{
    Super::BeginPlay();
    if (GetOwner()) m_collision->IgnoreActorWhenMoving(GetOwner(),true);
    if(m_explodeOnHit) 
    { 
        m_collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    
    }

}

//弾の飛ぶ方向と速さを設定する。
void AEnemyBomb::Launch(AEnemyCharacterBase* _source,AActor* _target,FVector _velocity,float _damage,bool _unblockable)
{
    m_source=_source; m_target=_target; m_damage=FMath::Max(0.f,_damage); m_unblockable=_unblockable;
    m_move->Velocity=_velocity;
}

//弾が触れた場所で着弾処理を行う。
void AEnemyBomb::Landed(const FHitResult& _hit, const FVector& _velocity)
{
    //| m_armed|同じ弾で着弾処理を二度行わないようにする。
    if(!HasAuthority() || m_armed || _hit.GetActor() == GetOwner()) { return; }
    if(!m_explodeOnHit && _hit.ImpactNormal.Z < m_floorNormal) { return; }
    m_armed=true;
   
    //着弾した弾の移動と接触判定を止める。
    m_move->StopMovementImmediately();
    m_move->Deactivate();
    m_collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    //攻撃が当たった位置。
    const FVector hitPoint = _hit.bStartPenetrating ? GetActorLocation() : FVector(_hit.ImpactPoint);
    m_groundPoint=hitPoint+_hit.ImpactNormal*EnemyBombTuning::hitOffset;
    m_groundNormal=_hit.ImpactNormal;
    if(m_debugHit)
    {
        UE_LOG(LogTemp, Display, TEXT("EnemyBomb Impact %s hit=%s point=%s time=%.3f immediate=%d"),
            *GetName(), *GetNameSafe(_hit.GetActor()), *m_groundPoint.ToString(), GetWorld()->GetTimeSeconds(), m_explodeOnHit);
    }

    //即時爆発の設定なら、待ち時間を挟まず爆発する。
    if(m_explodeOnHit) { Explode(); return; }

    //BPの着地点予告を出してから爆発を待つ。
    Armed();
    GetWorldTimerManager().SetTimer(m_timer,this,&AEnemyBomb::Explode,FMath::Max(EnemyBombTuning::minFuse,m_fuse),false);
}

//弾の移動が止まった時に着弾処理を行う。
void AEnemyBomb::Stopped(const FHitResult& _hit)
{
    Landed(_hit, FVector::ZeroVector);
}

//爆発のダメージと演出を発生させる。
void AEnemyBomb::Explode()
{
    //| m_armed|同じ弾で着弾処理を二度行わないようにする。
    if(!HasAuthority() || !m_armed) { return; }
    
    //基準となる位置。
    const FVector point=m_groundPoint;
   
    //狙っている相手。
    auto* target=m_target.Get(); auto* source=m_source.Get();
    if (IsValid(target) && target->CanBeDamaged() && FVector::DistSquared(point,target->GetActorLocation())<=FMath::Square(FMath::Max(1.f,m_radius)))
    {
        //進行方向の障害物の情報。
        FHitResult wall;
        
        //生成や当たり判定に使う設定。
        FCollisionQueryParams params(SCENE_QUERY_STAT(EnemyBomb),false,this);
        if (source) { params.AddIgnoredActor(source); }
       
        //爆発と相手の間に障害物があるか。
        const bool blocked=GetWorld()->LineTraceSingleByChannel(wall,point,target->GetActorLocation(),ECC_Visibility,params) && wall.GetActor()!=target;
       
        //相手の回避状態を調べる部品。
        const auto* combat=target->FindComponentByClass<UCombatComponent>();
        
        //回避できたかを確認する値。
        const bool dodge=combat && combat->IsDodging();
        
        //相手が攻撃を防いだ方法。
        const EDefenseResult defense=source ? source->ResolveTargetDefense(target) : EDefenseResult::None;
        
        //相手が攻撃を防いだか。
        const bool defended=defense==EDefenseResult::PerfectDodge || (!m_unblockable && defense!=EDefenseResult::None);
        
        //障害物や有効な防御がない相手だけにダメージを与える。
        if (!blocked && !dodge && !defended)
        {
            if(source && m_unblockable && defense==EDefenseResult::Guard) source->TargetGuardBroken(target);
            UGameplayStatics::ApplyDamage(target,m_damage,source ? source->GetController() : nullptr,source, m_unblockable ? UEBUnblockableDamageType::StaticClass() : UEBCombatDamageType::StaticClass());
        }
    }
    //爆発の見た目を出して弾を片付ける。
    if(source && source->m_audio) { source->m_audio->Play(EEnemySound::Blast, point); }
    ShowExplosion(point);
    if(m_debugHit) 
    { 
        UE_LOG(LogTemp, Display, TEXT("EnemyBomb Burst %s time=%.3f"), *GetName(), GetWorld()->GetTimeSeconds()); 
    }
    Destroy();
}

//着弾位置で爆発の見た目を再生する。
void AEnemyBomb::ShowExplosion_Implementation(FVector _point)
{
    if (GetNetMode()!=NM_DedicatedServer) BurstAt(_point);
}

//終了時にタイマーや登録を解除する。
void AEnemyBomb::EndPlay(const EEndPlayReason::Type _reason)
{
    GetWorldTimerManager().ClearTimer(m_timer);
    Super::EndPlay(_reason);
}
