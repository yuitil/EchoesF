#include "Enemy/Combat/EnemyHitEffect.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

//初期設定と必要な部品を用意する。
UEnemyHitEffect::UEnemyHitEffect()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

//相手との位置関係から命中位置を求める。
bool UEnemyHitEffect::EstimateContact(AActor* _attacker, FVector& _point, FVector& _normal) const
{
    //ダメージを受ける相手。
    const ACharacter* victim = Cast<ACharacter>(GetOwner());
    if(!victim || !IsValid(_attacker)) { return false; }
    //処理や攻撃の元となる対象。
    FVector source = _attacker->GetActorLocation();
    //今までで最も適した候補。
    float best = TNumericLimits<float>::Max();
    //命中位置の推定に使う候補。
    TArray<AActor*> sources;
    _attacker->GetAttachedActors(sources);
    sources.Add(_attacker);
    for (AActor* actor : sources)
    {
        if (!IsValid(actor)) { continue; }
        //調べるモデルの一覧。
        TArray<UMeshComponent*> meshes;
        actor->GetComponents(meshes);
        for (UMeshComponent* mesh : meshes)
        {
            if (!mesh->DoesSocketExist(m_base) || !mesh->DoesSocketExist(m_tip)) { continue; }
            //現在調べている候補。
            const FVector candidate = FMath::ClosestPointOnSegment(victim->GetActorLocation(), mesh->GetSocketLocation(m_base), mesh->GetSocketLocation(m_tip));
            //相手までの距離。
            const float distance = FVector::DistSquared(candidate, victim->GetActorLocation());
            if (distance < best) { best = distance; source = candidate; }
        }
    }
    //対象のボーン。
    FName bone;
    //相手までの距離。
    float distance = 0.f;
    if (victim->GetMesh()->K2_GetClosestPointOnPhysicsAsset(source, _point, _normal, bone, distance))
    {
        _normal = _normal.GetSafeNormal();
        if (_normal.IsNearlyZero()) { _normal = (source - victim->GetActorLocation()).GetSafeNormal(); }
        return !_point.ContainsNaN() && !_normal.ContainsNaN();
    }
    if(victim->GetCapsuleComponent()->GetClosestPointOnCollision(source, _point) < 0.f) { return false; }
    _normal = (source - _point).GetSafeNormal();
    if (_normal.IsNearlyZero()) { _normal = -victim->GetActorForwardVector(); }
    return !_point.ContainsNaN();
}

//体に当たった場所の演出を開始する。
void UEnemyHitEffect::PlayHit(AActor* _attacker, const FHitResult* _hit)
{
    if(!GetOwner()->HasAuthority() || !m_effect) { return; }
    FVector point, normal;
    if (_hit && _hit->GetActor() == GetOwner() && !_hit->bStartPenetrating && !_hit->ImpactPoint.ContainsNaN())
    {
        point = _hit->ImpactPoint;
        normal = _hit->ImpactNormal.GetSafeNormal();
    }
    else if(!EstimateContact(_attacker, point, normal)) { return; }
    ShowHit(point, normal);
}

//命中位置でエフェクトを再生する。
void UEnemyHitEffect::ShowHit_Implementation(FVector _point, FVector _normal)
{
    if(!m_effect || GetNetMode() == NM_DedicatedServer) { return; }
    UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, m_effect, _point + _normal * m_offset,
        _normal.Rotation(), m_scale, true, true, ENCPoolMethod::AutoRelease);
}

//相手に近い剣の位置を使う。剣がなければ体の接触位置を使う。
void UEnemyHitEffect::PlayClash(AActor* _attacker)
{
    if(!GetOwner()->HasAuthority() || !m_clashEffect || !IsValid(_attacker)) { return; }

    FVector point, normal;
    if(!EstimateContact(_attacker, point, normal)) { return; }

    //今までで最も適した候補。
    float best = TNumericLimits<float>::Max();
    //調べるアクターの一覧。
    TArray<AActor*> actors;
    GetOwner()->GetAttachedActors(actors);
    actors.Add(GetOwner());
    for(AActor* actor : actors)
    {
        if(!IsValid(actor)) { continue; }

        //調べるモデルの一覧。
        TArray<UMeshComponent*> meshes;
        actor->GetComponents(meshes);
        for(UMeshComponent* mesh : meshes)
        {
            if(!mesh->DoesSocketExist(m_base) || !mesh->DoesSocketExist(m_tip)) { continue; }

            //演出を表示する位置。
            const FVector spot = FMath::ClosestPointOnSegment(point, mesh->GetSocketLocation(m_base), mesh->GetSocketLocation(m_tip));
            //二つの値の差。
            const float gap = FVector::DistSquared(spot, _attacker->GetActorLocation());
            if(gap < best) { best = gap; point = spot; }
        }
    }

    ShowClash(point, normal);
}

//体の命中とは別の火花を出す。
void UEnemyHitEffect::ShowClash_Implementation(FVector _point, FVector _normal)
{
    if(!m_clashEffect || GetNetMode() == NM_DedicatedServer) { return; }

    UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, m_clashEffect, _point,
        _normal.Rotation(), m_clashScale, true, true, ENCPoolMethod::AutoRelease);
}
