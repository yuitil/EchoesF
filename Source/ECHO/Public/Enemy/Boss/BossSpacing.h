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
