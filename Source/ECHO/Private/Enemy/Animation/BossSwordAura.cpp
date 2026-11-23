#include "Enemy/Animation/BossSwordAura.h"
#include "Enemy/Combat/Combat.h"
#include "Animation/AnimNotifyLibrary.h"
#include "Components/SkeletalMeshComponent.h"

//ボスの剣に、通知で選んだ見た目をつける。
void UBossSwordAura::NotifyBegin(USkeletalMeshComponent* _mesh, UAnimSequenceBase*, float, const FAnimNotifyEventReference&)
{
    auto* owner = _mesh ? _mesh->GetOwner() : nullptr;
    auto* combat = owner ? owner->FindComponentByClass<UCombat>() : nullptr;
    if (!combat) { return; }
    combat->BeginBladeAura(m_material, m_mist, m_scale);
    combat->SetBladeGlow(m_width.X, m_glow.X);
}

//溜めの前後で光が急に変わらないようにする。
void UBossSwordAura::NotifyTick(USkeletalMeshComponent* _mesh, UAnimSequenceBase*, float, const FAnimNotifyEventReference& _event)
{
    auto* owner = _mesh ? _mesh->GetOwner() : nullptr;
    auto* combat = owner ? owner->FindComponentByClass<UCombat>() : nullptr;
    if (!combat) { return; }

    const float time = UAnimNotifyLibrary::GetCurrentAnimationNotifyStateTime(_event);
    const float charge = FMath::SmoothStep(0.f, FMath::Max(m_chargeTime, UE_SMALL_NUMBER), time);
    combat->SetBladeGlow(FMath::Lerp(m_width.X, m_width.Y, charge), FMath::Lerp(m_glow.X, m_glow.Y, charge));
}

//中断した場合もオーラを消す。
void UBossSwordAura::NotifyEnd(USkeletalMeshComponent* _mesh, UAnimSequenceBase*, const FAnimNotifyEventReference&)
{
    auto* owner = _mesh ? _mesh->GetOwner() : nullptr;
    auto* combat = owner ? owner->FindComponentByClass<UCombat>() : nullptr;
    if (combat) { combat->EndBladeAura(); }
}
