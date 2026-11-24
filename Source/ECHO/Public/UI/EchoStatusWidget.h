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
class UWidget;
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


	// --- 被弾したときの演出 ---
	//赤く光らせるバー（HPBarと同じ長さで上に重ねる）
	UPROPERTY(meta = (BindWidgetOptional))
	UProgressBar* HPFlashBar;

	//揺らすまとまり（HPゲージ全体が入っている Size Box）
	UPROPERTY(meta = (BindWidgetOptional))
	UWidget* HPRoot;

	//光る色
	UPROPERTY(EditAnywhere, Category = "Echo|HP|Damage")
	FLinearColor m_HPFlashColor = FLinearColor(1.f, 0.15f, 0.08f, 1.f);

	//光っている時間
	UPROPERTY(EditAnywhere, Category = "Echo|HP|Damage", meta = (ClampMin = "0"))
	float m_HPFlashTime = 0.35f;

	//揺れる時間
	UPROPERTY(EditAnywhere, Category = "Echo|HP|Damage", meta = (ClampMin = "0"))
	float m_ShakeTime = 0.25f;

	//揺れの大きさ（小さいダメージのとき／大きいダメージのとき）
	UPROPERTY(EditAnywhere, Category = "Echo|HP|Damage", meta = (ClampMin = "0"))
	float m_ShakeMin = 3.f;

	UPROPERTY(EditAnywhere, Category = "Echo|HP|Damage", meta = (ClampMin = "0"))
	float m_ShakeMax = 10.f;

	//最大HPのこの割合以上のダメージで、一番強く揺れる
	UPROPERTY(EditAnywhere, Category = "Echo|HP|Damage", meta = (ClampMin = "0.01", ClampMax = "1"))
	float m_ShakeFullDamage = 0.3f;


	// --- HPが少ないとき ---
	//画面の端を赤くする画像（画面いっぱいに置く）
	UPROPERTY(meta = (BindWidgetOptional))
	UImage* LowHPVignette;

	//最大HPのこの割合を下回ると赤くなり始める
	UPROPERTY(EditAnywhere, Category = "Echo|HP|LowHP", meta = (ClampMin = "0", ClampMax = "1"))
	float m_LowHPThreshold = 0.3f;

	//一番濃いとき（HPがほぼ0）の濃さ
	UPROPERTY(EditAnywhere, Category = "Echo|HP|LowHP", meta = (ClampMin = "0", ClampMax = "1"))
	float m_LowHPMaxOpacity = 0.6f;

	//脈打つ速さ（1秒あたりの回数）
	UPROPERTY(EditAnywhere, Category = "Echo|HP|LowHP", meta = (ClampMin = "0"))
	float m_LowHPPulseSpeed = 1.2f;

	// --- 回復したときのバーの増え方 ---
	//増える速さ（大きいほど速い。10 でおよそ0.3秒）
	UPROPERTY(EditAnywhere, Category = "Echo|HP|Heal", meta = (ClampMin = "0.1"))
	float m_HealFillSpeed = 10.f;

	//増えている間の光の色（HPFlashBar を使って光らせる）
	UPROPERTY(EditAnywhere, Category = "Echo|HP|Heal")
	FLinearColor m_HealFillColor = FLinearColor(0.7f, 1.f, 0.7f, 0.8f);

private:
	FDelegateHandle m_HPHandle;
	float m_HPRatio = 1.f;
	float m_TrailRatio = 1.f;
	float m_TrailWaitTime = 0.f;
	float m_FlashRemain = 0.f;      //光の残り時間
	float m_ShakeRemain = 0.f;      //揺れの残り時間
	float m_ShakeStrength = 0.f;    //今回の揺れの大きさ
	float m_VignetteOpacity = 0.f;  //画面端の赤の今の濃さ

	float m_DisplayHPRatio = 1.f;   //バーに表示しているHP（増えるときだけ、少し遅れて追いつく）
	float m_HealFillGlow = 0.f;     //増えている間に光らせる強さ

	void UpdateHPBar(float DeltaTime);

	void UpdateDamageFeedback(float DeltaTime);

	void HandleHPChanged(float Current, float Max);
	float ToHPBarPercent(float Ratio) const { return FMath::Lerp(m_HPBarStartRatio, m_HPBarEndRatio, Ratio); }

