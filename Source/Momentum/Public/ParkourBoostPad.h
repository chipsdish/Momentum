#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ParkourBoostPad.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class MOMENTUM_API AParkourBoostPad : public AActor
{
	GENERATED_BODY()

public:
	AParkourBoostPad();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Boost")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Boost")
	TObjectPtr<UStaticMeshComponent> PadMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Boost")
	TObjectPtr<UBoxComponent> TriggerVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Boost")
	TObjectPtr<UTextRenderComponent> LabelText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Boost")
	float BoostStrength = 1400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Boost")
	float VerticalBoost = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Boost")
	bool bOverrideHorizontalSpeed = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Boost")
	float ReuseCooldown = 0.25f;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnBoostOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	bool CanBoostActor(AActor* Actor) const;
	void RecordBoost(AActor* Actor);

	TMap<TWeakObjectPtr<AActor>, float> LastBoostTimes;
};
