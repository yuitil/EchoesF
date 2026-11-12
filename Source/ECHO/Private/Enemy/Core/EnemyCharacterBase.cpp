#include "Enemy/Core/EnemyCharacterBase.h"
#include "Enemy/Audio/EnemyAudio.h"
#include "Enemy/Animation/EnemySwing.h"
#include "Enemy/AI/EnemyTargeting.h"

#include "Enemy/Core/EnemyTraversal.h"
#include "Combat/GAS/CombatAbilitySystemComponent.h"
#include "Combat/GAS/CombatAttributeSet.h"
#include "Combat/GAS/CombatGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Enemy/Combat/Combat.h"

#include "Enemy/Combat/EnemyHitEffect.h"

#include "Enemy/Combat/Defense.h"
#include "Enemy/Abilities/ActionAbility.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "BrainComponent.h"
#include "AIController.h"
#include "TimerManager.h"
#include "NavigationSystem.h"
#include "Enemy/EnemyManager/EnemyManager.h"

//判定に使う幅や待ち時間。
namespace EnemyCharacterBaseTuning
{
	//体の当たり判定の半径。
	constexpr auto bodyRadius = 42.f;
	//体の当たり判定の半分の高さ。
	constexpr auto bodyHalfHeight = 96.0f;
	//行動終了を待つ時間の余裕。
	constexpr auto timeoutMargin = 3.f;
	//歩ける場所を探す横幅。
	constexpr auto navWidth = 30;
	//歩ける場所を探す高さ。
	constexpr auto navHeight = 150;
	//初期体力の基準。
	constexpr auto baseHealth = 600;
	//続けられるコンボの段数。
	constexpr auto maxComboSteps = 10;
	//攻撃後の待ち時間の下限。
	constexpr auto minRecovery = 0.05f;
	//経路を調べる分割数。
	constexpr auto pathSteps = 5;
	//経路を調べる分割数。
	constexpr auto pathStepCount = 5.f;
	//経路の横幅。
	constexpr auto pathWidth = 25;
	//地面との距離に持たせる余裕。
	constexpr auto floorMargin = 4;
	//計算に使う最小の刻み幅。
	constexpr auto minStep = .1f;
	//再生速度の下限。
	constexpr auto minRate = .01f;
	//抽選に使う重みの下限。
	constexpr auto minWeight = 0.01f;
}

//初期設定と必要な部品を用意する。
AEnemyCharacterBase::AEnemyCharacterBase()
{
	m_targeting = CreateDefaultSubobject<UEnemyTargeting>(TEXT("Targeting"));

	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(EnemyCharacterBaseTuning::bodyRadius, EnemyCharacterBaseTuning::bodyHalfHeight);
	m_abilitySystemComponent = CreateDefaultSubobject<UCombatAbilitySystemComponent>(TEXT("AbilitySystem"));
	m_attributeSet = CreateDefaultSubobject<UCombatAttributeSet>(TEXT("AttributeSet"));
	m_combat = CreateDefaultSubobject<UCombat>(TEXT("Combat"));
	m_traversal = CreateDefaultSubobject<UEnemyTraversal>(TEXT("Traversal"));
	m_defense = CreateDefaultSubobject<UDefense>(TEXT("Defense"));

	m_hitFx = CreateDefaultSubobject<UEnemyHitEffect>(TEXT("HitFX"));
	m_audio = CreateDefaultSubobject<UEnemyAudio>(TEXT("EnemyAudio"));
	m_swing = CreateDefaultSubobject<UEnemySwing>(TEXT("EnemySwing"));
}

//空中から戻った足元で鳴らす。
void AEnemyCharacterBase::Landed(const FHitResult& _hit)
{
	Super::Landed(_hit);
	if (m_audio) { m_audio->Play(EEnemySound::Land, _hit.ImpactPoint); }
}

//相手の防御結果を確認する標準処理。
EDefenseResult AEnemyCharacterBase::ResolveTargetDefense_Implementation(AActor* _target) const
{
	return EDefenseResult::None;
}

//設定に合わせた攻撃の再生速度を返す。
float AEnemyCharacterBase::GetAttackRate(const FEnemyActionData& _action) const
{
	//対象となる攻撃。
	const bool attack = _action.UsesAttackRate();
	return FMath::Clamp(m_definition && m_definition->m_commonRate && attack ? m_definition->m_attackRate : _action.m_playRate, EnemyCharacterBaseTuning::minStep, 2.f);
}

//攻撃に対して防御できる時間の幅を返す。
float AEnemyCharacterBase::GetDefenseWindow(EDefenseResult _result, FName _actionId) const
{
	//使用する行動の設定。
	const FEnemyActionData* action = GetActiveAction();
	if (!_actionId.IsNone())
	{
		action = m_definition ? m_definition->m_actions.FindByPredicate( [_actionId](const FEnemyActionData& item) 
			{
				return item.m_actionId == _actionId;
			}) : nullptr;
	}
	if (!action || action->m_damage <= 0.f) { return 0.f; }
	if (_result == EDefenseResult::PerfectDodge)
	{
		//回避できてもジャスト回避扱いにならない技を除く。
		if (!m_definition || !m_definition->m_perfectDodge) { return 0.f; }
		if (action->m_unblockable && m_definition->m_dodgeOnly) { return 0.f; }

		//技ごとの受付時間がなければ、敵の共通設定を使う。
		const float window = action->m_dodgeWindow > 0.f ? action->m_dodgeWindow : m_definition->m_dodgeWindow;
		return FMath::Clamp(window, 0.f, .5f);
	}
	if (action->m_unblockable) { return 0.f; }
	if (_result == EDefenseResult::Parry)
	{
		return FMath::Clamp(action->m_parryWindow, 0.f, .5f);
	}
	if (_result == EDefenseResult::Deflect)
	{
		return FMath::Clamp(action->m_deflectWindow, 0.f, .5f);
	}
	return 0.f;
}

