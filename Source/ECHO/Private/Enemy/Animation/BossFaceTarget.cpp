#include "Enemy/Animation/BossFaceTarget.h"
#include "Enemy/Core/EnemyCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"

//通知の区間に入った時だけ、移動の補正を有効にする。
void UBossFaceTarget::NotifyBegin(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim, float _duration, const FAnimNotifyEventReference& _reference)
{
    Super::NotifyBegin(_mesh, _anim, _duration, _reference);
    auto* enemy = _mesh ? Cast<AEnemyCharacterBase>(_mesh->GetOwner()) : nullptr;
    if(!enemy || !enemy->HasAuthority() || _mesh != enemy->GetMesh() || !enemy->IsActing()) { return; }
    if(m_aimMove) { enemy->SetAttackAim(true, m_stopRange); }
}

//最後の姿勢へ移る前に、移動の補正を解除する。
void UBossFaceTarget::NotifyEnd(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim, const FAnimNotifyEventReference& _reference)
{
    Super::NotifyEnd(_mesh, _anim, _reference);
    auto* enemy = _mesh ? Cast<AEnemyCharacterBase>(_mesh->GetOwner()) : nullptr;
    if(!enemy || _mesh != enemy->GetMesh()) { return; }
    if(m_aimMove) { enemy->SetAttackAim(false, m_stopRange); }
}

//通知の外では向きを変えず、攻撃の最後の姿勢を残す。
void UBossFaceTarget::NotifyTick(USkeletalMeshComponent* _mesh, UAnimSequenceBase* _anim, float _delta, const FAnimNotifyEventReference& _reference)
{
    Super::NotifyTick(_mesh, _anim, _delta, _reference);
    auto* enemy = _mesh ? Cast<AEnemyCharacterBase>(_mesh->GetOwner()) : nullptr;
    if(!enemy || !enemy->HasAuthority() || !enemy->IsActing() || !IsValid(enemy->m_combatTarget)) { return; }
    //表示用メッシュにも通知がある場合、二重に回さない。
    if(_mesh != enemy->GetMesh()) { return; }
    const FVector toward = (enemy->m_combatTarget->GetActorLocation() - enemy->GetActorLocation()).GetSafeNormal2D();
    if(toward.IsNearlyZero()) { return; }
    FRotator facing = enemy->GetActorRotation();
    facing.Yaw = FMath::FixedTurn(facing.Yaw, toward.Rotation().Yaw, FMath::Max(0.f, m_turnSpeed) * _delta);
    enemy->SetActorRotation(facing);
}
