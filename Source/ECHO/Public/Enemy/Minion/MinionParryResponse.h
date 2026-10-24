#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MinionParryResponse.generated.h"
class UAnimMontage;

//攻撃を受け流された敵に、反撃できる隙を作る。
UCLASS(ClassGroup=(Enemy),meta=(BlueprintSpawnableComponent))
class ECHO_API UMinionParryResponse : public UActorComponent
{
	GENERATED_BODY()

public:
	//受け流しによる隙を開始する。
	bool Begin(AActor* _defender);

public:
	//相手の受け流しが成功するか確認する。
	UFUNCTION(BlueprintCallable,Category="Parry")
	bool TryParry(AActor* defender,float inputAge);

public:
	//受け流された直後に止まる時間。
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Parry",meta=(ClampMin="0.01",ClampMax="0.5"))
	float m_freezeTime=.18f;

	//攻撃後の待ち時間。
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Parry",meta=(ClampMin="0.1"))
	float m_recovery=1.25f;

	//受け流された時の反応モンタージュ。
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Parry")
	TObjectPtr<UAnimMontage> m_reaction;

protected:
	//終了時にタイマーや登録を解除する。
	virtual void EndPlay(const EEndPlayReason::Type _reason) override;

private:
	//受け流し後の停止を解除する。
	void Finish();

private:
	//時間を待つためのタイマー。
	FTimerHandle m_timer;

	//アニメーションを止めているか。
	TWeakObjectPtr<UAnimMontage> m_paused;
};