//防御の入力タイミングが有効か調べる。
bool AEnemyCharacterBase::IsDefenseInputValid(EDefenseResult _result, float _elapsed) const
{
	//動作の判定に使う時間の幅。
	const float window = GetDefenseWindow(_result);
	return IsActing() && window > 0.f && FMath::IsFinite(_elapsed) && _elapsed >= 0.f && _elapsed <= window;
}

//二刀のガードには専用のモンタージュを使う。
UAnimMontage* AEnemyCharacterBase::GetMoveMontage(const FEnemyActionData& _action) const
{
	if (_action.m_kind == EEnemyAction::Guard && m_definition && m_definition->m_dualGuard && m_combat->HasTwoSwords())
	{
		return m_definition->m_dualGuard;
	}
	return _action.m_montage;
}

//持ち替えと技の再生が終わるまでの待ち時間を求める。
float AEnemyCharacterBase::GetActionTimeout(const FEnemyActionData& _action) const
{
	//技が未設定なら、短い待ち時間だけを返す。
	const auto* move = GetMoveMontage(_action);
	if (!move) { return EnemyCharacterBaseTuning::timeoutMargin; }

	//技の長さを実際の再生速度で割る。
	const float moveRate = FMath::Max(EnemyCharacterBaseTuning::minRate, GetAttackRate(_action) * move->RateScale);
	float wait = move->GetPlayLength() / moveRate + EnemyCharacterBaseTuning::timeoutMargin;

	//抜刀や納刀が先に入る場合は、その再生時間を足す。
	if (_action.ChangesSwordStance())
	{
		if (const auto* change = m_combat->GetBladeChange(_action.m_dualBlade))
		{
			//持ち替えには技とは別の再生速度を使う。
			const float changeRate = FMath::Max(EnemyCharacterBaseTuning::minRate, change->RateScale);
			wait += change->GetPlayLength() / changeRate;
		}
	}
	return wait;
}

//ジャスト回避の成功を相手へ通知する。
bool AEnemyCharacterBase::ApplyPerfectDodge(AActor* _target)
{

	//今の攻撃で狙っている相手に、一度だけ成功を通知する。
	if (!HasAuthority() || !IsActing() || WasPerfectDodged()) { return false; }
	if (!IsValid(_target) || _target != m_combatTarget) { return false; }

	//ダメージがあり、ジャスト回避を受け付ける技か確認する。
	const auto* action = GetActiveAction();
	if (!action || action->m_damage <= 0.f) { return false; }
	if (GetDefenseWindow(EDefenseResult::PerfectDodge) <= 0.f) { return false; }

	//攻撃が終了した後に届いた判定は使わない。
	auto* anim = GetMesh()->GetAnimInstance();
	if (!anim || !anim->Montage_IsPlaying(action->m_montage)) { return false; }

	//回避済みと記録し、次のコンボと打ち上げを取り消す。
	m_dodged = true;
	m_nextActionId = NAME_None;
	m_combat->EndLift();

	//回避に成功した相手へ、演出用の通知を一度だけ送る。
	if (_target->GetClass()->ImplementsInterface(UEBDefenseFeedback::StaticClass()))
	{
		IEBDefenseFeedback::Execute_AttackDefended(_target, this, EDefenseResult::PerfectDodge);
	}
	return true;
}

//経過時間に合わせて敵の状態を更新する。
void AEnemyCharacterBase::Tick(float _delta)
{
	Super::Tick(_delta);

	//打ち上げ後は地上の接近判断を待たず、予約した空中技を出す。
	if (HasAuthority() && !m_nextActionId.IsNone() && GetCharacterMovement()->IsFalling() && CanDecide())
	{
		RequestAction(m_nextActionId);
	}

	//使用する行動の設定。
	const auto* action = GetActiveAction();
	if (!HasAuthority() || !action || action->m_backSpeed <= 0.f || !GetCharacterMovement()->IsMovingOnGround())
	{
		return;
	}

	//アニメーションを再生する部品。
	const auto* anim = GetMesh()->GetAnimInstance();
	if (!anim || !anim->Montage_IsPlaying(action->m_montage)) { return; }

	//処理に使う時間。
	const float time = anim->Montage_GetPosition(action->m_montage);

	if (time >= action->m_backStart && time < action->m_backEnd)
	{
		//再生が遅くなった時は移動も遅くし、足だけ置いていかれないようにする。
		const float rate = FMath::Abs(anim->Montage_GetPlayRate(action->m_montage) * action->m_montage->RateScale);
		const float speed = action->m_backSpeed * rate;
		const FVector dir = m_findRetreatAngle ? m_evadeDir : EvadeDirection(*action);

		//地面に当たった情報。
		FNavLocation floor;

		//歩ける場所を調べる機能。
		auto* nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

		//次に使う区間または候補。
		const FVector next = GetActorLocation() + dir * speed * _delta;

		//足元の位置。
		const float feet = GetActorLocation().Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

		//次の足元の周りで、歩ける床を探す。
		const FVector nextFeet(next.X, next.Y, feet);
		const float width = EnemyCharacterBaseTuning::navWidth;
		const FVector searchArea(width, width, EnemyCharacterBaseTuning::navHeight);
		const bool foundFloor = nav && nav->ProjectPointToNavigation(nextFeet, floor, searchArea);

		//床が無い場所や、一歩で越せない段差では退避を止める。
		auto* movement = GetCharacterMovement();
		if (!foundFloor || FMath::Abs(floor.Location.Z - feet) > movement->MaxStepHeight)
		{
			movement->StopMovementImmediately();
			return;
		}
		GetCharacterMovement()->MaxWalkSpeed = speed;
		if (m_findRetreatAngle)
		{

			//退避の区間だけ床に沿って動かし、壁に当たる手前で止める。
			FHitResult hit;
			GetCharacterMovement()->SafeMoveUpdatedComponent(dir * speed * _delta, GetActorQuat(), true, hit);
			if (hit.IsValidBlockingHit()) { m_evadeDir = FindEvadePath(*action); }
		}
		else
		{
			AddMovementInput(dir, 1.f);
		}
	}
	else
	{
		GetCharacterMovement()->StopMovementImmediately();
	}
}

