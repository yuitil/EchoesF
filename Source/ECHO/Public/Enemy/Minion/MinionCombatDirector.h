#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MinionCombatDirector.generated.h"
class UMinionFormation;
//
UCLASS()
class ECHO_API UMinionCombatDirector : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    void Register(UMinionFormation* member);
    void Unregister(UMinionFormation* member);
    void Update();
    //
    AActor* GetPlayerGuard(AActor* player);
    void ClearPlayerGuard(UMinionFormation* member);
    //
    void PauseTarget(AActor* target,float duration);
private:
    TWeakObjectPtr<UMinionFormation> m_playerGuard;
    double m_guardCheck = 0.;
    TArray<TWeakObjectPtr<UMinionFormation>> m_members;
    double m_nextUpdate = 0.;
    TMap<TWeakObjectPtr<AActor>,double> m_pauses;
    //
    TMap<TWeakObjectPtr<AActor>,TWeakObjectPtr<UMinionFormation>> m_duelists;
    //
};
//
