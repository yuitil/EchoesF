#include "Enemy/Boss/BossApproach.h"
#include "Enemy/Boss/BossEnemyCharacter.h"
#include "Enemy/Boss/BossSpacing.h"
#include "Enemy/Core/EnemyTraversal.h"
#include "Enemy/Combat/Defense.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Net/UnrealNetwork.h"
#include "Components/PrimitiveComponent.h"

//初期設定と必要な部品を用意する。
UBossApproach::UBossApproach()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);
}

//ゲーム開始時に必要な状態を準備する。
void UBossApproach::BeginPlay()
{
    Super::BeginPlay();
    UpdateSword();
}

//他の端末にも伝える値を登録する。
void UBossApproach::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& _props) const
{
    Super::GetLifetimeReplicatedProps(_props);
    //通信で共有する値の登録先。
    auto& OutLifetimeProps = _props;
    DOREPLIFETIME(UBossApproach, m_swordOut);
}

//抜刀の状態に合わせて刀の表示を切り替える。
void UBossApproach::UpdateSword()
{
    if(!GetOwner() || m_swordTag.IsNone()) { return; }
    for(auto* part : GetOwner()->GetComponentsByTag(UPrimitiveComponent::StaticClass(), m_swordTag))
    {
        if(auto* mesh = Cast<UPrimitiveComponent>(part)) { mesh->SetVisibility(m_swordOut, true); }
    }
}

//抜刀を通知して刀を表示する。
void UBossApproach::ShowSword()
{
    if(!GetOwner() || !GetOwner()->HasAuthority()) { return; }
    m_swordOut = true;
    UpdateSword();
}

//次の戦闘に向けて状態を初期化する。
void UBossApproach::Reset()
{
    m_drawn = false;
    m_drawing = false;
    m_runTime = 0.f;
    m_swordOut = false;
    m_drawClip = nullptr;
    UpdateSword();
}

//経過時間に合わせて敵の状態を更新する。
void UBossApproach::TickComponent(float _delta, ELevelTick _type, FActorComponentTickFunction* _tick)
{
    Super::TickComponent(_delta, _type, _tick);

    //対象のボス。
    auto* boss = Cast<ABossEnemyCharacter>(GetOwner());
    if(!boss || !boss->HasAuthority()) { return; }

    //アニメーションを再生する部品。
    auto* anim = boss->GetMesh()->GetAnimInstance();
    if(!boss->IsCombatActive()) { m_drawing = false; m_runTime = 0.f; return; }

    //狙っている相手。
    auto* target = boss->m_combatTarget.Get();
    //敵の行動を操作するAI。
    auto* ai = Cast<AAIController>(boss->GetController());
    //移動を管理する部品。
    auto* move = boss->GetCharacterMovement();
    if(!IsValid(target) || !ai)
    {
        if(m_drawing && (!anim || !anim->Montage_IsPlaying(m_drawClip))) { m_drawing=false; ShowSword(); }
        m_runTime = 0.f;
        return;
    }

    //抜刀が終わるまでは、抜刀に合った速さで近づく。
    if(m_drawing)
    {
        m_drawing = anim && anim->Montage_IsPlaying(m_drawClip);
        //抜刀が終わるまでは、抜刀に合った速さで近づく。
        if(m_drawing)
        {
            move->MaxWalkSpeed = m_drawSpeed;
            m_pathTime -= _delta;
            if(m_pathTime <= 0.f && m_drawSpeed > 0.f)
            {
                ai->MoveToActor(target, boss->GetMeleeDistance(), false);
                m_pathTime = m_pathDelay;
            }
            return;
        }
        ShowSword();
        move->MaxWalkSpeed = m_runSpeed;
    }

    //相手までの距離。
    const float distance = FVector::Dist2D(boss->GetActorLocation(), target->GetActorLocation());
    //別の行動に邪魔されず動けるか。
    const bool free = !boss->IsActing() && !boss->m_defense->IsBusy() && !boss->m_traversal->IsTraversing();
    //相手の距離と動きから、その場・歩き・走りの抜刀を選ぶ。
    if(!m_drawn && anim && free && distance <= m_drawRange && ai->LineOfSightTo(target))
    {
        //歩き抜刀を選ぶか。
        const bool walking = target->GetVelocity().Size2D() <= m_waitSpeed;
        //その場で抜刀するか。
        const bool inPlace = distance <= boss->GetMeleeDistance();
        m_drawClip = inPlace ? m_draw : walking ? m_walkDraw : m_runDraw;
        if(!m_drawClip) { m_drawClip = m_draw; }
        if(!m_drawClip) { m_drawn = true; ShowSword(); return; }
        boss->m_spacing->Stop();
        boss->SetActorRotation((target->GetActorLocation() - boss->GetActorLocation()).GetSafeNormal2D().Rotation());
        m_drawn = true;
        m_drawing = boss->PlayAnimMontage(m_drawClip, m_drawRate) > 0.f;
        if(!m_drawing) { ShowSword(); return; }
        m_drawSpeed = inPlace ? 0.f : walking ? m_walkDrawSpeed : m_runDrawSpeed;
        move->MaxWalkSpeed = m_drawSpeed;
        if(inPlace) { ai->StopMovement(); move->StopMovementImmediately(); }
        else { ai->MoveToActor(target, boss->GetMeleeDistance(), false); }
        m_pathTime = m_pathDelay;
        m_runTime = 0.f;
        return;
    }

    //||別の行動中は全力走りまでの時間を数え直す。
    if(!free || boss->m_spacing->IsSpacing() || !move->IsMovingOnGround()) { m_runTime = 0.f; return; }

    //全力走りの条件で追跡中か。
    const bool chasing = ai->GetMoveStatus() == EPathFollowingStatus::Moving && distance > m_sprintRange;
    //追跡が続いた時だけ全力走りへ切り替える。
    m_runTime = chasing ? m_runTime + _delta : 0.f;
    //BPから設定した敵のデータ。
    const auto* data = boss->GetBossData();
    //今のフェーズの速度倍率。
    const float phaseSpeed = boss->m_bossPhase == EBossPhase::PhaseTwo && data ? data->m_phaseSpeed : 1.f;
    //移動する速さ。
    const float speed = (chasing && m_runTime >= m_sprintDelay ? m_sprintSpeed : m_runSpeed) * phaseSpeed;
    move->MaxWalkSpeed = FMath::FInterpConstantTo(move->MaxWalkSpeed, speed, _delta, m_speedChange);
}