//相手を打ち上げられるか調べる標準処理。
bool AEnemyCharacterBase::CanLiftTarget_Implementation(AActor* _target) const
{

	//対象のキャラクター。
	const auto* character = Cast<ACharacter>(_target);
	return m_airCombo && IsValid(character) && character->GetCharacterMovement()->IsMovingOnGround();
}

//敵の能力を管理する部品を返す。
UAbilitySystemComponent* AEnemyCharacterBase::GetAbilitySystemComponent() const
{
	return m_abilitySystemComponent;
}

//ゲーム開始時に必要な状態を準備する。
void AEnemyCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	//別の移動補正がある時は、その設定を上書きしない。
	if (!GetCharacterMovement()->ProcessRootMotionPostConvertToWorld.IsBound())
	{
		GetCharacterMovement()->ProcessRootMotionPostConvertToWorld.BindUObject( this, &AEnemyCharacterBase::AimAttackMove);
	}

	GetCharacterMovement()->MaxWalkSpeed = m_moveSpeed;
	m_abilitySystemComponent->InitializeCombat(this, this, m_attributeSet);

	if (HasAuthority())
	{
		m_abilitySystemComponent->GiveAbility(FGameplayAbilitySpec(UActionAbility::StaticClass(), 1));
	}

	SetHealthPool(m_definition ? m_definition->m_initialHealth : EnemyCharacterBaseTuning::baseHealth);
	m_healthHandle = m_abilitySystemComponent->GetGameplayAttributeValueChangeDelegate(m_attributeSet->GetHealthAttribute()).AddUObject(this, &AEnemyCharacterBase::HealthChanged);
	m_onHealthChanged.Broadcast(GetHealth(), GetMaxHealth());
}

//現在の体力を返す。
float AEnemyCharacterBase::GetHealth() const
{
	return m_attributeSet->GetHealth();
}

//最大体力を返す。
float AEnemyCharacterBase::GetMaxHealth() const
{
	return m_attributeSet->GetMaxHealth();
}

//体力を設定する。
void AEnemyCharacterBase::SetHealthPool(float _max)
{
	m_abilitySystemComponent->ResetHealth(_max);
}

//防御を確認して体力を減らす。
float AEnemyCharacterBase::TakeDamage(float _damage, const FDamageEvent& _damageEvent, AController* _instigator, AActor* _causer)
{
	if (!HasAuthority() || !FMath::IsFinite(_damage) || _damage <= 0 || !CanBeDamaged() || !IsCombatActive() || m_combat->m_bInvulnerable)
	{
		return 0.0f;
	}

	//実際に反映できた体力の変化量。
	float accepted = Super::TakeDamage(_damage, _damageEvent, _instigator, _causer);
	if (!FMath::IsFinite(accepted) || accepted <= 0) { return 0.0f; }

	//攻撃してきた相手。
	AActor* attacker = Cast<APawn>(_causer);
	if (!attacker)
	{
		attacker = _instigator && _instigator->GetPawn() ? _instigator->GetPawn() : _causer;
	}
	if (attacker && attacker->GetOwner() && !Cast<APawn>(attacker))
	{
		attacker = attacker->GetOwner();
	}

	OnAttackReceived(attacker);

	//受けたダメージの情報。
	const UEBCombatDamageType* damageInfo = _damageEvent.DamageTypeClass ? Cast<UEBCombatDamageType>(_damageEvent.DamageTypeClass->GetDefaultObject()) : nullptr;

	//ガードできない攻撃か。
	const bool unblockable = damageInfo && !damageInfo->m_bBlockable;

	//強攻撃として扱うか。
	const bool heavyAttack = (damageInfo && damageInfo->m_heavyAttack) || m_defense->IsObservedHeavyAttack(attacker);

	//処理の結果。
	EDefenseResult result = m_defense->ResolveHit(attacker, unblockable, heavyAttack);

	//弾かれた攻撃では、連続命中の回数を戻す。
	if (result == EDefenseResult::Deflect || result == EDefenseResult::Parry)
	{
		m_defense->RecordHeavyHit(attacker, false);
	}

	//ガード中も通常被弾も、同じ強攻撃の回数として数える。
	const bool strongGuard = HasStrongGuard();
	const bool blocked = result == EDefenseResult::Guard;
	if (result != EDefenseResult::Deflect && result != EDefenseResult::Parry && m_defense->RecordHeavyHit(attacker, heavyAttack && (!strongGuard || blocked)))
	{
		result = EDefenseResult::GuardBreak;
	}

	//防御で受けた時は体の命中と別の火花を出す。
	if (result == EDefenseResult::Guard || result == EDefenseResult::Deflect || result == EDefenseResult::Parry)
	{
		m_hitFx->PlayClash(attacker);
	}

	if (strongGuard && blocked)
	{
		//崩した一撃まで剣で受け、次の一撃からHPに通す。
		if (result == EDefenseResult::Guard) { OnStrongBlock(); }
		m_defense->PublishResult(attacker, result);
		return 0.f;
	}

	if (result == EDefenseResult::Deflect || result == EDefenseResult::Parry)
	{
		m_defense->PublishResult(attacker, result);
		return 0.0f;
	}

	if (result == EDefenseResult::Guard)
	{
		accepted *= FMath::Clamp(m_definition->m_guardDamageScale, 0.0f, 1.0f);
	}

	if (accepted <= 0.0f)
	{
		m_defense->PublishResult(attacker, result);
		return 0.0f;
	}

	//反映したダメージ。
	const float applied = FMath::Min(GetHealth(), accepted);

	if (applied > 0.f && result != EDefenseResult::Guard)
	{

		//ぶつかった場所の情報。
		const FHitResult* hit = _damageEvent.IsOfType(FPointDamageEvent::ClassID) ? &static_cast<const FPointDamageEvent&>(_damageEvent).HitInfo : nullptr;
		m_hitFx->PlayHit(attacker, hit);
	}

	OnAcceptedHit(attacker);

	m_defense->RememberHit(attacker);
	m_abilitySystemComponent->ApplyHealthDelta(-accepted, _instigator ? _instigator->GetPawn() : _causer, _causer);
	m_defense->PublishResult(attacker, result);

	//エネルギー加算は攻撃側で行う。敵から重ねて通知しない。
	return applied;
}

