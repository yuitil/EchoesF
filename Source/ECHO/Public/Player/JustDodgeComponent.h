//JustDodgeComponent.h
//ジャスト回避の演出と報酬
//・全体を一瞬スローにする（敵・分身・プレイヤー全員）
//・画面の彩度を下げる（スロー中は強く、報酬が続いている間は少しだけ）
//・一定時間、攻撃で得るエネルギーを倍にする
//時間はすべて「実時間（スローの影響を受けない秒数）」で数える

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "JustDodgeComponent.generated.h"

class UCameraComponent;

//ジャスト回避が成立したとき（あとで音やUIを足すとき用）
DECLARE_MULTICAST_DELEGATE(FOnJustDodge);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ECHO_API UJustDodgeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UJustDodgeComponent();

	//ジャスト回避が成立したときに呼ぶ
	void TriggerJustDodge();

	//攻撃で得るエネルギーの倍率（報酬中は m_EnergyRate、それ以外は1）
	float GetEnergyRate() const { return m_BuffRemain > 0.f ? m_EnergyRate : 1.f; }
	bool IsBuffActive() const { return m_BuffRemain > 0.f; }

	FOnJustDodge OnJustDodge;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	// --- スロー ---
	//スロー中の時間の速さ（0.2＝5分の1）
	UPROPERTY(EditAnywhere, Category = "JustDodge|Slow", meta = (ClampMin = "0.01", ClampMax = "1"))
	float m_SlowTimeDilation = 0.2f;

	//スローのまま止めておく時間（実時間）
	UPROPERTY(EditAnywhere, Category = "JustDodge|Slow", meta = (ClampMin = "0"))
	float m_SlowHoldTime = 0.3f;

	//元の速さに戻すのにかける時間（実時間）
	UPROPERTY(EditAnywhere, Category = "JustDodge|Slow", meta = (ClampMin = "0"))
	float m_SlowReturnTime = 0.2f;

	//次のスローを出せるまでの時間（実時間）。報酬はこの間も毎回つく
	UPROPERTY(EditAnywhere, Category = "JustDodge|Slow", meta = (ClampMin = "0"))
	float m_SlowCooldown = 1.5f;

	// --- 彩度（1＝通常 0＝白黒） ---
	//スロー中
	UPROPERTY(EditAnywhere, Category = "JustDodge|Saturation", meta = (ClampMin = "0", ClampMax = "1"))
	float m_SlowSaturation = 0.2f;

	//報酬が続いている間
	UPROPERTY(EditAnywhere, Category = "JustDodge|Saturation", meta = (ClampMin = "0", ClampMax = "1"))
	float m_BuffSaturation = 0.7f;

	//彩度が変わる速さ
	UPROPERTY(EditAnywhere, Category = "JustDodge|Saturation", meta = (ClampMin = "0.1"))
	float m_SaturationSpeed = 10.f;

	// --- 報酬 ---
	//エネルギーが倍になる時間（実時間）
	UPROPERTY(EditAnywhere, Category = "JustDodge|Reward", meta = (ClampMin = "0"))
	float m_BuffTime = 3.f;

	//エネルギーの倍率
	UPROPERTY(EditAnywhere, Category = "JustDodge|Reward", meta = (ClampMin = "1"))
	float m_EnergyRate = 2.f;

	//成立したときに画面に文字を出す
	UPROPERTY(EditAnywhere, Category = "JustDodge|Debug")
	bool m_bDebug = false;

private:
	UPROPERTY()
	TObjectPtr<UCameraComponent> m_Camera;

	bool m_bSlowing = false;       //スロー中（戻している途中も含む）
	float m_SlowElapsed = 0.f;     //スローが始まってからの実時間
	float m_CooldownRemain = 0.f;  //次のスローまでの残り
	float m_BuffRemain = 0.f;      //エネルギー倍の残り
	float m_Saturation = 1.f;      //今の彩度

	void SetSaturation(float Value);
};
