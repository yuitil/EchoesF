#include "Enemy/Animation/BossDrawSword.h"
#include "Enemy/Boss/BossApproach.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

//アニメーションの指定位置で処理を呼ぶ。
void UBossDrawSword::Notify(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim, const FAnimNotifyEventReference& _event)
{
    Super::Notify(_mesh, _anim, _event);
    if(!_mesh || !_mesh->GetOwner()) { return; }
    if(auto* approach = _mesh->GetOwner()->FindComponentByClass<UBossApproach>()) { approach->ShowSword(); }
}
