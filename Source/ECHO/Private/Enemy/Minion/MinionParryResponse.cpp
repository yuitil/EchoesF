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

//判定に使う幅や待ち時間。
namespace MinionParryResponseTuning
{
    //停止時間の下限。
    constexpr auto minPause = .08f;
    //再生速度の下限。
    constexpr auto minRate = .01f;
}

//相手の受け流しが成功するか確認する。
bool UMinionParryResponse::TryParry(AActor* _defender,float _inputAge)
{
    //処理の対象となる敵。
    auto* enemy=Cast<AMinionEnemyCharacter>(GetOwner());
    return enemy && FMath::IsFinite(_inputAge) && _inputAge>=0.f && enemy->IsDefenseInputValid(EDefenseResult::Parry,_inputAge) && Begin(_defender);
}

//受け流しによる隙を開始する。
bool UMinionParryResponse::Begin(AActor* _defender)
{
    //処理の対象となる敵。
    auto* enemy=Cast<AMinionEnemyCharacter>(GetOwner());
    if(!enemy || !enemy->HasAuthority() || !enemy->IsCombatActive() || !enemy->IsActing() || !IsValid(_defender) || _defender!=enemy->m_combatTarget || m_paused.IsValid()) { return false; }
    //使用する行動の設定。
    const auto* action=enemy->GetActiveAction(); if(!action || action->m_unblockable || action->m_damage<=0.f) { return false; }
    if(!enemy->m_combat->IsHitWindowOpen() || FVector::Dist2D(enemy->GetActorLocation(),_defender->GetActorLocation())>action->m_maxRange) { return false; }
    //アニメーションを再生する部品。
    auto* anim=enemy->GetMesh()->GetAnimInstance(); if(!anim || !anim->Montage_IsPlaying(action->m_montage)) { return false; }
    m_paused=action->m_montage; anim->Montage_Pause(m_paused.Get());
    enemy->m_combat->EndHitWindow();
    enemy->ShowGuardBreak();
    if(auto* ai=Cast<AAIController>(enemy->GetController())) ai->StopMovement();
    enemy->GetCharacterMovement()->StopMovementImmediately();
    GetWorld()->GetSubsystem<UMinionCombatDirector>()->PauseTarget(_defender,m_freezeTime+m_recovery);
    GetWorld()->GetTimerManager().SetTimer(m_timer,this,&UMinionParryResponse::Finish,FMath::Clamp(m_freezeTime,MinionParryResponseTuning::minRate,.5f),false);
    return true;
}

//受け流し後の停止を解除する。
void UMinionParryResponse::Finish()
{
    //処理の対象となる敵。
    auto* enemy=Cast<AMinionEnemyCharacter>(GetOwner());
    if(enemy && m_paused.IsValid())
    {
        if(auto* anim=enemy->GetMesh()->GetAnimInstance()) anim->Montage_Stop(MinionParryResponseTuning::minPause,m_paused.Get());
        if(enemy->IsCombatActive()) enemy->FinishParryReaction(m_reaction,m_recovery);
    }
    m_paused.Reset();
}

//終了時にタイマーや登録を解除する。
void UMinionParryResponse::EndPlay(const EEndPlayReason::Type _reason)
{
    GetWorld()->GetTimerManager().ClearTimer(m_timer);m_paused.Reset();Super::EndPlay(_reason);
}
