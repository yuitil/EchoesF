//SkillDataAsset.h
//強力な技（ゲージ技）の調整データ。BPのコンポーネントではなくデータアセットに置くことで、
//ヘッダー変更でBPが壊れても数値が消えないようにしている

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SkillDataAsset.generated.h"

class UAnimMontage;
class UNiagaraSystem;

// -----------------------------------------------------------------------
// 1ゲージ技：一閃（前方へ一瞬で駆け抜け、通り過ぎた敵をまとめて斬る）
// -----------------------------------------------------------------------
USTRUCT(BlueprintType)
struct FDashSlashData
{
	GENERATED_BODY()

	//消費ゲージ数
	UPROPERTY(EditAnywhere, Category = "DashSlash", meta = (ClampMin = "0"))
	int32 m_Cost = 1;

	//構え〜駆け抜けのモーション（未設定でも動く）
	UPROPERTY(EditAnywhere, Category = "DashSlash")
	TObjectPtr<UAnimMontage> Montage;

	//構えてから駆け出すまでの時間
	UPROPERTY(EditAnywhere, Category = "DashSlash", meta = (ClampMin = "0"))
	float m_WindupTime = 0.15f;

	//敵がいないときに進む距離
	UPROPERTY(EditAnywhere, Category = "DashSlash", meta = (ClampMin = "0"))
	float m_Distance = 1000.f;

	//敵がいるときに、敵を通り越して進む距離
	UPROPERTY(EditAnywhere, Category = "DashSlash", meta = (ClampMin = "0"))
	float m_Overshoot = 300.f;

	//進む距離の上限
	UPROPERTY(EditAnywhere, Category = "DashSlash", meta = (ClampMin = "0"))
	float m_MaxDistance = 1500.f;

	//狙う敵を探す範囲（ロックオンしていないとき）
	UPROPERTY(EditAnywhere, Category = "DashSlash", meta = (ClampMin = "0"))
	float m_SearchRadius = 1200.f;

	//駆け抜けにかかる時間（短いほど速い）
	UPROPERTY(EditAnywhere, Category = "DashSlash", meta = (ClampMin = "0.01"))
	float m_DashTime = 0.12f;

	//通り道の当たり判定の太さ
	UPROPERTY(EditAnywhere, Category = "DashSlash", meta = (ClampMin = "1"))
	float m_HitRadius = 120.f;

	UPROPERTY(EditAnywhere, Category = "DashSlash", meta = (ClampMin = "0"))
	float m_Damage = 80.f;

	//駆け抜けてから斬撃が入るまでの間（遅れて斬れる演出）
	UPROPERTY(EditAnywhere, Category = "DashSlash", meta = (ClampMin = "0"))
	float m_DamageDelay = 0.15f;

	//技が終わってから動けるようになるまで
	UPROPERTY(EditAnywhere, Category = "DashSlash", meta = (ClampMin = "0"))
	float m_RecoveryTime = 0.35f;
};

