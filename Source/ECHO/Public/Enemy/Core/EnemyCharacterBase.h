#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Enemy/Core/EnemyData.h"
#include "Combat/EBDefenseFeedback.h"
#include "EnemyCharacterBase.generated.h"

class UAbilitySystemComponent;
class UCombatAbilitySystemComponent;
class UCombatAttributeSet;
class UCombat;
class UEnemyTraversal;
class UDefense;
//
class UEnemyTargeting;
//
class UGameplayAbility;
class UBehaviorTree;
struct FOnAttributeChangeData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FEnemyHealthEvent, float, health, float, maxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEnemyStateEvent, EEnemyState, state);

UCLASS()
class ECHO_API AEnemyCharacterBase : public ACharacter, public IAbilitySystemInterface, public IEBDefenseFeedback
{
	GENERATED_BODY()

public:
	AEnemyCharacterBase();
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual float TakeDamage(float _damage, const FDamageEvent& _damageEvent, AController* _instigator, AActor* _causer) override;
	virtual void AttackDefended_Implementation(AActor* _defender, EDefenseResult _result) override;
	virtual float GetSpecialDefenseChance() const;
	virtual void ObserveTarget(AActor* _target){}
	static FName SelectComboBranch(const FEnemyActionData& _action, bool _hit, bool _blocked);
	void RecordAttackHit();
	
	bool WasAttackBlocked() const { return m_bActionBlocked; }
	
	const FEnemyActionData* GetActiveAction() const;
	
	UFUNCTION(BlueprintNativeEvent, Category = "Enemy | Defense")
	EDefenseResult ResolveTargetDefense(AActor* target) const;
    //
    UFUNCTION(BlueprintImplementableEvent, Category="Enemy|Defense")
    void TargetGuardBroken(AActor* target);
    //
	UFUNCTION(BlueprintCallable, Category = "Enemy | Defense")
	bool ApplyPerfectDodge(AActor* target);
	
	UFUNCTION(BlueprintPure, Category = "Enemy | Timing")
	float GetDefenseWindow(EDefenseResult result, FName actionId = NAME_None) const;
	UFUNCTION(BlueprintPure, Category = "Enemy | Timing")
	bool IsDefenseInputValid(EDefenseResult result, float elapsed) const;
	
	UFUNCTION(BlueprintPure, Category = "Enemy | Timing")
	bool IsAttackSlowed() const { return m_slowMontage != nullptr; }
	float GetAttackRate(const FEnemyActionData& action) const;
	float GetActionTimeout(const FEnemyActionData& action) const;
	void RestoreAttackRate();
	
	void FinishAction(bool _cancelled);

protected:
	virtual void BeginPlay() override;
	
	virtual void Tick(float _delta) override;
	float m_savedSpeed = 0.f;
	bool m_savedTurn = false;
	double m_actionStart = 0.;
	

	virtual void EndPlay(const EEndPlayReason::Type _reason) override;
	virtual void HandleHealthDepleted();
	virtual bool IsActionAllowed(const FEnemyActionData& _action) const { return true; }
	virtual float GetActionWeightMultiplier(const FEnemyActionData& _action) const { return 1.0f; }
	virtual void OnAcceptedHit(AActor* _attacker){}
	virtual void OnActionEnded(){}
	//
	virtual void OnAttackFinished() {}
	//
	void HealthChanged(const FOnAttributeChangeData& _data);
	void SetState(EEnemyState _newState);
	void SetHealthPool(float _max);
	int32 ChooseAction() const;
 
 FVector EvadeDirection(const FEnemyActionData& action) const;
 bool CanEvade(const FEnemyActionData& action) const;
 
	
	bool CanUseAction(int32 _actionIndex, float _distance = -1.0f) const;
	
	int32 FindComboAction(FName _actionId) const;

protected:
	FName m_nextActionId;
	
