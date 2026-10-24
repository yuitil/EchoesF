#include "Enemy/UI/EnemyHealthDisplay.h"
#include "Enemy/UI/EnemyHealthWidget.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

//BPで選んだHP画面に敵を登録する。
void UEnemyHealthDisplay::InitWidget()
{
    //BPで選んだHP画面に敵を登録する。
    Super::InitWidget();
    if(auto* widget = Cast<UEnemyHealthWidget>(GetUserWidgetObject()))
    {
        widget->SetEnemy(Cast<AEnemyCharacterBase>(GetOwner()));
    }
}

//ボスのHP画面を表示する。
void UEnemyHealthDisplay::BeginPlay()
{
    //ボスのHP画面を表示する。
    Super::BeginPlay();
    InitWidget();
    if(m_bossHUD && GetUserWidgetObject() && GetNetMode() != NM_DedicatedServer)
    {
        //以前のHP画面があれば重複を防ぐ。
        if(m_oldHUD)
        {
            //画面から外す旧HP表示。
            TArray<UUserWidget*> oldWidgets;
            UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, oldWidgets, m_oldHUD, false);
            for(auto* oldWidget : oldWidgets) { oldWidget->RemoveFromParent(); }
        }
        //頭上用の表示を隠して画面へ固定する。
        SetVisibility(false);
        GetUserWidgetObject()->AddToViewport();
    }
}

//敵が消えたらHP画面を残さない。
void UEnemyHealthDisplay::EndPlay(const EEndPlayReason::Type _reason)
{
    if(m_bossHUD)
    {
        if(auto* widget = GetUserWidgetObject()) { widget->RemoveFromParent(); }
    }
    //敵が消えたらHP画面を残さない。
    Super::EndPlay(_reason);
}
