#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EnemyHitEffect.generated.h"

class UNiagaraSystem;

//攻撃が当たった場所に効果を出す。
UCLASS(ClassGroup=(Enemy), meta=(BlueprintSpawnableComponent))
class ECHO_API UEnemyHitEffect : public UActorComponent
{
	GENERATED_BODY()

public:
	//攻撃が当たった場所に効果を出す。
	UEnemyHitEffect();

	//体に当たった場所の演出を開始する。
	void PlayHit(AActor* _attacker, const FHitResult* _hit = nullptr);

	//剣で受けた位置に火花を出す。
	void PlayClash(AActor* _attacker);

public:
	//相手との位置関係から命中位置を求める。
	UFUNCTION(BlueprintPure, Category="Enemy|Hit FX")
	bool EstimateContact(AActor* attacker, FVector& point, FVector& normal) const;

	//体への命中エフェクトをBPで表示する。
	UFUNCTION(NetMulticast, Unreliable)
	void ShowHit(FVector point, FVector normal);

	//剣の火花を全員に見せる。
	UFUNCTION(NetMulticast, Unreliable)
	void ShowClash(FVector _point, FVector _normal);

	//攻撃が相手へ当たった演出を全員に見せる。
	UFUNCTION(NetMulticast, Unreliable)
	void ShowStrike(FVector _point, FVector _normal);

public:
	//相手へ命中した時のNiagara。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Hit FX")
	TObjectPtr<UNiagaraSystem> m_strikeEffect;

	//相手への命中演出の大きさ。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Hit FX")
	FVector m_strikeScale = FVector(1.f);

	//体力に適用する効果。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Hit FX")
	TObjectPtr<UNiagaraSystem> m_effect;

	//表示する大きさ。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Hit FX")
	FVector m_scale = FVector(.2f);

	//剣で受けた時のNiagara。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Hit FX")
	TObjectPtr<UNiagaraSystem> m_clashEffect;

	//火花の大きさ。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Hit FX")
	FVector m_clashScale = FVector(1.f);

	//弾の出現位置のずらし量。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Hit FX")
	float m_offset = 2.f;

	//剣の根元のソケット名。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Hit FX")
	FName m_base = TEXT("BladeBase");

	//剣先のソケット名。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Hit FX")
	FName m_tip = TEXT("BladeTip");
};
