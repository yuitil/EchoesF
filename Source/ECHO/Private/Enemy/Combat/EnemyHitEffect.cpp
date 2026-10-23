#include "Enemy/Combat/EnemyHitEffect.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
//
UEnemyHitEffect::UEnemyHitEffect()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

bool UEnemyHitEffect::EstimateContact(AActor* attacker, FVector& point, FVector& normal) const
{
    const ACharacter* victim = Cast<ACharacter>(GetOwner());
    if (!victim || !IsValid(attacker)) { return false; }
    FVector source = attacker->GetActorLocation();
    float best = TNumericLimits<float>::Max();
    TArray<AActor*> sources;
    attacker->GetAttachedActors(sources);
    sources.Add(attacker);
    for (AActor* actor : sources)
    {
        if (!IsValid(actor)) { continue; }
        TArray<UMeshComponent*> meshes;
        actor->GetComponents(meshes);
        for (UMeshComponent* mesh : meshes)
        {
            if (!mesh->DoesSocketExist(m_base) || !mesh->DoesSocketExist(m_tip)) { continue; }
            const FVector candidate = FMath::ClosestPointOnSegment(victim->GetActorLocation(), mesh->GetSocketLocation(m_base), mesh->GetSocketLocation(m_tip));
            const float distance = FVector::DistSquared(candidate, victim->GetActorLocation());
            if (distance < best) { best = distance; source = candidate; }
        }
    }
    FName bone;
    float distance = 0.f;
    if (victim->GetMesh()->K2_GetClosestPointOnPhysicsAsset(source, point, normal, bone, distance))
    {
        normal = normal.GetSafeNormal();
        if (normal.IsNearlyZero()) { normal = (source - victim->GetActorLocation()).GetSafeNormal(); }
        return !point.ContainsNaN() && !normal.ContainsNaN();
    }
    if (victim->GetCapsuleComponent()->GetClosestPointOnCollision(source, point) < 0.f) { return false; }
    normal = (source - point).GetSafeNormal();
    if (normal.IsNearlyZero()) { normal = -victim->GetActorForwardVector(); }
    return !point.ContainsNaN();
}

void UEnemyHitEffect::PlayHit(AActor* attacker, const FHitResult* hit)
{
    if (!GetOwner()->HasAuthority() || !m_effect) { return; }
    FVector point, normal;
    if (hit && hit->GetActor() == GetOwner() && !hit->bStartPenetrating && !hit->ImpactPoint.ContainsNaN())
    {
        point = hit->ImpactPoint;
        normal = hit->ImpactNormal.GetSafeNormal();
    }
    else if (!EstimateContact(attacker, point, normal)) { return; }
    ShowHit(point, normal);
}

void UEnemyHitEffect::ShowHit_Implementation(FVector point, FVector normal)
{
    if (!m_effect || GetNetMode() == NM_DedicatedServer) { return; }
    UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, m_effect, point + normal * m_offset,
        normal.Rotation(), m_scale, true, true, ENCPoolMethod::AutoRelease);
}
//
