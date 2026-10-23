#include "Enemy/Minion/MinionEnemyCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "Engine/World.h"
#include "Enemy/Core/EnemyData.h"
#include "Animation/AnimMontage.h"
#include "Enemy/Minion/MinionFormation.h"
#include "Enemy/Minion/MinionParryResponse.h"
//
AMinionEnemyCharacter::AMinionEnemyCharacter()
{
    m_formation=CreateDefaultSubobject<UMinionFormation>(TEXT("Formation"));
    m_parry=CreateDefaultSubobject<UMinionParryResponse>(TEXT("ParryResponse"));
}

bool AMinionEnemyCharacter::IsActionAllowed(const FEnemyActionData& action) const
{
    return action.m_kind==EEnemyAction::Guard || action.m_kind==EEnemyAction::Dodge || m_formation->HasTurn();
}

void AMinionEnemyCharacter::OnAttackFinished() 
{ 
    m_formation->ReleaseTurn(); 
}

//
void AMinionEnemyCharacter::OnActionEnded()
{
    Super::OnActionEnded();
    if (!m_actionSuccess)
    {
        m_formation->ReleaseTurn();
    }
}

//
void AMinionEnemyCharacter::AttackDefended_Implementation(AActor* defender, EDefenseResult result)
{
    if(result==EDefenseResult::Parry) 
    { 
        m_parry->Begin(defender); 
        return; 
    }
    Super::AttackDefended_Implementation(defender,result);
    if (!IsActing() && (result == EDefenseResult::Parry || result == EDefenseResult::Deflect))
    {
        m_formation->ReleaseTurn();
    }
}

void AMinionEnemyCharacter::FinishParryReaction(UAnimMontage* montage,float duration)
{
    CancelAction();
    const float length=montage ? PlayAnimMontage(montage) : 0.f;
    m_readyTime=GetWorld()->GetTimeSeconds()+FMath::Max(FMath::Max(.1f,duration),length);
    m_formation->ReleaseTurn();
}

//
void AMinionEnemyCharacter::HandleHealthDepleted()
{
    Super::HandleHealthDepleted();
    DetachFromControllerPendingDestroy();
    const float length = m_definition && m_definition->m_deathMontage ? m_definition->m_deathMontage->GetPlayLength() / FMath::Max(.01f, FMath::Abs(m_definition->m_deathMontage->RateScale)) : 0.f;
    SetLifeSpan(FMath::Clamp(length, 0.f, 30.f) + FMath::Max(.1f, m_corpseTime));
}

//
void AMinionEnemyCharacter::OnAcceptedHit(AActor* attacker)
{
    Super::OnAcceptedHit(attacker);
    //
    m_formation->Provoke(attacker);
    //
    const double now = GetWorld()->GetTimeSeconds();
    if (!IsCombatActive() || !m_hitMontage || now < m_nextHit || (IsActing() && !m_interruptAttack)) { return; }
    CancelAction();
    if (auto* ai = Cast<AAIController>(GetController()))
    {
        ai->StopMovement();
    }
    GetCharacterMovement()->StopMovementImmediately();
    const float length = PlayAnimMontage(m_hitMontage);
    if (length > 0.f)
    {
        m_readyTime = now + length;
        m_nextHit = now + FMath::Max(length,m_hitCooldown);
    }
}
//
