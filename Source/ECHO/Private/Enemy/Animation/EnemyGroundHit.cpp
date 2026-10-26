#include "Enemy/Animation/EnemyGroundHit.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Enemy/Audio/EnemyAudio.h"
#include "Components/SkeletalMeshComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraShakeBase.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundConcurrency.h"
#include "Engine/World.h"

//空中では出さず、実際に見つかった床に演出を置く。
void UEnemyGroundHit::Notify(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim, const FAnimNotifyEventReference& _event)
{
    Super::Notify(_mesh, _anim, _event);
    auto* enemy = _mesh ? Cast<AEnemyCharacterBase>(_mesh->GetOwner()) : nullptr;
    if(!enemy || _mesh != enemy->GetMesh() || !enemy->IsActing() || enemy->GetNetMode() == NM_DedicatedServer) { return; }

    //手の真下にある地面を探す。
    const FTransform pose = !m_bone.IsNone() && _mesh->DoesSocketExist(m_bone) ? _mesh->GetSocketTransform(m_bone) : enemy->GetActorTransform();
    const FVector point = pose.TransformPosition(m_offset);
    const FVector up(0.f, 0.f, m_depth);
    FHitResult hit;
    FCollisionQueryParams query(SCENE_QUERY_STAT(EnemyGroundHit), false, enemy);
    if(!enemy->GetWorld()->LineTraceSingleByObjectType(hit, point + up, point - up, FCollisionObjectQueryParams(ECC_WorldStatic), query)) { return; }

    //壁ではなく上向きの面だけを使う。
    if(hit.ImpactNormal.Z <= 0.f) { return; }
    const FVector floor = hit.ImpactPoint + hit.ImpactNormal * m_floorGap;
    if(m_dust)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(enemy, m_dust, floor,
            FRotationMatrix::MakeFromZ(hit.ImpactNormal).Rotator(), m_scale, true, true, ENCPoolMethod::AutoRelease);
    }

    //距離によって音と揺れを弱くする。
    if(m_sound && enemy->m_audio)
    {
        UGameplayStatics::PlaySoundAtLocation(enemy, m_sound, floor, FRotator::ZeroRotator, m_gain, 1.f, 0.f,
            enemy->m_audio->m_range, enemy->m_audio->m_limit, enemy);
    }
    if(m_shake) { UGameplayStatics::PlayWorldCameraShake(enemy, m_shake, floor, 0.f, m_range); }
}
