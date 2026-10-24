#include "Enemy/Animation/EnemyAfterimageTrail.h"
#include "Enemy/Animation/EnemyAfterimage.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Engine/World.h"

//モデル更新後に残像を作るようにする。
UEnemyAfterimageTrail::UEnemyAfterimageTrail()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

//通知から残像を開始する。
void UEnemyAfterimageTrail::Start(bool _allowStill, float _duration)
{
    Stop();
    m_left = _duration;
    m_active = true;
    m_allowStill = _allowStill;
    m_wait = 0.f;
    m_lastPos = GetOwner()->GetActorLocation();
}

//通知から残像を終了する。
void UEnemyAfterimageTrail::Stop()
{
    m_active = false;
    //次の動作へ前の残像を持ち越さない。
    for(auto& image : m_images) { if(image.IsValid()) { image->Destroy(); } }
    m_images.Empty();
}

//表示した残像を終了時に片付ける。
void UEnemyAfterimageTrail::EndPlay(const EEndPlayReason::Type _reason)
{
    Stop();
    for(auto& image : m_images) { if(image.IsValid()) { image->Destroy(); } }
    m_images.Empty();
    Super::EndPlay(_reason);
}

//移動距離と間隔を調べて残像を出す。
void UEnemyAfterimageTrail::TickComponent(float _delta, ELevelTick _type, FActorComponentTickFunction* _tick)
{
    Super::TickComponent(_delta, _type, _tick);
    if(!m_active || !m_imageClass || GetNetMode() == NM_DedicatedServer) { return; }

    //中断や死亡で通知が終わらなかった時も停止する。
    auto* enemy = Cast<AEnemyCharacterBase>(GetOwner());
    if(!enemy || !enemy->IsCombatActive() || !enemy->GetMesh()->GetAnimInstance() || !enemy->GetMesh()->GetAnimInstance()->IsAnyMontagePlaying()) { Stop(); return; }

    //終わり際は新しく出さず、残っている姿を消し切る。
    auto* anim = enemy->GetMesh()->GetAnimInstance();
    const float rate = FMath::Abs(anim->Montage_GetPlayRate(anim->GetCurrentActiveMontage()));
    m_left -= _delta * rate;
    const auto* settings = m_imageClass.GetDefaultObject();
    if(m_left <= settings->m_life * rate) { return; }

    m_wait += _delta;
    if(m_wait < FMath::Max(.03f, m_interval) || (!m_allowStill && FVector::DistSquared(m_lastPos, enemy->GetActorLocation()) < FMath::Square(FMath::Max(1.f, m_distance)))) { return; }

    //見た目用の部品が指定されていれば、そちらの姿を写す。
    USkeletalMeshComponent* mesh = enemy->GetMesh();
    if(!m_meshTag.IsNone())
    {
        for(auto* part : enemy->GetComponentsByTag(USkeletalMeshComponent::StaticClass(), m_meshTag))
        {
            auto* choice = Cast<USkeletalMeshComponent>(part);
            if(choice && choice->IsVisible() && choice->GetSkeletalMeshAsset()) { mesh = choice; break; }
        }
    }

    m_images.RemoveAll([](const auto& _image) { return !_image.IsValid(); });
    if(m_images.Num() >= FMath::Max(1, m_limit)) { return; }

    FActorSpawnParameters params;
    params.Owner = enemy;
    params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* image = GetWorld()->SpawnActor<AEnemyAfterimage>(m_imageClass, mesh->GetComponentTransform(), params);
    if(image) { image->Capture(mesh); m_images.Add(image); }
    m_lastPos = enemy->GetActorLocation();
    m_wait = 0.f;
}
