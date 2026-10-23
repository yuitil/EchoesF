#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MinionFormation.generated.h"
class AEnemyCharacterBase;
//
UCLASS(ClassGroup=(Enemy), meta=(BlueprintSpawnableComponent))
class ECHO_API UMinionFormation : public UActorComponent
{
    GENERATED_BODY()
public:
    UMinionFormation();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Formation") bool m_enabled = true;
    //
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Formation") bool m_holdDuel = true;
    //
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Formation", meta=(ClampMin="1",ClampMax="2")) int32 m_attackers = 2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Formation", meta=(ClampMin="200")) float m_radius = 450.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Formation", meta=(ClampMin="0",ClampMax="45")) float m_orbitRate = 12.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Formation", meta=(ClampMin="0.2")) float m_secondDelay = 1.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Formation", meta=(ClampMin="0")) float m_restTime = 3.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Formation", meta=(ClampMin="1")) float m_turnTimeout = 8.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Formation", meta=(ClampMin="100")) float m_joinRange=1800.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Formation", meta=(ClampMin="30",ClampMax="150")) float m_gapAngle=60.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Formation", meta=(ClampMin="0",ClampMax="180")) float m_openAngle=70.f;
    UPROPERTY(BlueprintReadOnly, Category="Formation") bool m_hasTurn = false;
    UPROPERTY(BlueprintReadOnly, Category="Formation") FVector m_ringGoal;
    UFUNCTION(BlueprintPure, Category="Formation") bool HasTurn() const { return !m_enabled || m_hasTurn; }
    UFUNCTION(BlueprintCallable, Category="Formation") void ReleaseTurn();
    AEnemyCharacterBase* Enemy() const;
    void PrepareAttack() { SetOrbit(false); }
    double m_granted = 0.;
    double m_ready = 0.;
    //
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Formation|Timing", meta=(ClampMin="0")) float m_observeTime=2.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Formation|Timing", meta=(ClampMin="0.1")) float m_gapTime=1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Formation|Timing", meta=(ClampMin="0")) float m_groupRest=1.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Formation|Movement", meta=(ClampMin="50")) float m_orbitSpeed=180.f;
    double m_gapSince=-1.;
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type reason) override;
    virtual void TickComponent(float delta, ELevelTick type, FActorComponentTickFunction* fn) override;
private:
    void SetOrbit(bool enabled);
    bool m_orbit=false;
    bool m_oldOrient=false;
    bool m_oldDesired=false;
    bool m_oldYaw=false;
    float m_oldSpeed=0.f;
    TWeakObjectPtr<AActor> m_target;
    double m_nextMove = 0.;
};
//