protected:
	//----------------
	//回復瓶
	//----------------
	//丸＋数字を入れた Overlay（使ったときにこれごと膨らむ）
	UPROPERTY(meta = (BindWidgetOptional))
	UWidget* HealRoot;

	//台座の丸
	UPROPERTY(meta = (BindWidgetOptional))
	UImage* HealCircle;

	//十字キー↓のアイコン
	UPROPERTY(meta = (BindWidgetOptional))
	UImage* HealArrow;

	//ポーションのアイコン
	UPROPERTY(meta = (BindWidgetOptional))
	UImage* HealIcon;

	//残り回数の数字
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* HealCountText;

	//使えるとき
	UPROPERTY(EditAnywhere, Category = "Echo|Heal")
	FLinearColor m_HealReadyColor = FLinearColor::White;

	//回復動作中（連続で使えない間）
	UPROPERTY(EditAnywhere, Category = "Echo|Heal")
	FLinearColor m_HealBusyColor = FLinearColor(0.35f, 0.35f, 0.35f, 1.f);

	//残り0
	UPROPERTY(EditAnywhere, Category = "Echo|Heal")
	FLinearColor m_HealEmptyColor = FLinearColor(0.3f, 0.3f, 0.3f, 0.45f);

	//使った瞬間に膨らむ大きさ
	UPROPERTY(EditAnywhere, Category = "Echo|Heal", meta = (ClampMin = "1"))
	float m_HealPopScale = 1.25f;

	// --- HPが少ないときの回復の主張 ---
	//回復アイコンの後ろに敷く光
	UPROPERTY(meta = (BindWidgetOptional))
	UImage* HealGlow;

	//寄せる緑（台座の丸・十字キー・数字）
	UPROPERTY(EditAnywhere, Category = "Echo|Heal|LowHP")
	FLinearColor m_HealAlertColor = FLinearColor(0.35f, 1.f, 0.45f, 1.f);

	//後ろの光の色
	UPROPERTY(EditAnywhere, Category = "Echo|Heal|LowHP")
	FLinearColor m_HealGlowColor = FLinearColor(0.2f, 1.f, 0.35f, 0.9f);

	//HPが少ないときの大きさ
	UPROPERTY(EditAnywhere, Category = "Echo|Heal|LowHP", meta = (ClampMin = "1"))
	float m_HealAlertScale = 1.3f;

	//脈打つときに、さらに膨らむ量（0.08 なら +8%）
	UPROPERTY(EditAnywhere, Category = "Echo|Heal|LowHP", meta = (ClampMin = "0"))
	float m_HealAlertPulseScale = 0.08f;

	//後ろの光の大きさ（回復アイコンに対する倍率）
	UPROPERTY(EditAnywhere, Category = "Echo|Heal|LowHP", meta = (ClampMin = "0.1"))
	float m_HealGlowScale = 1.6f;

private:
	FDelegateHandle m_HealCountHandle;
	int32 m_HealCount = -1;      //前回の回数（減ったかどうかを見る）
	float m_HealScale = 1.f;
	float m_HealAlert = 0.f;     //主張の強さ（0：ふだん → 1：HPが少ない）

	void HandleHealCountChanged(int32 Count, int32 Max);
	void UpdateHeal(float DeltaTime);


	//----------------
	//エネルギーゲージの演出
	//----------------
