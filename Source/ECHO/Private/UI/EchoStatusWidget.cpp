//EchoStatusWidget.cpp

#include "UI/EchoStatusWidget.h"
#include "Player/ActionCharacter.h"
#include "Player/SkillComponent.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Texture2D.h"

void UEchoStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();

	//光る画像は最初は透明にして、クリックなどの判定も持たせない
	if (EnergyStockFlash)
	{
		EnergyStockFlash->SetVisibility(ESlateVisibility::HitTestInvisible);
		EnergyStockFlash->SetColorAndOpacity(FLinearColor::Transparent);
	}

	//WBP で設定してあるゲージの色を、色合いの基準として覚えておく
	if (EnergyBar)
	{
		m_EnergyBaseColor = EnergyBar->GetFillColorAndOpacity();
	}

	//回復の後ろの光も、最初は透明＆クリック判定なし
	if (HealGlow)
	{
		HealGlow->SetVisibility(ESlateVisibility::HitTestInvisible);
		HealGlow->SetColorAndOpacity(FLinearColor::Transparent);
	}

	TryBindPlayer();
}

void UEchoStatusWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	//生成時にプレイヤーがまだいなかった場合だけ、見つかるまで再試行する
	if (!Player.IsValid())
	{
		TryBindPlayer();
		return;
	}

	//回復瓶：使えない間は色を変える／使った瞬間に少し膨らむ
	UpdateHeal(InDeltaTime);

	//被弾したときの赤い光と揺れ
	UpdateDamageFeedback(InDeltaTime);

	//エネルギーゲージ：使ったときは滑らかに減らす
	UpdateEnergyBar(InDeltaTime);

	//エネルギー：1本たまったときの光と、最大のときの鼓動
	UpdateEnergyFeedback(InDeltaTime);

	//HPバー：回復したときは滑らかに増やす
	UpdateHPBar(InDeltaTime);

	//HPバー、減った分の帯を、少し遅れて追いかけさせる
	if (m_TrailRatio > m_HPRatio)
	{
		if (m_TrailWaitTime > 0.f)
		{
			m_TrailWaitTime -= InDeltaTime;
		}
		else
		{
			m_TrailRatio = FMath::Max(m_HPRatio, m_TrailRatio - m_TrailSpeed * InDeltaTime);
		}
		if (HPTrailBar) HPTrailBar->SetPercent(ToHPBarPercent(m_TrailRatio));
	}

	//点滅：切り替わる瞬間だけテクスチャを更新する（毎フレーム更新しない）
	if (m_BlinkInterval > 0.f && GetWorld())
	{
		const float Time = GetWorld()->GetTimeSeconds();
		const bool bOn = FMath::Fmod(Time, m_BlinkInterval * 2.f) < m_BlinkInterval;
		if (bOn != m_bBlinkOn)
		{
			m_bBlinkOn = bOn;
			if (m_CachedStates.Contains(EGhostLifecycleState::Warning))
			{
				ApplyCapsuleTextures();
			}
		}
	}
}

void UEchoStatusWidget::NativeDestruct()
{
	if (AActionCharacter* P = Player.Get())
	{
		P->OnEnergyChanged.Remove(m_EnergyHandle);
		P->OnGhostsChanged.Remove(m_GhostsHandle);
		P->OnHPChanged.Remove(m_HPHandle);
		P->OnHealCountChanged.Remove(m_HealCountHandle);
	}
	Super::NativeDestruct();
}

