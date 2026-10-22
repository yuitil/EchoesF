#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlayerBridge.generated.h"

class AEnemyCharacterBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPlayerHealthEvent, float, health, float, maxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPlayerDownEvent);

UCLASS()
class ECHO_API APlayerBridge : public AActor
{
	GENERATED_BODY()
	
public:	
	APlayerBridge();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "EnableDemohealth"))
	bool m_bUseDemoHealth = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Max Health", ClampMin = "1"))
	float m_maxHealth = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Boss"))
	TObjectPtr<AEnemyCharacterBase> m_boss;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Health"))
	float m_health = 200.f;

	UPROPERTY(EditAnywhere, BlueprintAssignable, meta = (DisplayName = "OnHealthChanged"))
	FPlayerHealthEvent m_onHealthChanged;

	UPROPERTY(EditAnywhere, BlueprintAssignable, meta = (DisplayName = "Player Down"))
	FPlayerDownEvent m_playerDown;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type _reason) override;;

private:
	UFUNCTION(Category = "Enemy")
	void Damaged(AActor* _actor, float _damage, const class UDamageType* _damageType, AController* _instigator, AActor* _causer);

private:
	UPROPERTY(meta = (DisplayName = "ObservedPlayer"))
	TObjectPtr<AActor> m_observedPlayer;


};
