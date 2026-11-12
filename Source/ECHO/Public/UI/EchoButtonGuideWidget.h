//EchoButtonGuideWidget.h
//左下のボタンガイド（上：攻撃 左：召喚 右：回避 下：ジャンプ 中央：RB）
//・押している間、そのボタンを拡大縮小する
//・ジャンプは空中で二段目が残っていれば二段の絵に切り替える
//・召喚はゲージ1本分まで、下から青くたまっていく

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"
#include "EchoButtonGuideWidget.generated.h"

class UWidget;
class UImage;
class UProgressBar;
class UTexture2D;
class APlayerController;
class UTextBlock;

UCLASS()
class ECHO_API UEchoButtonGuideWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// --- WBP側に同じ名前のウィジェットを置くと自動で紐づく ---
	//ボタンごとのまとまり（台座＋アイコンを入れた Overlay）。押したときにこれごと拡大縮小する
	UPROPERTY(meta = (BindWidget))
	UWidget* Btn_Attack;

	UPROPERTY(meta = (BindWidget))
	UWidget* Btn_Summon;

	UPROPERTY(meta = (BindWidget))
	UWidget* Btn_Dodge;

	UPROPERTY(meta = (BindWidget))
	UWidget* Btn_Jump;

	UPROPERTY(meta = (BindWidget))
	UWidget* Btn_RB;

	//アイコン（絵を切り替えるもの）
	UPROPERTY(meta = (BindWidget))
	UImage* Icon_Attack;

	UPROPERTY(meta = (BindWidget))
	UImage* Icon_Summon;

	UPROPERTY(meta = (BindWidget))
	UImage* Icon_Dodge;

	UPROPERTY(meta = (BindWidget))
	UImage* Icon_Jump;

	//召喚ボタンの台座（下から水がたまる）
	UPROPERTY(meta = (BindWidget))
	UProgressBar* SummonFill;

	// --- 押したかどうかを見るキー（キーボードでも反応させたいときはここに追加） ---
	UPROPERTY(EditAnywhere, Category = "Guide|Keys")
	TArray<FKey> m_AttackKeys = { EKeys::Gamepad_FaceButton_Top };

	UPROPERTY(EditAnywhere, Category = "Guide|Keys")
	TArray<FKey> m_SummonKeys = { EKeys::Gamepad_FaceButton_Left };

	UPROPERTY(EditAnywhere, Category = "Guide|Keys")
	TArray<FKey> m_DodgeKeys = { EKeys::Gamepad_FaceButton_Right };

	UPROPERTY(EditAnywhere, Category = "Guide|Keys")
	TArray<FKey> m_JumpKeys = { EKeys::Gamepad_FaceButton_Bottom };

	UPROPERTY(EditAnywhere, Category = "Guide|Keys")
	TArray<FKey> m_RBKeys = { EKeys::Gamepad_RightShoulder };

	// --- 押したときの拡大縮小 ---
	//押している間の大きさ（1より小さいと縮む、1より大きいと膨らむ）
	UPROPERTY(EditAnywhere, Category = "Guide|Press", meta = (ClampMin = "0.1"))
	float m_PressedScale = 0.85f;

	//大きさが変わる速さ
	UPROPERTY(EditAnywhere, Category = "Guide|Press", meta = (ClampMin = "1"))
	float m_ScaleSpeed = 25.f;

	// --- ジャンプ ---
	//地上の絵（未設定ならWBPで Icon_Jump に入れた絵を使う）
	UPROPERTY(EditAnywhere, Category = "Guide|Jump")
	UTexture2D* m_JumpTexture = nullptr;

	//空中で二段目が残っているときの絵
	UPROPERTY(EditAnywhere, Category = "Guide|Jump")
	UTexture2D* m_DoubleJumpTexture = nullptr;

	//二段目を使い切ったときのアイコンの濃さ
	UPROPERTY(EditAnywhere, Category = "Guide|Jump", meta = (ClampMin = "0", ClampMax = "1"))
	float m_JumpUsedOpacity = 0.35f;

	// --- 召喚 ---
	//水位が実際の値に追いつく速さ（小さいほどゆっくりたまる）
	UPROPERTY(EditAnywhere, Category = "Guide|Summon", meta = (ClampMin = "0.1"))
	float m_SummonFillSpeed = 6.f;

	//たまりきった瞬間に一度だけ膨らませる大きさ
	UPROPERTY(EditAnywhere, Category = "Guide|Summon", meta = (ClampMin = "1"))
	float m_SummonReadyPopScale = 1.25f;

	// --- RBを押している間に差し替えるアイコン（未設定ならそのまま） ---
	UPROPERTY(EditAnywhere, Category = "Guide|Skill")
	UTexture2D* m_AttackSkillTexture = nullptr;

	UPROPERTY(EditAnywhere, Category = "Guide|Skill")
	UTexture2D* m_SummonSkillTexture = nullptr;

	UPROPERTY(EditAnywhere, Category = "Guide|Skill")
	UTexture2D* m_DodgeSkillTexture = nullptr;

	UPROPERTY(EditAnywhere, Category = "Guide|Skill")
	UTexture2D* m_JumpSkillTexture = nullptr;

	// --- RBを押している間に表示する消費ゲージ数（0＝未実装：数字を出さず薄くする） ---
	UPROPERTY(EditAnywhere, Category = "Guide|Skill", meta = (ClampMin = "0"))
	int32 m_AttackSkillCost = 1;   //RB+Y 一閃

	UPROPERTY(EditAnywhere, Category = "Guide|Skill", meta = (ClampMin = "0"))
	int32 m_SummonSkillCost = 0;   //RB+X 未実装

	UPROPERTY(EditAnywhere, Category = "Guide|Skill", meta = (ClampMin = "0"))
	int32 m_DodgeSkillCost = 3;    //RB+B テンペスト

	UPROPERTY(EditAnywhere, Category = "Guide|Skill", meta = (ClampMin = "0"))
	int32 m_JumpSkillCost = 0;     //RB+A 未実装

	//ゲージが足りない技の濃さ
	UPROPERTY(EditAnywhere, Category = "Guide|Skill", meta = (ClampMin = "0", ClampMax = "1"))
	float m_UnaffordableOpacity = 0.35f;

	//コストの数字（置かなくてもエラーにならない）
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Cost_Attack;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Cost_Summon;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Cost_Dodge;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Cost_Jump;

private:
	//WBPで入れてある通常の絵（RBを離したときに戻す用）
	UPROPERTY()
	TObjectPtr<UTexture2D> m_NormalAttack;

	UPROPERTY()
	TObjectPtr<UTexture2D> m_NormalSummon;

	UPROPERTY()
	TObjectPtr<UTexture2D> m_NormalDodge;

	//今の大きさ [0]攻撃 [1]召喚 [2]回避 [3]ジャンプ [4]RB
	float m_Scales[5] = { 1.f, 1.f, 1.f, 1.f, 1.f };

	float m_SummonFillShown = 0.f;   //画面に出している水位
	bool m_bSummonReady = false;     //前のフレームでたまりきっていたか

	static bool IsAnyKeyDown(const APlayerController* PC, const TArray<FKey>& Keys);
	static void SetIcon(UImage* Img, UTexture2D* Tex);
	void UpdateScale(int32 Index, UWidget* Target, bool bPressed, float DeltaTime);
	void UpdateCost(UTextBlock* Text, UWidget* Button, UImage* Icon, UTexture2D* SkillTex, int32 Cost, bool bSkillMode, int32 Stock);
};
