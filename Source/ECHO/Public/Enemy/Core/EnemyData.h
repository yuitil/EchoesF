#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Combat/CombatTypes.h"
#include "Animation/AnimMontage.h"
#include "EnemyData.generated.h"

UENUM(BlueprintType)
enum class EEnemyAction : uint8
{
	Slash,
	Heavy,
	Chase,
	Counter,
	Dodge,
	Guard,
	Air,
	Recovery,
	Presentaiton
};


UENUM(BlueprintType)
enum class EEnemyState : uint8
{
	Alive,
	Suspended,
	Dead
};

USTRUCT(BlueprintType)
struct ECHO_API FEnemyActionData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (DisplayName = "Id"))
	FName m_actionId;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Air", meta = (DisplayName = "Air Only"))
	bool m_airOnly = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Air", meta = (DisplayName = "Target Must Be Airborne"))
	bool m_targetAir = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Air", meta = (DisplayName = "Target Must Be Grounded"))
	bool m_targetGround = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Defense", meta = (DisplayName = "Only When Target Attacks"))
	bool m_reactOnly = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Air", meta = (DisplayName = "Max Height Gap", ClampMin = "0"))
	float m_heightGap = 120.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Air", meta = (DisplayName = "Launch On Hit"))
	bool m_launch = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Air", meta = (DisplayName = "Lift Speed", ClampMin = "0"))
	float m_liftSpeed = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Air", meta = (DisplayName = "Lift Gravity", ClampMin = "0.1"))
	float m_liftGravity = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Air", meta = (DisplayName = "Lift Duration", ClampMin = "0.1", ClampMax = "3"))
	float m_liftTime = 1.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Movement", meta = (DisplayName = "Backstep Speed", ClampMin = "0"))
	float m_backSpeed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Movement", meta = (DisplayName = "Evade Direction", ClampMin = "-180", ClampMax = "180"))
	float m_evadeAngle = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Movement", meta = (DisplayName = "Backstep Start", ClampMin = "0"))
	float m_backStart = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Movement", meta = (DisplayName = "Backstep End", ClampMin = "0"))
	float m_backEnd = 0.f;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (DisplayName = "Montage"))
	TObjectPtr<UAnimMontage> m_montage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (DisplayName = "Kind"))
	EEnemyAction m_kind = EEnemyAction::Slash;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (DisplayName = "Min Distance", ClampMin = "0"))
	float m_minRange = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (DisplayName = "Max Distance", ClampMin = "0"))
	float m_maxRange = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (DisplayName = "Weight", ClampMin = "0.01"))
	float m_weight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (DisplayName = "Damage", ClampMin = "0"))
	float m_damage = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (DisplayName = "Cooldown", ClampMin = "0"))
	float m_cooldown = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (DisplayName = "Recovery", ClampMin = "0"))
	float m_recovery = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (DisplayName = "PlayRate", ClampMin = "0.1", ClampMax = "2"))
	float m_playRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (DisplayName = "RootMotionScale", ClampMin = "0", ClampMax = "1"))
	float m_rootScale = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (DisplayName = "Reviewed"))
	bool m_reviewed = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Combo", meta = (DisplayName = "HitNextId"))
	FName m_hitNextId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Combo", meta = (DisplayName = "MissNextId"))
	FName m_missNextId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Combo", meta = (DisplayName = "BlockedNextId"))
	FName m_blockedNextId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Combo", meta = (DisplayName = "ComboEntry"))
	bool m_comboEntry = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Combo", meta = (DisplayName = "ComboGap", ClampMin = "0"))
	float m_comboGap = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Combat", meta = (DisplayName = "Unblockable"))
	bool m_unblockable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Timing", meta = (DisplayName = "Dodge Window", ClampMin = "0", ClampMax = "0.5", ToolTip = "0 uses the common Perfect Dodge Window. Seconds before blade contact."))
	float m_dodgeWindow = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Timing", meta = (DisplayName = "Parry Window", ClampMin = "0", ClampMax = "0.5"))
	float m_parryWindow = 0.16f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy | Timing", meta = (DisplayName = "Deflect Window", ClampMin = "0", ClampMax = "0.5"))
	float m_deflectWindow = 0.12f;

};

