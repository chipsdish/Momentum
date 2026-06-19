#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ParkourCheckpoint.generated.h"

class UBoxComponent;

UCLASS()
class MOMENTUM_API AParkourCheckpoint : public AActor
{
	GENERATED_BODY()

public:
	AParkourCheckpoint();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Checkpoint")
	TObjectPtr<UBoxComponent> TriggerVolume;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Checkpoint")
	FTransform RespawnOffset = FTransform::Identity;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnCheckpointOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
