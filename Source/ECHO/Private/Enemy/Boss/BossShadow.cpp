#include "Enemy/Boss/BossShadow.h"
#include "Enemy/Audio/EnemyAudio.h"
#include "Enemy/Combat/Combat.h"
#include "Enemy/Boss/BossShadowSummon.h"
#include "Enemy/Boss/BossApproach.h"
#include "Enemy/Boss/BossSpacing.h"
#include "Enemy/Boss/BossObservation.h"
#include "Enemy/Boss/BossPosture.h"
#include "Enemy/AI/EnemyTargeting.h"
#include "Enemy/UI/EnemyHealthDisplay.h"
#include "Components/MeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Ghost/GhostCharacter/GhostCharacter.h"
#include "Ghost/GhostEnemy/GhostEnemyComponent.h"
#include "EngineUtils.h"

//オーラの素材や位置はBPから設定する。
ABossShadow::ABossShadow()
{
	m_aura = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ShadowAura"));
	m_aura->SetupAttachment(GetRootComponent());
	m_aura->SetAutoActivate(false);
}

//現在いる場所の床へ、出現と消滅に共通の煙を出す。
void ABossShadow::PlaySmoke()
{
	PlaySmokeAt(GetActorLocation());
}

//指定した場所の床に影の煙を出す。
void ABossShadow::PlaySmokeAt(const FVector& _place)
{
	if ((!m_smoke && !m_gate) || GetNetMode() == NM_DedicatedServer) { return; }

	//壁やキャラクターではなく、足元の床を調べる。
	const FVector start = _place;
	FHitResult hit;
	FCollisionQueryParams params(SCENE_QUERY_STAT(ShadowSmoke), false, this);
	if (!GetWorld()->LineTraceSingleByObjectType(hit, start, start - FVector(0.f, 0.f, m_floorDepth), FCollisionObjectQueryParams(ECC_WorldStatic), params))
	{
		return;
	}

	//影が消えた後も、残った煙はNiagaraの寿命で消える。
	for (auto* effect : { m_smoke.Get(), m_gate.Get() })
	{
		if (!effect) { continue; }
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, effect, hit.ImpactPoint + hit.ImpactNormal * m_floorOffset, FRotationMatrix::MakeFromZ(hit.ImpactNormal).Rotator(), m_smokeScale, true, true, ENCPoolMethod::AutoRelease);
	}
}

//HP画面と再召喚を止めてから開始する。
void ABossShadow::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	TInlineComponentArray<UEnemyHealthDisplay*> bars(this);
	for (auto* bar : bars)
	{
		bar->m_bossHUD = false;
		bar->SetWidgetClass(nullptr);
		bar->SetVisibility(false);
	}
	m_summon->m_shadowClass = nullptr;
	m_summon->SetComponentTickEnabled(false);
	m_posture->m_enabled = false;
	m_spacing->m_enabled = false;
	m_approach->SetComponentTickEnabled(false);
	m_targeting->m_lockTarget = true;
	m_targeting->m_targetClones = true;
}

//影の戦闘と出現の演出を始める。
void ABossShadow::BeginPlay()
{
	Super::BeginPlay();
	if (m_audio) { m_audio->Play(EEnemySound::Spawn, GetActorLocation()); }

	//開始時の登録でTickが戻っても、本体用の抜刀と召喚を動かさない。
	m_approach->SetComponentTickEnabled(false);
	m_summon->SetComponentTickEnabled(false);
	m_approach->ShowSword();
	PlaySmoke();
	if (GetNetMode() != NM_DedicatedServer) { m_aura->Activate(true); }
	if (GetNetMode() == NM_DedicatedServer) { return; }

	//BPで影の見た目に指定した部品を覚えておく。
	TInlineComponentArray<UMeshComponent*> meshes(this);
	for (auto* mesh : meshes)
	{
		if (mesh->ComponentHasTag(TEXT("ShadowLook"))) { m_fadeMeshes.Add(mesh); }
	}
	SetFade(0.f);
}

//担当する分身と寿命を固定する。
void ABossShadow::Setup(AActor* _target, float _life)
{
	m_targeting->m_fixedTarget = _target;
	m_combatTarget = _target;
	m_life = FMath::Max(m_showDelay + m_riseTime, _life);

	//通常召喚でも次の分身を探す範囲を引き継ぐ。
	if (auto* boss = Cast<ABossEnemyCharacter>(GetOwner()))
	{
		m_cloneRange = boss->m_summon->m_range;
	}
}

