// Fill out your copyright notice in the Description page of Project Settings.


#include "RespawnPoint/RespawnPoint.h"
#include "GameFramework/Character.h"
#include "Components/StaticMeshComponent.h" 
#include "Components/BoxComponent.h"
#include "GameMode/EchoesGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "GameMode/EchoesGameMode.h"
#include "Player/ActionCharacter.h"

ARespawnPoint::ARespawnPoint()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// スタティックメッシュを作成してルートコンポーネントに設定
	m_StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	RootComponent = m_StaticMeshComponent;

	m_PlayerDitectionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("PlayerDitectionBox"));
	m_PlayerDitectionBox->SetupAttachment(RootComponent);
	m_PlayerDitectionBox->SetBoxExtent(FVector(200.f, 200.f, 200.f));
	m_PlayerDitectionBox->SetGenerateOverlapEvents(true);
	m_PlayerDitectionBox->SetCollisionProfileName(TEXT("Trigger"));

	m_PlayerDitectionBox->OnComponentBeginOverlap.AddDynamic(this, &ARespawnPoint::OnOverlapBegin);
	m_PlayerDitectionBox->OnComponentEndOverlap.AddDynamic(this, &ARespawnPoint::OnOverlapEnd);
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
	
	if (m_IsPlayerOverlapping) {
		if (m_OverlappingPlayer.IsValid()) {
			//ここにHP加算処理書いて

			float HealAmount = m_HPAdditionAmount * DeltaTime;

			m_OverlappingPlayer->AddHp(HealAmount);
		}
	}
}

void ARespawnPoint::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {
	if (!OtherActor->ActorHasTag("Player"))return;
	AActionCharacter* PlayerCharacter = Cast<AActionCharacter>(OtherActor);

	if (!PlayerCharacter)return;

	m_IsPlayerOverlapping = true;

	m_OverlappingPlayer = PlayerCharacter;

	//現在のワールドのゲームモードを取得
	AGameModeBase* GameMode = UGameplayStatics::GetGameMode(this);

	AEchoesGameMode* EchoesGameMode = Cast<AEchoesGameMode>(GameMode);

	if (EchoesGameMode)
	{
		FTransform NewRespawnTransform = GetActorTransform();

		EchoesGameMode->SetRespawnTransform(NewRespawnTransform);
	}
}


void ARespawnPoint::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex){
	if (!OtherActor->ActorHasTag("Player"))return;

	m_IsPlayerOverlapping = false;
	m_OverlappingPlayer = nullptr;
}


