#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "BossExcaliburHit.generated.h"

//斬撃と地面の衝撃波に入った相手へ、振り抜きでダメージを与える。
UCLASS(meta = (DisplayName = "Boss Excalibur Hit"))
class ECHO_API UBossExcaliburHit : public UAnimNotify
{
	GENERATED_BODY()

public:

	//ボスから見た斬撃の到達距離。
	UPROPERTY(EditAnywhere, Category = "Blade", meta = (ClampMin = "0", Units = "cm"))
	float m_bladeRange = 520.f;

	//正面から左右に斬撃が届く角度。
	UPROPERTY(EditAnywhere, Category = "Blade", meta = (ClampMin = "0", ClampMax = "180", Units = "deg"))
	float m_bladeAngle = 80.f;

	//斬撃が届く、腰から上下の幅。
	UPROPERTY(EditAnywhere, Category = "Blade", meta = (ClampMin = "0", Units = "cm"))
	float m_bladeHeight = 125.f;

	//床の波紋が届く半径。
	UPROPERTY(EditAnywhere, Category = "Wave", meta = (ClampMin = "0", Units = "cm"))
	float m_waveRange = 420.f;

	//床の波紋をジャンプで飛び越せる高さ。
	UPROPERTY(EditAnywhere, Category = "Wave", meta = (ClampMin = "0", Units = "cm"))
	float m_waveHeight = 60.f;

	//攻撃データの基礎ダメージに掛ける斬撃の割合。
	UPROPERTY(EditAnywhere, Category = "Blade", meta = (ClampMin = "0"))
	float m_bladeDamage = .45f;

	//攻撃データの基礎ダメージに掛ける衝撃波の割合。
	UPROPERTY(EditAnywhere, Category = "Wave", meta = (ClampMin = "0"))
	float m_waveDamage = .55f;

	//モンタージュの指定フレームで、相手ごとに合計を一度だけ与える。
	virtual void Notify(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim,
		const FAnimNotifyEventReference& _event) override;
};
