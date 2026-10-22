#include "Ghost/AI/GhostAIController.h"
#include "Ghost/GhostCharacter/GhostCharacter.h"
#include "Player/CombatComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"

AGhostAIController::AGhostAIController() :
    m_AttackRange(150.f),
    m_AttackCooldown(1.2f),
    m_AttackTimer(0.f)
{
    PrimaryActorTick.bCanEverTick = true;
}

void AGhostAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    // ポゼッション完了時にプレイヤーをターゲットとして設定
    TargetPawn = FindPlayerPawn();
}

void AGhostAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!TargetPawn)
    {
        TargetPawn = FindPlayerPawn();
        if (!TargetPawn) return;
    }

    const float DistToTarget = FVector::Dist(
        GetPawn()->GetActorLocation(),
        TargetPawn->GetActorLocation());

    // ------------------------------------------------------------------
    // 移動：攻撃射程外なら接近
    // ------------------------------------------------------------------
    if (DistToTarget > m_AttackRange)
    {
        MoveToActor(TargetPawn, m_AttackRange * 0.8f);
    }

    // ------------------------------------------------------------------
    // 攻撃：射程内ならクールダウンを消費して攻撃
    // ------------------------------------------------------------------
    m_AttackTimer += DeltaTime;
    if (DistToTarget <= m_AttackRange && m_AttackTimer >= m_AttackCooldown)
    {
        TryAttack();
        m_AttackTimer = 0.f;
    }
}

APawn* AGhostAIController::FindPlayerPawn() const
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    return PC ? PC->GetPawn() : nullptr;
}

void AGhostAIController::TryAttack()
{
    AGhostCharacter* Ghost = Cast<AGhostCharacter>(GetPawn());
    if (!Ghost || !Ghost->CombatComponent) return;

    if (!Ghost->CombatComponent->IsAttacking())
    {
        Ghost->CombatComponent->ExecuteAttack();
    }
}