#include "Enemy/Animation/EnemyDeathFade.h"
#include "Components/MeshComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Enemy/Minion/FlyingMinion.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

//死亡中だけ時間を進める。
UEnemyDeathFade::UEnemyDeathFade()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

//死亡アニメーションの再生時間を待つ。
void UEnemyDeathFade::Start(float _delay, UMaterialInterface* _material, FSimpleDelegate _finished)
{
	if (IsFading()) { return; }
	m_material = _material;
	m_age = -FMath::Max(0.f, _delay);
	m_finished = MoveTemp(_finished);
	SetComponentTickEnabled(true);
}

//輪郭を少しずつ消してから敵を片付ける。
void UEnemyDeathFade::TickComponent(float _delta, ELevelTick _type, FActorComponentTickFunction* _tick)
{
	Super::TickComponent(_delta, _type, _tick);
	m_age += _delta;
	if (m_age < 0.f) { return; }

	if (const auto* fly = Cast<AFlyingMinion>(GetOwner()))
	{
		if (fly->GetCharacterMovement()->IsFalling()) { m_age = 0.f; return; }
	}

	if (!m_ash && m_material && GetNetMode() != NM_DedicatedServer)
	{
		m_ash = UMaterialInstanceDynamic::Create(m_material, this);
		TInlineComponentArray<UMeshComponent*> meshes(GetOwner());
		for (auto* mesh : meshes)
		{
			if (!mesh->IsA<USkinnedMeshComponent>() && !mesh->IsA<UStaticMeshComponent>()) { continue; }
			for (int32 slot = 0; slot < mesh->GetNumMaterials(); ++slot)
			{
				m_slots.Add({mesh, slot, TStrongObjectPtr<UMaterialInterface>(mesh->GetMaterial(slot))});
				auto* original = mesh->GetMaterial(slot);
				float value = 0.f;
				if (original && original->GetScalarParameterValue(FMaterialParameterInfo(TEXT("SpawnReveal")), value))
				{
					auto* fade = mesh->CreateDynamicMaterialInstance(slot, original);
					if (fade) { m_reveal.Add(fade); }
				}
				else { mesh->SetMaterial(slot, m_ash); }
			}
		}
	}

	const float progress = FMath::Clamp(m_age / FMath::Max(.05f, m_duration), 0.f, 1.f);
	for (const auto& fade : m_reveal)
	{
		fade->SetScalarParameterValue(TEXT("SpawnReveal"), 1.f - progress);
	}
	if (m_ash) { m_ash->SetScalarParameterValue(TEXT("Opacity"), 1.f - progress); }
	if (progress < 1.f) { return; }

	for (const auto& slot : m_slots)
	{
		if (slot.mesh.IsValid()) { slot.mesh->SetMaterial(slot.index, slot.material.Get()); }
	}
	m_slots.Reset();
	m_reveal.Reset();
	m_ash = nullptr;
	SetComponentTickEnabled(false);
	FSimpleDelegate finish = MoveTemp(m_finished);
	finish.ExecuteIfBound();
}
