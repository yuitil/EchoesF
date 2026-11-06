//JustDodgeComponent.cpp

#include "Player/JustDodgeComponent.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

UJustDodgeComponent::UJustDodgeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UJustDodgeComponent::BeginPlay()
{
	Super::BeginPlay();

	//彩度を変えるカメラ（プレイヤーの FollowCamera）
	m_Camera = GetOwner() ? GetOwner()->FindComponentByClass<UCameraComponent>() : nullptr;
}

void UJustDodgeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	//スロー中に消えたときのために、時間の速さを元に戻しておく
	if (m_bSlowing)
	{
		UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
		m_bSlowing = false;
	}
	SetSaturation(1.f);

	Super::EndPlay(EndPlayReason);
}

void UJustDodgeComponent::TriggerJustDodge()
{
	//報酬は毎回つける（残り時間を満タンに戻す）
	m_BuffRemain = m_BuffTime;

	//スローは連発しないようにする（乱戦で毎回止まらないように）
	if (m_CooldownRemain <= 0.f)
	{
		m_bSlowing = true;
		m_SlowElapsed = 0.f;
		m_CooldownRemain = m_SlowCooldown;
		UGameplayStatics::SetGlobalTimeDilation(this, m_SlowTimeDilation);
	}

	OnJustDodge.Broadcast();

	if (m_bDebug && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Cyan, TEXT("Just Dodge!"));
	}
}

void UJustDodgeComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UWorld* World = GetWorld();
	if (!World) return;

	//スローやヒットストップの影響を受けない実時間
	const float RealDt = World->DeltaRealTimeSeconds;

	m_CooldownRemain = FMath::Max(0.f, m_CooldownRemain - RealDt);
	m_BuffRemain = FMath::Max(0.f, m_BuffRemain - RealDt);

	// --- スロー：止めておく → なめらかに戻す ---
	bool bHolding = false;
	if (m_bSlowing)
	{
		m_SlowElapsed += RealDt;

		if (m_SlowElapsed < m_SlowHoldTime)
		{
			bHolding = true;
			UGameplayStatics::SetGlobalTimeDilation(this, m_SlowTimeDilation);
		}
		else
		{
			const float T = (m_SlowReturnTime > 0.f)
				? FMath::Clamp((m_SlowElapsed - m_SlowHoldTime) / m_SlowReturnTime, 0.f, 1.f)
				: 1.f;
			const float Dilation = FMath::Lerp(m_SlowTimeDilation, 1.f, FMath::SmoothStep(0.f, 1.f, T));
			UGameplayStatics::SetGlobalTimeDilation(this, Dilation);

			if (T >= 1.f) m_bSlowing = false;
		}
	}

	// --- 彩度：スロー中は強く、報酬中は少し、それ以外は元に戻す ---
	float Target = 1.f;
	if (bHolding)               Target = m_SlowSaturation;
	else if (m_BuffRemain > 0.f) Target = m_BuffSaturation;

	SetSaturation(FMath::FInterpTo(m_Saturation, Target, RealDt, m_SaturationSpeed));
}

void UJustDodgeComponent::SetSaturation(float Value)
{
	m_Saturation = Value;
	if (!m_Camera) return;

	//カメラのポストプロセスで画面全体の彩度を変える（UIには影響しない）
	FPostProcessSettings& PP = m_Camera->PostProcessSettings;
	PP.bOverride_ColorSaturation = !FMath::IsNearlyEqual(Value, 1.f, 0.001f);
	PP.ColorSaturation = FVector4(Value, Value, Value, 1.f);
}