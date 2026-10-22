// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "BossComboRoute.generated.h"

UCLASS(meta = (DisplayName = "BossComboRoute"))
class ECHO_API UBossComboRoute : public UAnimNotify
{
	GENERATED_BODY()

public:
	UBossComboRoute();
	virtual void Notify(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim, const FAnimNotifyEventReference& _reference) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo", meta = (DisplayName = "From Section"))
	FName m_from = TEXT("Attack1");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo", meta = (DisplayName = "Next Section"))
	FName m_next = TEXT("Attack2");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo", meta = (DisplayName = "Recovery Section"))
	FName m_recover = TEXT("Recover1");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo", meta = (DisplayName = "Continue Chance", ClampMin = "0", ClampMax = "1"))
	float m_chance = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo", meta = (DisplayName = "Max Distance", ClampMin = "0"))
	float m_range = 250.0f;
};
