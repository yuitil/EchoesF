#include "Enemy/Animation/BossAnimation.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Enemy/Combat/Combat.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Enemy/Abilities/ActionAbility.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"

//動作を確かめるための幅や待ち時間。
namespace BossAnimationTuning
{
	//動作をなじませる時間の下限。
	constexpr auto minBlend = .05f;
}

//移動速度と空中の状態をアニメーションへ渡す。
void UBossAnimInstance::NativeUpdateAnimation(float _deltaSeconds)
{
	Super::NativeUpdateAnimation(_deltaSeconds);

	//処理の対象となる敵。
	AEnemyCharacterBase* enemy = Cast<AEnemyCharacterBase>(TryGetPawnOwner());
	if (!enemy) { return; }

	//現在の移動速度。
	FVector velocity = enemy->GetVelocity();
	velocity.Z = 0.f;
	//大きくした体の歩幅に合わせ、足を速く回しすぎないようにする。
	const float size = FMath::Max(KINDA_SMALL_NUMBER, enemy->GetActorScale3D().GetAbsMax());
	m_speed = velocity.Size() / size;
	m_state = enemy->m_state;
	m_dualStance = enemy->m_combat && enemy->m_combat->HasTwoSwords();

	//キャラクターを基準にした値。
	FVector local = enemy->GetActorRotation().UnrotateVector(velocity);
	//止まった瞬間に、横歩きから正面の姿勢へ飛ばさない。
	if (m_speed > 1) { m_direction = FMath::RadiansToDegrees(FMath::Atan2(local.Y, local.X)); }
	m_bInAir = enemy->GetCharacterMovement()->IsFalling();
}

//通知の区間に入った時の処理を行う。
void UBossCombatWindow::NotifyBegin(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim, float _duration, const FAnimNotifyEventReference& _reference)
{
	Super::NotifyBegin(_mesh, _anim, _duration, _reference);

	//処理の対象となる敵。
	AEnemyCharacterBase* enemy = _mesh ? Cast<AEnemyCharacterBase>(_mesh->GetOwner()) : nullptr;
	if (!enemy || _mesh != enemy->GetMesh() || !enemy->IsActing()) { return; }

	if (m_window == EBossWindow::Damage)
	{
		enemy->m_combat->BeginHitWindow(m_damageScale, m_hitBone, m_tipOffset, m_leftBlade);
	}

	if (m_window == EBossWindow::DodgeInvulnerability)
	{
		enemy->m_combat->m_bInvulnerable = true;
	}

	if (m_window == EBossWindow::Guard) { enemy->m_combat->m_bGuarding = true; }
}

//通知の区間を出た時の処理を行う。
void UBossCombatWindow::NotifyEnd(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim, const FAnimNotifyEventReference& _reference)
{
	Super::NotifyEnd(_mesh, _anim, _reference);

	//処理の対象となる敵。
	AEnemyCharacterBase* enemy = _mesh ? Cast<AEnemyCharacterBase>(_mesh->GetOwner()) : nullptr;
	if (!enemy || _mesh != enemy->GetMesh()) { return; }

	if (m_window == EBossWindow::Damage) { enemy->m_combat->EndHand(m_leftBlade); }

	if (m_window == EBossWindow::DodgeInvulnerability)
	{
		enemy->m_combat->m_bInvulnerable = false;
	}

	if (m_window == EBossWindow::Guard) { enemy->m_combat->m_bGuarding = false; }
}

//アニメーションの指定位置で処理を呼ぶ。
void UBossPresentationCue::Notify(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim, const FAnimNotifyEventReference& _reference)
{
	Super::Notify(_mesh, _anim, _reference);
}

//アニメーションの指定位置で処理を呼ぶ。
void UBossRecovery::Notify(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim, const FAnimNotifyEventReference& _reference)
{
	Super::Notify(_mesh, _anim, _reference);

	//処理の対象となる敵。
	auto* enemy = _mesh ? Cast<AEnemyCharacterBase>(_mesh->GetOwner()) : nullptr;
	if (!enemy || !enemy->HasAuthority() || !enemy->IsActing()) { return; }

	//能力とタグを管理する部品。
	auto* asc = enemy->GetAbilitySystemComponent();

	//対象のモンタージュ。
	auto* montage = Cast<UAnimMontage>(_anim);
	const auto* action = enemy->GetActiveAction();

	//抜刀の通知では、その後に振る攻撃を終わらせない。
	if (!asc || !montage || !action || enemy->GetMoveMontage(*action) != montage || asc->GetCurrentMontage() != montage)
	{
		return;
	}

	enemy->m_combat->EndHitWindow();
	if (!m_next.IsNone())
	{
		if (montage->GetSectionIndex(m_next) != INDEX_NONE)
			asc->CurrentMontageJumpToSection(m_next);
		return;
	}
	if (auto* ability = Cast<UActionAbility>(asc->GetAnimatingAbility()))
		ability->FinishFromNotify(m_blend);
}

//通知の区間内で移動を更新する。
void UBossDive::NotifyTick(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim,
	float _delta, const FAnimNotifyEventReference& _reference)
{
	Super::NotifyTick(_mesh, _anim, _delta, _reference);

	//処理の対象となる敵。
	auto* enemy = _mesh ? Cast<AEnemyCharacterBase>(_mesh->GetOwner()) : nullptr;

	if (!enemy || !enemy->HasAuthority() || !enemy->IsActing()) { return; }

	//移動を管理する部品。
	auto* move = enemy->GetCharacterMovement();
	auto* asc = enemy->GetAbilitySystemComponent();

	//対象のモンタージュ。
	auto* montage = Cast<UAnimMontage>(_anim);
	if (!asc || !montage || asc->GetCurrentMontage() != montage) { return; }
	if (move->IsMovingOnGround())
	{
		if (!m_land.IsNone() && montage->GetSectionIndex(m_land) != INDEX_NONE)
		{
			asc->CurrentMontageJumpToSection(m_land);
			return;
		}
	}
	if (!move->IsFalling()) { return; }

	//現在の移動速度。
	FVector velocity = FVector::ZeroVector;
	if (IsValid(enemy->m_combatTarget))
	{

		//前回からの経過時間や値の差。
		FVector delta = enemy->m_combatTarget->GetActorLocation() - enemy->GetActorLocation();

		//二つの値の差。
		float gap = FMath::Max(0.f, delta.Size2D() - enemy->GetMeleeDistance());
		velocity = delta.GetSafeNormal2D() * FMath::Min(m_speed, gap / FMath::Max(BossAnimationTuning::minBlend, _delta * 2.f));
	}
	velocity.Z = -m_fall;
	move->Velocity = velocity;
}

//本体のメッシュだけに合図する。
void UBossLeftSword::Notify(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim,
	const FAnimNotifyEventReference& _ref)
{
	Super::Notify(_mesh, _anim, _ref);
	auto* enemy = _mesh ? Cast<AEnemyCharacterBase>(_mesh->GetOwner()) : nullptr;
	if (!enemy || enemy->GetMesh() != _mesh || !enemy->IsCombatActive()) { return; }
	const auto* action = enemy->GetActiveAction();

	//中断した抜刀の通知で、次の技の剣を出し入れしない。
	if (action && action->ChangesSwordStance() && action->m_dualBlade == m_draw)
	{
		enemy->m_combat->UpdateLeftBlade(m_draw);
	}
}
