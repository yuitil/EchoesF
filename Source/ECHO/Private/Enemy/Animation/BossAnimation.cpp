// Fill out your copyright notice in the Description page of Project Settings.

#include "Enemy/Animation/BossAnimation.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Enemy/Combat/Combat.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

void UBossAnimInstance::NativeUpdateAnimation(float _deltaSeconds)
{
	Super::NativeUpdateAnimation(_deltaSeconds);

	AEnemyCharacterBase* enemy = Cast<AEnemyCharacterBase>(TryGetPawnOwner());
	if (!enemy) { return; }

	FVector velocity = enemy->GetVelocity();
	velocity.Z = 0.f;
	m_speed = velocity.Size();
	m_state = enemy->m_state;

	FVector local = enemy->GetActorRotation().UnrotateVector(velocity);
	m_direction = m_speed > 1 ? FMath::RadiansToDegrees(FMath::Atan2(local.Y, local.X)) : 0;
	m_bInAir = enemy->GetCharacterMovement()->IsFalling();
}

void UBossCombatWindow::NotifyBegin(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim, float _duration, const FAnimNotifyEventReference& _reference)
{
	Super::NotifyBegin(_mesh, _anim, _duration, _reference);

	AEnemyCharacterBase* enemy = _mesh ? Cast<AEnemyCharacterBase>(_mesh->GetOwner()) : nullptr;
	if (!enemy || !enemy->IsActing()) { return; }

	if (m_window == EBossWindow::Damage)
	{
		enemy->m_combat->BeginHitWindow(m_damageScale);
	}

	if (m_window == EBossWindow::DodgeInvulnerability)
	{
		enemy->m_combat->m_bInvulnerable = true;
	}

	if (m_window == EBossWindow::Guard)
	{
		enemy->m_combat->m_bGuarding = true;
	}
}

void UBossCombatWindow::NotifyEnd(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim, const FAnimNotifyEventReference& _reference)
{
	Super::NotifyEnd(_mesh, _anim, _reference);

	AEnemyCharacterBase* enemy = _mesh ? Cast<AEnemyCharacterBase>(_mesh->GetOwner()) : nullptr;
	if (!enemy) { return; }

	if (m_window == EBossWindow::Damage)
	{
		enemy->m_combat->EndHitWindow();
	}

	if (m_window == EBossWindow::DodgeInvulnerability)
	{
		enemy->m_combat->m_bInvulnerable = false;
	}

	if (m_window == EBossWindow::Guard)
	{
		enemy->m_combat->m_bGuarding = false;
	}
}

void UBossPresentationCue::Notify(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim, const FAnimNotifyEventReference& _reference)
{
	Super::Notify(_mesh, _anim, _reference);
}


#include "Enemy/Abilities/ActionAbility.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"

void UBossRecovery::Notify(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim, const FAnimNotifyEventReference& _reference)
{
	Super::Notify(_mesh, _anim, _reference);
	auto* enemy = _mesh ? Cast<AEnemyCharacterBase>(_mesh->GetOwner()) : nullptr;
	if (!enemy || !enemy->HasAuthority() || !enemy->IsActing()) { return; }
	auto* asc = enemy->GetAbilitySystemComponent();
	auto* montage = Cast<UAnimMontage>(_anim);
	if (!asc || !montage || asc->GetCurrentMontage() != montage) { return; }

	enemy->m_combat->EndHitWindow();
	if (!m_next.IsNone())
	{
		if (montage->GetSectionIndex(m_next) != INDEX_NONE) asc->CurrentMontageJumpToSection(m_next);
		return;
	}
	if (auto* ability = Cast<UActionAbility>(asc->GetAnimatingAbility())) ability->FinishFromNotify(m_blend);
}

void UBossDive::NotifyTick(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim, float _delta, const FAnimNotifyEventReference& _reference)
{
	Super::NotifyTick(_mesh, _anim, _delta, _reference);

	auto* enemy = _mesh ? Cast<AEnemyCharacterBase>(_mesh->GetOwner()) : nullptr;

	if (!enemy || !enemy->HasAuthority() || !enemy->IsActing()) { return; }

	auto* move = enemy->GetCharacterMovement();auto* asc = enemy->GetAbilitySystemComponent();
	auto* montage = Cast<UAnimMontage>(_anim);
	if (!asc || !montage || asc->GetCurrentMontage() != montage) { return; }
	if (move->IsMovingOnGround()) { if (!m_land.IsNone() && montage->GetSectionIndex(m_land) != INDEX_NONE) asc->CurrentMontageJumpToSection(m_land); return; }
	if (!move->IsFalling()) { return; }
	FVector velocity = FVector::ZeroVector;
	if (IsValid(enemy->m_combatTarget))
	{
		FVector delta = enemy->m_combatTarget->GetActorLocation() - enemy->GetActorLocation();
		float gap = FMath::Max(0.f, delta.Size2D() - enemy->GetMeleeDistance());
		velocity = delta.GetSafeNormal2D() * FMath::Min(m_speed, gap / FMath::Max(.05f, _delta * 2.f));
	}
	velocity.Z = -m_fall;move->Velocity = velocity;
}
