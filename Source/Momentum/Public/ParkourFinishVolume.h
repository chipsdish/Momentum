#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ParkourFinishVolume.generated.h"

class UBoxComponent;

UCLASS()
class MOMENTUM_API AParkourFinishVolume : public AActor
{
	GENERATED_BODY()

public:
	AParkourFinishVolume();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Finish")
	TObjectPtr<UBoxComponent> TriggerVolume;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnFinishOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
