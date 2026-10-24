#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MinionFormation.generated.h"
class AEnemyCharacterBase;

//待機する敵を周囲へ動かし、攻撃の順番を守る。
UCLASS(ClassGroup=(Enemy), meta=(BlueprintSpawnableComponent))
class ECHO_API UMinionFormation : public UActorComponent
{
	GENERATED_BODY()

public:
	//待機する敵を周囲へ動かし、攻撃の順番を守る。
	UMinionFormation();

	//攻撃してきた相手への反撃待ちか調べる。
	bool IsProvoked() const;

	//反撃する相手を返す。
	AActor* GetCounterTarget() const { return m_counterTarget.Get(); }

	//反撃の予約を使用する。
	void ConsumeCounter() { m_counterUntil = 0.; }

	//この部品を持つ敵を返す。
	AEnemyCharacterBase* Enemy() const;

	//攻撃役として前に出る準備をする。
	void PrepareAttack() { SetOrbit(false); }

public:
	//攻撃してきた相手への反撃を予約する。
	UFUNCTION(BlueprintCallable, Category="Formation|Counter")
	void Provoke(AActor* attacker);

	//反撃を開始する時間か調べる。
	UFUNCTION(BlueprintPure, Category="Formation|Counter")
	bool WantsCounter() const;

	//攻撃に参加する順番が来ているか返す。
	UFUNCTION(BlueprintPure, Category="Formation")
	bool HasTurn() const { return !m_enabled || m_hasTurn; }

	//攻撃する役目を終えて待機へ戻る。
	UFUNCTION(BlueprintCallable, Category="Formation")
	void ReleaseTurn();

public:
	//攻撃の順番を受け取った時刻。
	double m_granted = 0.;

	//次に攻撃できる時刻。
	double m_ready = 0.;

	//隙を見つけ始めた時刻。
	double m_gapSince=-1.;

public:
	//この機能を使うか。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Formation")
	bool m_enabled = true;

	//攻撃担当を維持するか。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Formation")
	bool m_holdDuel = true;

	//相手に近い敵を優先するか。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Formation")
	bool m_nearestFirst = true;

	//もう一体の加勢を許すか。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Formation|Counter")
	bool m_allowSupport = false;

	//攻撃されてから反撃までの待ち時間。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Formation|Counter", meta=(ClampMin="0.2"))
	float m_counterDelay = .7f;

	//反撃を待てる時間。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Formation|Counter", meta=(ClampMin="1"))
	float m_counterTime = 8.f;

	//同時に攻撃できる敵の数。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Formation", meta=(ClampMin="1",ClampMax="2"))
	int32 m_attackers = 2;

	//判定に使う半径。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Formation", meta=(ClampMin="200"))
	float m_radius = 450.f;

	//相手の周囲を回る速さ。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Formation", meta=(ClampMin="0",ClampMax="45"))
	float m_orbitRate = 12.f;

	//二体目が攻撃するまでの待ち時間。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Formation", meta=(ClampMin="0.2"))
	float m_secondDelay = 1.5f;

	//攻撃後に順番を待つ時間。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Formation", meta=(ClampMin="0"))
	float m_restTime = 3.f;

	//一回の攻撃担当を続ける制限時間。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Formation", meta=(ClampMin="1"))
	float m_turnTimeout = 8.f;

	//戦闘に参加できる距離。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Formation", meta=(ClampMin="100"))
	float m_joinRange=1800.f;

	//加勢するために空けたい角度。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Formation", meta=(ClampMin="30",ClampMax="150"))
	float m_gapAngle=60.f;

	//相手の隙と判断する角度。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Formation", meta=(ClampMin="0",ClampMax="180"))
	float m_openAngle=70.f;

	//攻撃する順番を持っているか。
	UPROPERTY(BlueprintReadOnly, Category="Formation")
	bool m_hasTurn = false;

	//周囲で待機する移動先。
	UPROPERTY(BlueprintReadOnly, Category="Formation")
	FVector m_ringGoal;

	//攻撃前に様子を見る時間。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Formation|Timing", meta=(ClampMin="0"))
	float m_observeTime=2.f;

	//隙が続くのを確認する時間。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Formation|Timing", meta=(ClampMin="0.1"))
	float m_gapTime=1.f;

	//同じ相手へ次の敵が攻撃するまでの時間。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Formation|Timing", meta=(ClampMin="0"))
	float m_groupRest=1.5f;

	//周回中の移動速度。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Formation|Movement", meta=(ClampMin="50"))
	float m_orbitSpeed=180.f;

protected:
	//ゲーム開始時に必要な状態を準備する。
	virtual void BeginPlay() override;

	//終了時にタイマーや登録を解除する。
	virtual void EndPlay(const EEndPlayReason::Type _reason) override;

	//経過時間に合わせて敵の状態を更新する。
	virtual void TickComponent(float _delta, ELevelTick _type, FActorComponentTickFunction* _fn) override;

private:
	//相手を向いた周回移動を切り替える。
	void SetOrbit(bool _enabled);

private:
	//攻撃されたため反撃する相手。
	TWeakObjectPtr<AActor> m_counterTarget;

	//反撃できる期限。
	double m_counterUntil = 0.;

	//反撃を始められる時刻。
	double m_counterReady = 0.;

	//周回用の移動設定を使っているか。
	bool m_orbit=false;

	//変更前の移動方向へ向く設定。
	bool m_oldOrient=false;

	//変更前のAIの向きに合わせる設定。
	bool m_oldDesired=false;

	//変更前の左右の向きの設定。
	bool m_oldYaw=false;

	//変更前の移動速度。
	float m_oldSpeed=0.f;

	//現在狙っている相手。
	TWeakObjectPtr<AActor> m_target;

	//次に周回の移動先を更新する時刻。
	double m_nextMove = 0.;
};
