#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ParkourRespawnVolume.generated.h"

class UBoxComponent;

UCLASS()
class MOMENTUM_API AParkourRespawnVolume : public AActor
{
	GENERATED_BODY()

public:
	AParkourRespawnVolume();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Respawn")
	TObjectPtr<UBoxComponent> TriggerVolume;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRespawnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
