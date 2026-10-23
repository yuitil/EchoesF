#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EnemyAnimBlueprintTools.generated.h"
class UAnimBlueprint;
class UBlendSpace;
class UBehaviorTree;
class UAnimInstance;
class UAnimSequence;
//
UCLASS()
class ECHOEDITOR_API UEnemyAnimBlueprintTools : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    //
    UFUNCTION(BlueprintCallable, Category="Enemy|Editor")
    static FString InspectServices(UBehaviorTree* tree);
    //
    UFUNCTION(BlueprintCallable, Category="Enemy|Editor", meta=(DevelopmentOnly))
    static bool SetMovementUpperPose(UAnimBlueprint* blueprint, UAnimSequence* pose, FName bone);
    //
    UFUNCTION(BlueprintCallable, Category="Enemy|Editor", meta=(DevelopmentOnly))
    static FString InspectMovement(UAnimInstance* instance);
    //
    UFUNCTION(BlueprintCallable, Category="Enemy|Editor", meta=(DevelopmentOnly))
    static FString InspectLocomotion(UAnimBlueprint* blueprint);
    UFUNCTION(BlueprintCallable, Category="Enemy|Editor", meta=(DevelopmentOnly))
    static bool ReplaceLocomotion(UAnimBlueprint* blueprint, UBlendSpace* blend);
    //
    UFUNCTION(BlueprintCallable, Category="Enemy|Editor", meta=(DevelopmentOnly))
    static bool WrapLocomotion(UAnimBlueprint* blueprint);
    //
    UFUNCTION(BlueprintCallable, Category="Enemy|Editor", meta=(DevelopmentOnly))
    static bool SetupLocomotion(UAnimBlueprint* blueprint, UBlendSpace* blend);
    UFUNCTION(BlueprintCallable, Category="Enemy|Editor", meta=(DevelopmentOnly))
    static int32 SetupMinionTree(UBehaviorTree* tree);
    //
    UFUNCTION(BlueprintCallable, Category="Enemy|Editor", meta=(DevelopmentOnly))
    static int32 SplitMinionService(UBehaviorTree* tree);
    //
};
//
