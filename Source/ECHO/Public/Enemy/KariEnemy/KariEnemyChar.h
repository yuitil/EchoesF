//AEnemyChar.h
//ダメージを受けるだけの仮エネミークラス

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "KariEnemyChar.generated.h"

//ヒット時にプレイヤーへ通知するデリゲート
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyHit, float, Damage);

//従来の仮の敵キャラクター。
UCLASS()
class ECHO_API AEnemyChar : public ACharacter
{
    GENERATED_BODY()

public:
    //従来の仮の敵キャラクター。
    AEnemyChar();

    //プレイヤーのCombatComponentから呼ぶ
    virtual float TakeDamage(
        float DamageAmount,
        const FDamageEvent& DamageEvent,
        AController* EventInstigator,
        AActor* DamageCauser) override;

    //ヒット時のデリゲート
    UPROPERTY(BlueprintAssignable)
    FOnEnemyHit OnEnemyHit;

protected:
    //ゲーム開始時に必要な状態を準備する。
    virtual void BeginPlay() override;

    //最大体力を読み書きする窓口。
    UPROPERTY(EditAnywhere, Category = "Enemy")
    float MaxHealth = 100.f;

    //現在の体力を読み書きする窓口。
    UPROPERTY(VisibleAnywhere, Category = "Enemy")
    float CurrentHealth;
};
