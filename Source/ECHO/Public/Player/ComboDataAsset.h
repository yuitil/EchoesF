//ComboDataAsset.h
//コンボの設定データ。BPから切り離して保存することで、BPが壊れても設定が残る

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Player/CombatComponent.h"   //FComboStepData
#include "ComboDataAsset.generated.h"

UCLASS(BlueprintType)
class ECHO_API UComboDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	//各段の設定（モーション、ダメージ、踏み込み、エネルギーなど）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
	TArray<FComboStepData> Steps;
};