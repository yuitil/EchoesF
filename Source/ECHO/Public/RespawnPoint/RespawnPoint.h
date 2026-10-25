// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RespawnPoint.generated.h"

class AActionCharacter;
class UBoxComponent;
class UStaticMeshComponent;
class UNiagaraComponent;

UCLASS()
class ECHO_API ARespawnPoint : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ARespawnPoint();

	virtual void Tick(float DeltaTime) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	                    int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);


	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	                  int32 OtherBodyIndex);

	//HP加算量
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rotation")
	float m_HPAdditionAmount = 100.f;

	//プレイヤーが重なってるかどうか
	bool m_IsPlayerOverlapping = false;

	//重なったプレイヤー
	TWeakObjectPtr<AActionCharacter> m_OverlappingPlayer;

	UPROPERTY(VisibleAnywhere, Category = "Visual")
	TObjectPtr<UStaticMeshComponent> m_StaticMeshComponent;

	UPROPERTY(VisibleAnywhere, Category = "Collision")
	TObjectPtr<UBoxComponent> m_PlayerDitectionBox;
	
	// 回復オーラ
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect")
	TObjectPtr<UNiagaraComponent> m_HealAura;
};
