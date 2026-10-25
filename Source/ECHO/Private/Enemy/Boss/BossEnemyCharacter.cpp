#include "Enemy/Boss/BossEnemyCharacter.h"
#include "Enemy/Boss/BossShadowSummon.h"
#include "Enemy/Boss/BossObservation.h"
#include "Enemy/Boss/BossPosture.h"
#include "Enemy/Animation/EnemyAfterimageTrail.h"
#include "Enemy/Boss/BossApproach.h"

#include "Enemy/Boss/BossSpacing.h"

#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "BrainComponent.h"

//動作を確かめるための幅や待ち時間。
namespace BossEnemyCharacterTuning
{
    //フェーズ切り替えの待ち時間の下限。
    constexpr auto minTransition = .2f;
}

//初期設定と必要な部品を用意する。
ABossEnemyCharacter::ABossEnemyCharacter()
{
	m_summon = CreateDefaultSubobject<UBossShadowSummon>(TEXT("ShadowSummon"));
	m_posture = CreateDefaultSubobject<UBossPosture>(TEXT("Posture"));
	m_afterimage = CreateDefaultSubobject<UEnemyAfterimageTrail>(TEXT("Afterimage"));
	m_approach = CreateDefaultSubobject<UBossApproach>(TEXT("Approach"));
	m_observation = CreateDefaultSubobject<UBossObservation>(TEXT("Observation"));

	m_spacing = CreateDefaultSubobject<UBossSpacing>(TEXT("Spacing"));

}

//ボス専用の設定を返す。
UBossData* ABossEnemyCharacter::GetBossData() const
{
	return Cast<UBossData>(m_definition);
}

//ゲーム開始時に必要な状態を準備する。
void ABossEnemyCharacter::BeginPlay()
{

	if (!GetBossData())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: Assign a BossData asset to Definition in the boss Blueprint."), *GetName());
	}

	Super::BeginPlay();
	PhaseLook(false);
}

//体力がなくなった時の処理を行う。
void ABossEnemyCharacter::HandleHealthDepleted()
{

	m_spacing->Stop();

	//BPから設定した敵のデータ。
	const UBossData* data = GetBossData();
	
	//一度目の体力切れでは死亡せず、立ち上がりへ進む。
	if (m_bossPhase == EBossPhase::PhaseOne && data && data->m_bSecondLife)
	{
		SetLife(EBossPhase::Transition);
		//フェーズ1で見た相手の癖を、次のフェーズへ残す。
		m_observation->Freeze();

		if (AAIController* controller = Cast<AAIController>(GetController()))
		{
			if (UBrainComponent* brain = controller->GetBrainComponent()) { brain->StopLogic(TEXT("Phase transition")); }
		}
		
		//残っている攻撃を止めて切り替えに備える。
		CancelAction();
		if (AAIController* controller = Cast<AAIController>(GetController()))
		{
			controller->StopMovement();
		}
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
		SetHealthPool(data->m_phaseTwoHealth);
		
		//次の処理まで待つ時間。
		float delay = FMath::Max(BossEnemyCharacterTuning::minTransition, data->m_transitionTime);
		if (data->m_transitionMontage)
		{
			delay = FMath::Max(delay, PlayAnimMontage(data->m_transitionMontage));
		}
		GetWorldTimerManager().SetTimer(m_transitionTimer, this, &ABossEnemyCharacter::FinishTransition, delay, false);
		return;
	}
	
	//最終フェーズの体力切れは通常の死亡として扱う。
	m_bossPhase = EBossPhase::Dead;
	Super::HandleHealthDepleted();
	m_onLifeChanged.Broadcast(m_bossPhase);
	BossDefeated();
}

//ボスのフェーズを切り替える。
void ABossEnemyCharacter::SetLife(EBossPhase _newLife)
{
	m_posture->Reset();
	m_afterimage->Stop();
	m_bossPhase = _newLife;
	SetState(m_bossPhase == EBossPhase::Dead ? EEnemyState::Dead : m_bossPhase == EBossPhase::Transition ? EEnemyState::Suspended : EEnemyState::Alive);
	m_onLifeChanged.Broadcast(m_bossPhase);
	if(m_bossPhase == EBossPhase::PhaseOne || m_bossPhase == EBossPhase::PhaseTwo) { PhaseLook(m_bossPhase == EBossPhase::PhaseTwo); }
}

