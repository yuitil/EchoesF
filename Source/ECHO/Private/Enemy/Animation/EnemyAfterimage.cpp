#include "Enemy/Animation/EnemyAfterimage.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

//当たり判定のない表示用モデルを用意する。
AEnemyAfterimage::AEnemyAfterimage()
{
    PrimaryActorTick.bCanEverTick = true;
    m_pose = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("Pose"));
    SetRootComponent(m_pose);
    m_pose->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    m_pose->SetGenerateOverlapEvents(false);
    m_pose->SetCastShadow(false);
    m_pose->SetCanEverAffectNavigation(false);
}

//元の敵の姿勢と位置を写す。
void AEnemyAfterimage::Capture(USkeletalMeshComponent* _source)
{
    if(!_source || !_source->GetSkeletalMeshAsset() || !m_material) { Destroy(); return; }

    m_pose->SetSkinnedAssetAndUpdate(_source->GetSkeletalMeshAsset());
    m_pose->SetWorldTransform(_source->GetComponentTransform());
    m_pose->CopyPoseFromSkeletalComponent(_source);
    m_fade = UMaterialInstanceDynamic::Create(m_material, this);
    m_fade->SetScalarParameterValue(m_fadeName, m_opacity);
    for(int32 i = 0; i < m_pose->GetNumMaterials(); ++i) { m_pose->SetMaterial(i, m_fade); }
    m_age = 0.f;
    SetLifeSpan(FMath::Max(.05f, m_life));
}

//時間に合わせて薄くする。
void AEnemyAfterimage::Tick(float _delta)
{
    Super::Tick(_delta);
    m_age += _delta;
    if(!m_fade) { return; }

    const float hold = FMath::Clamp(m_hold, 0.f, FMath::Max(0.f, m_life - .05f));
    const float fade = 1.f - FMath::Clamp((m_age - hold) / FMath::Max(.05f, m_life - hold), 0.f, 1.f);
    m_fade->SetScalarParameterValue(m_fadeName, m_opacity * fade);
}
