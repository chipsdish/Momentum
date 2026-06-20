#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ParkourPlayerController.generated.h"

class AParkourBuildCameraPawn;
class AParkourBuildManager;
class AParkourBuildPiece;
class AParkourTransformGizmo;
class UUserWidget;

UCLASS()
class MOMENTUM_API AParkourPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AParkourPlayerController();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupInputComponent() override;

	UFUNCTION(BlueprintCallable, Category = "Parkour|UI")
	void ShowMainMenu();

	UFUNCTION(BlueprintCallable, Category = "Parkour|UI")
	void StartTestLevel();

	UFUNCTION(BlueprintCallable, Category = "Parkour|UI")
	void QuitGame();

	UFUNCTION(BlueprintCallable, Category = "Parkour|UI")
	void StartGameplaySession();

	UFUNCTION(BlueprintCallable, Category = "Parkour|UI")
	void TogglePauseMenu();

	UFUNCTION(BlueprintCallable, Category = "Parkour|UI")
	void ReturnToMainMenu();

	UFUNCTION(BlueprintCallable, Category = "Parkour|UI")
	void RestartCurrentLevel();

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	void ToggleBuildMode();

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	void SetBuildModeEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Parkour|Build")
	bool IsBuildModeEnabled() const { return bBuildModeEnabled; }

	UFUNCTION(BlueprintCallable, Category = "Parkour|Run")
	void HandleRunFinished(float FinalTime);

	UFUNCTION(BlueprintImplementableEvent, Category = "Parkour|Run")
	void OnRunFinished(float FinalTime);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parkour|UI")
	TSubclassOf<UUserWidget> MainMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parkour|UI")
	TSubclassOf<UUserWidget> HUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parkour|UI")
	TSubclassOf<UUserWidget> BuildWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parkour|Build")
	TSubclassOf<AParkourBuildCameraPawn> BuildCameraPawnClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parkour|Levels")
	FName MainMenuLevelName = TEXT("MainMenu");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parkour|Levels")
	FName TestLevelName = TEXT("TestLevel");

protected:
	void SetMenuInputMode();
	void SetBuildInputMode();
	void SetGameplayInputMode();
	void CreateHUD();
	void RemoveWidget(TObjectPtr<UUserWidget>& Widget);
	AParkourBuildManager* FindBuildManager() const;
	void HandleBuildPrimaryPressed();
	void HandleBuildPrimaryReleased();
	void UpdateBuildDrag();
	void SyncGizmoToSelection();

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> MainMenuWidget;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> HUDWidget;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> BuildWidget;

	UPROPERTY(Transient)
	TObjectPtr<APawn> GameplayPawn;

	UPROPERTY(Transient)
	TObjectPtr<AParkourBuildCameraPawn> BuildCameraPawn;

	UPROPERTY(Transient)
	TObjectPtr<AParkourTransformGizmo> TransformGizmo;

	bool bDraggingGizmo = false;

	bool bBuildModeEnabled = false;
};
