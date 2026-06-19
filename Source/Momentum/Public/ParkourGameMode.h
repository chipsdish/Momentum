#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ParkourGameMode.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FParkourRunFinishedSignature, float, FinalTime);

UCLASS()
class MOMENTUM_API AParkourGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AParkourGameMode();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Parkour|Run")
	void StartRunTimer();

	UFUNCTION(BlueprintCallable, Category = "Parkour|Run")
	void ResetRunTimer();

	UFUNCTION(BlueprintCallable, Category = "Parkour|Run")
	void SetRunTimerPaused(bool bPaused);

	UFUNCTION(BlueprintPure, Category = "Parkour|Run")
	float GetElapsedRunTime() const;

	UFUNCTION(BlueprintPure, Category = "Parkour|Run")
	bool IsRunFinished() const { return bRunFinished; }

	UFUNCTION(BlueprintCallable, Category = "Parkour|Run")
	void FinishRun(APawn* FinishingPawn);

	UFUNCTION(BlueprintCallable, Category = "Parkour|Respawn")
	void SetRespawnTransform(const FTransform& NewRespawnTransform);

	UFUNCTION(BlueprintCallable, Category = "Parkour|Respawn")
	void RespawnPlayer(AController* Controller);

	UPROPERTY(BlueprintAssignable, Category = "Parkour|Run")
	FParkourRunFinishedSignature OnRunFinished;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Run")
	bool bAutoStartRunTimer = false;

protected:
	FTransform ResolveRespawnTransform(AController* Controller);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Respawn")
	FTransform CurrentRespawnTransform = FTransform::Identity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Respawn")
	bool bHasRespawnTransform = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Run")
	bool bRunActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Run")
	bool bRunPaused = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Run")
	bool bRunFinished = false;

	float RunStartTime = 0.0f;
	float PauseStartTime = 0.0f;
	float AccumulatedPauseTime = 0.0f;
	float FinalRunTime = 0.0f;
};
