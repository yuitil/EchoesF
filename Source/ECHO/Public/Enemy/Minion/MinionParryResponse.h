#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MinionParryResponse.generated.h"
class UAnimMontage;
//
UCLASS(ClassGroup=(Enemy),meta=(BlueprintSpawnableComponent))
class ECHO_API UMinionParryResponse : public UActorComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Parry",meta=(ClampMin="0.01",ClampMax="0.5")) float m_freezeTime=.18f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Parry",meta=(ClampMin="0.1")) float m_recovery=1.25f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Parry") TObjectPtr<UAnimMontage> m_reaction;
    UFUNCTION(BlueprintCallable,Category="Parry") bool TryParry(AActor* defender,float inputAge);
    bool Begin(AActor* defender);
protected:
    virtual void EndPlay(const EEndPlayReason::Type reason) override;
private:
    void Finish();
    FTimerHandle m_timer;
    TWeakObjectPtr<UAnimMontage> m_paused;
};
//
