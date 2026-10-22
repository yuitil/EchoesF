//EchoStatusWidget.cpp

#include "UI/EchoStatusWidget.h"
#include "Player/ActionCharacter.h"
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

	BuildCapsules(P->GetMaxGhostCount());

	//初期表示
	HandleEnergyChanged(P->GetCurrentEnergy(), P->GetMaxEnergy());
	HandleGhostsChanged();
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

	//画像の左の四角ぶんをずらして、中身の部分だけが0→100%になるように変換
	if (EnergyBar)
	{
		EnergyBar->SetPercent(FMath::Lerp(m_BarStartRatio, m_BarEndRatio, Ratio));
	}

	//左の数字：ストック数（0〜5）
	if (StockText)
	{
		StockText->SetText(FText::AsNumber(Stock));
	}
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