bool UEchoStatusWidget::TryBindPlayer()
{
	AActionCharacter* P = Cast<AActionCharacter>(GetOwningPlayerPawn());
	if (!P) return false;

	Player = P;
	m_EnergyHandle = P->OnEnergyChanged.AddUObject(this, &UEchoStatusWidget::HandleEnergyChanged);
	m_GhostsHandle = P->OnGhostsChanged.AddUObject(this, &UEchoStatusWidget::HandleGhostsChanged);
	m_HPHandle = P->OnHPChanged.AddUObject(this, &UEchoStatusWidget::HandleHPChanged);
	m_HealCountHandle = P->OnHealCountChanged.AddUObject(this, &UEchoStatusWidget::HandleHealCountChanged);

	BuildCapsules(P->GetMaxGhostCount());

	//初期表示
	HandleEnergyChanged(P->GetCurrentEnergy(), P->GetMaxEnergy());
	HandleGhostsChanged();
	HandleHPChanged(P->GetCurrentHP(), P->GetMaxHP());
	HandleHealCountChanged(P->GetHealCount(), P->GetMaxHealCount());
	return true;
}

void UEchoStatusWidget::BuildCapsules(int32 Count)
{
	if (!CapsuleBox) return;

	CapsuleBox->ClearChildren();
	CapsuleImages.Reset();

	//最大召喚数ぶんカプセルを並べる
	for (int32 i = 0; i < Count; ++i)
	{
		UImage* Img = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Img->SetDesiredSizeOverride(m_CapsuleSize);

		if (UHorizontalBoxSlot* BoxSlot = CapsuleBox->AddChildToHorizontalBox(Img))
		{
			BoxSlot->SetPadding(FMargin(0.f, 0.f, m_CapsuleSpacing, 0.f));
			BoxSlot->SetVerticalAlignment(VAlign_Center);
		}
		CapsuleImages.Add(Img);
	}
}

void UEchoStatusWidget::HandleEnergyChanged(float Current, float Max)
{
	if (!Player.IsValid()) return;

	const float PerStock = Player->GetEnergyPerStock();
	const int32 MaxStock = Player->GetMaxEnergyStock();
	const int32 Stock = Player->GetEnergyStock();

	//今の1本の溜まり具合（最大ストック時は満タンのまま止める）
	float Ratio = 1.f;
	if (Stock < MaxStock && PerStock > 0.f)
	{
		Ratio = FMath::Clamp((Current - Stock * PerStock) / PerStock, 0.f, 1.f);
	}

	//ゲージ全体に対する割合（実際の表示は、ため中の分も合わせて UpdateEnergyBar で行う）
	m_EnergyTotalMax = PerStock * MaxStock;
	m_EnergyTargetRatio = m_EnergyTotalMax > 0.f ? FMath::Clamp(Current / m_EnergyTotalMax, 0.f, 1.f) : 0.f;

	//--- 演出：本数が増えた瞬間／最大かどうか ---
	if (m_LastStock >= 0 && Stock > m_LastStock)
	{
		//増えた区間を光らせる（一度に何本も増えたら、その範囲まとめて）
		m_FlashFromStock = m_LastStock;
		m_FlashToStock = Stock;
		m_StockFlashRemain = m_StockFlashTime;
		m_EnergyPopScale = m_StockPopScale;
	}
	else if (Stock < m_LastStock)
	{
		//使ったときは光を止める
		m_StockFlashRemain = 0.f;
	}

	const bool bMax = (MaxStock > 0 && Stock >= MaxStock);
	if (bMax && !m_bEnergyMax)
	{
		//最大になった瞬間は「たまった光」が先に出るので、鼓動はそのあとから始める
		m_BeatTime = (m_LastStock >= 0) ? -m_StockFlashTime : 0.f;
	}
	m_bEnergyMax = bMax;
	m_LastStock = Stock;
	m_MaxStockCache = MaxStock;
}

void UEchoStatusWidget::HandleGhostsChanged()
{
	if (!Player.IsValid()) return;

	//分身の状態を保存して、カプセルの見た目を更新（色の決め方は GetCapsuleTexture にまとめてある）
	Player->GetGhostStates(m_CachedStates);
	ApplyCapsuleTextures();
}

