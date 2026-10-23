#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyBomb.generated.h"
class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UNiagaraSystem;
class AEnemyCharacterBase;
//
UCLASS()
class ECHO_API AEnemyBomb : public AActor
{
    GENERATED_BODY()
public:
    AEnemyBomb();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USphereComponent> m_collision;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> m_mesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UProjectileMovementComponent> m_move;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bomb") float m_fuse = .9f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bomb") float m_radius = 180.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bomb") TObjectPtr<UNiagaraSystem> m_explosion;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bomb") FVector m_fxScale = FVector(.5f);
    UFUNCTION(BlueprintCallable, Category="Bomb")
    void Launch(AEnemyCharacterBase* source, AActor* target, FVector velocity, float damage, bool unblockable);
    UFUNCTION(BlueprintImplementableEvent, Category="Bomb") void Armed();
    //
    UPROPERTY(BlueprintReadOnly, Category="Bomb") FVector m_groundPoint;
    UPROPERTY(BlueprintReadOnly, Category="Bomb") FVector m_groundNormal = FVector::UpVector;
    //
    //
    UFUNCTION(BlueprintImplementableEvent, Category="Bomb") void BurstAt(FVector point);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bomb", meta=(ClampMin="0.1",ClampMax="1")) float m_floorNormal = .55f;
    UPROPERTY(BlueprintReadOnly, Category="Bomb") bool m_armed = false;
    //
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type reason) override;
private:
    UFUNCTION() void Landed(const FHitResult& hit, const FVector& velocity);
    UFUNCTION(NetMulticast, Unreliable) void ShowExplosion(FVector point);
    void Explode();
    TWeakObjectPtr<AEnemyCharacterBase> m_source;
    TWeakObjectPtr<AActor> m_target;
    FTimerHandle m_timer;
    float m_damage = 0.f;
    bool m_unblockable = false;
};
//
