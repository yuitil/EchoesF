// Fill out your copyright notice in the Description page of Project Settings.


#include "RespawnPoint/RespawnPoint.h"
#include "GameFramework/Character.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "GameMode/EchoesGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "GameMode/EchoesGameMode.h"
#include "Player/ActionCharacter.h"
#include "NiagaraComponent.h"

ARespawnPoint::ARespawnPoint()
{
	PrimaryActorTick.bCanEverTick = true;

	// BoxCollision（Root）
	m_PlayerDitectionBox =
		CreateDefaultSubobject<UBoxComponent>(TEXT("PlayerDitectionBox"));

	RootComponent = m_PlayerDitectionBox;

	m_PlayerDitectionBox->SetBoxExtent(FVector(200.f, 200.f, 200.f));
	m_PlayerDitectionBox->SetGenerateOverlapEvents(true);
	m_PlayerDitectionBox->SetCollisionProfileName(TEXT("Trigger"));

	// StaticMesh
	m_StaticMeshComponent =
		CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));

	m_StaticMeshComponent->SetupAttachment(RootComponent);

	// Niagara
	m_HealAura =
		CreateDefaultSubobject<UNiagaraComponent>(TEXT("HealAura"));

	m_HealAura->SetupAttachment(RootComponent);
	m_HealAura->SetAutoActivate(false);

	// Overlapイベント
	m_PlayerDitectionBox->OnComponentBeginOverlap.AddDynamic(
		this, &ARespawnPoint::OnOverlapBegin);

	m_PlayerDitectionBox->OnComponentEndOverlap.AddDynamic(
		this, &ARespawnPoint::OnOverlapEnd);
}


// Called when the game starts or when spawned
void ARespawnPoint::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ARespawnPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (m_IsPlayerOverlapping)
	{
		if (m_OverlappingPlayer.IsValid())
		{
			//ここにHP加算処理書いて

			float HealAmount = m_HPAdditionAmount * DeltaTime;

			m_OverlappingPlayer->AddHp(HealAmount);
		}
	}
}

void ARespawnPoint::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                   const FHitResult& SweepResult)
{
	AActionCharacter* PlayerCharacter = Cast<AActionCharacter>(OtherActor);

	if (!PlayerCharacter || !PlayerCharacter->ActorHasTag(TEXT("Player")))
		return;

	m_IsPlayerOverlapping = true;
	m_OverlappingPlayer = PlayerCharacter;

	// 回復オーラ再生
	if (m_HealAura)
	{
		m_HealAura->SetVisibility(true);

		// 一度完全に停止
		m_HealAura->DeactivateImmediate();

		// Niagaraを再初期化
		m_HealAura->ReinitializeSystem();

		// 最初から再生
		m_HealAura->Activate(true);
	}

	// リスポーン地点更新
	AEchoesGameMode* EchoesGameMode =
		Cast<AEchoesGameMode>(UGameplayStatics::GetGameMode(this));

	if (EchoesGameMode)
	{
		EchoesGameMode->SetRespawnTransform(GetActorTransform());
	}
}


void ARespawnPoint::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (m_OverlappingPlayer.Get() != OtherActor) return;

	m_IsPlayerOverlapping = false;
	m_OverlappingPlayer = nullptr;

	// 回復オーラ停止
	if (m_HealAura)
	{
		// エフェクトを即座に停止
		m_HealAura->DeactivateImmediate();

		// 非表示
		m_HealAura->SetVisibility(false);
	}
}
