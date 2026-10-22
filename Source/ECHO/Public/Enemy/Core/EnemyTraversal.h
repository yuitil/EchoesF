#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyTraversal.generated.h"

UCLASS(ClassGroup=(Enemy), meta=(BlueprintSpawnableComponent))
class ECHO_API UEnemyTraversal : public UActorComponent
{
 GENERATED_BODY()
public:
 UEnemyTraversal();
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal") bool m_enabled = true;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal", meta=(ClampMin="0", DisplayName="Max Climb Height")) float m_up = 140.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal", meta=(ClampMin="0", DisplayName="Max Drop Height")) float m_down = 200.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal", meta=(ClampMin="50", DisplayName="Search Distance")) float m_range = 360.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal", meta=(ClampMin="10", DisplayName="Obstacle Distance")) float m_probe = 130.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal", meta=(ClampMin="10", DisplayName="Arc Clearance")) float m_clearance = 65.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal", meta=(ClampMin="100", DisplayName="Max Horizontal Speed")) float m_speed = 700.f;
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traversal", meta=(ClampMin="0.1", DisplayName="Retry Interval")) float m_interval = .4f;
 UFUNCTION(BlueprintPure, Category="Traversal") bool IsTraversing() const { return m_running; }
 UFUNCTION(BlueprintCallable, Category="Traversal") bool TryTraverse(AActor* target);
protected:
 virtual void TickComponent(float delta, ELevelTick type, FActorComponentTickFunction* function) override;
private:
 bool m_running = false;
 double m_next = 0.;
 double m_started = 0.;
};