UTexture2D* UEchoStatusWidget::GetCapsuleTexture(int32 Index) const
{
	//分身がいない枠はグレー
	if (!m_CachedStates.IsValidIndex(Index)) return CapsuleEmptyTexture;

	switch (m_CachedStates[Index])
	{
	case EGhostLifecycleState::Warning:
		//消える直前：青とグレーを交互に
		return m_bBlinkOn ? CapsuleActiveTexture : CapsuleEmptyTexture;

	case EGhostLifecycleState::Corrupted:
	case EGhostLifecycleState::Enemy:
		//敵化した分身は赤
		return CapsuleEnemyTexture ? CapsuleEnemyTexture : CapsuleActiveTexture;

	case EGhostLifecycleState::Dead:
		return CapsuleEmptyTexture;

	default:
		//通常（Friendly）は青
		return CapsuleActiveTexture;
	}
}

void UEchoStatusWidget::ApplyCapsuleTextures()
{
	for (int32 i = 0; i < CapsuleImages.Num(); ++i)
	{
		if (UTexture2D* Tex = GetCapsuleTexture(i))
		{
			CapsuleImages[i]->SetBrushFromTexture(Tex, false);
		}
	}
}

void UEchoStatusWidget::HandleHPChanged(float Current, float Max)
{
	const float NewRatio = Max > 0.f ? FMath::Clamp(Current / Max, 0.f, 1.f) : 0.f;

	if (NewRatio < m_HPRatio)
	{
		//減ったときは少し待ってから帯を追いかけさせる
		m_TrailWaitTime = m_TrailDelay;

		//赤く光らせて揺らす（減った量が大きいほど強く揺れる）
		const float Damage = m_HPRatio - NewRatio;
		m_FlashRemain = m_HPFlashTime;
		m_ShakeRemain = m_ShakeTime;
		m_ShakeStrength = FMath::Lerp(m_ShakeMin, m_ShakeMax,
			FMath::Clamp(Damage / m_ShakeFullDamage, 0.f, 1.f));
	}

	m_HPRatio = NewRatio;

	//減ったときは、表示もすぐ合わせる（増えたときは UpdateHPBar で滑らかに増やす）
	if (m_DisplayHPRatio > m_HPRatio)
	{
		m_DisplayHPRatio = m_HPRatio;
	}

	if (HPBar)      HPBar->SetPercent(ToHPBarPercent(m_DisplayHPRatio));
	if (HPFlashBar) HPFlashBar->SetPercent(ToHPBarPercent(m_DisplayHPRatio));
	if (HPTrailBar) HPTrailBar->SetPercent(ToHPBarPercent(m_TrailRatio));
}

void UEchoStatusWidget::HandleHealCountChanged(int32 Count, int32 Max)
{
	if (HealCountText)
	{
		HealCountText->SetText(FText::AsNumber(Count));
	}

	//減ったときだけ膨らませる（最初の表示や、リスポーンで補充したときは膨らませない）
	if (m_HealCount >= 0 && Count < m_HealCount)
	{
		m_HealScale = m_HealPopScale;
	}
	m_HealCount = Count;
}

