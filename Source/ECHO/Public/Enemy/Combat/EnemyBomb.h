#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemyBomb.generated.h"
class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UNiagaraSystem;
class AEnemyCharacterBase;
class UCameraShakeBase;

//弾が当たった場所で爆発とダメージを管理する。
UCLASS()
class ECHO_API AEnemyBomb : public AActor
{
	GENERATED_BODY()

public:
	//弾が当たった場所で爆発とダメージを管理する。
	AEnemyBomb();

public:
	//弾の飛ぶ方向と速さを設定する。
	UFUNCTION(BlueprintCallable, Category="Bomb")
	void Launch(AEnemyCharacterBase* source, AActor* target, FVector velocity, float damage, bool unblockable);

	//着地点の予告をBPで表示する。
	UFUNCTION(BlueprintImplementableEvent, Category="Bomb")
	void Armed();

	//指定位置の爆発をBPで表示する。
	UFUNCTION(BlueprintImplementableEvent, Category="Bomb")
	void BurstAt(FVector point);

public:
	//何かに当たった瞬間に爆発するか。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bomb")
	bool m_explodeOnHit = true;

	//確認用の判定を表示するか。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bomb|Debug")
	bool m_debugHit = false;

	//爆発時のカメラの揺れ。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bomb|Camera")
	TSubclassOf<UCameraShakeBase> m_cameraShake;

	//カメラが揺れる範囲。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bomb|Camera", meta=(ClampMin="0"))
	float m_shakeRange = 900.f;

	//弾の当たり判定。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USphereComponent> m_collision;

	//表示するモデル。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> m_mesh;

	//弾の移動を行う部品。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UProjectileMovementComponent> m_move;

	//着弾から爆発までの時間。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bomb")
	float m_fuse = .9f;

	//判定に使う半径。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bomb")
	float m_radius = 180.f;

	//爆発に使うエフェクト。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bomb")
	TObjectPtr<UNiagaraSystem> m_explosion;

	//エフェクトの大きさ。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bomb")
	FVector m_fxScale = FVector(.5f);

	//着地点の予告位置。
	UPROPERTY(BlueprintReadOnly, Category="Bomb")
	FVector m_groundPoint;

	//着地点の面の向き。
	UPROPERTY(BlueprintReadOnly, Category="Bomb")
	FVector m_groundNormal = FVector::UpVector;

	//地面として扱う傾きの基準。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bomb", meta=(ClampMin="0.1",ClampMax="1"))
	float m_floorNormal = .55f;

	//着弾して爆発待ちか。
	UPROPERTY(BlueprintReadOnly, Category="Bomb")
	bool m_armed = false;

protected:
	//ゲーム開始時に必要な状態を準備する。
	virtual void BeginPlay() override;

	//終了時にタイマーや登録を解除する。
	virtual void EndPlay(const EEndPlayReason::Type _reason) override;

private:
	//爆発のダメージと演出を発生させる。
	void Explode();

private:
	//弾が触れた場所で着弾処理を行う。
	UFUNCTION()
	void Landed(const FHitResult& hit, const FVector& velocity);

	//弾の移動が止まった時に着弾処理を行う。
	UFUNCTION()
	void Stopped(const FHitResult& hit);

	//爆発のエフェクトを表示する。
	UFUNCTION(NetMulticast, Unreliable)
	void ShowExplosion(FVector point);

private:
	//処理や攻撃の元となる対象。
	TWeakObjectPtr<AEnemyCharacterBase> m_source;

	//現在狙っている相手。
	TWeakObjectPtr<AActor> m_target;

	//時間を待つためのタイマー。
	FTimerHandle m_timer;

	//与えるダメージ。
	float m_damage = 0.f;

	//ガードできない攻撃か。
	bool m_unblockable = false;
};
