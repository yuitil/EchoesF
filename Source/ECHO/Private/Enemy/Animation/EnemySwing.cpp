#include "Enemy/Animation/EnemySwing.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Enemy/Combat/Combat.h"
#include "Enemy/Audio/EnemyAudio.h"
#include "Components/SkeletalMeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

//剣の更新後に、短い区間だけ動かす。
UEnemySwing::UEnemySwing()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

//前のフレームの剣を読まないようにする。
void UEnemySwing::BeginPlay()
{
    Super::BeginPlay();
    auto* enemy = Cast<AEnemyCharacterBase>(GetOwner());
    if(enemy && enemy->GetMesh()) { AddTickPrerequisiteComponent(enemy->GetMesh()); }
}

//一振りごとに音と位置を初期化する。
void UEnemySwing::Start()
{
    Stop();
    m_open = true;
    SetComponentTickEnabled(true);
    auto* enemy = Cast<AEnemyCharacterBase>(GetOwner());
    FVector base, tip;
    if(!enemy || !enemy->m_combat || !enemy->m_combat->GetWeaponPoints(base, tip)) { return; }
    m_lastTip = enemy->GetActorTransform().InverseTransformPosition(tip);
    m_hasTip = true;
}

//中断した線を次の攻撃へつながない。
void UEnemySwing::ClearTrail()
{
    if(IsValid(m_fx)) { m_fx->DestroyComponent(); }
    m_fx = nullptr;
}

//通知終了や攻撃中断で必ず止める。
void UEnemySwing::Stop()
{
    ClearTrail();
    m_open = false;
    m_hasTip = false;
    m_played = false;
    SetComponentTickEnabled(false);
}

//移動そのものを剣の振りと間違えないようにする。
void UEnemySwing::TickComponent(float _delta, ELevelTick _type, FActorComponentTickFunction* _tick)
{
    Super::TickComponent(_delta, _type, _tick);
    auto* enemy = Cast<AEnemyCharacterBase>(GetOwner());
    if(!m_open || !enemy || !enemy->IsActing() || !enemy->m_combat || !enemy->m_combat->IsHitWindowOpen()) { Stop(); return; }
    if(_delta <= SMALL_NUMBER) { return; }

    //攻撃判定と同じ剣の位置を使う。
    FVector base, tip;
    if(!enemy->m_combat->GetWeaponPoints(base, tip)) { ClearTrail(); m_hasTip = false; return; }
    const FVector localTip = enemy->GetActorTransform().InverseTransformPosition(tip);
    const float step = m_hasTip ? FVector::Distance(localTip, m_lastTip) : 0.f;
    m_lastTip = localTip;
    m_hasTip = true;
    if(step > m_maxStep || step / _delta < m_minSpeed) { ClearTrail(); return; }

    //剣が動き出した一回だけ音を出す。
    const FVector point = FMath::Lerp(base, tip, m_tipPart);
    if(!m_played)
    {
        if(enemy->m_audio) { enemy->m_audio->Play(EEnemySound::Swing, point, m_gain); }
        m_played = true;
    }

    //素材は敵BPに設定されたものを使う。
    if(!m_trail || GetNetMode() == NM_DedicatedServer) { return; }
    if(!IsValid(m_fx))
    {
        m_fx = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, m_trail, point, FRotator::ZeroRotator,
            FVector::OneVector, true, true, ENCPoolMethod::None, false);
    }
    if(m_fx) { m_fx->SetWorldLocation(point); }
}

//死亡・解除・マップ終了でも残さない。
void UEnemySwing::EndPlay(const EEndPlayReason::Type _reason)
{
    Stop();
    Super::EndPlay(_reason);
}