void UEchoStatusWidget::UpdateHeal(float DeltaTime)
{
	AActionCharacter* P = Player.Get();
	if (!P) return;

	const bool bHasHeal = P->GetHealCount() > 0;
	const bool bBusy = P->IsHealing();

	//残り0 → 薄く／回復動作中 → 暗く／それ以外 → 通常
	FLinearColor Color = m_HealReadyColor;
	if (!bHasHeal)  Color = m_HealEmptyColor;
	else if (bBusy) Color = m_HealBusyColor;

	// --- HPが少ないときの「回復して！」 ---
	//画面端が赤くなるのと同じ条件。回復が残っていて、回復動作中でないときだけ
	const bool bLowHP = (m_HPRatio > 0.f && m_HPRatio < m_LowHPThreshold);
	const float AlertTarget = (bLowHP && bHasHeal && !bBusy) ? 1.f : 0.f;
	m_HealAlert = FMath::FInterpTo(m_HealAlert, AlertTarget, DeltaTime, 6.f);

	//画面端の赤と同じリズムで脈打つ（0〜1）。実時間なのでスロー中も同じ速さ
	const float Time = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.f;
	const float Pulse = 0.5f + 0.5f * FMath::Sin(Time * m_LowHPPulseSpeed * 2.f * PI);

	//緑に寄せる（ポーションの絵は色を崩さないよう、そのまま）
	const FLinearColor AlertColor = FMath::Lerp(Color, m_HealAlertColor, m_HealAlert * (0.6f + 0.4f * Pulse));

	if (HealCircle)    HealCircle->SetColorAndOpacity(AlertColor);
	if (HealArrow)     HealArrow->SetColorAndOpacity(AlertColor);
	if (HealIcon)      HealIcon->SetColorAndOpacity(Color);
	if (HealCountText) HealCountText->SetColorAndOpacity(FSlateColor(AlertColor));

	// --- 大きさ：使った瞬間のポンッ × HPが少ないときの拡大と鼓動 ---
	m_HealScale = FMath::FInterpTo(m_HealScale, 1.f, DeltaTime, 12.f);
	const float AlertScale = 1.f + m_HealAlert * ((m_HealAlertScale - 1.f) + m_HealAlertPulseScale * Pulse);

	if (HealRoot)  HealRoot->SetRenderScale(FVector2D(m_HealScale * AlertScale));
	if (HealArrow) HealArrow->SetRenderScale(FVector2D(AlertScale));

	// --- 後ろの緑の光：鼓動に合わせて濃くなる ---
	if (HealGlow)
	{
		FLinearColor Glow = m_HealGlowColor;
		Glow.A *= m_HealAlert * (0.4f + 0.6f * Pulse);
		HealGlow->SetColorAndOpacity(Glow);
		HealGlow->SetRenderScale(FVector2D(m_HealGlowScale * (0.95f + 0.1f * Pulse)));
	}
}

void UEchoStatusWidget::UpdateDamageFeedback(float DeltaTime)
{
	// --- 赤く光る：一瞬で光って、だんだん消える ---
	if (HPFlashBar)
	{
		m_FlashRemain = FMath::Max(0.f, m_FlashRemain - DeltaTime);
		const float Alpha = (m_HPFlashTime > 0.f) ? m_FlashRemain / m_HPFlashTime : 0.f;

		FLinearColor Color = m_HPFlashColor;
		Color.A *= Alpha;
		HPFlashBar->SetFillColorAndOpacity(Color);
	}

	// --- 揺れ：ランダムにずらして、だんだん小さくする ---
	if (HPRoot)
	{
		if (m_ShakeRemain > 0.f)
		{
			m_ShakeRemain = FMath::Max(0.f, m_ShakeRemain - DeltaTime);
			const float Power = m_ShakeStrength * ((m_ShakeTime > 0.f) ? m_ShakeRemain / m_ShakeTime : 0.f);
			HPRoot->SetRenderTranslation(FVector2D(
				FMath::FRandRange(-Power, Power),
				FMath::FRandRange(-Power, Power)));
		}
		else
		{
			HPRoot->SetRenderTranslation(FVector2D::ZeroVector);
		}
	}

	// --- HPが少ないとき：画面の端を赤く、ゆっくり脈打たせる ---
	if (LowHPVignette)
	{
		float Target = 0.f;
		if (m_HPRatio > 0.f && m_HPRatio < m_LowHPThreshold)
		{
			//少ないほど濃く（しきい値ちょうどで3分の1、0に近いと最大）
			const float Danger = 1.f - m_HPRatio / m_LowHPThreshold;
			const float Time = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.f;
			const float Pulse = 0.75f + 0.25f * FMath::Sin(Time * m_LowHPPulseSpeed * 2.f * PI);
			Target = FMath::Lerp(0.35f, 1.f, Danger) * m_LowHPMaxOpacity * Pulse;
		}
		m_VignetteOpacity = FMath::FInterpTo(m_VignetteOpacity, Target, DeltaTime, 4.f);
		LowHPVignette->SetRenderOpacity(m_VignetteOpacity);
	}
}

