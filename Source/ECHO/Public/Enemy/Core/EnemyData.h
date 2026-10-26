#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Combat/CombatTypes.h"
#include "Animation/AnimMontage.h"
#include "GameplayTagContainer.h"
#include "EnemyData.generated.h"

//敵が選ぶ行動の種類。
UENUM(BlueprintType)
enum class EEnemyAction : uint8
{
	//通常の斬撃。
	Slash,
	//強攻撃。
	Heavy,
	//追跡用の攻撃。
	Chase,
	//反撃。
	Counter,
	//回避。
	Dodge,
	//通常のガード。
	Guard,
	//空中の攻撃。
	Air,
	//攻撃後の戻り動作。
	Recovery,
	//演出用の動作。
	Presentaiton
};

//敵が戦闘できるかを示す状態。
UENUM(BlueprintType)
enum class EEnemyState : uint8
{
	//戦闘できる状態。
	Alive,
	//行動を一時停止中。
	Suspended,
	//死亡した状態。
	Dead
};

//一つの攻撃の条件と再生設定。
USTRUCT(BlueprintType)
struct ECHO_API FEnemyActionData
{
	GENERATED_BODY()

	//攻撃を見分ける名前。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (DisplayName = "Id"))
	FName m_actionId;

    //必殺技として抽選するか。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Combat", meta=(DisplayName="Special Attack"))
    bool m_special = false;

    //攻撃判定の根元のボーン。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Combat", meta=(DisplayName="Hit Base Bone"))
    FName m_hitBase;

    //攻撃判定の先端のボーン。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Combat", meta=(DisplayName="Hit Tip Bone"))
    FName m_hitTip;

    //根元のボーンから判定をずらす位置。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Combat", meta=(DisplayName="Hit Base Offset"))
    FVector m_hitBaseOffset = FVector::ZeroVector;

    //先端のボーンから剣先までの位置。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Combat", meta=(DisplayName="Hit Tip Offset"))
    FVector m_hitTipOffset = FVector::ZeroVector;

    //攻撃中に付けるタグ。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|GAS", meta=(DisplayName="Active Tags"))
    FGameplayTagContainer m_activeTags;

    //攻撃を使うために必要なタグ。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|GAS", meta=(DisplayName="Required Tags"))
    FGameplayTagContainer m_requiredTags;

    //攻撃を使えなくするタグ。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|GAS", meta=(DisplayName="Blocked Tags"))
    FGameplayTagContainer m_blockedTags;

	//自分が空中にいる時だけ使うか。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Air", meta = (DisplayName = "Air Only"))
	bool m_airOnly = false;

	//相手が空中にいる時だけ使うか。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Air", meta = (DisplayName = "Target Must Be Airborne"))
	bool m_targetAir = false;

	//空中の命中時に、相手と上下の速さを合わせる。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Air", meta = (DisplayName = "Follow Air Hit"))
	bool m_airFollow = false;

	//相手が地上にいる時だけ使うか。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Air", meta = (DisplayName = "Target Must Be Grounded"))
	bool m_targetGround = false;

	//相手の攻撃に反応する時だけ使うか。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Defense", meta = (DisplayName = "Only When Target Attacks"))
	bool m_reactOnly = false;

	//攻撃できる高さの差。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Air", meta = (DisplayName = "Max Height Gap", ClampMin = "0"))
	float m_heightGap = 120.f;

	//命中した相手を打ち上げるか。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Air", meta = (DisplayName = "Launch On Hit"))
	bool m_launch = false;

	//打ち上げる速度。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Air", meta = (DisplayName = "Lift Speed", ClampMin = "0"))
	float m_liftSpeed = 0.f;

	//空中コンボ中の重力倍率。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Air", meta = (DisplayName = "Lift Gravity", ClampMin = "0.1"))
	float m_liftGravity = 1.f;

	//空中コンボ用の重力を保つ時間。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Air", meta = (DisplayName = "Lift Duration", ClampMin = "0.1", ClampMax = "3"))
	float m_liftTime = 1.5f;

	//回避中の移動速度。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Movement", meta = (DisplayName = "Backstep Speed", ClampMin = "0"))
	float m_backSpeed = 0.f;

	//正面を基準にした回避方向。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Movement", meta = (DisplayName = "Evade Direction", ClampMin = "-180", ClampMax = "180"))
	float m_evadeAngle = 180.f;

	//回避移動を始める再生位置。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Movement", meta = (DisplayName = "Backstep Start", ClampMin = "0"))
	float m_backStart = 0.f;

	//回避移動を終える再生位置。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Movement", meta = (DisplayName = "Backstep End", ClampMin = "0"))
	float m_backEnd = 0.f;

	//再生するモンタージュ。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (DisplayName = "Montage"))
	TObjectPtr<UAnimMontage> m_montage;

	//行動の種類。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (DisplayName = "Kind"))
	EEnemyAction m_kind = EEnemyAction::Slash;

