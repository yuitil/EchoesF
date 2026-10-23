#include "Enemy/Minion/MinionParryResponse.h"
#include "Enemy/Minion/MinionEnemyCharacter.h"
#include "Enemy/Minion/MinionFormation.h"
#include "Enemy/Minion/MinionCombatDirector.h"
#include "Enemy/Combat/Combat.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"
//
bool UMinionParryResponse::TryParry(AActor* defender,float inputAge)
{
    auto* enemy=Cast<AMinionEnemyCharacter>(GetOwner());
    return enemy && FMath::IsFinite(inputAge) && inputAge>=0.f && enemy->IsDefenseInputValid(EDefenseResult::Parry,inputAge) && Begin(defender);
}
bool UMinionParryResponse::Begin(AActor* defender)
{
    auto* enemy=Cast<AMinionEnemyCharacter>(GetOwner());
    if(!enemy || !enemy->HasAuthority() || !enemy->IsCombatActive() || !enemy->IsActing() || !IsValid(defender) || defender!=enemy->m_combatTarget || m_paused.IsValid()) return false;
    const auto* action=enemy->GetActiveAction(); if(!action || action->m_unblockable || action->m_damage<=0.f) return false;
    if(!enemy->m_combat->IsHitWindowOpen() || FVector::Dist2D(enemy->GetActorLocation(),defender->GetActorLocation())>action->m_maxRange) return false;
    auto* anim=enemy->GetMesh()->GetAnimInstance(); if(!anim || !anim->Montage_IsPlaying(action->m_montage)) return false;
    m_paused=action->m_montage; anim->Montage_Pause(m_paused.Get());
    enemy->m_combat->EndHitWindow();
    if(auto* ai=Cast<AAIController>(enemy->GetController())) ai->StopMovement();
    enemy->GetCharacterMovement()->StopMovementImmediately();
    GetWorld()->GetSubsystem<UMinionCombatDirector>()->PauseTarget(defender,m_freezeTime+m_recovery);
    GetWorld()->GetTimerManager().SetTimer(m_timer,this,&UMinionParryResponse::Finish,FMath::Clamp(m_freezeTime,.01f,.5f),false);
    return true;
}
void UMinionParryResponse::Finish()
{
    auto* enemy=Cast<AMinionEnemyCharacter>(GetOwner());
    if(enemy && m_paused.IsValid())
    {
        if(auto* anim=enemy->GetMesh()->GetAnimInstance()) anim->Montage_Stop(.08f,m_paused.Get());
        if(enemy->IsCombatActive()) enemy->FinishParryReaction(m_reaction,m_recovery);
    }
    m_paused.Reset();
}
void UMinionParryResponse::EndPlay(const EEndPlayReason::Type reason)
{
    GetWorld()->GetTimerManager().ClearTimer(m_timer);m_paused.Reset();Super::EndPlay(reason);
}
//
