//EchoButtonGuideWidget.cpp

#include "EchoButtonGuideWidget.h"
#include "Player/ActionCharacter.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/Texture2D.h"
#include "Components/TextBlock.h"

namespace
{
	//m_Scales の番号
	constexpr int32 IDX_ATTACK = 0;
	constexpr int32 IDX_SUMMON = 1;
	constexpr int32 IDX_DODGE = 2;
	constexpr int32 IDX_JUMP = 3;
	constexpr int32 IDX_RB = 4;
}

void UEchoButtonGuideWidget::NativeConstruct()
{
	Super::NativeConstruct();

	//WBPで入れたアイコンを「通常の絵」として覚えておく
	auto GetTex = [](UImage* Img) -> UTexture2D*
		{
			return Img ? Cast<UTexture2D>(Img->GetBrush().GetResourceObject()) : nullptr;
		};
	m_NormalAttack = GetTex(Icon_Attack);
	m_NormalSummon = GetTex(Icon_Summon);
	m_NormalDodge = GetTex(Icon_Dodge);

	if (!m_JumpTexture) m_JumpTexture = GetTex(Icon_Jump);
}

void UEchoButtonGuideWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	// --- 押している間の拡大縮小 ---
	const bool bRB = IsAnyKeyDown(PC, m_RBKeys);
	UpdateScale(IDX_ATTACK, Btn_Attack, IsAnyKeyDown(PC, m_AttackKeys), InDeltaTime);
	UpdateScale(IDX_SUMMON, Btn_Summon, IsAnyKeyDown(PC, m_SummonKeys), InDeltaTime);
	UpdateScale(IDX_DODGE, Btn_Dodge, IsAnyKeyDown(PC, m_DodgeKeys), InDeltaTime);
	UpdateScale(IDX_JUMP, Btn_Jump, IsAnyKeyDown(PC, m_JumpKeys), InDeltaTime);
	UpdateScale(IDX_RB, Btn_RB, bRB, InDeltaTime);

	// --- RBを押している間はスキルの絵に差し替える（未設定なら通常のまま） ---
	SetIcon(Icon_Attack, (bRB && m_AttackSkillTexture) ? m_AttackSkillTexture : m_NormalAttack.Get());
	SetIcon(Icon_Summon, (bRB && m_SummonSkillTexture) ? m_SummonSkillTexture : m_NormalSummon.Get());
	SetIcon(Icon_Dodge, (bRB && m_DodgeSkillTexture) ? m_DodgeSkillTexture : m_NormalDodge.Get());

	AActionCharacter* Player = Cast<AActionCharacter>(GetOwningPlayerPawn());
	if (!Player) return;

	// --- RBを押している間：消費ゲージ数を出し、足りない技は薄くする ---
	const int32 Stock = Player->GetEnergyStock();
	UpdateCost(Cost_Attack, Btn_Attack, Icon_Attack, m_AttackSkillTexture, m_AttackSkillCost, bRB, Stock);
	UpdateCost(Cost_Summon, Btn_Summon, Icon_Summon, m_SummonSkillTexture, m_SummonSkillCost, bRB, Stock);
	UpdateCost(Cost_Dodge, Btn_Dodge, Icon_Dodge, m_DodgeSkillTexture, m_DodgeSkillCost, bRB, Stock);
	UpdateCost(Cost_Jump, Btn_Jump, Icon_Jump, m_JumpSkillTexture, m_JumpSkillCost, bRB, Stock);

	// --- ジャンプ：空中で二段目が残っていれば二段の絵、使い切ったら薄くする ---
	UTexture2D* JumpTex = m_JumpTexture;
	float JumpOpacity = 1.f;
	const UCharacterMovementComponent* Move = Player->GetCharacterMovement();
	if (Move && Move->IsFalling())
	{
		//JumpCurrentCount：今何回跳んだか　JumpMaxCount：最大回数（ActionCharacterで2）
		if (Player->JumpCurrentCount < Player->JumpMaxCount)
		{
			if (m_DoubleJumpTexture) JumpTex = m_DoubleJumpTexture;
		}
		else
		{
			JumpOpacity = m_JumpUsedOpacity;
		}
	}
	if (bRB && m_JumpSkillTexture)
	{
		JumpTex = m_JumpSkillTexture;
		JumpOpacity = 1.f;
	}
	SetIcon(Icon_Jump, JumpTex);
	if (Icon_Jump) Icon_Jump->SetRenderOpacity(JumpOpacity);

	// --- 召喚：ゲージ1本分まで下から水がたまる（1本たまったら召喚できる） ---
	const float PerStock = Player->GetEnergyPerStock();
	const float Target = (PerStock > 0.f) ? FMath::Clamp(Player->GetCurrentEnergy() / PerStock, 0.f, 1.f) : 0.f;
	m_SummonFillShown = FMath::FInterpTo(m_SummonFillShown, Target, InDeltaTime, m_SummonFillSpeed);
	if (SummonFill) SummonFill->SetPercent(m_SummonFillShown);

	//水が上まで届いた瞬間に一度だけ膨らませる
	const bool bReady = (Target >= 1.f) && (m_SummonFillShown >= 0.98f);
	if (bReady && !m_bSummonReady)
	{
		m_Scales[IDX_SUMMON] = m_SummonReadyPopScale;
	}
	m_bSummonReady = bReady;
}

bool UEchoButtonGuideWidget::IsAnyKeyDown(const APlayerController* PC, const TArray<FKey>& Keys)
{
	for (const FKey& Key : Keys)
	{
		if (Key.IsValid() && PC->IsInputKeyDown(Key)) return true;
	}
	return false;
}

void UEchoButtonGuideWidget::SetIcon(UImage* Img, UTexture2D* Tex)
{
	//同じ絵なら何もしない（毎フレーム差し替えない）
	if (!Img || !Tex) return;
	if (Img->GetBrush().GetResourceObject() == Tex) return;
	Img->SetBrushFromTexture(Tex, false);
}

void UEchoButtonGuideWidget::UpdateScale(int32 Index, UWidget* Target, bool bPressed, float DeltaTime)
{
	//押している間は m_PressedScale へ、離したら 1 へなめらかに戻す
	const float Goal = bPressed ? m_PressedScale : 1.f;
	m_Scales[Index] = FMath::FInterpTo(m_Scales[Index], Goal, DeltaTime, m_ScaleSpeed);
	if (Target) Target->SetRenderScale(FVector2D(m_Scales[Index]));
}

void UEchoButtonGuideWidget::UpdateCost(UTextBlock* Text, UWidget* Button, UImage* Icon, UTexture2D* SkillTex, int32 Cost, bool bSkillMode, int32 Stock)
{
	//数字はRBを押している間だけ（コスト0＝未実装は出さない）
	const bool bShow = bSkillMode && Cost > 0;
	if (Text)
	{
		Text->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (bShow) Text->SetText(FText::AsNumber(Cost));
	}

	//RB中はアイコンを隠して台座だけにする（スキル用の絵を設定したときはそれを出す）
	if (Icon)
	{
		const bool bHideIcon = bSkillMode && !SkillTex;
		Icon->SetVisibility(bHideIcon ? ESlateVisibility::Hidden : ESlateVisibility::HitTestInvisible);
	}

	//RB中、ゲージが足りない技・未実装の技はボタンごと薄くする
	if (Button)
	{
		const bool bDim = bSkillMode && (Cost <= 0 || Stock < Cost);
		Button->SetRenderOpacity(bDim ? m_UnaffordableOpacity : 1.f);
	}
}