//敵の行動状態を切り替える。
void AEnemyCharacterBase::SetState(EEnemyState _newState)
{
	m_state = _newState;
	if (!IsCombatActive()) { m_defense->ResetDefense(); }

	m_abilitySystemComponent->SetLooseGameplayTagCount(CombatTags::StateDead, m_state == EEnemyState::Dead ? 1 : 0);
	m_abilitySystemComponent->SetLooseGameplayTagCount(CombatTags::StateSuspended, m_state == EEnemyState::Suspended ? 1 : 0);
	m_onStateChanged.Broadcast(m_state);
}

//体力の変化を受けて死亡などを確認する。
void AEnemyCharacterBase::HealthChanged(const FOnAttributeChangeData& _data)
{
	m_onHealthChanged.Broadcast(_data.NewValue, GetMaxHealth());
	if (_data.NewValue <= 0.0f && IsCombatActive()) { HandleHealthDepleted(); }
}

//体力がなくなった時の処理を行う。
void AEnemyCharacterBase::HandleHealthDepleted()
{
	if (m_audio) { m_audio->Play(EEnemySound::Death, GetActorLocation()); }
	SetState(EEnemyState::Dead);
	CancelAction();
	SetCanBeDamaged(false);

	if (AAIController* aiController = Cast<AAIController>(GetController()))
	{
		aiController->StopMovement();

		if (UBrainComponent* brain = aiController->GetBrainComponent())
		{
			brain->StopLogic(TEXT("Dead"));
		}
	}

	//死亡後に移動や接触が残らないように止める。
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	//死亡用のモンタージュが設定されていれば再生する。
	if (m_definition && m_definition->m_deathMontage)
	{
		PlayAnimMontage(m_definition->m_deathMontage);
	}

	//イベントで使用される敵なら死亡を通知する
	if (m_IsEventEnemy)
	{
		UEnemyManager* EnemyManager = GetWorld()->GetSubsystem<UEnemyManager>();
		if (!EnemyManager) { return; }

		EnemyManager->NotifyEventEnemyDefeated(this);
	}

	EnemyDefeted();
}

//受け流しなどを選ぶ確率を返す。
float AEnemyCharacterBase::GetSpecialDefenseChance() const
{
	return m_definition ? m_definition->m_defenseChance : 0.0f;
}

//剣が届く間合いを返す。
float AEnemyCharacterBase::GetMeleeDistance() const
{

	//キャラクターの当たり判定。
	const auto* capsule = IsValid(m_combatTarget) ? m_combatTarget->FindComponentByClass<UCapsuleComponent>() : nullptr;

	//判定に使う半径。
	const float radius = capsule ? capsule->GetScaledCapsuleRadius() : 0.f;
	return FMath::Max(m_meleeDist, GetCapsuleComponent()->GetScaledCapsuleRadius() + radius + m_bodyMargin);
}

//次の行動を判断できる状態か調べる。
bool AEnemyCharacterBase::CanDecide() const
{
	return !IsRepositioning() && !m_traversal->IsTraversing() && IsCombatActive() && !IsActing() && !m_defense->IsBusy() && IsValid(m_combatTarget) && GetWorld()->GetTimeSeconds() >= m_readyTime;
}

//攻撃を使う条件がそろっているか調べる。
bool AEnemyCharacterBase::CanUseAction(int32 _actionIndex, float _distance) const
{
	if (!m_definition || !m_definition->m_actions.IsValidIndex(_actionIndex) || !IsValid(m_combatTarget))
	{
		return false;
	}

	//使用する行動の設定。
	const FEnemyActionData& action = m_definition->m_actions[_actionIndex];

	// !m_abilitySystemComponent->HasAllMatchingGameplayTags|必要なタグがない場合や、禁止タグがある場合は選ばない。
	if (!m_abilitySystemComponent || !m_abilitySystemComponent->HasAllMatchingGameplayTags(action.m_requiredTags) || m_abilitySystemComponent->HasAnyMatchingGameplayTags(action.m_blockedTags))
	{
		return false;
	}

	//対象のキャラクター。
	const auto* character = Cast<ACharacter>(m_combatTarget.Get());

	//地上・空中・相手の状態が攻撃の条件に合うか調べる。
	if (action.m_airOnly != GetCharacterMovement()->IsFalling()) { return false; }

	if (action.m_targetGround && (!character || !character->GetCharacterMovement()->IsMovingOnGround()))
	{
		return false;
	}
	if (action.m_reactOnly && !m_defense->IsTargetAttacking(m_combatTarget)) { return false; }

	if (action.m_targetAir && (!character || !character->GetCharacterMovement()->IsFalling()))
	{
		return false;
	}
	if (FMath::Abs(GetActorLocation().Z - m_combatTarget->GetActorLocation().Z) > action.m_heightGap)
	{
		return false;
	}
	if (action.m_launch && !CanLiftTarget(m_combatTarget)) { return false; }
	if (action.m_launch && GetWorld()->GetTimeSeconds() < m_nextLift) { return false; }
	if (action.m_backSpeed > 0.f && !CanEvade(action)) { return false; }

	//再使用までの待ち時間。
	const double* cooldown = m_actionCooldowns.Find(action.m_actionId);

	//相手までの距離。
	const float distance = _distance >= 0.f ? _distance : FVector::Dist2D(GetActorLocation(), m_combatTarget->GetActorLocation());

	return action.m_reviewed && action.m_montage && !action.m_actionId.IsNone() && action.m_kind != EEnemyAction::Presentaiton && IsActionAllowed(action) &&
		(!cooldown || *cooldown <= GetWorld()->GetTimeSeconds()) && distance >= action.m_minRange && distance <= action.m_maxRange;
}

