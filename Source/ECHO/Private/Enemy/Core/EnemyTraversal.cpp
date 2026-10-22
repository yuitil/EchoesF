#include "Enemy/Core/EnemyTraversal.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Engine/World.h"

UEnemyTraversal::UEnemyTraversal()
{
 PrimaryComponentTick.bCanEverTick = true;
}

bool UEnemyTraversal::TryTraverse(AActor* target)
{
 auto* enemy = Cast<AEnemyCharacterBase>(GetOwner());
 if (!m_enabled || m_running || !enemy || !enemy->HasAuthority() || !IsValid(target) || !enemy->CanDecide() || !enemy->GetPendingComboId().IsNone()) return false;
 auto* move = enemy->GetCharacterMovement();
 if (!move->IsMovingOnGround()) return false;
 const FVector start = enemy->GetActorLocation();
 const FVector toward = (target->GetActorLocation()-start).GetSafeNormal2D();
 const float distance = FVector::Dist2D(start,target->GetActorLocation());
 if (toward.IsNearlyZero() || distance < enemy->GetMeleeDistance()) return false;
 const float half = enemy->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
 const float radius = enemy->GetCapsuleComponent()->GetScaledCapsuleRadius();
 const FVector feet = start-FVector(0,0,half);
 FCollisionQueryParams params(SCENE_QUERY_STAT(EnemyTraversal),false,enemy);
 FHitResult obstacle;
 const bool wall = GetWorld()->LineTraceSingleByChannel(obstacle,feet+FVector(0,0,25),feet+toward*m_probe+FVector(0,0,25),ECC_Pawn,params);
 const bool targetBelow = target->GetActorLocation().Z < start.Z-move->MaxStepHeight;
 if (!wall && !targetBelow) return false;
 auto* nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
 if (!nav) return false;
 const float gravity = -move->GetGravityZ();
 if (gravity < 1.f) return false;
 for (float reach = FMath::Max(radius*2.f+20.f, wall ? obstacle.Distance+radius+25.f : m_probe); reach <= FMath::Min(m_range,distance-60.f); reach += 35.f)
 {
  const FVector sample = feet+toward*reach;
  FHitResult floor;
  if (!GetWorld()->LineTraceSingleByChannel(floor,sample+FVector(0,0,m_up+25.f),sample-FVector(0,0,m_down+10.f),ECC_Pawn,params) || !move->IsWalkable(floor)) continue;
  const float height = floor.ImpactPoint.Z-feet.Z;
  if (height > m_up || height < -m_down || FMath::Abs(height) <= move->MaxStepHeight || (height>0.f && !wall)) continue;
  FNavLocation landing;
  if (!nav->ProjectPointToNavigation(floor.ImpactPoint,landing,FVector(25,25,25)) || FMath::Abs(landing.Location.Z-floor.ImpactPoint.Z)>20.f) continue;
  const FVector end = floor.ImpactPoint+FVector(0,0,half+3.f);
  const FCollisionShape capsule = FCollisionShape::MakeCapsule(radius,half);
  if (GetWorld()->OverlapBlockingTestByChannel(end,FQuat::Identity,ECC_Pawn,capsule,params)) continue;
  const float dz = end.Z-start.Z;
  const float vz = FMath::Sqrt(2.f*gravity*(FMath::Max(0.f,dz)+m_clearance));
  const float time = (vz+FMath::Sqrt(FMath::Max(0.f,vz*vz-2.f*gravity*dz)))/gravity;
  FVector velocity=(end-start)/time; velocity.Z=vz;
  if (velocity.Size2D()>m_speed) continue;
  bool clear=true; FVector previous=start;
  for (int32 i=1;i<=32;++i)
  {
   const float t=time*i/32.f;
   const FVector next=start+velocity*t-FVector(0,0,.5f*gravity*t*t);
   FHitResult hit;
   if (GetWorld()->SweepSingleByChannel(hit,previous,next,FQuat::Identity,ECC_Pawn,capsule,params)) { clear=false; break; }
   previous=next;
  }
  if (!clear) continue;
  if (auto* ai=Cast<AAIController>(enemy->GetController())) ai->StopMovement();
  move->StopMovementImmediately();
  enemy->SetActorRotation(toward.Rotation());
  m_running=true; m_started=GetWorld()->GetTimeSeconds(); m_next=m_started+time+m_interval;
  enemy->LaunchCharacter(velocity,true,true);
  return true;
 }
 return false;
}

void UEnemyTraversal::TickComponent(float delta,ELevelTick type,FActorComponentTickFunction* function)
{
 Super::TickComponent(delta,type,function);
 auto* enemy=Cast<AEnemyCharacterBase>(GetOwner());
 if (!enemy || !enemy->HasAuthority()) return;
 const double now=GetWorld()->GetTimeSeconds();
 if (m_running)
 {
  if (!enemy->IsCombatActive() || now-m_started>3. || (now-m_started>.1 && enemy->GetCharacterMovement()->IsMovingOnGround())) m_running=false;
  return;
 }
 if (now<m_next) return;
 m_next=now+FMath::Max(.1f,m_interval);
 TryTraverse(enemy->m_combatTarget);
}
