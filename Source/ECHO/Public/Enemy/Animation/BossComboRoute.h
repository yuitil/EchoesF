//Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "BossComboRoute.generated.h"

//次の攻撃へ進むか、構えに戻るかを決める。
UCLASS(meta = (DisplayName = "BossComboRoute"))
class ECHO_API UBossComboRoute : public UAnimNotify
{
	GENERATED_BODY()

public:
	//次の攻撃へ進むか、構えに戻るかを決める。
	UBossComboRoute();

	//アニメーションの指定位置で処理を呼ぶ。
	virtual void Notify(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim, const FAnimNotifyEventReference& _reference) override;

public:
	//今のコンボの区間。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo", meta = (DisplayName = "From Section"))
	FName m_from = TEXT("Attack1");

	//次につなぐ区間名。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo", meta = (DisplayName = "Next Section"))
	FName m_next = TEXT("Attack2");

	//コンボをやめた時に戻る区間名。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo", meta = (DisplayName = "Recovery Section"))
	FName m_recover = TEXT("Recover1");

	//次のコンボへ進む確率。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo", meta = (DisplayName = "Continue Chance", ClampMin = "0", ClampMax = "1"))
	float m_chance = 1.0f;

	//次のコンボへ進める相手との距離。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo", meta = (DisplayName = "Max Distance", ClampMin = "0"))
	float m_range = 250.0f;
};
