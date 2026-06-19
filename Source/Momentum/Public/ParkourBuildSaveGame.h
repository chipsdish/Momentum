#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ParkourTypes.h"
#include "ParkourBuildSaveGame.generated.h"

UCLASS()
class MOMENTUM_API UParkourBuildSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Parkour|Build")
	TArray<FParkourBuildLayoutData> Layouts;
};