	//攻撃できる最小距離。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (DisplayName = "Min Distance", ClampMin = "0"))
	float m_minRange = 0.0f;

	//攻撃できる最大距離。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (DisplayName = "Max Distance", ClampMin = "0"))
	float m_maxRange = 250.0f;

	//攻撃の抽選で選ばれやすくする値。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (DisplayName = "Weight", ClampMin = "0"))
	float m_weight = 1.0f;

	//与えるダメージ。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (DisplayName = "Damage", ClampMin = "0"))
	float m_damage = 10.0f;

	//再使用までの待ち時間。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (DisplayName = "Cooldown", ClampMin = "0"))
	float m_cooldown = 2.0f;

	//攻撃後の待ち時間。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (DisplayName = "Recovery", ClampMin = "0"))
	float m_recovery = 0.6f;

	//この攻撃の再生速度。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (DisplayName = "PlayRate", ClampMin = "0.1", ClampMax = "2"))
	float m_playRate = 1.0f;

	//アニメーションによる移動の倍率。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (DisplayName = "RootMotionScale", ClampMin = "0", ClampMax = "1"))
	float m_rootScale = 0.0f;

	//内容を確認済みの攻撃か。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (DisplayName = "Reviewed"))
	bool m_reviewed = false;

	//命中した時につなぐ攻撃名。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Combo", meta = (DisplayName = "HitNextId"))
	FName m_hitNextId;

    //命中後の候補。空欄ならHitNextIdを使う。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Combo", meta=(DisplayName="Hit Follow Ups"))
    TArray<FName> m_hitChoices;

	//外れた時につなぐ攻撃名。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Combo", meta = (DisplayName = "MissNextId"))
	FName m_missNextId;

	//防がれた時につなぐ攻撃名。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Combo", meta = (DisplayName = "BlockedNextId"))
	FName m_blockedNextId;

	//コンボの最初に選べるか。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Combo", meta = (DisplayName = "ComboEntry"))
	bool m_comboEntry = true;

	//コンボ間の待ち時間。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Combo", meta = (DisplayName = "ComboGap", ClampMin = "0"))
	float m_comboGap = 0.15f;

	//ガードできない攻撃か。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Combat", meta = (DisplayName = "Unblockable"))
	bool m_unblockable = false;

	//ジャスト回避を受け付ける時間の幅。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Timing", meta = (DisplayName = "Dodge Window", ClampMin = "0", ClampMax = "0.5", ToolTip = "0 uses the common Perfect Dodge Window. Seconds before blade contact."))
	float m_dodgeWindow = 0.f;

	//受け流しを受け付ける時間の幅。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Timing", meta = (DisplayName = "Parry Window", ClampMin = "0", ClampMax = "0.5"))
	float m_parryWindow = 0.16f;

	//弾きを受け付ける時間の幅。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Timing", meta = (DisplayName = "Deflect Window", ClampMin = "0", ClampMax = "0.5"))
	float m_deflectWindow = 0.12f;

};

//敵の攻撃や防御をエディターで設定する。
UCLASS()
class ECHO_API UEnemyData : public UDataAsset
{
	GENERATED_BODY()

public:
	//戦闘開始時の体力。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (DisplayName = "InitialHealth", ClampMin = "1"))
	float m_initialHealth = 600.0f;

	//必殺技を選ぶ確率。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Combat", meta=(ClampMin="0", ClampMax="1", DisplayName="Special Attack Chance"))
	float m_specialChance = .2f;

	//特殊防御を選ぶ確率。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "SpecialDefenseChacne", ClampMin = "0", ClampMax = "1"))
	float m_defenseChance = 0.2f;

	//死亡時に再生するモンタージュ。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (DisplayName = "DeathMontage"))
	TObjectPtr<UAnimMontage> m_deathMontage;

	//この敵が使う攻撃の一覧。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (DisplayName = "Actions"))
	TArray<FEnemyActionData> m_actions;

	//コンボを続ける最大段数。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Combo", meta = (DisplayName = "MaxComboSteps", ClampMin = "1", ClampMax = "10"))
	int32 m_maxComboSteps = 3;

    //打ち上げ成功後、別の打ち上げも待たせる秒数。
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Enemy|Air", meta=(DisplayName="Launch Cooldown", ClampMin="0"))
    float m_liftCooldown = 0.f;

	//通常のガードを使うか。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "EnableNormalGuard"))
	bool m_useGuard = true;

	//攻撃された相手を覚えておく時間。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "Guard After Hit", ClampMin = "0"))
	float m_guardMemory = 4.f;

	//回避を選ぶ確率。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "Evade Choice Chance", ClampMin = "0", ClampMax = "1"))
	float m_evadeChance = .18f;

	//ガードで受けた時のモンタージュ。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "Guard Hit Montage"))
	TObjectPtr<UAnimMontage> m_guardHit;

