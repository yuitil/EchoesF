#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat.generated.h"

class UMeshComponent;

//武器が当たった相手へダメージを渡す。
UCLASS( ClassGroup=(Enemy), meta=(BlueprintSpawnableComponent))
class ECHO_API UCombat : public UActorComponent
{
	GENERATED_BODY()

public:
	//武器が当たった相手へダメージを渡す。
	UCombat();

	//BPで指定した剣の判定を接続する。
	virtual void BeginPlay() override;

	//打ち上げ後の重力を元に戻す。
	void EndLift();

	//終了時にタイマーや登録を解除する。
	virtual void EndPlay(const EEndPlayReason::Type _reason) override;

	//剣の攻撃判定を有効にする。
	void BeginHitWindow(float _damageScale = 1.0f, FName _bone = NAME_None, FVector _tip = FVector::ZeroVector);

	//剣の攻撃判定を終了する。
	void EndHitWindow();

	//攻撃判定と演出で同じ剣の位置を読む。
	bool GetWeaponPoints(FVector& _base, FVector& _tip) const;

	//攻撃判定が有効か返す。
	bool IsHitWindowOpen() const { return m_bHitWindow; }

	//攻撃と防御の判定を初期化する。
	void ResetWindows();

	//経過時間に合わせて敵の状態を更新する。
	virtual void TickComponent(float _deltaTime, ELevelTick _type, FActorComponentTickFunction* _function) override;

public:
	//攻撃判定に使う武器を設定する。
	UFUNCTION(BlueprintCallable, Category = "Enemy | Combat")
	bool SetWeaponMesh(UMeshComponent* _mesh, FName _base = TEXT("BladeBase"), FName _tip = TEXT("BladeTip"));

public:
	//狙っている相手以外も、剣が直接当たれば判定する。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Combat")
	bool m_hitNearby = false;

	//BPの剣部品に付けるタグ。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Combat")
	FName m_weaponTag;

	//攻撃判定の根元ソケット。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "WeaponBaseSocket"))
	FName m_baseSocket = TEXT("hand_r");

	//攻撃判定の先端ソケット。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "WeaponTipSocket"))
	FName m_tipSocket = TEXT("SwordTip");

	//判定に使う半径。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Radius", ClampMin = "1"))
	float m_radius = 12.f;

	//確認用の判定を表示するか。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Debug"))
	bool m_bDebug = false;

	//回避の無敵時間中か。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Invulnerable"))
	bool m_bInvulnerable = false;

	//ガード判定が有効か。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Guarding"))
	bool m_bGuarding = false;

private:
    //剣のタグから判定部品をつなぎ直す。
    void BindWeapon();

	//命中した相手を打ち上げる。
	void BeginLift(class ACharacter* _target, const struct FEnemyActionData& _action);

private:
    //今の通知で指定された武器のボーン。
    FName m_hitBone;

    //今の通知で指定された剣先の位置。
    FVector m_tipOffset = FVector::ZeroVector;

	//打ち上げ中の相手。
	TWeakObjectPtr<class ACharacter> m_liftTarget;

	//打ち上げ前の相手の重力。
	float m_targetGravity = 1.f;

	//打ち上げ前の自分の重力。
	float m_selfGravity = 1.f;

	//空中コンボ中の重力倍率。
	float m_liftGravity = 1.f;

	//空中コンボ用の重力を終える時刻。
	double m_liftEnd = 0.0;

	//武器のソケットで判定するか。
	bool m_useWeapon = false;

	//現在の攻撃判定のダメージ倍率。
	float m_hitScale = 1.0f;

	//攻撃判定の区間内か。
	bool m_bHitWindow = false;

	//前回の剣の位置があるか。
	bool m_hasPreviousHit = false;

	//前回の剣先の位置。
	FVector m_previousTip = FVector::ZeroVector;

	//前回の剣の根元の位置。
	FVector m_previousBase = FVector::ZeroVector;

	//回避を確認している相手。
	TWeakObjectPtr<AActor> m_dodgeTarget;

	//前回確認した時に回避していたか。
	bool m_wasDodging = false;

	//相手が回避を始めた時刻。
	double m_dodgeStart = -100.;

	//相手が回避を始めた位置。
	FVector m_dodgePos = FVector::ZeroVector;

	//前回の相手の位置。
	FVector m_lastTargetPos = FVector::ZeroVector;

	//相手の当たり判定の半径。
	float m_dodgeRadius = 0.f;

	//相手の当たり判定の半分の高さ。
	float m_dodgeHalf = 0.f;

	//今回の攻撃で既に当たった相手。
	TSet<TWeakObjectPtr<AActor>> m_hitActors;

private:
	//攻撃判定に使う武器のモデル。
	UPROPERTY(Transient)
	TObjectPtr<UMeshComponent> m_weapon;
};
