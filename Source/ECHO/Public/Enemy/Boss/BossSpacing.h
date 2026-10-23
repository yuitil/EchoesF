#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BossSpacing.generated.h"

//
UCLASS(ClassGroup=(Enemy), meta=(BlueprintSpawnableComponent))
class ECHO_API UBossSpacing : public UActorComponent
{
    GENERATED_BODY()
public:
    UBossSpacing();
    //
    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category="Boss|Spacing")
    bool IsTargetDown(AActor* target) const;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Spacing")
    bool m_onlyDown = false;
    //
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Spacing")
    bool m_watchIdle = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Spacing", meta=(ClampMin="0.2"))
    float m_idleDelay = 1.2f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Spacing", meta=(ClampMin="0"))
    float m_idleSpeed = 80.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Spacing", meta=(ClampMin="100"))
    float m_watchRange = 900.f;
    //
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Spacing")
    FName m_downTag;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Spacing")
    TArray<TObjectPtr<class UAnimMontage>> m_downMontages;
    //
    void Queue();
    void Stop();
    UFUNCTION(BlueprintPure, Category="Boss|Spacing")
    bool IsSpacing() const { return m_active; }
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Spacing")
    bool m_enabled = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Spacing", meta=(ClampMin="0.1"))
    float m_duration = 2.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Spacing", meta=(ClampMin="0"))
    float m_cooldown = 6.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Spacing", meta=(ClampMin="1"))
    float m_distance = 400.f;
    //
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Spacing")
    FName m_retreatAction;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Spacing", meta=(ClampMin="0.1",ClampMax="1"))
    float m_retreatRatio = .85f;
    //
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Spacing", meta=(ClampMin="1"))
    float m_speed = 200.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Spacing", meta=(ClampMin="0", ClampMax="60"))
    float m_orbitAngle = 25.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Recovery", meta=(ClampMin="0"))
    float m_healRate = 5.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Recovery", meta=(ClampMin="0", ClampMax="1"))
    float m_healLimit = .1f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss|Recovery", meta=(ClampMin="0"))
    float m_healDelay = 1.f;
    void Reset();
protected:
    virtual void TickComponent(float delta, ELevelTick type, FActorComponentTickFunction* tick) override;
    virtual void EndPlay(const EEndPlayReason::Type reason) override;
private:
    double m_idleSince = -1.;
    bool m_watching = false;
    bool m_active = false;
    bool m_moving = false;
    bool m_oldTurn = false;
    float m_oldSpeed = 0.f;
    float m_side = 1.f;
    float m_healed = 0.f;
    double m_start = 0.;
    double m_next = 0.;
    double m_repath = 0.;
};
//