	FName m_lastAction;
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> m_slowMontage;
	float m_savedRate = 1.f;
	//
	float m_slowStart = 0.f;
	float m_slowEnd = 0.f;
	FName m_slowSection;
	//
	
	int32 m_comboSteps = 0;

	bool m_bActionHit = false;

	bool m_bActionBlocked = false;

	int32 m_activeIndex = INDEX_NONE;

	double m_readyTime = 0.0;

	TMap<FName, double> m_actionCooldowns;

	FDelegateHandle m_healthHandle;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy | Movement", meta = (ClampMin = "0"))
	float m_moveSpeed = 350.0f;

public:	
	UFUNCTION(BlueprintPure, Category = "Enemy | Combo")
	FName GetPendingComboId() const { return m_nextActionId; }

	UFUNCTION(BlueprintPure, Category = "Enemy | Combo")
	int32 GetComboStep() const { return m_comboSteps; }

	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool IsCombatActive() const { return m_state == EEnemyState::Alive; }

	UFUNCTION(BlueprintPure, Category = "Enemy")
	float GetHealth() const;

	UFUNCTION(BlueprintPure, Category = "Enemy")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool IsActing() const { return m_activeIndex != INDEX_NONE; }

	UFUNCTION(BlueprintPure, Category = "Enemy")
	bool CanDecide() const;
	//
	virtual bool IsRepositioning() const { return false; }
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy|Movement", meta=(DisplayName="Stationary"))
    bool m_stationary = false;
	//

	UFUNCTION(BlueprintNativeEvent, Category = "Enemy | Air")
	bool CanLiftTarget(AActor* target) const;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Air", meta = (DisplayName = "Enable Air Combo"))
	bool m_airCombo = false;
	
	UFUNCTION(BlueprintPure, Category = "Enemy | Movement")
	float GetMeleeDistance() const;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Movement", meta = (ClampMin = "1", DisplayName = "Melee Distance"))
	float m_meleeDist = 160.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy | Movement", meta = (ClampMin = "0", DisplayName = "Body Margin"))
	float m_bodyMargin = 30.f;
	UFUNCTION(BlueprintCallable, Category = "Enemy")	
	bool RequestAction(FName _actionId = NAME_None);
	const FEnemyActionData* GetReadyAction(FName _actionId) const;
	
	bool CanRushAttack(float _distance) const;
	
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	void CancelAction();

	UFUNCTION(BlueprintCallable, Category = "Enemy")
	virtual void ResetEncounter();

	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy")
	void EnemyDefeted();

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (DisplayName = "ASC"))
	TObjectPtr<UCombatAbilitySystemComponent> m_abilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (DisplayName = "AttributeSet"))
	TObjectPtr<UCombatAttributeSet> m_attributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (DisplayName = "Combat"))
	TObjectPtr<UCombat> m_combat;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy")
	TObjectPtr<UEnemyTraversal> m_traversal;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (DisplayName = "Defense"))
	TObjectPtr<UDefense> m_defense;
	//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Enemy")
	TObjectPtr<UEnemyTargeting> m_targeting;
	//

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (DisplayName = "Definition"))
	TObjectPtr<UEnemyData> m_definition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy", meta = (DisplayName = "BehaviorTree"))
	TObjectPtr<UBehaviorTree> m_behaviorTree;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy", meta = (DisplayName = "State"))
	EEnemyState m_state = EEnemyState::Alive;

	UPROPERTY(BlueprintReadOnly, Category = "Enemy", meta = (DisplayName = "CombatTarget"))
	TObjectPtr<AActor> m_combatTarget;

	UPROPERTY(BlueprintAssignable, Category = "Enemy", meta = (DisplayName = "OnHealthChanged"))
	FEnemyHealthEvent m_onHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Enemy", meta = (DisplayName = "OnStateChanged"))
	FEnemyStateEvent m_onStateChanged;

public:
	int32 m_actionSerial = 0;
	bool m_actionSuccess = false;
};
