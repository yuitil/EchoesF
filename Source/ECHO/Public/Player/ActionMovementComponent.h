//ActionMovementComponent.h
//移動・物理の拡張コンポーネント
//ダッシュ、2段ジャンプ時の空中制御、着地時の硬直など

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ActionMovementComponent.generated.h"

UCLASS()
class ECHO_API UActionMovementComponent : public UCharacterMovementComponent
{
    GENERATED_BODY()

public:
    UActionMovementComponent();

    //キャラクターの移動における加速度と摩擦を設定する関数
    void SetMovementWeight(float NewAcceleration, float NewFriction);

    //ダッシュ状態への切り替え、最大歩行速度を切り替える関数
    void SetSprinting(bool _bIsSprinting);

    //現在ダッシュ中かどうか取得する関数
    bool IsSprinting() const { return m_bIsSprinting; }

    //着地後の硬直処理を開始する関数
    void StartLandingRecovery();

protected:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // --- 歩行・ダッシュ速度設定 ---

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ActionMovement")
    float m_BaseWalkSpeed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ActionMovement")
    float m_SprintSpeed;

    //現在ダッシュ中かどうかのフラグ
    bool m_bIsSprinting;

public:
    // --- ジャンプ・空中制御関連パラメータ ---

    //１段目ジャンプ時の空中横移動の効き具合
    UPROPERTY(EditAnywhere, Category = "Jump")
    float m_AirControlFirstJump;

    //２段目ジャンプ時の空中横移動の効き具合
    UPROPERTY(EditAnywhere, Category = "Jump")
    float m_AirControlSecondJump;

    //２段目ジャンプ発動時に与えられる上方向（Z）の初速
    UPROPERTY(EditAnywhere, Category = "Jump")
    float m_SecondJumpZVelocity;

    //空中で入力方向に加える固定の力
    UPROPERTY(EditAnywhere, Category = "Jump")
    float m_AirPushForce;

    //空中の横方向最大速度
    UPROPERTY(EditAnywhere, Category = "Jump")
    float m_AirMaxHorizontalSpeed;


    //空中攻撃中の滞空（重力を止めてその場に留まる）
    void SetAirHover(bool bEnable);
    bool IsAirHovering() const { return m_bAirHover; }

private:
    bool m_bAirHover = false;
    float m_HoverCachedGravityScale = 1.f;

public:
    // --- 着地硬直関連パラメータ ---

    //着地後の速度回復時間
    UPROPERTY(EditAnywhere, Category = "Landing")
    float m_LandingRecoveryTime;

    //着地硬直中の速度倍率
    UPROPERTY(EditAnywhere, Category = "Landing")
    float m_LandingSpeedMultiplier;

    //回避の開始と終了
    void StartDodge(const FVector& Direction, float Force);
    void EndDodge();

    bool IsDodging() const { return m_bIsDodging; }

private:
    //現在回避中かどうか
    bool m_bIsDodging;
    float m_CachedGravityScale;
    float m_CachedGroundFriction;

    //現在着地硬直の回復処理中か
    bool m_bIsLandingRecovery;
    //着地硬直開始からの経過時間
    float m_LandingRecoveryElapsed;
};