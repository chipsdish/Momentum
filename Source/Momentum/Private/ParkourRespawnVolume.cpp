#include "ParkourRespawnVolume.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "ParkourGameMode.h"

AParkourRespawnVolume::AParkourRespawnVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	SetRootComponent(TriggerVolume);
	TriggerVolume->SetBoxExtent(FVector(2000.0f, 2000.0f, 200.0f));
	TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AParkourRespawnVolume::BeginPlay()
{
	Super::BeginPlay();
	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AParkourRespawnVolume::OnRespawnOverlap);
}

void AParkourRespawnVolume::OnRespawnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !GetWorld())
	{
		return;
	}

	if (AParkourGameMode* ParkourGameMode = GetWorld()->GetAuthGameMode<AParkourGameMode>())
	{
		ParkourGameMode->RespawnPlayer(Pawn->GetController());
	}
}
