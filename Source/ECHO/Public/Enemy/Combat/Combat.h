#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat.generated.h"

class UMeshComponent;

UCLASS( ClassGroup=(Enemy), meta=(BlueprintSpawnableComponent))
class ECHO_API UCombat : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombat();

	UFUNCTION(BlueprintCallable, Category = "Enemy | Combat")
	bool SetWeaponMesh(UMeshComponent* _mesh, FName _base = TEXT("BladeBase"), FName _tip = TEXT("BladeTip"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "WeaponBaseSocket"))
	FName m_baseSocket = TEXT("hand_r");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "WeaponTipSocket"))
	FName m_tipSocket = TEXT("SwordTip");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Radius", ClampMin = "1"))
	float m_radius = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Debug"))
	bool m_bDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Invulnerable"))
	bool m_bInvulnerable = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Guarding"))
	bool m_bGuarding = false;

	void EndLift();
	virtual void EndPlay(const EEndPlayReason::Type _reason) override;
	
	void BeginHitWindow(float _damageScale = 1.0f);
	void EndHitWindow();
	bool IsHitWindowOpen() const { return m_bHitWindow; }
	void ResetWindows();
	virtual void TickComponent(float _deltaTime, ELevelTick _type, FActorComponentTickFunction* _function) override;

private:
	void BeginLift(class ACharacter* _target, const struct FEnemyActionData& _action);
	TWeakObjectPtr<class ACharacter> m_liftTarget;
	float m_targetGravity = 1.f;
	float m_selfGravity = 1.f;
	float m_liftGravity = 1.f;
	double m_liftEnd = 0.0;

	UPROPERTY(Transient)
	TObjectPtr<UMeshComponent> m_weapon;

	bool m_useWeapon = false;

	float m_hitScale = 1.0f;
	bool m_bHitWindow = false;
	bool m_hasPreviousHit = false;
	FVector m_previousTip = FVector::ZeroVector;
	FVector m_previousBase = FVector::ZeroVector;
	TWeakObjectPtr<AActor> m_dodgeTarget;
	bool m_wasDodging = false;
	double m_dodgeStart = -100.;
	FVector m_dodgePos = FVector::ZeroVector;
	FVector m_lastTargetPos = FVector::ZeroVector;
	float m_dodgeRadius = 0.f;
	float m_dodgeHalf = 0.f;
	
	TSet<TWeakObjectPtr<AActor>> m_hitActors;
};
