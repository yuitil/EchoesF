//EchoStatusWidget.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Ghost/Data/GhostTypes.h"
#include "EchoStatusWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UHorizontalBox;
class UImage;
class UTexture2D;
class AActionCharacter;

UCLASS(Abstract)
class ECHO_API UEchoStatusWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// --- WBP側に同じ名前のウィジェットを置くと自動で紐づく ---
	UPROPERTY(meta = (BindWidget))
	UProgressBar* EnergyBar;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* StockText;

	UPROPERTY(meta = (BindWidget))
	UHorizontalBox* CapsuleBox;

	// --- カプセルの見た目 ---
	UPROPERTY(EditAnywhere, Category = "Echo|Capsule")
	UTexture2D* CapsuleEmptyTexture;    //空き枠（グレー）

	UPROPERTY(EditAnywhere, Category = "Echo|Capsule")
	UTexture2D* CapsuleActiveTexture;   //召喚中（青）

	UPROPERTY(EditAnywhere, Category = "Echo|Capsule")
	UTexture2D* CapsuleEnemyTexture;    //敵化した分身（赤）※敵化をオンにしたとき用

	//消える直前の点滅間隔（秒）
	UPROPERTY(EditAnywhere, Category = "Echo|Capsule")
	float m_BlinkInterval = 0.25f;

	UPROPERTY(EditAnywhere, Category = "Echo|Capsule")
	FVector2D m_CapsuleSize = FVector2D(40.f, 74.f);

	UPROPERTY(EditAnywhere, Category = "Echo|Capsule")
	float m_CapsuleSpacing = 6.f;

	// --- ゲージ画像の中で、中身が始まる位置と終わる位置（横幅に対する割合） ---
	UPROPERTY(EditAnywhere, Category = "Echo|Gauge", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float m_BarStartRatio = 0.200f;

	UPROPERTY(EditAnywhere, Category = "Echo|Gauge", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float m_BarEndRatio = 0.970f;

private:
	TWeakObjectPtr<AActionCharacter> Player;
	FDelegateHandle m_EnergyHandle;
	FDelegateHandle m_GhostsHandle;

	UPROPERTY()
	TArray<UImage*> CapsuleImages;

	bool TryBindPlayer();
	void BuildCapsules(int32 Count);
	void HandleEnergyChanged(float Current, float Max);
	void HandleGhostsChanged();

	TArray<EGhostLifecycleState> m_CachedStates;   //各カプセルに対応する分身の状態
	bool m_bBlinkOn = true;                        //点滅の今の状態

	void ApplyCapsuleTextures();
	UTexture2D* GetCapsuleTexture(int32 Index) const;

	//----------------
	//HPバー
	//----------------
protected:
	UPROPERTY(meta = (BindWidgetOptional))
	UProgressBar* HPBar;

	UPROPERTY(meta = (BindWidgetOptional))
	UProgressBar* HPTrailBar;      //減った分を遅れて追いかける帯

	UPROPERTY(EditAnywhere, Category = "Echo|HP", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float m_HPBarStartRatio = 0.283f;

	UPROPERTY(EditAnywhere, Category = "Echo|HP", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float m_HPBarEndRatio = 0.931f;

	//被弾してから帯が追いかけ始めるまでの時間
	UPROPERTY(EditAnywhere, Category = "Echo|HP")
	float m_TrailDelay = 0.4f;

	//帯が減る速さ（1秒あたりのHP割合）
	UPROPERTY(EditAnywhere, Category = "Echo|HP")
	float m_TrailSpeed = 0.8f;

private:
	FDelegateHandle m_HPHandle;
	float m_HPRatio = 1.f;
	float m_TrailRatio = 1.f;
	float m_TrailWaitTime = 0.f;

	void HandleHPChanged(float Current, float Max);
	float ToHPBarPercent(float Ratio) const { return FMath::Lerp(m_HPBarStartRatio, m_HPBarEndRatio, Ratio); }
};