protected:
	//膨らませる・脈打たせるまとまり（エネルギーゲージ一式が入っているウィジェット）
	UPROPERTY(meta = (BindWidgetOptional))
	UWidget* EnergyRoot;

	//たまった区間に重ねて光らせる画像（EnergyBar と同じ Overlay に入れる）
	UPROPERTY(meta = (BindWidgetOptional))
	UImage* EnergyStockFlash;

	// --- ゲージの色（WBP で設定してある色に掛け合わせる。白(1,1,1)なら今のまま） ---
	//ふだんの色合い（1より小さい値で暗く・濃くなる）
	UPROPERTY(EditAnywhere, Category = "Echo|Energy|Color")
	FLinearColor m_EnergyTint = FLinearColor(0.6f, 0.8f, 1.f, 1.f);

	//最大のときの色合い（1より大きい値で明るくなる）
	UPROPERTY(EditAnywhere, Category = "Echo|Energy|Color")
	FLinearColor m_EnergyMaxTint = FLinearColor(1.3f, 1.5f, 2.f, 1.f);

	// --- 1本たまったとき ---
	//光の色
	UPROPERTY(EditAnywhere, Category = "Echo|Energy|Stock")
	FLinearColor m_StockFlashColor = FLinearColor(0.7f, 0.95f, 1.f, 0.85f);

	//光っている時間（秒）
	UPROPERTY(EditAnywhere, Category = "Echo|Energy|Stock", meta = (ClampMin = "0.01"))
	float m_StockFlashTime = 0.45f;

	//光る区間が膨らむ大きさ
	UPROPERTY(EditAnywhere, Category = "Echo|Energy|Stock", meta = (ClampMin = "1"))
	float m_StockFlashScale = 1.35f;

	//ゲージ全体がポンッと膨らむ大きさ
	UPROPERTY(EditAnywhere, Category = "Echo|Energy|Stock", meta = (ClampMin = "1"))
	float m_StockPopScale = 1.08f;

	// --- 最大のとき（ドクンドクン） ---
	//鼓動1セットの間隔（秒）
	UPROPERTY(EditAnywhere, Category = "Echo|Energy|Max", meta = (ClampMin = "0.2"))
	float m_MaxBeatInterval = 0.9f;

	//1回目の「ドクン」から2回目までの間（秒）
	UPROPERTY(EditAnywhere, Category = "Echo|Energy|Max", meta = (ClampMin = "0"))
	float m_MaxBeatGap = 0.6f;

	//1回の「ドクン」の長さ（秒）
	UPROPERTY(EditAnywhere, Category = "Echo|Energy|Max", meta = (ClampMin = "0.02"))
	float m_MaxBeatLength = 1.5f;

	//鼓動で膨らむ量（0.1 なら 1.1倍）
	UPROPERTY(EditAnywhere, Category = "Echo|Energy|Max", meta = (ClampMin = "0"))
	float m_MaxBeatScale = 0.1f;

	//最大のときの光の濃さ（ふだん／鼓動の瞬間）
	UPROPERTY(EditAnywhere, Category = "Echo|Energy|Max", meta = (ClampMin = "0", ClampMax = "1"))
	float m_MaxGlowBase = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Echo|Energy|Max", meta = (ClampMin = "0", ClampMax = "1"))
	float m_MaxGlowPeak = 0.6f;

private:
	FLinearColor m_EnergyBaseColor = FLinearColor::White;  //WBP で設定してあるゲージの色
	int32 m_LastStock = -1;          //前回の本数（増えたかどうかを見る）
	int32 m_MaxStockCache = 0;       //最大本数
	int32 m_FlashFromStock = 0;      //光らせる区間（何本目から）
	int32 m_FlashToStock = 0;        //光らせる区間（何本目まで）
	float m_StockFlashRemain = 0.f;  //光の残り時間
	float m_EnergyPopScale = 1.f;    //ポンッと膨らんだ大きさ
	bool  m_bEnergyMax = false;      //最大かどうか
	float m_BeatTime = 0.f;          //鼓動の経過時間
	float m_EnergyTargetRatio = 0.f;     //実際のエネルギー（ゲージ全体に対する割合）
	float m_EnergyDisplayRatio = -1.f;   //ゲージに表示している量（減るときだけ遅れて追いつく）
	float m_EnergyTotalMax = 0.f;        //ゲージ全体のエネルギー量
	float m_ChargeReservedRatio = 0.f;   //ため中に減らした分（ゲージ全体に対する割合）
	int32 m_LastChargeLevel = 0;         //前回のため段階（上がった瞬間を見る）
	float m_ChargeLevelFlash = 0.f;      //段階が上がった瞬間の光（1 → 0）

	void UpdateEnergyFeedback(float DeltaTime);
	float CalcBeat() const;
	void PlaceStockFlash(float FromRatio, float ToRatio);

	void UpdateEnergyBar(float DeltaTime);
};
