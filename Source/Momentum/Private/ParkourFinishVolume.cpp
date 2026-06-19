#include "ParkourFinishVolume.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "ParkourGameMode.h"

AParkourFinishVolume::AParkourFinishVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	SetRootComponent(TriggerVolume);
	TriggerVolume->SetBoxExtent(FVector(160.0f, 80.0f, 180.0f));
	TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AParkourFinishVolume::BeginPlay()
{
	Super::BeginPlay();
	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AParkourFinishVolume::OnFinishOverlap);
}

void AParkourFinishVolume::OnFinishOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !GetWorld())
	{
		return;
	}

	if (AParkourGameMode* ParkourGameMode = GetWorld()->GetAuthGameMode<AParkourGameMode>())
	{
		ParkourGameMode->FinishRun(Pawn);
	}
}
