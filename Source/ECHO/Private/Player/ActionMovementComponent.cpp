//ActionMovementComponent.cpp
//アクションムーブメントコンポーネントソース

#include "Player/ActionMovementComponent.h"

UActionMovementComponent::UActionMovementComponent() :
    m_BaseWalkSpeed(700.f),
    m_SprintSpeed(1150.f),
    m_bIsSprinting(false),
    m_bIsDodging(false),
    m_AirControlFirstJump(0.3f),
    m_AirControlSecondJump(0.1f),
    m_SecondJumpZVelocity(2200.f),
    m_AirPushForce(1500.f),
    m_AirMaxHorizontalSpeed(800.f),
    m_LandingRecoveryTime(0.2f),
    m_LandingSpeedMultiplier(0.3f),
    m_bIsLandingRecovery(false),
    m_LandingRecoveryElapsed(0.f)
{
    PrimaryComponentTick.bCanEverTick = true;

    //アクションゲームらしいきびきびとした挙動にするための初期設定
    MaxAcceleration = 2048.f;             //加速度を高くし、レスポンスを向上
    GroundFriction = 8.f;                 //地面摩擦を高め、滑りを抑える
    BrakingDecelerationWalking = 2048.f;  //歩行時のブレーキ力を強める
    GravityScale = 6.f;                   //重力を強くし、もっさりした滞空時間を減らす
    BrakingDecelerationFalling = 2048.f;  //空中でのブレーキ力

    MaxWalkSpeed = m_BaseWalkSpeed;
}

void UActionMovementComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    //空中時の移動制御
    if (IsFalling())
    {
        //空中では進行方向へ強制的にキャラクターを回転させない
        bOrientRotationToMovement = false;

        //回避中でなければ、空中でも入力方向に対して力を加える
        if (!m_bIsDodging)
        {
            FVector InputVector = GetLastInputVector();
            if (!InputVector.IsNearlyZero())
            {
                //入力方向に力を正規化（斜め速度アップを防ぐ）
                FVector InputDir = InputVector.GetSafeNormal2D();

                //入力方向に力を加算
                Velocity += InputDir * m_AirPushForce * DeltaTime;

                //水平方向の速度を抽出し、最大速度制限をかける
                FVector HorizontalVelocity = FVector(Velocity.X, Velocity.Y, 0.f);
                if (HorizontalVelocity.Size() > m_AirMaxHorizontalSpeed)
                {
                    //制限を超えている場合はクランプする
                    HorizontalVelocity = HorizontalVelocity.GetSafeNormal() * m_AirMaxHorizontalSpeed;
                    Velocity.X = HorizontalVelocity.X;
                    Velocity.Y = HorizontalVelocity.Y;
                }
            }
        }
    }
    else
    {
        //地面にいる時は進行方向へキャラクターを向かわせる
        bOrientRotationToMovement = true;
    }

    //最大速度の決定
    float DesiredSpeed = m_bIsSprinting ? m_SprintSpeed : m_BaseWalkSpeed;

    //着地硬直の回復処理
    if (m_bIsLandingRecovery)
    {
        m_LandingRecoveryElapsed += DeltaTime;

        //回復の進行度を0.0～1.0の範囲で算出
        float Alpha = FMath::Clamp(m_LandingRecoveryElapsed / m_LandingRecoveryTime, 0.f, 1.f);

        //最初は重く、後から急速に元の速度に戻るような手触りにする
        float EasedAlpha = FMath::InterpEaseIn(0.f, 1.f, Alpha, 2.f);

        DesiredSpeed *= FMath::Lerp(m_LandingSpeedMultiplier, 1.f, EasedAlpha);

        //回復時間が終了したら、速度を完全に戻す
        if (Alpha >= 1.f)
        {
            m_bIsLandingRecovery = false;
        }
    }

    MaxWalkSpeed = DesiredSpeed;
}

void UActionMovementComponent::SetMovementWeight(
    float NewAcceleration, float NewFriction)
{
    MaxAcceleration = NewAcceleration;
    GroundFriction = NewFriction;
}

void UActionMovementComponent::SetSprinting(bool _bIsSprinting)
{
    m_bIsSprinting = _bIsSprinting;
}

void UActionMovementComponent::StartLandingRecovery()
{
    m_LandingRecoveryElapsed = 0.f;
    m_bIsLandingRecovery = true;
}

void UActionMovementComponent::SetAirHover(bool bEnable)
{
    if (m_bAirHover == bEnable) return;
    m_bAirHover = bEnable;

    if (bEnable)
    {
        m_HoverCachedGravityScale = GravityScale;
        GravityScale = 0.f;
        Velocity = FVector::ZeroVector;   //上昇・落下の勢いを止めてその場に留まる
    }
    else
    {
        GravityScale = m_HoverCachedGravityScale;
    }
}

void UActionMovementComponent::StartDodge(const FVector& Direction, float Force)
{
    m_bIsDodging = true;

    //元の物理パラメータを退避
    m_CachedGravityScale = GravityScale;
    m_CachedGroundFriction = GroundFriction;

    //回避中だけ重力と摩擦を無効化し、慣性を切る
    GravityScale = 0.f;
    GroundFriction = 0.f;
    Velocity = FVector::ZeroVector;

    //水平方向へ瞬間的な推進力を与える
    FVector Dir = Direction;
    Dir.Z = 0.f;
    Launch(Dir.GetSafeNormal() * Force);
}

void UActionMovementComponent::EndDodge()
{
    if (!m_bIsDodging) return;   //二重呼び出しガード

    GravityScale = m_CachedGravityScale;
    GroundFriction = m_CachedGroundFriction;

    Velocity.X = 0.f;
    Velocity.Y = 0.f;

    m_bIsDodging = false;
}
