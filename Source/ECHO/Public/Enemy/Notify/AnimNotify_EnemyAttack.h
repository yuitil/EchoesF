//Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_EnemyAttack.generated.h"


//アニメーションの通知から従来の敵の攻撃を呼ぶ。
UCLASS()
class ECHO_API UAnimNotify_EnemyAttack : public UAnimNotify
{
	GENERATED_BODY()

public:
	//Notify
	virtual void Notify(USkeletalMeshComponent* _meshComp, UAnimSequenceBase* _animation) override;

	//通知の表示名を返す。
	FString GetNotifyName_Implementation() const;
};
