#include "Enemy/Animation/EnemyAnimationTools.h"
#include "Animation/BlendSpace.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
//
bool UEnemyAnimationTools::RebuildBlendSpace(UBlendSpace* asset)
{
#if WITH_EDITOR
    if (!asset) return false;
    asset->Modify();
    asset->ValidateSampleData();
    asset->ResampleData();
    asset->MarkPackageDirty();
    return true;
#else
    return false;
#endif
}
//

//
float UEnemyAnimationTools::SampleWeight(UBlendSpace* asset, FVector input)
{
    if (!asset) return 0.f;
    TArray<FBlendSampleData> samples;
    int32 cache = INDEX_NONE;
    asset->GetSamplesFromBlendInput(input, samples, cache, true);
    float weight = 0.f;
    for (const auto& sample : samples) weight += sample.TotalWeight;
    return weight;
}
//

//
bool UEnemyAnimationTools::AddSample(UBlendSpace* asset, UAnimSequence* clip, FVector point)
{
#if WITH_EDITOR
    if (!asset || !clip) return false;
    asset->Modify();
    const int32 index = asset->AddSample(clip,point);
    asset->ValidateSampleData();
    asset->ResampleData();
    asset->MarkPackageDirty();
    return index != INDEX_NONE;
#else
    return false;
#endif
}
//

//
UAnimSequence* UEnemyAnimationTools::GetSingleClip(UAnimMontage* montage)
{
    if (!montage || montage->SlotAnimTracks.Num()!=1 || montage->SlotAnimTracks[0].AnimTrack.AnimSegments.Num()!=1) return nullptr;
    return Cast<UAnimSequence>(montage->SlotAnimTracks[0].AnimTrack.AnimSegments[0].GetAnimReference());
}
bool UEnemyAnimationTools::SetSingleClip(UAnimMontage* montage, UAnimSequence* clip)
{
#if WITH_EDITOR
    if (!GetSingleClip(montage) || !clip || montage->GetSkeleton()!=clip->GetSkeleton()) return false;
    montage->Modify();
    montage->SlotAnimTracks[0].AnimTrack.AnimSegments[0].SetAnimReference(clip);
    montage->MarkPackageDirty();
    return true;
#else
    return false;
#endif
}
//

//
float UEnemyAnimationTools::GetBoneHeight(USkeletalMesh* mesh, FName bone)
{
    if (!mesh) return 0.f;
    const auto& skeleton = mesh->GetRefSkeleton();
    int32 index = skeleton.FindBoneIndex(bone);
    FTransform pose = FTransform::Identity;
    while (index != INDEX_NONE)
    {
        pose *= skeleton.GetRefBonePose()[index];
        index = skeleton.GetParentIndex(index);
    }
    return pose.GetLocation().Z;
}
//
