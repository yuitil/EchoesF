#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EnemyAnimationTools.generated.h"
class UBlendSpace;
class UAnimSequence;
class UAnimMontage;
class USkeletalMesh;
//
UCLASS()
class ECHOEDITOR_API UEnemyAnimationTools : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Enemy|Editor", meta=(DevelopmentOnly))
    static bool RebuildBlendSpace(UBlendSpace* asset);
    UFUNCTION(BlueprintCallable, Category="Enemy|Editor", meta=(DevelopmentOnly))
    static float SampleWeight(UBlendSpace* asset, FVector input);
    UFUNCTION(BlueprintCallable, Category="Enemy|Editor", meta=(DevelopmentOnly))
    static bool AddSample(UBlendSpace* asset, UAnimSequence* clip, FVector point);
    UFUNCTION(BlueprintCallable, Category="Enemy|Editor", meta=(DevelopmentOnly))
    static UAnimSequence* GetSingleClip(UAnimMontage* montage);
    UFUNCTION(BlueprintCallable, Category="Enemy|Editor", meta=(DevelopmentOnly))
    static bool SetSingleClip(UAnimMontage* montage, UAnimSequence* clip);
    UFUNCTION(BlueprintCallable, Category="Enemy|Editor", meta=(DevelopmentOnly))
    static float GetBoneHeight(USkeletalMesh* mesh, FName bone);
};
//
