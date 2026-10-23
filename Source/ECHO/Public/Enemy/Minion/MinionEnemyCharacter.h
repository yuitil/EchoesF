#pragma once
#include "CoreMinimal.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "MinionEnemyCharacter.generated.h"
class UMinionFormation;
class UMinionParryResponse;
//
UCLASS()
class ECHO_API AMinionEnemyCharacter : public AEnemyCharacterBase
{
    GENERATED_BODY()
public:
    AMinionEnemyCharacter();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy") TObjectPtr<UMinionFormation> m_formation;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy") TObjectPtr<UMinionParryResponse> m_parry;
    void FinishParryReaction(UAnimMontage* montage,float duration);
    virtual void AttackDefended_Implementation(AActor* defender, EDefenseResult result) override;
    //
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Death", meta=(ClampMin="0.1"))
    float m_corpseTime = 2.f;
    //
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Hit")
    TObjectPtr<UAnimMontage> m_hitMontage;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Hit", meta=(ClampMin="0"))
    float m_hitCooldown = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Hit")
    bool m_interruptAttack = false;
protected:
    virtual bool IsActionAllowed(const FEnemyActionData& action) const override;
    virtual void OnAttackFinished() override;
    //
    virtual void OnActionEnded() override;
    virtual void HandleHealthDepleted() override;
    virtual void OnAcceptedHit(AActor* attacker) override;
private:
    double m_nextHit = 0.;
};
//