void UEchoStatusWidget::UpdateEnergyFeedback(float DeltaTime)
{
	// --- 最大のとき：鼓動を進める ---
	float Beat = 0.f;
	if (m_bEnergyMax && m_ChargeReservedRatio <= 0.f)
	{
		m_BeatTime += DeltaTime;
		Beat = CalcBeat();
	}

	// --- ゲージの色：ふだんは濃い青、最大のときは明るい青 ---
	if (EnergyBar)
	{
		const FLinearColor Tint = m_bEnergyMax ? m_EnergyMaxTint : m_EnergyTint;
		EnergyBar->SetFillColorAndOpacity(m_EnergyBaseColor * Tint);
	}

	// --- ゲージ全体の大きさ（1本たまったときのポンッ × 最大のときの鼓動） ---
	m_EnergyPopScale = FMath::FInterpTo(m_EnergyPopScale, 1.f, DeltaTime, 10.f);
	if (EnergyRoot)
	{
		EnergyRoot->SetRenderScale(FVector2D(m_EnergyPopScale * (1.f + m_MaxBeatScale * Beat)));
	}

	// --- たまった区間の光 ---
	if (!EnergyStockFlash) return;

	float Alpha = 0.f;
	float Scale = 1.f;
	if (m_StockFlashRemain > 0.f && m_MaxStockCache > 0)
	{
		//たまった瞬間：その区間がパッと光って、膨らみながら消える
		m_StockFlashRemain = FMath::Max(0.f, m_StockFlashRemain - DeltaTime);
		const float T = m_StockFlashRemain / m_StockFlashTime;   //1 → 0
		Alpha = T;
		Scale = FMath::Lerp(1.f, m_StockFlashScale, 1.f - T * T);
		PlaceStockFlash(
			static_cast<float>(m_FlashFromStock) / m_MaxStockCache,
			static_cast<float>(m_FlashToStock) / m_MaxStockCache);
	}
	else if (m_ChargeReservedRatio > 0.f)
	{
		//ため中：減らした区間を脈打つように光らせる（段階が上がった瞬間は強く、少し膨らむ）
		const float Time = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.f;
		const float Pulse = 0.35f + 0.25f * FMath::Sin(Time * 2.f * PI * 3.f);
		Alpha = FMath::Max(Pulse, m_ChargeLevelFlash);
		Scale = 1.f + 0.2f * m_ChargeLevelFlash;
		PlaceStockFlash(m_EnergyDisplayRatio, m_EnergyTargetRatio);
	}
	else if (m_bEnergyMax)
	{
		//最大のとき：ゲージ全体を、鼓動に合わせて光らせる
		Alpha = FMath::Lerp(m_MaxGlowBase, m_MaxGlowPeak, Beat);
		PlaceStockFlash(0.f, 1.f);
	}

	FLinearColor Color = m_StockFlashColor;
	Color.A *= Alpha;
	EnergyStockFlash->SetColorAndOpacity(Color);
	EnergyStockFlash->SetRenderScale(FVector2D(Scale));
}

float UEchoStatusWidget::CalcBeat() const
{
	//「ドクン（強）、ドクン（弱）」を 0〜1 で返す
	if (m_BeatTime < 0.f || m_MaxBeatInterval <= 0.f) return 0.f;

	const float T = FMath::Fmod(m_BeatTime, m_MaxBeatInterval);
	auto Bump = [this](float X)
		{
			return (X >= 0.f && X < m_MaxBeatLength) ? FMath::Sin(PI * X / m_MaxBeatLength) : 0.f;
		};
	return FMath::Max(Bump(T), 0.7f * Bump(T - m_MaxBeatGap));
}

