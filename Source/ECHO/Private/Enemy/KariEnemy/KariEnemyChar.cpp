//KariEnemyChar.cpp

#include "Enemy/KariEnemy/KariEnemyChar.h"

//従来の仮の敵キャラクター。
AEnemyChar::AEnemyChar()
{
    PrimaryActorTick.bCanEverTick = false;
}

//ゲーム開始時に必要な状態を準備する。
void AEnemyChar::BeginPlay()
{
    Super::BeginPlay();
    CurrentHealth = MaxHealth;
}

//防御を確認して体力を減らす。
float AEnemyChar::TakeDamage(
    float DamageAmount,
    const FDamageEvent& DamageEvent,
    AController* EventInstigator,
    AActor* DamageCauser)
{
    //実際に与えたダメージ。
    float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0.f, MaxHealth);

    //ヒットを通知
    OnEnemyHit.Broadcast(DamageAmount);

    UE_LOG(LogTemp, Log, TEXT("EnemyHP: %.1f / %.1f"), CurrentHealth, MaxHealth);

    return ActualDamage;
}
