#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyHitEffect.generated.h"

class UNiagaraSystem;
//
UCLASS(ClassGroup=(Enemy), meta=(BlueprintSpawnableComponent))
class ECHO_API UEnemyHitEffect : public UActorComponent
{
    GENERATED_BODY()
public:
    UEnemyHitEffect();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Hit FX")
    TObjectPtr<UNiagaraSystem> m_effect;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Hit FX")
    FVector m_scale = FVector(.2f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Hit FX")
    float m_offset = 2.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Hit FX")
    FName m_base = TEXT("BladeBase");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Hit FX")
    FName m_tip = TEXT("BladeTip");
    UFUNCTION(BlueprintPure, Category="Enemy|Hit FX")
    bool EstimateContact(AActor* attacker, FVector& point, FVector& normal) const;
    void PlayHit(AActor* attacker, const FHitResult* hit = nullptr);
    UFUNCTION(NetMulticast, Unreliable)
    void ShowHit(FVector point, FVector normal);
};
//
