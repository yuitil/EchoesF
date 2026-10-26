#include "Enemy/Boss/BossShadow.h"
#include "Enemy/Audio/EnemyAudio.h"
#include "Enemy/Boss/BossShadowSummon.h"
#include "Enemy/Boss/BossApproach.h"
#include "Enemy/Boss/BossSpacing.h"
#include "Enemy/Boss/BossPosture.h"
#include "Enemy/AI/EnemyTargeting.h"
#include "Enemy/UI/EnemyHealthDisplay.h"
#include "Components/MeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
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
    if((!m_smoke && !m_gate) || GetNetMode() == NM_DedicatedServer) { return; }
    
    //壁やキャラクターではなく、足元の床を調べる。
    const FVector start = GetActorLocation();
    FHitResult hit;
    FCollisionQueryParams params(SCENE_QUERY_STAT(ShadowSmoke), false, this);
    if(!GetWorld()->LineTraceSingleByObjectType(hit, start, start - FVector(0.f, 0.f, m_floorDepth), FCollisionObjectQueryParams(ECC_WorldStatic), params)) { return; }
    
    //影が消えた後も、残った煙はNiagaraの寿命で消える。
    for(auto* effect : {m_smoke.Get(), m_gate.Get()})
    {
        if(!effect) { continue; }
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, effect, hit.ImpactPoint + hit.ImpactNormal * m_floorOffset,
            FRotationMatrix::MakeFromZ(hit.ImpactNormal).Rotator(), m_smokeScale, true, true, ENCPoolMethod::AutoRelease);
    }
}

//HP画面と再召喚を止めてから開始する。
void ABossShadow::PostInitializeComponents()
{
    Super::PostInitializeComponents();
    TInlineComponentArray<UEnemyHealthDisplay*> bars(this);
    for(auto* bar : bars) 
    { 
        bar->m_bossHUD = false; bar->SetWidgetClass(nullptr); 
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

//本体と同じメッシュを、BPで選んだ影の色にする。
void ABossShadow::BeginPlay()
{
    Super::BeginPlay();
    if(m_audio) { m_audio->Play(EEnemySound::Spawn, GetActorLocation()); }
    //開始時の登録でTickが戻っても、本体用の抜刀と召喚を動かさない。
    m_approach->SetComponentTickEnabled(false);
    m_summon->SetComponentTickEnabled(false);
    m_approach->ShowSword();
    PlaySmoke();
    if(GetNetMode() != NM_DedicatedServer) 
    {
        m_aura->Activate(true);
    }
    TInlineComponentArray<UMeshComponent*> meshes(this);
    for(auto* mesh : meshes)
    {
        if(!m_shadowMat || !mesh->IsVisible()) { continue; }
        for(int32 slot = 0; slot < mesh->GetNumMaterials(); ++slot)
        {
            auto* mat = mesh->CreateDynamicMaterialInstance(slot, m_shadowMat);
            if(mat) { mat->SetScalarParameterValue(m_fadeParam, 0.f); m_mats.Add(mat); }
        }
    }
}

//担当する分身と寿命を固定する。
void ABossShadow::Setup(AActor* _target, float _life)
{
    m_targeting->m_fixedTarget = _target;
    m_combatTarget = _target;
    m_life = FMath::Max(m_riseTime, _life);
}

//出現が終わるまで戦闘を待つ。
bool ABossShadow::IsRepositioning() const
{
    return m_fading || m_age < m_riseTime || (m_join && (m_joinDone || m_age < m_joinDelay));
}

//本体のフェーズ2の攻撃を抽選候補にする。
bool ABossShadow::IsActionAllowed(const FEnemyActionData& _action) const
{
    const auto* data = GetBossData();
    if(m_join && m_actionSerial == 0 && !m_joinAction.IsNone() && _action.m_actionId != m_joinAction) { return false; }
    return !m_fading && (!data || data->m_phaseTwoActions.IsEmpty() || data->m_phaseTwoActions.Contains(_action.m_actionId));
}

//固定の攻撃順ではなく既存の重み抽選を使う。
float ABossShadow::GetActionWeightMultiplier(const FEnemyActionData& _action) const
{
    return 1.f;
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
    if(m_join && (!m_actionSuccess || GetPendingComboId().IsNone())) { m_joinDone = true; }
}

//分身がいない時は、本体に続いて一つのコンボだけ行う。
void ABossShadow::StartJoin(FName _action, float _delay, float _range)
{
    m_join = true;
    m_joinAction = _action;
    m_joinDelay = FMath::Max(m_riseTime, _delay);
    m_cloneRange = _range;
}

//新しく出された分身を優先する。プレイヤー側には変更を加えない。
AActor* ABossShadow::FindClone() const
{
    AActor* target = nullptr;
    float best = FMath::Square(m_cloneRange);
    for(TActorIterator<AGhostCharacter> it(GetWorld()); it; ++it)
    {
        auto* ghost = *it;
        if(!ghost->EnemyComponent || ghost->IsHidden() || !ghost->CanBeDamaged()) { continue; }
        const auto state = ghost->EnemyComponent->GetLifecycleState();
        if(state != EGhostLifecycleState::Friendly && state != EGhostLifecycleState::Warning) { continue; }
        const float distance = FVector::DistSquared2D(GetActorLocation(), ghost->GetActorLocation());
        if(distance < best) { best = distance; target = ghost; }
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
    if(m_fading) { return; }
    m_fading = true;
    if(m_audio) { m_audio->Play(EEnemySound::Fade, GetActorLocation()); }
    PlaySmoke();
    m_aura->Deactivate();
    CancelAction();
    StopAnimMontage();
    SetState(EEnemyState::Dead);
    SetCanBeDamaged(false);
    SetActorEnableCollision(false);
    GetCharacterMovement()->StopMovementImmediately();
    if(auto* ai = Cast<AAIController>(GetController()))
    {
        ai->StopMovement();
        if(ai->GetBrainComponent()) { ai->GetBrainComponent()->StopLogic(TEXT("Shadow ended")); }
    }
}

//寿命と対象の消滅を確認して見た目を薄くする。
void ABossShadow::Tick(float _delta)
{
    Super::Tick(_delta);
    m_age += _delta;
    //連携中に分身が出たら、プレイヤーへの攻撃を止めて担当を変える。
    if(HasAuthority() && m_join && !m_fading)
    {
        if(auto* clone = FindClone())
        {
            m_join = false;
            CancelAction();
            m_targeting->m_fixedTarget = clone;
            m_combatTarget = clone;
        }
        else if(m_joinDone || m_age >= m_joinLimit) { FadeOut(); }
    }
    if(HasAuthority() && !m_fading && (m_age >= m_life || !m_targeting->SelectTarget())) 
    {
        FadeOut();
    }
    if(m_fading) { m_fadeAge += _delta; }
    const float fade = m_fading ? 1.f - FMath::Clamp(m_fadeAge / FMath::Max(UE_SMALL_NUMBER, m_fadeTime), 0.f, 1.f) : FMath::Clamp(m_age / FMath::Max(UE_SMALL_NUMBER, m_riseTime), 0.f, 1.f);
    for(const auto& mat : m_mats) 
    { 
        if(mat) 
        { 
            mat->SetScalarParameterValue(m_fadeParam, fade); 
        }
    }
    if(HasAuthority() && m_fading && m_fadeAge >= m_fadeTime)
    {
        //影のAIも残さず片付ける。
        if(auto* ai = GetController()) 
        { 
            ai->UnPossess(); 
            ai->Destroy();
        }
        Destroy();
    }
}
