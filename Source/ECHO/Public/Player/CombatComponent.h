//CombatComponent.h
//コンバットコンポーネントヘッダー
//
//戦闘統括コンポーネント
//コンボの状態管理、現在装備中の武器
//回避、アクショントリガー、無敵時間

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

//敵にヒットした際、キャラクタークラスにダメージを通知するデリゲート
DECLARE_MULTICAST_DELEGATE_OneParam(FOnHitEnemyDelegate, float);

USTRUCT(BlueprintType)
struct FComboStepData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	UAnimMontage* Montage = nullptr;

	//攻撃判定の球体の半径
	UPROPERTY(EditAnywhere)
	float HitRadius = 60.f;

	//攻撃判定を前方へ伸ばす長さ
	UPROPERTY(EditAnywhere)
	float HitRange = 100.f;

	//敵に与える基本ダメージ
	UPROPERTY(EditAnywhere)
	float Damage = 20.f;

	//吹き飛ばし力（0なら吹き飛ばしなし）
	UPROPERTY(EditAnywhere)
	float LaunchForce = 0.f;

	//この攻撃が敵1体に当たったときに得るエネルギー
	UPROPERTY(EditAnywhere)
	float m_EnergyGain = 10.f;

	//コンボ入力受付開始タイミング
	UPROPERTY(EditAnywhere)
	float ComboWindowStart = 0.5f;

	//コンボリセットまでの時間
	UPROPERTY(EditAnywhere)
	float ComboResetTime = 1.0f;

	//アニメーションの再生速度
	UPROPERTY(EditAnywhere)
	float m_PlayRate = 1.f;
	//アニメーションの再生開始位置(秒)、振りかぶりを省略できる
	UPROPERTY(EditAnywhere)
	float m_StartTime = 0.f;

	//敵がいないときの踏み込み距離
	UPROPERTY(EditAnywhere)
	float m_LungeDistance = 40.f;
	//敵に向かって詰める最大距離
	UPROPERTY(EditAnywhere)
	float m_LungeMaxDistance = 250.f;
	//踏み込みにかける時間（0なら踏み込まない）
	UPROPERTY(EditAnywhere)
	float m_LungeDuration = 0.15f;

	//空中攻撃時の前進距離（0なら前進しない）
	UPROPERTY(EditAnywhere)
	float m_AirLungeDistance = 30.f;
};

class UComboDataAsset;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ECHO_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatComponent();

	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	//攻撃
	void ExecuteAttack();
	//回避（OverrideDirection を渡すとその方向へ回避。AI行動用）
	void ExecuteDodge(const FVector& OverrideDirection = FVector::ZeroVector);
	//キャラクターの着地時などに、空中回避の使用済みフラグをリセットする関数
	void ResetAirDodge();

	//着地時に呼ぶ：空中攻撃の使用済みフラグをリセット
	void ResetAirAttack();

	//AnimationNotifyから呼ばれる、当たり判定
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void CheckHit();

	//被弾などで攻撃を強制的に中断する
	void CancelAttack() { OnComboResetTimeout(); }

	//AnimationNotifyから呼ばれる、コンボの次の段階への移行を受け付ける関数
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void OpenComboWindow();

	//AnimationNotifyから呼ばれる、コンボ受付終了関数
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void CloseComboWindow();

	//コンボの段階を取得する関数、AI行動用
	int32 GetCurrentComboIndex() const { return m_iCurrentComboIndex; }

	FOnHitEnemyDelegate OnHitEnemy;

	//攻撃中に移動を制限するための関数
	bool IsPlayingAttackMontage() const;

	//プレイヤーが向きたい方向を渡す関数
	void SetDesiredDirection(const FVector& Dir) { m_DesiredDirection = Dir; }

