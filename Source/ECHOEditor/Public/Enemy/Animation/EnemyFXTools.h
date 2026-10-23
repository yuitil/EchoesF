#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EnemyFXTools.generated.h"
class UBlueprint;
class UNiagaraSystem;
class UMaterialInterface;
class UMaterial;
//
UCLASS()
class ECHOEDITOR_API UEnemyFXTools : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    //
    UFUNCTION(BlueprintCallable, Category="Enemy|Editor")
    static FString CheckExplosion(UBlueprint* blueprint, bool repair);
    UFUNCTION(BlueprintCallable, Category="Enemy|Editor")
    static bool UpdateGroundBindings(UBlueprint* bp);
    UFUNCTION(BlueprintCallable, Category="Enemy|Editor")
    static bool AmberEye(UMaterial* mat);
    UFUNCTION(BlueprintCallable, Category="Enemy|Editor")
    static bool ConfigureGroundRing(UNiagaraSystem* system, UMaterialInterface* material);
    UFUNCTION(BlueprintCallable, Category="Enemy|Editor")
    static bool SetupGroundFX(UBlueprint* blueprint, UNiagaraSystem* system);
    UFUNCTION(BlueprintCallable, Category="Enemy|Editor")
    static bool StoneSurface(UMaterial* material);
    UFUNCTION(BlueprintCallable, Category="Enemy|Editor")
    static bool AddWarningGlow(UMaterial* material);
    UFUNCTION(BlueprintCallable, Category="Enemy|Editor")
    static int32 SetSpriteMaterial(UNiagaraSystem* system, UMaterialInterface* material);
    UFUNCTION(BlueprintCallable, Category="Enemy|Editor")
    static bool SetupBombFX(UBlueprint* blueprint, UNiagaraSystem* trail, FVector scale);
};
//
