#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "EnemyHealthDisplay.generated.h"

//BPで選んだHP表示を敵へつなぎ、敵が消えたら片付ける。
UCLASS(ClassGroup=(Enemy), meta=(BlueprintSpawnableComponent))
class ECHO_API UEnemyHealthDisplay : public UWidgetComponent
{
    GENERATED_BODY()

public:
    //HP表示へ持ち主の敵を渡す。
    virtual void InitWidget() override;

protected:
    //開始時に表示場所を決める。
    virtual void BeginPlay() override;
    //敵が消える時に画面の表示も片付ける。
    virtual void EndPlay(const EEndPlayReason::Type _reason) override;

public:
    //ボスのHPを画面に固定して表示する。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|UI")
    bool m_bossHUD = false;

    //重なって表示しないように外す旧HP画面。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|UI")
    TSubclassOf<UUserWidget> m_oldHUD;
};