//出現が終わるまで戦闘を待つ。
bool ABossShadow::IsRepositioning() const
{
	const auto* owner = Cast<ABossEnemyCharacter>(GetOwner());
	if (owner && owner->m_summon->IsWaitingForSwap(this)) { return true; }
	return m_fading || m_age < m_showDelay + m_riseTime || GetWorld()->GetTimeSeconds() < m_followTime || (m_join && (m_joinDone || m_age < m_joinDelay));
}

//本体のフェーズ2の攻撃を抽選候補にする。
bool ABossShadow::IsActionAllowed(const FEnemyActionData& _action) const
{
	const auto* data = GetBossData();
	if (m_join && m_actionSerial == 0 && !m_joinAction.IsNone() && _action.m_actionId != m_joinAction)
	{
		return false;
	}
    //連携の最初は、ガードや退避ではなく攻撃から始める。
    if (m_join && m_actionSerial == 0 && _action.m_damage <= 0.f) { return false; }
	return !m_fading && (!data || data->m_phaseTwoActions.IsEmpty() || data->m_phaseTwoActions.Contains(_action.m_actionId));
}

//固定の攻撃順ではなく既存の重み抽選を使う。
float ABossShadow::GetActionWeightMultiplier(const FEnemyActionData& _action) const
{

	//同じ相手には本体と違う攻撃を選びやすくする。
	const auto* boss = Cast<ABossEnemyCharacter>(GetOwner());
	const auto* target = Cast<APawn>(m_combatTarget.Get());
	const float learned = boss && target && target->IsPlayerControlled() ? boss->m_observation->MoveWeight(_action.m_actionId) : 1.f;
	const auto* move = boss ? boss->GetActiveAction() : nullptr;
	if (boss && boss->m_combatTarget == m_combatTarget && move && move->m_actionId == _action.m_actionId)
	{
		return learned * FMath::Clamp(m_sameMoveWeight, 0.f, 1.f);
	}
	return learned;
}

//コンボ内のつながりを保ち、終わった後の間だけ変える。
void ABossShadow::OnAttackFinished()
{
	m_readyTime += FMath::FRandRange(FMath::Max(0.f, m_waitMin), FMath::Max(m_waitMin, m_waitMax));
}

//連携が終わった次の更新で分身を確認してから消える。
void ABossShadow::OnActionEnded()
{
	Super::OnActionEnded();
	if (m_join && (!m_actionSuccess || GetPendingComboId().IsNone())) { m_joinDone = true; }
}

//分身がいない時は、本体に続いて一つのコンボだけ行う。
void ABossShadow::StartJoin(FName _action, float _delay, float _range)
{
	m_join = true;
	m_joinAction = _action;
	m_joinDelay = FMath::Max(m_showDelay + m_riseTime, _delay);
	m_cloneRange = _range;
}

//新しく出された分身を優先する。プレイヤー側には変更を加えない。
AActor* ABossShadow::FindClone() const
{
	AActor* target = nullptr;
	float best = FMath::Square(m_cloneRange);
	for (TActorIterator<AGhostCharacter> it(GetWorld()); it; ++it)
	{
		auto* ghost = *it;
		if (!ghost->EnemyComponent || ghost->IsHidden() || !ghost->CanBeDamaged()) { continue; }
		const auto state = ghost->EnemyComponent->GetLifecycleState();
		if (state != EGhostLifecycleState::Friendly && state != EGhostLifecycleState::Warning)
		{
			continue;
		}

		//もう一体の影が受け持つ分身は選ばない。
		const auto* boss = Cast<ABossEnemyCharacter>(GetOwner());
		bool taken = false;
		if (boss && boss->m_summon)
		{
			for (auto* shadow : { boss->m_summon->m_shadow.Get(), boss->m_summon->m_secondShadow.Get() })
			{
				if (IsValid(shadow) && shadow != this && shadow->IsCombatActive() &&
					shadow->m_combatTarget == ghost)
				{
					taken = true;
				}
			}
		}
		if (taken) { continue; }
		const float distance = FVector::DistSquared2D(GetActorLocation(), ghost->GetActorLocation());
		if (distance < best)
		{
			best = distance;
			target = ghost;
		}
	}
	return target;
}

//倒された影は立ち上がらずに消える。
void ABossShadow::HandleHealthDepleted()
{
	FadeOut();
}

