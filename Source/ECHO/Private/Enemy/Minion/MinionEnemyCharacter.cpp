#include "Enemy/Minion/MinionEnemyCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "Engine/World.h"
#include "Enemy/Core/EnemyData.h"
#include "Animation/AnimMontage.h"
#include "Enemy/Minion/MinionFormation.h"
#include "Enemy/Minion/MinionParryResponse.h"

//判定に使う幅や待ち時間。
namespace MinionEnemyCharacterTuning
{
    //死亡演出を待つ時間の上限。
    constexpr auto maxDeathTime = 30.f;
    
    //待ち時間の下限。
    constexpr auto minWait = .1f;
    
    //死亡アニメーションの速度の下限。
    constexpr auto minDeathRate = .01f;
}

//初期設定と必要な部品を用意する。
AMinionEnemyCharacter::AMinionEnemyCharacter()
{
    m_formation=CreateDefaultSubobject<UMinionFormation>(TEXT("Formation"));
    m_parry=CreateDefaultSubobject<UMinionParryResponse>(TEXT("ParryResponse"));
}

//現在の状態で選べる攻撃か調べる。
bool AMinionEnemyCharacter::IsActionAllowed(const FEnemyActionData& _action) const
{
    return _action.m_kind==EEnemyAction::Guard || _action.m_kind==EEnemyAction::Dodge || m_formation->HasTurn();
}

//攻撃終了後の次の行動を準備する。
void AMinionEnemyCharacter::OnAttackFinished()
{
    m_formation->ReleaseTurn();
}

//行動終了後の状態を整える。
void AMinionEnemyCharacter::OnActionEnded()
{
    Super::OnActionEnded();
    if (!m_actionSuccess)
    {
        m_formation->ReleaseTurn();
    }
}

//攻撃を防がれた結果を受け取る。
void AMinionEnemyCharacter::AttackDefended_Implementation(AActor* _defender, EDefenseResult _result)
{
    if(_result==EDefenseResult::Parry)
    {
        m_parry->Begin(_defender);
        return;
    }
    Super::AttackDefended_Implementation(_defender,_result);
    if (!IsActing() && (_result == EDefenseResult::Parry || _result == EDefenseResult::Deflect))
    {
        m_formation->ReleaseTurn();
    }
}

//受け流された後の停止を解除する。
void AMinionEnemyCharacter::FinishParryReaction(UAnimMontage* _montage,float _duration)
{
    CancelAction();
    
    //対象の長さ。
    const float length=_montage ? PlayAnimMontage(_montage) : 0.f;
    m_readyTime=GetWorld()->GetTimeSeconds()+FMath::Max(FMath::Max(MinionEnemyCharacterTuning::minWait,_duration),length);
    m_formation->ReleaseTurn();
}

//体力がなくなった時の処理を行う。
void AMinionEnemyCharacter::HandleHealthDepleted()
{
    Super::HandleHealthDepleted();
    DetachFromControllerPendingDestroy();
    
    //対象の長さ。
    const float length = m_definition && m_definition->m_deathMontage ? m_definition->m_deathMontage->GetPlayLength() / FMath::Max(MinionEnemyCharacterTuning::minDeathRate, FMath::Abs(m_definition->m_deathMontage->RateScale)) : 0.f;
    SetLifeSpan(FMath::Clamp(length, 0.f, MinionEnemyCharacterTuning::maxDeathTime) + FMath::Max(MinionEnemyCharacterTuning::minWait, m_corpseTime));
}

//ダメージを受けた後の反応を行う。
void AMinionEnemyCharacter::OnAcceptedHit(AActor* _attacker)
{
    Super::OnAcceptedHit(_attacker);

    m_formation->Provoke(_attacker);

    //現在の時刻。
    const double now = GetWorld()->GetTimeSeconds();
    if(!IsCombatActive() || !m_hitMontage || now < m_nextHit || (IsActing() && !m_interruptAttack)) { return; }
    CancelAction();
    if (auto* ai = Cast<AAIController>(GetController()))
    {
        ai->StopMovement();
    }
    GetCharacterMovement()->StopMovementImmediately();
   
    //対象の長さ。
    const float length = PlayAnimMontage(m_hitMontage);
    if (length > 0.f)
    {
        m_readyTime = now + length;
        m_nextHit = now + FMath::Max(length,m_hitCooldown);
    }
}