protected:
	//BPからコンボルートや各段数のパラメータを設定する配列
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo")
	TArray<FComboStepData> ComboSteps;

	//コンボの設定データ（設定されていれば、こちらの内容を ComboSteps として使う）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo")
	TObjectPtr<UComboDataAsset> ComboData;

	//攻撃開始時に入力方向へ向き直るまでの時間（0なら瞬時）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo")
	float m_AttackTurnDuration;

	//吸い付き対象を探す距離
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Assist")
	float m_AssistRange = 400.f;
	//吸い付く角度（基準方向から左右それぞれ何度まで）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Assist")
	float m_AssistHalfAngle = 15.f;
	//敵の中心からこの距離で踏み込みを止める（敵にめり込まないように）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Assist")
	float m_LungeStopDistance = 150.f;
	//吸い付き対象をデバッグ表示する
	UPROPERTY(EditAnywhere, Category = "Combat|Assist")
	bool m_bDebugAssist = false;

	//回避時に押し出す瞬発力な力
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Dodge")
	float m_DodgeForce;

	//回避アクションの持続時間
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Dodge")
	float m_DodgeDuration;

	//次に回避が使用可能になるまでのクールダウン
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Dodge")
	float m_DodgeCooldown;

	//攻撃の当たり判定（黄色・赤の球）を表示する
	UPROPERTY(EditAnywhere, Category = "Combat|Debug")
	bool m_bDebugDrawHit = false;

private:
	//攻撃方向を決める（吸い付き対象がいればOutTargetに入る）
	FVector ResolveAttackDirection(ACharacter* Owner, AActor*& OutTarget) const;
	//基準方向の近くにいる敵を探す
	AActor* FindAssistTarget(ACharacter* Owner, const FVector& BaseDir) const;
	//指定方向への向き直りを開始
	void StartTurn(ACharacter* Owner, const FVector& Dir);
	//踏み込みを開始
	void StartLunge(ACharacter* Owner, const FVector& Dir, const AActor* Target, const FComboStepData& Step);

	// --- 空中攻撃 ---
	bool m_bAirAttack = false;        //今のコンボが空中で始まったか
	bool m_bAirComboAllowed = false;  //空中で次の段に進めるか（前の段が当たったか）
	bool m_bHasAirAttacked = false;   //この滞空中にすでに空中攻撃したか

	void SetAirHover(bool bEnable);
	void EndAirAttack();
	void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// --- コンボ制御の内容状態変数 ---
	int32 m_iCurrentComboIndex;     //現在コンボの何段目か
	bool m_bIsAttacking;            //現在攻撃モーション中か
	bool m_bComboWindowOpen;        //現在次のコンボ入力を受ける状態か
	bool m_bComboInputBuffered;     //受付窓口が開く前に攻撃ボタンが押された場合の「先行入力」

	FVector m_DesiredDirection = FVector::ZeroVector;   //攻撃開始時に向く方向
	FRotator m_TurnStartRot;     //回転開始時の向き
	FRotator m_TurnTargetRot;    //回転後の向き
	float m_TurnElapsed;   //回転開始からの経過時間

	// --- タイマーハンドル ---
	FTimerHandle ComboResetTimerHandle;
	FTimerHandle DodgeTimerHandle;
	FTimerHandle DodgeCoolDownTimerHandle;

	// --- 回避制御の内部状態変数 ---
	bool m_bCanDodge;               //現在回避可能かどうか
	bool m_bHasAirDodged;           //空中回避を既に実行したか
	bool m_bIsDodging;

	//多段ヒット防止、当たった敵を記録するリスト
	TArray<AActor*> HitActorsThisAttack;

	//指定されたコンボ段数のアニメーション再生とデータ初期化を行う関数
	void ExecuteComboStep(int32 StepIndex);
	//コンボ入力が間に合わずにタイマーが満了した際のリセット関数
	void OnComboResetTimeout();
	//回避時間が終了した際に物理状態を元に戻す関数
	void EndDodge();
	//クールダウンタイマー終了時に回避状態に戻す関数
	void ResetDodgeCooldown();


public:
	bool IsAttacking() const { return m_bIsAttacking; }
	bool IsDodging()   const { return m_bIsDodging; }
};