//攻撃と移動を止め、当たり判定を消す。
void ABossShadow::FadeOut()
{
	if (m_fading) { return; }
	m_fading = true;
	if (m_audio) { m_audio->Play(EEnemySound::Fade, GetActorLocation()); }
	PlaySmoke();
	m_aura->Deactivate();
	CancelAction();
	StopAnimMontage();
	SetState(EEnemyState::Dead);
	SetCanBeDamaged(false);
	SetActorEnableCollision(false);
	GetCharacterMovement()->StopMovementImmediately();
	if (auto* ai = Cast<AAIController>(GetController()))
	{
		ai->StopMovement();
		if (ai->GetBrainComponent())
		{
			ai->GetBrainComponent()->StopLogic(TEXT("Shadow ended"));
		}
	}
}

//本体を待っている影も、出現が終われば交代できる。
bool ABossShadow::CanRelay() const
{
	return IsCombatActive() && !m_fading && m_age >= m_showDelay + m_riseTime && !IsActing() && (!m_join || (!m_joinDone && m_age >= m_joinDelay));
}

//次に動ける瞬間、交代位置から一撃を返す。
void ABossShadow::RelayStrike(FName _action)
{
	m_followTime = 0.;
	m_strikeId = _action;
	m_strikeEnd = GetWorld()->GetTimeSeconds() + 1.f;
}

//寿命と対象の消滅を確認して見た目を薄くする。
void ABossShadow::Tick(float _delta)
{
	Super::Tick(_delta);
	m_age += _delta;

	//本体の援護より先に、まだ誰も狙っていない分身を探す。
	if (HasAuthority() && !m_fading)
	{
		AActor* target = m_targeting->SelectTarget();
		const double now = GetWorld()->GetTimeSeconds();
		if (!Cast<AGhostCharacter>(target) && (!target || now >= m_nextSearch))
		{
			m_nextSearch = now + FMath::Max(.05f, m_searchDelay);
			if (auto* clone = FindClone())
			{
				m_join = false;
				CancelAction();
				m_followTime = m_strikeEnd = 0.;
				Setup(clone, m_life);
			}
		}
		if (m_join && (m_joinDone || m_age >= m_joinLimit)) { FadeOut(); }
	}

	//同じ相手への新しい攻撃は、本体の振り終わりに続ける。
	auto* boss = Cast<ABossEnemyCharacter>(GetOwner());
	if (HasAuthority() && !m_fading && !IsActing() && boss && boss->m_combatTarget == m_combatTarget && m_strikeEnd == 0. && boss->m_combat && (boss->m_combat->IsHitWindowOpen() || boss->m_summon->CanCover(*this)))
	{
		m_followTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.f, m_followGap);
	}
	if (HasAuthority() && m_strikeEnd > 0.)
	{
		if (!IsCombatActive() || GetWorld()->GetTimeSeconds() > m_strikeEnd)
		{
			m_strikeEnd = 0.;
		}
		else if (CanDecide())
		{
			m_strikeEnd = 0.;
			if (!RequestAction(m_strikeId)) { RequestAction(); }
		}
	}
	if (HasAuthority() && !m_fading && (m_age >= m_life || !m_targeting->SelectTarget()))
	{
		FadeOut();
	}
	if (m_fading) { m_fadeAge += _delta; }

	//出現中は濃くし、消滅中は薄くする。
	const float age = m_fading ? m_fadeAge : FMath::Max(0.f, m_age - m_showDelay);
	const float duration = m_fading ? m_fadeTime : m_riseTime;
	const float progress = FMath::Clamp(age / FMath::Max(UE_SMALL_NUMBER, duration), 0.f, 1.f);
	const float fade = m_fading ? 1.f - progress : progress;
	SetFade(fade);
	if (HasAuthority() && m_fading && m_fadeAge >= m_fadeTime)
	{

		//影のAIも残さず片付ける。
		if (auto* ai = GetController())
		{
			ai->UnPossess();
			ai->Destroy();
		}
		Destroy();
	}
}


//出現と消滅の途中だけ、BPで選んだ素材の濃さを変える。
void ABossShadow::SetFade(float _amount)
{
	if (m_lastFade == _amount) { return; }
	m_lastFade = _amount;
	for (const auto& mesh : m_fadeMeshes)
	{
		if (IsValid(mesh)) { mesh->SetScalarParameterValueOnMaterials(m_fadeParam, _amount); }
	}
}
