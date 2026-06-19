#include "ParkourCheckpoint.h"

#include "Components/BoxComponent.h"
#include "ParkourCharacter.h"
#include "ParkourGameMode.h"

AParkourCheckpoint::AParkourCheckpoint()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	SetRootComponent(TriggerVolume);
	TriggerVolume->SetBoxExtent(FVector(120.0f, 120.0f, 120.0f));
	TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AParkourCheckpoint::BeginPlay()
{
	Super::BeginPlay();
	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AParkourCheckpoint::OnCheckpointOverlap);
}

void AParkourCheckpoint::OnCheckpointOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!Cast<AParkourCharacter>(OtherActor) || !GetWorld())
	{
		return;
	}

	if (AParkourGameMode* ParkourGameMode = GetWorld()->GetAuthGameMode<AParkourGameMode>())
	{
		ParkourGameMode->SetRespawnTransform(RespawnOffset * GetActorTransform());
	}
}