//立ち上がりを終えてフェーズ2を開始する。
void ABossEnemyCharacter::FinishTransition()
{
	if(m_bossPhase != EBossPhase::Transition) { return; }

	StopAnimMontage();
	
	//残っている攻撃を止めて切り替えに備える。
	CancelAction();
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->MaxWalkSpeed = m_moveSpeed;
	
	//移動を使える状態へ戻してからフェーズ2を始める。
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	SetLife(EBossPhase::PhaseTwo);
	m_readyTime = GetWorld()->GetTimeSeconds() + (GetBossData() ? FMath::Max(0.f, GetBossData()->m_phaseRecovery) : 0.f);
	if (AAIController* controller = Cast<AAIController>(GetController()))
	{
		if (UBrainComponent* brain = controller->GetBrainComponent()) { brain->RestartLogic(); }
	}
}

//受け流しなどを選ぶ確率を返す。
float ABossEnemyCharacter::GetSpecialDefenseChance() const
{
	//BPから設定した敵のデータ。
	const UBossData* data = GetBossData();
	if(data && m_bossPhase == EBossPhase::PhaseOne && GetHealth() > GetMaxHealth() * data->m_counterHealth) { return 0.f; }
	return data ? (m_bossPhase == EBossPhase::PhaseTwo ? data->m_phaseTwoChance : data->m_phaseOneChance) : Super::GetSpecialDefenseChance();
}

//現在の状態で選べる攻撃か調べる。
bool ABossEnemyCharacter::IsActionAllowed(const FEnemyActionData& _action) const
{
	//刀を抜き終わってから攻撃を選ぶ。
	if(m_approach && !m_approach->HasDrawn()) { return false; }
	
	//BPから設定した敵のデータ。
	const UBossData* data = GetBossData();
	if(!data) { return true; }

	//このフェーズで選べる攻撃。
	const TArray<FName>& allowedActions = m_bossPhase == EBossPhase::PhaseTwo ? data->m_phaseTwoActions : data->m_phaseOneActions;
	return allowedActions.IsEmpty() || allowedActions.Contains(_action.m_actionId);
}

//状況に合わせて攻撃の選びやすさを返す。
float ABossEnemyCharacter::GetActionWeightMultiplier(const FEnemyActionData& _action) const
{
	//フェーズ2は観察した癖を使い、分身が多ければ広い攻撃も選ぶ。
	float weight = m_bossPhase == EBossPhase::PhaseTwo ? m_observation->AdaptWeight(uint8(_action.m_kind)) : 1.0f;
	if(m_summon->GetCloneCount() >= m_summon->m_cloneLimit && m_summon->m_crowdMoves.Contains(_action.m_actionId))
	{
		weight *= m_summon->m_crowdWeight;
	}
	return weight;
}

//相手の攻撃や移動を観察する。
void ABossEnemyCharacter::ObserveTarget(AActor* _target)
{
	if (m_bossPhase == EBossPhase::PhaseOne)
	{
		m_observation->Observe(_target, GetBossData());
	}
}

//ダメージを受けた後の反応を行う。
void ABossEnemyCharacter::OnAcceptedHit(AActor* _attacker)
{
	if (m_bossPhase == EBossPhase::PhaseOne && _attacker == m_combatTarget)
	{
		m_observation->RecordHit();
	}
}

//行動終了後の状態を整える。
void ABossEnemyCharacter::OnActionEnded()
{
	m_observation->MarkBossActionEnded();
}

//次の戦闘に向けて状態を初期化する。
void ABossEnemyCharacter::ResetEncounter()
{
	if(!HasAuthority()) { return; }

	GetWorld()->GetTimerManager().ClearTimer(m_transitionTimer);
	Super::ResetEncounter();
	m_summon->Reset();
	m_observation->Reset();
	m_approach->Reset();

	m_spacing->Reset();

	SetLife(EBossPhase::PhaseOne);
}

//終了時にタイマーや登録を解除する。
void ABossEnemyCharacter::EndPlay(const EEndPlayReason::Type _reason)
{
	GetWorld()->GetTimerManager().ClearTimer(m_transitionTimer);
	Super::EndPlay(_reason);
}

//間合いを取り直しているか返す。
bool ABossEnemyCharacter::IsRepositioning() const
{
 return (m_approach && m_approach->IsDrawing()) || (m_spacing && m_spacing->IsSpacing());
}

//攻撃終了後の次の行動を準備する。
void ABossEnemyCharacter::OnAttackFinished()
{
 m_spacing->Queue();
}

//被弾した分を体勢ゲージにも伝える。
float ABossEnemyCharacter::TakeDamage(float _damage, const FDamageEvent& _event, AController* _instigator, AActor* _causer)
{
    //致命傷によるフェーズ切り替えでは体勢を減らさない。
    const EBossPhase phase = m_bossPhase;
    const float taken = Super::TakeDamage(_damage, _event, _instigator, _causer);
    if(taken > 0.f && phase == m_bossPhase && IsCombatActive()) { m_posture->Hit(taken); }
    return taken;
}
