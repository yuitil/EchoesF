// Fill out your copyright notice in the Description page of Project Settings.


#include "Event/EventTriggerVolume/EventTriggerVolume.h"
#include "Components/BoxComponent.h"

// Sets default values
AEventTriggerVolume::AEventTriggerVolume()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	m_TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	RootComponent = m_TriggerBox;
	m_TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
	m_TriggerBox->SetGenerateOverlapEvents(true);

}

// Called when the game starts or when spawned
void AEventTriggerVolume::BeginPlay()
{
	Super::BeginPlay();

	if (!m_TriggerBox)return;
	m_TriggerBox->OnComponentBeginOverlap.AddUniqueDynamic(this, &AEventTriggerVolume::OnOverlapBegin);
}

// Called every frame
void AEventTriggerVolume::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AEventTriggerVolume::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (GEngine) {
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Blue, TEXT("Event Start"));
	}
}