//突進攻撃を開始できるか調べる。
bool AEnemyCharacterBase::CanRushAttack(float _distance) const
{
	if (!CanDecide() || !m_definition || !m_nextActionId.IsNone()) { return false; }
	for (int32 i = 0; i < m_definition->m_actions.Num(); ++i)
	{
		//使用する行動の設定。
		const FEnemyActionData& action = m_definition->m_actions[i];
		if (action.m_comboEntry && action.m_kind != EEnemyAction::Guard && CanUseAction(i, _distance))
		{
			return true;
		}
	}
	return false;
}

//次のコンボの設定を探す。
int32 AEnemyCharacterBase::FindComboAction(FName _actionId) const
{
	if (!m_definition || _actionId.IsNone()) { return INDEX_NONE; }
	for (int32 i = 0; i < m_definition->m_actions.Num(); ++i)
	{
		if (m_definition->m_actions[i].m_actionId == _actionId && CanUseAction(i) && m_definition->m_actions[i].m_kind != EEnemyAction::Guard)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

//次の一手を出せない距離・状態なら予約を外す。
void AEnemyCharacterBase::CheckComboRange()
{
	if (IsActing() || m_nextActionId.IsNone() || !CanDecide()) { return; }
	if (FindComboAction(m_nextActionId) != INDEX_NONE) { return; }
	m_nextActionId = NAME_None;
	m_comboSteps = 0;
}

//距離と状態に合う攻撃を抽選する。
int32 AEnemyCharacterBase::ChooseAction() const
{
	if (!m_definition || !m_combatTarget) { return INDEX_NONE; }

	//受け流し後の反撃を、通常の行動より先に選ぶ。
	const int32 counter = FindComboAction(m_defense->GetCounterAction());
	if (counter != INDEX_NONE) { return counter; }

	//攻撃された後のガード予約があれば先に使う。
	if (m_defense->HasGuardRequest() && m_defense->ShouldStartGuard())
	{ 
		for (int32 i = 0; i < m_definition->m_actions.Num(); ++i)
		{
			if (m_definition->m_actions[i].m_kind == EEnemyAction::Guard && CanUseAction(i))
			{
				return i;
			}
		}
	}

	//次につなぐ攻撃。
	const int32 followUp = FindComboAction(m_nextActionId);
	if (followUp != INDEX_NONE) { return followUp; }

	//相手が攻撃中なら、設定した確率で回避を選ぶ。
	if (m_defense->IsTargetAttacking(m_combatTarget) && FMath::FRand() < m_definition->m_evadeChance)
	{

		//選択できる候補。
		TArray<int32> options;
		for (int32 i = 0; i < m_definition->m_actions.Num(); ++i)
			if (m_definition->m_actions[i].m_kind == EEnemyAction::Dodge && m_definition->m_actions[i].m_reactOnly && CanUseAction(i))
			{
				options.Add(i);
			}
		if (!options.IsEmpty()) { return options[FMath::RandRange(0, options.Num() - 1)]; }
	}

	//ガードが必要な時はガード用の行動を探す。
	if (m_defense->ShouldStartGuard())
	{
		for (int32 i = 0; i < m_definition->m_actions.Num(); ++i)
		{
			if (m_definition->m_actions[i].m_kind == EEnemyAction::Guard && CanUseAction(i))
			{
				return i;
			}
		}
	}

	//必殺技の候補。
	TArray<int32> specials;
	for (int32 i = 0; i < m_definition->m_actions.Num(); ++i)
	{
		if (m_definition->m_actions[i].m_special && m_definition->m_actions[i].m_weight > 0.f &&
			m_definition->m_actions[i].m_comboEntry && CanUseAction(i))
		{
			specials.Add(i);
		}
	}

	if (!specials.IsEmpty() && FMath::FRand() < FMath::Clamp(m_definition->m_specialChance, 0.f, 1.f))
	{
		return specials[FMath::RandRange(0, specials.Num() - 1)];
	}

	//今の距離や再使用時間で選べる技だけを数える。
	float sum = 0;
	float comboSum = 0.f;
	TArray<TPair<int32, float>> fullCombos;
	const bool preferCombo = m_definition->m_comboAfter > 0 && m_attacksSinceCombo >= m_definition->m_comboAfter;

	//今回抽選する候補。
	TArray<TPair<int32, float>> choices;
	for (int32 i = 0; i < m_definition->m_actions.Num(); ++i)
	{
		//使用する行動の設定。
		const FEnemyActionData& action = m_definition->m_actions[i];

		// action.m_special|開始条件を満たさない行動は抽選から外す。
		if (!CanUseAction(i) || action.m_special || !action.m_comboEntry || action.m_kind == EEnemyAction::Guard || action.m_reactOnly)
		{
			continue;
		}

		//同じ攻撃が続く時の選ばれやすさの倍率。
		const float repeat = action.m_actionId == m_lastAction ? FMath::Clamp(m_definition->m_repeatWeight, EnemyCharacterBaseTuning::minWeight, 1.f) : 1.f;

		//抽選での選ばれやすさ。
		const float weight = FMath::Max(0.f, action.m_weight) * GetActionWeightMultiplier(action) * repeat;

		if (weight > 0)
		{
			sum += weight;
			choices.Emplace(i, weight);
			if (preferCombo && action.m_fullCombo)
			{
				fullCombos.Emplace(i, weight);
				comboSum += weight;
			}
		}
	}

	//連撃が届かない時は、通常の攻撃候補を使う。
	if (!fullCombos.IsEmpty())
	{
		choices = MoveTemp(fullCombos);
		sum = comboSum;
	}

	//抽選で選ぶための値。
	float pick = FMath::FRandRange(0.0f, sum);
	for (const auto& choice : choices)
	{
		pick -= choice.Value;
		if (pick <= 0) { return choice.Key; }
	}

	return choices.IsEmpty() ? INDEX_NONE : choices.Last().Key;
}

//状況に合わせて次のコンボを選ぶ。
FName AEnemyCharacterBase::SelectComboBranch(const FEnemyActionData& _action, bool _hit, bool _blocked)
{
	return _blocked ? _action.m_blockedNextId : (_hit ? _action.m_hitNextId : _action.m_missNextId);
}

//今回の攻撃が命中したことを記録する。
void AEnemyCharacterBase::RecordAttackHit()
{
	if (IsActing()) { m_bActionHit = true; }
}

//ガード崩れのエフェクトを表示する。
void AEnemyCharacterBase::ShowGuardBreak_Implementation()
{
	if (m_audio) { m_audio->Emit_Implementation(EEnemySound::Break, GetActorLocation(), 1.f); }
	if (GetNetMode() != NM_DedicatedServer) { GuardBreakFX(); }
}

//攻撃を防がれた結果を受け取る。
void AEnemyCharacterBase::AttackDefended_Implementation(AActor* _defender, EDefenseResult _result)
{
	if (!IsActing() || !HasAuthority() || _defender != m_combatTarget || _result == EDefenseResult::None)
	{
		return;
	}

	//ジャスト回避の成功を通知する。
	if (_result == EDefenseResult::PerfectDodge)
	{
		ApplyPerfectDodge(_defender);
		return;
	}

	//使用する行動の設定。
	const auto* action = GetActiveAction();
	if (action && action->m_unblockable && (_result == EDefenseResult::Guard || _result == EDefenseResult::Deflect || _result == EDefenseResult::Parry))
	{
		return;
	}

	//相手のガードを崩せた時は命中として記録する。
	if (_result == EDefenseResult::GuardBreak)
	{
		m_bActionBlocked = false;
		m_bActionHit = true;
	}
	else
	{
		m_bActionBlocked = true;
	}
	m_defense->ReactToDefendedAttack(_result);
}

//今選べる攻撃を返す。
const FEnemyActionData* AEnemyCharacterBase::GetReadyAction(FName _actionId) const
{
	if (!CanDecide()) { return nullptr; }

	//配列の中の位置。
	const int32 index = FindComboAction(_actionId);
	return index != INDEX_NONE ? &m_definition->m_actions[index] : nullptr;
}

//指定した攻撃の実行を依頼する。
bool AEnemyCharacterBase::RequestAction(FName _actionId)
{
	if (!CanDecide() || !HasAuthority()) { return false; }

	m_activeIndex = _actionId.IsNone() ? ChooseAction() : FindComboAction(_actionId);

	if (m_activeIndex == INDEX_NONE)
	{
		m_nextActionId = NAME_None;
		m_comboSteps = 0;
		return false;
	}

	if (m_definition->m_actions[m_activeIndex].m_kind == EEnemyAction::Guard)
	{
		m_defense->ConsumeGuardRequest();
	}

	//コンボを続けるか。
	const bool continuing = !m_nextActionId.IsNone() && m_definition->m_actions[m_activeIndex].m_actionId == m_nextActionId;
	m_comboSteps = continuing ? m_comboSteps + 1 : 1;
	m_nextActionId = NAME_None;
	m_dodged = false;
	m_bActionHit = false;
	m_bActionBlocked = false;

	m_savedSpeed = GetCharacterMovement()->MaxWalkSpeed;
	m_savedTurn = GetCharacterMovement()->bOrientRotationToMovement;
	if (m_definition->m_actions[m_activeIndex].m_backSpeed > 0.f)
	{
		m_evadeDir = FindEvadePath(m_definition->m_actions[m_activeIndex]);
		GetCharacterMovement()->bOrientRotationToMovement = false;
	}
	m_actionStart = GetWorld()->GetTimeSeconds();

	++m_actionSerial;
	m_actionSuccess = false;

	//空中にいるか。
	const bool airborne = GetCharacterMovement()->IsFalling();

	//上下方向の速度。
	const float verticalSpeed = GetCharacterMovement()->Velocity.Z;

	if (AAIController* aiController = Cast<AAIController>(GetController()))
	{
		aiController->StopMovement();
	}

	GetCharacterMovement()->StopMovementImmediately();
	if (airborne) { GetCharacterMovement()->Velocity.Z = verticalSpeed; }

	//相手へ向かう方向。
	FVector targetDirection = m_combatTarget->GetActorLocation() - GetActorLocation();
	targetDirection.Z = 0;
	if (!targetDirection.IsNearlyZero()) { SetActorRotation(targetDirection.Rotation()); }

	m_abilitySystemComponent->AddLooseGameplayTags(m_definition->m_actions[m_activeIndex].m_activeTags);

	m_abilitySystemComponent->SetLooseGameplayTagCount(CombatTags::StateActing, 1);
	if (!m_abilitySystemComponent->TryActivateAbilityByClass(UActionAbility::StaticClass()))
	{
		FinishAction(true);
		return false;
	}

	//反撃が始まったら予約を消し、重ねて発動しない。
	if (IsActing() && m_definition->m_actions[m_activeIndex].m_kind == EEnemyAction::Counter)
	{
		m_defense->ClearCounter();
	}

	return IsActing();
}

//現在実行している攻撃の設定を返す。
const FEnemyActionData* AEnemyCharacterBase::GetActiveAction() const
{
	if (!m_definition || !m_definition->m_actions.IsValidIndex(m_activeIndex))
	{
		return nullptr;
	}
	return &m_definition->m_actions[m_activeIndex];
}

//攻撃を終えて次の行動に備える。
void AEnemyCharacterBase::FinishAction(bool _cancelled)
{
	m_aimMove = false;

	//使用する行動の設定。
	const FEnemyActionData* action = GetActiveAction();

	//相手が回避したか。
	const bool dodged = WasPerfectDodged();
	m_dodged = false;

	if (action && action->m_backSpeed > 0.f)
	{
		GetCharacterMovement()->MaxWalkSpeed = m_savedSpeed;
		GetCharacterMovement()->bOrientRotationToMovement = m_savedTurn;
		GetCharacterMovement()->StopMovementImmediately();
	}

	m_nextActionId = NAME_None;

	if (action)
	{
		if (!_cancelled && action->m_kind != EEnemyAction::Guard && action->m_kind != EEnemyAction::Dodge)
		{
			m_lastAction = action->m_actionId;
			if (m_definition->m_comboAfter > 0 && action->m_damage > 0.f)
			{
				m_attacksSinceCombo = action->m_fullCombo ? 0 : FMath::Min(m_attacksSinceCombo + 1, m_definition->m_comboAfter);
			}
			if (action->m_damage > 0.f)
			{
				RecordMoveResult(m_lastAction, m_bActionHit && !m_bActionBlocked);
			}
		}
		m_actionCooldowns.Add(action->m_actionId, GetWorld()->GetTimeSeconds() + action->m_cooldown);

		//成功した打ち上げの後は、次の打ち上げまで待つ。
		if (action->m_launch && m_bActionHit)
		{
			m_nextLift = GetWorld()->GetTimeSeconds() + FMath::Max(0.f, m_definition->m_liftCooldown);
		}

		if (!_cancelled && !dodged && m_definition && m_comboSteps < FMath::Clamp(m_definition->m_maxComboSteps, 1, EnemyCharacterBaseTuning::maxComboSteps))
		{
			m_nextActionId = SelectComboBranch(*action, m_bActionHit, m_bActionBlocked);

			//今の高さや距離で使える続きだけを抽選する。
			if (m_bActionHit && !m_bActionBlocked && !action->m_hitChoices.IsEmpty())
			{

				//今使える追撃の候補。
				TArray<FName> choices;
				for (const FName next : action->m_hitChoices)
				{
					if (FindComboAction(next) != INDEX_NONE) { choices.AddUnique(next); }
				}
				if (!choices.IsEmpty())
				{
					m_nextActionId = choices[FMath::RandRange(0, choices.Num() - 1)];
				}
			}
		}

		//最終段で打ち上げた時も、相手が浮いた場合だけ空中の締めを一度許す。
		if (!_cancelled && !dodged && m_definition && action->m_launch && m_bActionHit && !m_bActionBlocked && m_nextActionId.IsNone() && m_comboSteps == FMath::Clamp(m_definition->m_maxComboSteps, 1, EnemyCharacterBaseTuning::maxComboSteps))
		{
			const int32 finish = FindComboAction(action->m_hitNextId);
			if (finish != INDEX_NONE && m_definition->m_actions[finish].m_targetAir && m_definition->m_actions[finish].m_airOnly)
			{
				m_nextActionId = action->m_hitNextId;
			}
		}

		if (FindComboAction(m_nextActionId) == INDEX_NONE) { m_nextActionId = NAME_None; }

		m_readyTime = GetWorld()->GetTimeSeconds() + (m_nextActionId.IsNone() ? FMath::Max(EnemyCharacterBaseTuning::minRecovery, action->m_recovery) : FMath::Max(0.f, action->m_comboGap));
	}

	if (action && action->m_airOnly && m_nextActionId.IsNone()) { m_combat->EndLift(); }

	if (m_nextActionId.IsNone()) { m_comboSteps = 0; }

	if (action) m_abilitySystemComponent->RemoveLooseGameplayTags(action->m_activeTags);

	m_activeIndex = INDEX_NONE;

	if (!_cancelled) { m_defense->AttackFinished(); }

	m_combat->ResetWindows();
	m_abilitySystemComponent->SetLooseGameplayTagCount(CombatTags::StateActing, 0);
	m_actionSuccess = !_cancelled;
	OnActionEnded();

	if (!_cancelled && action && action->m_damage > 0.f && m_nextActionId.IsNone())
	{
		OnAttackFinished();
	}
}

//現在の攻撃を中断する。
void AEnemyCharacterBase::CancelAction()
{
	m_combat->EndLift();

	m_abilitySystemComponent->CancelAbilities();
	if (IsActing()) { FinishAction(true); }

	m_nextActionId = NAME_None;
	m_comboSteps = 0;
	m_combat->ResetWindows();
}

//次の戦闘に向けて状態を初期化する。
void AEnemyCharacterBase::ResetEncounter()
{
	m_attacksSinceCombo = 0;
	if (!HasAuthority()) { return; }
	CancelAction();
	StopAnimMontage();
	m_actionCooldowns.Empty();
	m_nextLift = 0.;

	m_lastAction = NAME_None;

	m_defense->ResetDefense();
	m_readyTime = 0.0;
	m_combatTarget = nullptr;
	SetCanBeDamaged(true);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	SetState(EEnemyState::Alive);
	SetHealthPool(m_definition ? m_definition->m_initialHealth : EnemyCharacterBaseTuning::baseHealth);
	if (AAIController* aiController = Cast<AAIController>(GetController()))
	{
		if (UBrainComponent* brain = aiController->GetBrainComponent())
		{
			brain->RestartLogic();
		}
	}
}

//終了時にタイマーや登録を解除する。
void AEnemyCharacterBase::EndPlay(const EEndPlayReason::Type _reason)
{
	if (GetCharacterMovement()->ProcessRootMotionPostConvertToWorld.IsBoundToObject(this))
	{
		GetCharacterMovement()->ProcessRootMotionPostConvertToWorld.Unbind();
	}
	m_abilitySystemComponent->GetGameplayAttributeValueChangeDelegate(m_attributeSet->GetHealthAttribute()).Remove(m_healthHandle);
	CancelAction();
	Super::EndPlay(_reason);
}

//通知の区間と、相手に残す距離を受け取る。
void AEnemyCharacterBase::SetAttackAim(bool _enabled, float _range)
{
	m_aimMove = _enabled;
	m_aimRange = FMath::Max(0.f, _range);
}

//ワープせず、通常の衝突判定へ渡す移動量だけを調整する。
FTransform AEnemyCharacterBase::AimAttackMove(
	const FTransform& _move, UCharacterMovementComponent* _movement, float _delta)
{
	if (!m_aimMove || !IsActing() || !IsValid(m_combatTarget)) { return _move; }

	//相手に近づきすぎない範囲だけ進む。
	const FVector toward = m_combatTarget->GetActorLocation() - GetActorLocation();
	const float gap = FMath::Max(0.f, toward.Size2D() - m_aimRange);
	FVector step = toward.GetSafeNormal2D() * FMath::Min(_move.GetTranslation().Size2D(), gap);
	step.Z = _move.GetTranslation().Z;

	//向きは通知で少しずつ変え、素材の旋回と二重に回さない。
	FTransform result = _move;
	result.SetTranslation(step);
	result.SetRotation(FQuat::Identity);
	return result;
}

//回避する方向を求める。
FVector AEnemyCharacterBase::EvadeDirection(const FEnemyActionData& _action) const
{
	//正面の方向。
	FVector forward = IsValid(m_combatTarget) ? (m_combatTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal2D() : GetActorForwardVector();
	return forward.RotateAngleAxis(_action.m_evadeAngle, FVector::UpVector);
}

//障害物を避けて回避できるか調べる。
bool AEnemyCharacterBase::CanEvade(const FEnemyActionData& _action) const
{
	return !FindEvadePath(_action).IsNearlyZero();
}

//後ろ、左後ろ、右後ろの順に、最後まで移動できる向きを探す。
FVector AEnemyCharacterBase::FindEvadePath(const FEnemyActionData& _action) const
{
	const FVector dir = EvadeDirection(_action);
	if (HasEvadePath(_action, dir)) { return dir; }
	if (!m_findRetreatAngle || !FMath::IsNearlyEqual(FMath::Abs(_action.m_evadeAngle), 180.f))
	{
		return FVector::ZeroVector;
	}
	constexpr float diagonal = 45.f;
	for (float angle : {-diagonal, diagonal, -diagonal * 1.5f, diagonal * 1.5f})
	{
		const FVector side = dir.RotateAngleAxis(angle, FVector::UpVector);
		if (HasEvadePath(_action, side)) { return side; }
	}
	return FVector::ZeroVector;
}

//床の高さと体が通る幅を候補ごとに確認する。
bool AEnemyCharacterBase::HasEvadePath(const FEnemyActionData& _action, FVector _dir) const
{
	//歩ける場所を調べる機能。
	auto* nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!nav || !_action.m_montage || !GetCharacterMovement()->IsMovingOnGround())
	{
		return false;
	}

	//調べ始める位置。
	const FVector start = GetActorLocation();

	//前回からの経過時間や値の差。
	const FVector delta = _dir * _action.m_backSpeed * FMath::Max(0.f, _action.m_backEnd - _action.m_backStart);

	//足元の位置。
	const float feet = start.Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	for (int i = 1; i <= EnemyCharacterBaseTuning::pathSteps; ++i)
	{
		//地面に当たった情報。
		FNavLocation floor;

		//退避の経路を等間隔に調べる。
		const float fraction = i / EnemyCharacterBaseTuning::pathStepCount;
		const FVector sample = FVector(start.X, start.Y, feet) + delta * fraction;
		const float width = EnemyCharacterBaseTuning::pathWidth;
		const FVector searchArea(width, width, EnemyCharacterBaseTuning::navHeight);
		if (!nav->ProjectPointToNavigation(sample, floor, searchArea)) { return false; }

		//歩いて越えられない高さなら、この方向を候補から外す。
		const float stepHeight = FMath::Abs(floor.Location.Z - feet);
		if (stepHeight > GetCharacterMovement()->MaxStepHeight) { return false; }
	}

	//ぶつかった場所の情報。
	FHitResult hit;
	FCollisionQueryParams params(SCENE_QUERY_STAT(EnemyEvade), false, this);

	//足元の床を拾わずに、体が壁や敵にぶつからないか調べる。
	const float margin = EnemyCharacterBaseTuning::floorMargin;
	const FVector from = start + FVector(0, 0, margin);
	const FVector to = start + delta + FVector(0, 0, margin);
	const auto* capsule = GetCapsuleComponent();
	const float radius = FMath::Max(1.f, capsule->GetScaledCapsuleRadius() - 2);
	const float halfHeight = FMath::Max(1.f, capsule->GetScaledCapsuleHalfHeight() - margin);
	const auto body = FCollisionShape::MakeCapsule(radius, halfHeight);
	return !GetWorld()->SweepSingleByChannel(hit, from, to, FQuat::Identity, ECC_Pawn, body, params);
}