#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "ParkourBuildCameraPawn.generated.h"

class UCameraComponent;
class UFloatingPawnMovement;
class UInputAction;
class UInputMappingContext;
class AParkourBuildManager;

UCLASS()
class MOMENTUM_API AParkourBuildCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	AParkourBuildCameraPawn();

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Build Camera")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Build Camera")
	TObjectPtr<UFloatingPawnMovement> FloatingMovement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Build Camera")
	float BaseFlySpeed = 1600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Build Camera")
	float BoostFlySpeed = 4200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Build Camera")
	float SpeedStep = 250.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parkour|Input")
	TObjectPtr<UInputMappingContext> BuildMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parkour|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parkour|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parkour|Input")
	TObjectPtr<UInputAction> ElevateAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parkour|Input")
	TObjectPtr<UInputAction> BoostAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parkour|Input")
	TObjectPtr<UInputAction> AdjustSpeedAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parkour|Input")
	TObjectPtr<UInputAction> DeleteSelectedAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parkour|Input")
	TObjectPtr<UInputAction> DuplicateSelectedAction;

protected:
	void AddBuildMappingContext() const;
	void RemoveBuildMappingContext() const;
	AParkourBuildManager* FindBuildManager() const;

	void Input_Move(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);
	void Input_Elevate(const FInputActionValue& Value);
	void Input_BoostStarted(const FInputActionValue& Value);
	void Input_BoostCompleted(const FInputActionValue& Value);
	void Input_AdjustSpeed(const FInputActionValue& Value);
	void Input_DeleteSelected(const FInputActionValue& Value);
	void Input_DuplicateSelected(const FInputActionValue& Value);
};
