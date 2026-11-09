#include "Enemy/Animation/BossBladeTrail.h"
#include "Enemy/Animation/EnemySwing.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimMontage.h"

//モンタージュで決めた開始と終了を斬撃へ渡す。
void UBossBladeTrail::NotifyBegin(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim, float _duration, const FAnimNotifyEventReference& _ref)
{
    Super::NotifyBegin(_mesh, _anim, _duration, _ref);
    auto* enemy = _mesh ? Cast<AEnemyCharacterBase>(_mesh->GetOwner()) : nullptr;
    auto* move = Cast<UAnimMontage>(_anim);
    const auto* event = _ref.GetNotify();
    if(!enemy || !move || !event || _mesh != enemy->GetMesh() || !enemy->IsActing() || !enemy->m_swing || !enemy->m_swing->UsesTrailNotify())
    {
        return;
    }
    enemy->m_swing->StartBlade(move, m_left, event->GetTime(), event->GetTime() + _duration);
}