void UEchoStatusWidget::PlaceStockFlash(float FromRatio, float ToRatio)
{
	if (!EnergyBar || !EnergyStockFlash) return;

	//EnergyBar の実際の大きさから、光らせる区間の位置と幅を出す
	//（左の四角ぶんをずらす変換は、SetPercent と同じ m_BarStartRatio / m_BarEndRatio を使う）
	const FVector2D BarSize = EnergyBar->GetCachedGeometry().GetLocalSize();
	if (BarSize.X <= 0.f) return;

	const float X0 = BarSize.X * FMath::Lerp(m_BarStartRatio, m_BarEndRatio, FromRatio);
	const float X1 = BarSize.X * FMath::Lerp(m_BarStartRatio, m_BarEndRatio, ToRatio);

	EnergyStockFlash->SetDesiredSizeOverride(FVector2D(X1 - X0, BarSize.Y));
	EnergyStockFlash->SetRenderTranslation(FVector2D(X0, 0.f));
}

void UEchoStatusWidget::UpdateHPBar(float DeltaTime)
{
	const bool bFilling = m_DisplayHPRatio < m_HPRatio;

	if (bFilling)
	{
		//最初は速く、最後はなめらかに（ただし最後がダラダラ長引かないよう、最低限の速さも足す）
		const float Smooth = FMath::FInterpTo(m_DisplayHPRatio, m_HPRatio, DeltaTime, m_HealFillSpeed);
		const float Linear = m_DisplayHPRatio + 0.3f * DeltaTime;
		m_DisplayHPRatio = FMath::Min(m_HPRatio, FMath::Max(Smooth, Linear));

		if (HPBar)      HPBar->SetPercent(ToHPBarPercent(m_DisplayHPRatio));
		if (HPFlashBar) HPFlashBar->SetPercent(ToHPBarPercent(m_DisplayHPRatio));
	}

	//減った分の帯は、増えていくバーより短く残らないようにそろえる
	if (m_TrailRatio < m_DisplayHPRatio)
	{
		m_TrailRatio = m_DisplayHPRatio;
		if (HPTrailBar) HPTrailBar->SetPercent(ToHPBarPercent(m_TrailRatio));
	}

	//増えている間は、バーを明るい緑に光らせる（被弾の赤い光が出ているときは、そちらを優先）
	m_HealFillGlow = FMath::FInterpTo(m_HealFillGlow, bFilling ? 1.f : 0.f, DeltaTime, 12.f);
	if (HPFlashBar && m_FlashRemain <= 0.f && m_HealFillGlow > 0.01f)
	{
		FLinearColor Color = m_HealFillColor;
		Color.A *= m_HealFillGlow;
		HPFlashBar->SetFillColorAndOpacity(Color);
	}
}

void UEchoStatusWidget::UpdateEnergyBar(float DeltaTime)
{
	//ため中は、減らした分（放ったら使う分）をゲージから引いて見せる
	const USkillComponent* Skill = Player.IsValid() ? Player->SkillComponent : nullptr;
	const float Drained = Skill ? Skill->GetChargeDrainedEnergy() : 0.f;
	m_ChargeReservedRatio = (m_EnergyTotalMax > 0.f) ? FMath::Min(Drained / m_EnergyTotalMax, m_EnergyTargetRatio) : 0.f;
	m_EnergyDisplayRatio = m_EnergyTargetRatio - m_ChargeReservedRatio;

	//段階が上がった瞬間：ゲージをポンッと膨らませて、強く光らせる
	const int32 ChargeLevel = (Skill && Skill->IsCharging()) ? Skill->GetChargeLevel() : 0;
	if (ChargeLevel > m_LastChargeLevel)
	{
		m_EnergyPopScale = m_StockPopScale;
		m_ChargeLevelFlash = 1.f;
	}
	m_LastChargeLevel = ChargeLevel;
	m_ChargeLevelFlash = FMath::Max(0.f, m_ChargeLevelFlash - DeltaTime * 3.f);

	//（画像の左の四角ぶんをずらして、中身の部分だけが0→100%になるように変換して表示）
	if (EnergyBar) EnergyBar->SetPercent(FMath::Lerp(m_BarStartRatio, m_BarEndRatio, m_EnergyDisplayRatio));
}
