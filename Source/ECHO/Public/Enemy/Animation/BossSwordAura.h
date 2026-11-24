#pragma once

#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "BossSwordAura.generated.h"

class UNiagaraSystem;
class UMaterialInterface;

//モンタージュで指定した区間だけ、剣にオーラをまとう。
UCLASS(meta = (DisplayName = "Boss Sword Aura"))
class ECHO_API UBossSwordAura : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	//刃に沿って流れる色と模様。
	UPROPERTY(EditAnywhere, Category = "Aura")
	TObjectPtr<UMaterialInterface> m_material;

	//刃の周りに漂う煙。
	UPROPERTY(EditAnywhere, Category = "Aura")
	TObjectPtr<UNiagaraSystem> m_mist;

	//刀身に対する煙の大きさ。
	UPROPERTY(EditAnywhere, Category = "Aura", meta = (ClampMin = "0"))
	float m_scale = .1f;

	//構えで表示する。
	virtual void NotifyBegin(USkeletalMeshComponent* _mesh, UAnimSequenceBase*, float, const FAnimNotifyEventReference&) override;

	//構え始めから光が最大になるまでのモンタージュ上の秒数。
	UPROPERTY(EditAnywhere, Category = "Aura", meta = (ClampMin = "0.01"))
	float m_chargeTime = .155f;

	//構え始めと振り抜き時の幅。
	UPROPERTY(EditAnywhere, Category = "Aura")
	FVector2D m_width = FVector2D(.3f, 2.f);

	//構え始めと振り抜き時の明るさ。
	UPROPERTY(EditAnywhere, Category = "Aura")
	FVector2D m_glow = FVector2D(.65f, 4.8f);

	//剣を振る直前まで、実際の再生位置に合わせて光を育てる。
	virtual void NotifyTick(USkeletalMeshComponent* _mesh, UAnimSequenceBase*, float, const FAnimNotifyEventReference& _event) override;

	//振り終わりで消す。
	virtual void NotifyEnd(USkeletalMeshComponent* _mesh, UAnimSequenceBase*, const FAnimNotifyEventReference&) override;
};
