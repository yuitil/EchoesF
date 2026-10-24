#include "Enemy/Animation/BossAfterimageWindow.h"
#include "Enemy/Animation/EnemyAfterimageTrail.h"
#include "Components/SkeletalMeshComponent.h"

//残像の区間を開始する。
void UBossAfterimageWindow::NotifyBegin(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _clip, float _time, const FAnimNotifyEventReference& _ref)
{
    if(!_mesh || !_mesh->GetOwner()) { return; }
    if(auto* trail = _mesh->GetOwner()->FindComponentByClass<UEnemyAfterimageTrail>()) { trail->Start(m_allowStill, _time); }
}

//残像の区間を終了する。
void UBossAfterimageWindow::NotifyEnd(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _clip, const FAnimNotifyEventReference& _ref)
{
    if(!_mesh || !_mesh->GetOwner()) { return; }
    if(auto* trail = _mesh->GetOwner()->FindComponentByClass<UEnemyAfterimageTrail>()) { trail->Stop(); }
}