	//ガード姿勢を保つ時間。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "Guard Hit Hold", ClampMin = "0"))
	float m_guardHold = .25f;

	//反撃に使う攻撃名。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "Counter Action"))
	FName m_counter;

	//反撃を待てる時間。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "Counter Opportunity", ClampMin = "0"))
	float m_counterTime = 1.5f;

	//相手が攻撃中かを調べるモンタージュ。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "Guard Attack Montages"))
	TArray<TObjectPtr<UAnimMontage>> m_guardAttacks;

	//同じ攻撃を続ける時の選ばれやすさ。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Combo", meta = (DisplayName = "Repeat Weight", ClampMin = "0", ClampMax = "1"))
	float m_repeatWeight = 0.15f;

	//攻撃の再生速度を共通設定にするか。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Timing", meta = (DisplayName = "Use Common Attack Rate"))
	bool m_commonRate = false;

	//共通の攻撃再生速度。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Timing", meta = (DisplayName = "Attack Rate", ClampMin = "0.1", ClampMax = "2"))
	float m_attackRate = 0.65f;

	//ジャスト回避による減速を使うか。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Timing", meta = (DisplayName = "Perfect Dodge Enabled"))
	bool m_perfectDodge = true;

	//ジャスト回避を受け付ける時間の幅。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Timing", meta = (DisplayName = "Perfect Dodge Window", ClampMin = "0.01", ClampMax = "0.5"))
	float m_dodgeWindow = 0.18f;

	//ジャスト回避された時の再生速度の倍率。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Timing", meta = (DisplayName = "Perfect Dodge Slow Rate", ClampMin = "0.1", ClampMax = "1"))
	float m_slowRate = 0.3f;

	//ガードを判断する距離。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "GuardDecisionDistance", ClampMin = "0"))
	float m_guardRange = 250.f;

	//防御できる正面の角度。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "DefenseFrontAngle", ClampMin = "0", ClampMax = "360"))
	float m_defenseFrontAngle = 120.f;

	//ガード時に受けるダメージの倍率。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "GuardDamageMultiplier", ClampMin = "0", ClampMax = "1"))
	float m_guardDamageScale = 0.35f;

	//弾きや受け流しを使うか。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "EnableSpecialDefense"))
	bool m_useSpecialDefense = true;

	//特殊防御を試す間隔。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "SpecialAttemptInterval", ClampMin = "0.05"))
	float m_defenseInterval = 1.0f;

	//弾きの選ばれやすさ。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "DeflectWeight", ClampMin = "0"))
	float m_deflectWeight = 1.0f;

	//受け流しの選ばれやすさ。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "ParryWeight", ClampMin = "0"))
	float m_parryWeight = 1.0f;

	//弾きに使うモンタージュ。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "DeflectMontage"))
	TObjectPtr<UAnimMontage> m_deflectMontage;

	//受け流しに使うモンタージュ。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "ParryMontage"))
	TObjectPtr<UAnimMontage> m_parryMontage;

	//よろけに使うモンタージュ。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "AttackDeflectedMontage"))
	TObjectPtr<UAnimMontage> m_staggerMontage;

	//弾いた後の待ち時間。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "DeflectRecovery", ClampMin = "0.05"))
	float m_deflectRecovery = 0.6f;

	//受け流した後の待ち時間。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "ParryRecovery", ClampMin = "0.05"))
	float m_parryRecovery = 0.35f;

	//弾かれた後の無防備な時間。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "DeflectedStun", ClampMin = "0.05"))
	float m_deflectedStun = 0.9f;

	//受け流された後の無防備な時間。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "ParriedStun", ClampMin = "0.05"))
	float m_parriedStun = 0.5f;

	//連続強攻撃によるガード崩れを使うか。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | GuardBreak", meta = (DisplayName = "EnableGuardBreak"))
	bool m_useGuardBreak = true;

	//ガード崩れに必要な強攻撃の回数。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | GuardBreak", meta = (DisplayName = "HeavyHitToBreak", ClampMin = "1"))
	int32 m_heavyHitsCount = 2;

	//強攻撃を連続として扱う時間。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | GuardBreak", meta = (DisplayName = "HeavyChainTimeout", ClampMin = "0.1"))
	float m_heavyTimeout = 3.0f;

	//ガード崩れで無防備になる時間。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | GuardBreak", meta = (DisplayName = "GuardBreakStun", ClampMin = "0.1"))
	float m_guardBreakTime = 2.0f;

	//ガード崩れに使うモンタージュ。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | GuardBreak", meta = (DisplayName = "GuardBreakMontage"))
	TObjectPtr<UAnimMontage> m_guardBreakMontage;

	//相手の強攻撃として扱うモンタージュ。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | GuardBreak", meta = (DisplayName = "ObservedHeavyAttackMontage"))
	TArray<TObjectPtr<UAnimMontage>> m_observedHeavyMontage;
};
