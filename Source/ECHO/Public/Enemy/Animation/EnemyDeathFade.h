#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyDeathFade.generated.h"

//死亡動作を待って体と装備を一緒に薄くする。
UCLASS()
class ECHO_API UEnemyDeathFade : public UActorComponent
{
	GENERATED_BODY()

public:
	UEnemyDeathFade();

	//消え切るまでの秒数。
	UPROPERTY(EditDefaultsOnly, Category = "Death", meta = (ClampMin = "0.05"))
	float m_duration = .85f;

	//最終姿勢の後に実行する処理を受け取る。
	void Start(float _delay, UMaterialInterface* _material, FSimpleDelegate _finished);

	//この部品が消滅を管理しているか。
	bool IsFading() const { return m_finished.IsBound(); }

private:
	virtual void TickComponent(float _delta, ELevelTick _type, FActorComponentTickFunction* _tick) override;

	//消滅が終わった時に破棄またはプールへ返す。
	FSimpleDelegate m_finished;
	float m_age = 0.f;

	//BPから受け取った、装備用の予備素材。
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> m_material;

	//出現用の素材は元の模様を保って逆向きに消す。
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> m_reveal;

	//出現用の素材を持たない装備にも同じ濃さを使う。
	UPROPERTY(Transient)
	TObjectPtr<class UMaterialInstanceDynamic> m_ash;

	//プールで再使用する時のために元の素材を覚える。
	struct FSlot
	{
		TWeakObjectPtr<UMeshComponent> mesh;
		int32 index;
		TStrongObjectPtr<UMaterialInterface> material;
	};
	TArray<FSlot> m_slots;
};