// -----------------------------------------------------------------------
// 3ゲージ技：テンペスト（敵のまわりを何度も飛び交いながら斬りまくる範囲攻撃）
// -----------------------------------------------------------------------
USTRUCT(BlueprintType)
struct FTempestData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Tempest", meta = (ClampMin = "0"))
	int32 m_Cost = 3;

	//true：近くの敵を中心に飛び回る　false：発動した場所を中心に斬りまくる
	UPROPERTY(EditAnywhere, Category = "Tempest")
	bool m_bCenterOnTarget = false;

	//技の間、カメラを発動した位置に止めておく
	UPROPERTY(EditAnywhere, Category = "Tempest")
	bool m_bFreezeCamera = true;

	//四方八方に斬るモーション。斬る瞬間に「SkillHit」、締めの一撃に「SkillFinish」の通知を置く
	UPROPERTY(EditAnywhere, Category = "Tempest|Animation")
	TObjectPtr<UAnimMontage> Montage;

	//true：通知が来た瞬間のキャラの位置を中心に当てる　false：発動した場所を中心に当てる
	UPROPERTY(EditAnywhere, Category = "Tempest|Animation")
	bool m_bHitAroundCharacter = true;

	// --- 俯瞰カメラ ---
	//技の間、カメラを引いて上から見下ろす
	UPROPERTY(EditAnywhere, Category = "Tempest|Camera")
	bool m_bOverheadCamera = true;

	//俯瞰のときのカメラの距離
	UPROPERTY(EditAnywhere, Category = "Tempest|Camera", meta = (ClampMin = "0"))
	float m_CameraArmLength = 900.f;

	//見下ろす角度（マイナスで下向き）
	UPROPERTY(EditAnywhere, Category = "Tempest|Camera", meta = (ClampMin = "-89", ClampMax = "0"))
	float m_CameraPitch = -50.f;

	//切り替えにかける時間（秒）
	UPROPERTY(EditAnywhere, Category = "Tempest|Camera", meta = (ClampMin = "0.01"))
	float m_CameraBlendTime = 0.25f;

	//カメラを固定する位置を、キャラの正面方向へずらす量
	UPROPERTY(EditAnywhere, Category = "Tempest|Camera")
	float m_CameraForwardOffset = 120.f;

	//1撃ごとに順番に再生する斬りモーション（空でも動く）
	UPROPERTY(EditAnywhere, Category = "Tempest")
	TArray<TObjectPtr<UAnimMontage>> SlashMontages;

	//最後の締めのモーション
	UPROPERTY(EditAnywhere, Category = "Tempest")
	TObjectPtr<UAnimMontage> FinishMontage;

	//飛び交う回数（締めは含まない）
	UPROPERTY(EditAnywhere, Category = "Tempest", meta = (ClampMin = "1"))
	int32 m_HitCount = 8;

	//1撃ごとの間隔
	UPROPERTY(EditAnywhere, Category = "Tempest", meta = (ClampMin = "0.02"))
	float m_Interval = 0.18f;

	//1回の移動にかかる時間
	UPROPERTY(EditAnywhere, Category = "Tempest", meta = (ClampMin = "0.01"))
	float m_DashTime = 0.08f;

	//中心にする敵を探す範囲
	UPROPERTY(EditAnywhere, Category = "Tempest", meta = (ClampMin = "0"))
	float m_SearchRadius = 900.f;

	//中心のまわりを飛び回る半径
	UPROPERTY(EditAnywhere, Category = "Tempest", meta = (ClampMin = "0"))
	float m_OrbitRadius = 250.f;

	//1撃ごとの範囲（中心から）
	UPROPERTY(EditAnywhere, Category = "Tempest", meta = (ClampMin = "1"))
	float m_HitRadius = 450.f;

	UPROPERTY(EditAnywhere, Category = "Tempest", meta = (ClampMin = "0"))
	float m_Damage = 20.f;

	UPROPERTY(EditAnywhere, Category = "Tempest", meta = (ClampMin = "1"))
	float m_FinishRadius = 600.f;

	UPROPERTY(EditAnywhere, Category = "Tempest", meta = (ClampMin = "0"))
	float m_FinishDamage = 80.f;

	//締めの一撃で敵を外側へ飛ばす強さ（横方向）
	UPROPERTY(EditAnywhere, Category = "Tempest", meta = (ClampMin = "0"))
	float m_FinishKnockback = 1200.f;

	//締めの一撃で敵を浮かせる強さ（上方向）
	UPROPERTY(EditAnywhere, Category = "Tempest", meta = (ClampMin = "0"))
	float m_FinishKnockUp = 300.f;

	UPROPERTY(EditAnywhere, Category = "Tempest", meta = (ClampMin = "0"))
	float m_RecoveryTime = 0.4f;
};

// -----------------------------------------------------------------------
// 最終奥義：ため斬り（エクスカリバー型）
// ためた段数ぶんゲージを消費し、向いている方向へV字に広がる光の斬撃を放つ
// 段数が上がるほど、長く・太く・多段になる
// -----------------------------------------------------------------------
USTRUCT(BlueprintType)
struct FUltimateLevelData
{
	GENERATED_BODY()

