#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EnemyHealthWidget.generated.h"

class AEnemyCharacterBase;
class UProgressBar;
class UTextBlock;
class UWidgetAnimation;
class UImage;

//敵の体力とガードが崩れた状態をBPの画面へ渡す。
UCLASS(Abstract)
class ECHO_API UEnemyHealthWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    //表示する敵を受け取る。
    UFUNCTION(BlueprintCallable, Category="Enemy|UI")
    void SetEnemy(AEnemyCharacterBase* _enemy);

protected:
    //体力と表示時間を更新する。
    virtual void NativeTick(const FGeometry& _shape, float _delta) override;

public:
    //ダメージを受けた時のBP演出。
    UPROPERTY(Transient, BlueprintReadOnly, Category="Enemy|UI", meta=(BindWidgetAnimOptional))
    TObjectPtr<UWidgetAnimation> m_hitAnim;

    //ガードが崩れた時のBP演出。
    UPROPERTY(Transient, BlueprintReadOnly, Category="Enemy|UI", meta=(BindWidgetAnimOptional))
    TObjectPtr<UWidgetAnimation> m_breakAnim;

    //現在の体力を示すバー。
    UPROPERTY(BlueprintReadOnly, Category="Enemy|UI", meta=(BindWidgetOptional))
    TObjectPtr<UProgressBar> m_healthBar;

    //減った体力を遅れて追うバー。
    UPROPERTY(BlueprintReadOnly, Category="Enemy|UI", meta=(BindWidgetOptional))
    TObjectPtr<UProgressBar> m_trailBar;

    //ガード崩れを知らせる文字。
    UPROPERTY(BlueprintReadOnly, Category="Enemy|UI", meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> m_breakText;

    //今のフェーズを知らせる文字。
    UPROPERTY(BlueprintReadOnly, Category="Enemy|UI", meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> m_phaseText;

public:
    //ダメージを受けた敵だけ表示する。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|UI")
    bool m_hitOnly = false;

    //ダメージ後に表示する秒数。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|UI", meta=(ClampMin="0"))
    float m_showTime = 5.f;

    //残り色が減り始めるまでの秒数。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|UI", meta=(ClampMin="0"))
    float m_trailDelay = .3f;

    //表示と非表示を切り替える速さ。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|UI", meta=(ClampMin="0.01"))
    float m_fadeSpeed = 4.f;

    //残り色が体力に追いつく速さ。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|UI", meta=(ClampMin="0.01"))
    float m_trailSpeed = .7f;

    //フェーズ1の表示名。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|UI")
    FText m_phaseOne = NSLOCTEXT("EnemyUI", "PhaseOne", "第一幕");

    //フェーズ2の表示名。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|UI")
    FText m_phaseTwo = NSLOCTEXT("EnemyUI", "PhaseTwo", "第二幕");

    //立ち上がる間の表示名。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|UI")
    FText m_riseText = NSLOCTEXT("EnemyUI", "Rise", "再起");

public:
    //このWidgetを頭上の体勢ゲージとして使う。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|UI")
    bool m_postureMode = false;

    //体力が増えた時にバーが満ちる速さ。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|UI", meta=(ClampMin="0.01"))
    float m_fillSpeed = .7f;

    //フェーズ2でBPのオーラ演出を繰り返す。
    UPROPERTY(Transient, BlueprintReadOnly, Category="Enemy|UI", meta=(BindWidgetAnimOptional))
    TObjectPtr<UWidgetAnimation> m_phaseAura;

public:
    //フェーズ2のオーラを表示するBP内の画像名。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|UI")
    FName m_phaseGlowName = TEXT("AuraVeil");

    //BPの動くマテリアルで表示するフェーズ2のオーラ。
    UPROPERTY(Transient)
    TObjectPtr<UImage> m_phaseGlow;

private:
    //前回は立ち上がり中だったか。
    bool m_wasRising = false;

    //前回のフェーズ2表示。
    bool m_wasPhaseTwo = false;

    //画面で表示している割合。
    float m_shown = 1.f;

    //表示する敵。敵の削除を妨げない。
    TWeakObjectPtr<AEnemyCharacterBase> m_enemy;

    //残り色の割合。
    float m_trail = 1.f;

    //前回確認した体力。
    float m_lastHealth = 0.f;

    //表示を続ける残り秒数。
    float m_showLeft = 0.f;

    //残り色が減るまでの残り秒数。
    float m_trailLeft = 0.f;

    //前回ガードが崩れていたか。
    bool m_wasBroken = false;
};
