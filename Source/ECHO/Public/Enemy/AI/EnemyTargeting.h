#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyTargeting.generated.h"
//
UCLASS(ClassGroup=(Enemy), meta=(BlueprintSpawnableComponent))
class ECHO_API UEnemyTargeting : public UActorComponent
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Enemy|Target")
    AActor* SelectTarget();
    //
    UFUNCTION(BlueprintCallable, Category="Enemy|Target")
    AActor* SelectMinionTarget();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Target")
    bool m_preferClones = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Target")
    bool m_keepPlayerDuel = true;
    //
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Target")
    bool m_targetClones = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Target", meta=(ClampMin="0"))
    float m_range = 2500.f;
    //
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Target", meta=(ClampMin="0"))
    float m_giveUpRange = 3500.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Target", meta=(ClampMin="0.2"))
    float m_pathCheck = 1.f;
    //
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Target", meta=(ClampMin="0", ClampMax="1"))
    float m_keepWeight = .75f;
    //
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Target")
    bool m_followTarget = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Target", meta=(ClampMin="0"))
    float m_chaseRange = 20000.f;
    //
private:
    bool IsCandidate(AActor* actor) const;
};
//