	//放つときのモーション（空なら ChargeMontage の Release セクションへ進む）
	UPROPERTY(EditAnywhere, Category = "Ultimate")
	TObjectPtr<UAnimMontage> Montage;

	//この段階のとき、剣にまとわせるエフェクト（段階が上がると差し替わる）
	UPROPERTY(EditAnywhere, Category = "Ultimate")
	TObjectPtr<UNiagaraSystem> ChargeEffect;

	//放った瞬間の光の斬撃エフェクト（キャラの位置・向きに出す）
	UPROPERTY(EditAnywhere, Category = "Ultimate")
	TObjectPtr<UNiagaraSystem> BeamEffect;

	//斬撃が届く長さ
	UPROPERTY(EditAnywhere, Category = "Ultimate", meta = (ClampMin = "1"))
	float m_Length = 1000.f;

	//V字の広がり（片側の角度）
	UPROPERTY(EditAnywhere, Category = "Ultimate", meta = (ClampMin = "0", ClampMax = "60"))
	float m_HalfAngle = 10.f;

	//根元の幅（片側）。目の前の敵を取りこぼさないため
	UPROPERTY(EditAnywhere, Category = "Ultimate", meta = (ClampMin = "0"))
	float m_BaseWidth = 120.f;

	//上下の判定の高さ（片側）
	UPROPERTY(EditAnywhere, Category = "Ultimate", meta = (ClampMin = "1"))
	float m_HalfHeight = 300.f;

	//1ヒットあたりのダメージ
	UPROPERTY(EditAnywhere, Category = "Ultimate", meta = (ClampMin = "0"))
	float m_Damage = 50.f;

	//ヒット数（光が当たり続ける間の多段ヒット）
	UPROPERTY(EditAnywhere, Category = "Ultimate", meta = (ClampMin = "1"))
	int32 m_HitCount = 1;

	UPROPERTY(EditAnywhere, Category = "Ultimate", meta = (ClampMin = "0.02"))
	float m_HitInterval = 0.1f;

	UPROPERTY(EditAnywhere, Category = "Ultimate", meta = (ClampMin = "0"))
	float m_RecoveryTime = 0.6f;
};

USTRUCT(BlueprintType)
struct FUltimateData
{
	GENERATED_BODY()

	//ため中のモーション
	//「掲げる → ため(ループ) → 振り下ろし」を1本のモンタージュにして、
	//振り下ろしを m_ReleaseSectionName のセクションにしておくと、離した瞬間にそこへ飛ぶ
	UPROPERTY(EditAnywhere, Category = "Ultimate")
	TObjectPtr<UAnimMontage> ChargeMontage;

	//振り下ろしのセクション名
	UPROPERTY(EditAnywhere, Category = "Ultimate")
	FName m_ReleaseSectionName = TEXT("Release");

	//ため中のエフェクトを付ける剣のソケット（None なら剣の原点）
	UPROPERTY(EditAnywhere, Category = "Ultimate")
	FName m_ChargeEffectSocket;

	//1段階上がるのにかかる時間
	UPROPERTY(EditAnywhere, Category = "Ultimate", meta = (ClampMin = "0.05"))
	float m_ChargeStepTime = 0.5f;

	//ため中の移動速度の倍率
	UPROPERTY(EditAnywhere, Category = "Ultimate", meta = (ClampMin = "0", ClampMax = "1"))
	float m_ChargeMoveSpeedRate = 0.3f;

	//離してから斬撃が出るまで（振り下ろした瞬間に合わせる）
	UPROPERTY(EditAnywhere, Category = "Ultimate", meta = (ClampMin = "0"))
	float m_ReleaseDelay = 0.25f;

	//段階ごとの性能 [0]=1段 … [4]=5段（要素数が最大段数になる）
	UPROPERTY(EditAnywhere, Category = "Ultimate")
	TArray<FUltimateLevelData> Levels;
};

UCLASS(BlueprintType)
class ECHO_API USkillDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	USkillDataAsset();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	FDashSlashData DashSlash;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	FTempestData Tempest;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	FUltimateData Ultimate;
};
