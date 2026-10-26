#include "Enemy/Animation/BossCrowdHit.h"
#include "Enemy/Boss/BossCrowd.h"
#include "Enemy/Boss/BossEnemyCharacter.h"
#include "Components/SkeletalMeshComponent.h"

//別の表示用メッシュによる二重実行を防ぐ。
void UBossCrowdHit::Notify(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim, const FAnimNotifyEventReference& _event)
{
    auto* boss = _mesh ? Cast<ABossEnemyCharacter>(_mesh->GetOwner()) : nullptr;
    if(!boss || _mesh != boss->GetMesh() || !boss->m_crowd) { return; }
    boss->m_crowd->Release();
}