UCLASS()
class ECHO_API UEnemyData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (DisplayName = "InitialHealth", ClampMin = "1"))
	float m_initialHealth = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "SpecialDefenseChacne", ClampMin = "0", ClampMax = "1"))
	float m_defenseChance = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (DisplayName = "DeathMontage"))
	TObjectPtr<UAnimMontage> m_deathMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (DisplayName = "Actions"))
	TArray<FEnemyActionData> m_actions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Combo", meta = (DisplayName = "MaxComboSteps", ClampMin = "1", ClampMax = "10"))
	int32 m_maxComboSteps = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "EnableNormalGuard"))
	bool m_useGuard = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "Guard After Hit", ClampMin = "0"))
	float m_guardMemory = 4.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "Evade Choice Chance", ClampMin = "0", ClampMax = "1"))
	float m_evadeChance = .18f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "Guard Hit Montage"))
	TObjectPtr<UAnimMontage> m_guardHit;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "Guard Hit Hold", ClampMin = "0"))
	float m_guardHold = .25f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "Counter Action"))
	FName m_counter;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "Counter Opportunity", ClampMin = "0"))
	float m_counterTime = 1.5f;



	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "Guard Attack Montages"))
	TArray<TObjectPtr<UAnimMontage>> m_guardAttacks;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Combo", meta = (DisplayName = "Repeat Weight", ClampMin = "0", ClampMax = "1"))
	float m_repeatWeight = 0.15f;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Timing", meta = (DisplayName = "Use Common Attack Rate"))
	bool m_commonRate = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Timing", meta = (DisplayName = "Attack Rate", ClampMin = "0.1", ClampMax = "2"))
	float m_attackRate = 0.65f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Timing", meta = (DisplayName = "Perfect Dodge Enabled"))
	bool m_perfectDodge = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Timing", meta = (DisplayName = "Perfect Dodge Window", ClampMin = "0.01", ClampMax = "0.5"))
	float m_dodgeWindow = 0.18f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Timing", meta = (DisplayName = "Perfect Dodge Slow Rate", ClampMin = "0.1", ClampMax = "1"))
	float m_slowRate = 0.3f;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "GuardDecisionDistance", ClampMin = "0"))
	float m_guardRange = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "DefenseFrontAngle", ClampMin = "0", ClampMax = "360"))
	float m_defenseFrontAngle = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "GuardDamageMultiplier", ClampMin = "0", ClampMax = "1"))
	float m_guardDamageScale = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "EnableSpecialDefense"))
	bool m_useSpecialDefense = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "SpecialAttemptInterval", ClampMin = "0.05"))
	float m_defenseInterval = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "DeflectWeight", ClampMin = "0"))
	float m_deflectWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "ParryWeight", ClampMin = "0"))
	float m_parryWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "DeflectMontage"))
	TObjectPtr<UAnimMontage> m_deflectMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "ParryMontage"))
	TObjectPtr<UAnimMontage> m_parryMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "AttackDeflectedMontage"))
	TObjectPtr<UAnimMontage> m_staggerMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "DeflectRecovery", ClampMin = "0.05"))
	float m_deflectRecovery = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "ParryRecovery", ClampMin = "0.05"))
	float m_parryRecovery = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "DeflectedStun", ClampMin = "0.05"))
	float m_deflectedStun = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Defense", meta = (DisplayName = "ParriedStun", ClampMin = "0.05"))
	float m_parriedStun = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | GuardBreak", meta = (DisplayName = "EnableGuardBreak"))
	bool m_useGuardBreak = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | GuardBreak", meta = (DisplayName = "HeavyHitToBreak", ClampMin = "1"))
	int32 m_heavyHitsCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | GuardBreak", meta = (DisplayName = "HeavyChainTimeout", ClampMin = "0.1"))
	float m_heavyTimeout = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | GuardBreak", meta = (DisplayName = "GuardBreakStun", ClampMin = "0.1"))
	float m_guardBreakTime = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | GuardBreak", meta = (DisplayName = "GuardBreakMontage"))
	TObjectPtr<UAnimMontage> m_guardBreakMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | GuardBreak", meta = (DisplayName = "ObservedHeavyAttackMontage"))
	TArray<TObjectPtr<UAnimMontage>> m_observedHeavyMontage;
};
