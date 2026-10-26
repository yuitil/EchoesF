#include "Enemy/Animation/EnemySoundNotify.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

//プレビューや、動いていない時の足音は鳴らさない。
void UEnemySoundNotify::Notify(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim, const FAnimNotifyEventReference& _event)
{
    Super::Notify(_mesh, _anim, _event);
    //再生している敵。
    auto* enemy = _mesh ? Cast<AEnemyCharacterBase>(_mesh->GetOwner()) : nullptr;
    if(!enemy || _mesh != enemy->GetMesh()) { return; }
    if(m_sound == EEnemySound::Step && (!enemy->IsCombatActive() || enemy->GetCharacterMovement()->IsFalling() || enemy->GetVelocity().SizeSquared2D() <= KINDA_SMALL_NUMBER)) { return; }

    //BPの音設定を持つ部品。
    auto* sound = enemy->FindComponentByClass<UEnemyAudio>();
    if(!sound) { return; }
    sound->Play(m_sound, _mesh->DoesSocketExist(m_socket) ? _mesh->GetSocketLocation(m_socket) : enemy->GetActorLocation(), m_gain);
}
