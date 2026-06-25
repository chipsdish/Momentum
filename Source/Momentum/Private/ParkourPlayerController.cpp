#include "ParkourPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ParkourBuildCameraPawn.h"
#include "ParkourBuildManager.h"
#include "ParkourBuildPiece.h"
#include "ParkourBuildToolWidget.h"
#include "ParkourGameMode.h"
#include "ParkourHUDWidget.h"
#include "ParkourMainMenuWidget.h"
#include "ParkourTransformGizmo.h"

AParkourPlayerController::AParkourPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	bShowMouseCursor = false;
	MainMenuWidgetClass = UParkourMainMenuWidget::StaticClass();
	HUDWidgetClass = UParkourHUDWidget::StaticClass();
	BuildWidgetClass = UParkourBuildToolWidget::StaticClass();
}

void AParkourPlayerController::BeginPlay()
{
	Super::BeginPlay();

	const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this, true);
	if (CurrentLevelName == MainMenuLevelName.ToString())
	{
		ShowMainMenu();
	}
	else
	{
		StartGameplaySession();
	}
}

void AParkourPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bBuildModeEnabled)
	{
		UpdateBuildDrag();
		SyncGizmoToSelection();
	}
}

void AParkourPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindAction(TEXT("BuildPrimary"), IE_Pressed, this, &AParkourPlayerController::HandleBuildPrimaryPressed);
	InputComponent->BindAction(TEXT("BuildPrimary"), IE_Released, this, &AParkourPlayerController::HandleBuildPrimaryReleased);
	InputComponent->BindAction(TEXT("BuildSecondary"), IE_Pressed, this, &AParkourPlayerController::HandleBuildSecondaryPressed);
	InputComponent->BindAction(TEXT("BuildSecondary"), IE_Released, this, &AParkourPlayerController::HandleBuildSecondaryReleased);
}

void AParkourPlayerController::ShowMainMenu()
{
	RemoveWidget(HUDWidget);
	RemoveWidget(BuildWidget);

	if (MainMenuWidgetClass && !MainMenuWidget)
	{
		MainMenuWidget = CreateWidget<UUserWidget>(this, MainMenuWidgetClass);
		if (MainMenuWidget)
		{
			MainMenuWidget->AddToViewport();
		}
	}

	SetMenuInputMode();
}

void AParkourPlayerController::StartTestLevel()
{
	SetGameplayInputMode();
	UGameplayStatics::OpenLevel(this, TestLevelName);
}

void AParkourPlayerController::QuitGame()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

void AParkourPlayerController::StartGameplaySession()
{
	RemoveWidget(MainMenuWidget);
	CreateHUD();
	SetGameplayInputMode();

	if (AParkourGameMode* ParkourGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AParkourGameMode>() : nullptr)
	{
		ParkourGameMode->StartRunTimer();
	}
}

void AParkourPlayerController::TogglePauseMenu()
{
	ReturnToMainMenu();
}

void AParkourPlayerController::ReturnToMainMenu()
{
	UGameplayStatics::OpenLevel(this, MainMenuLevelName);
}

void AParkourPlayerController::RestartCurrentLevel()
{
	const FName CurrentLevelName(*UGameplayStatics::GetCurrentLevelName(this, true));
	UGameplayStatics::OpenLevel(this, CurrentLevelName);
}

void AParkourPlayerController::ToggleBuildMode()
{
	SetBuildModeEnabled(!bBuildModeEnabled);
}

void AParkourPlayerController::SetBuildModeEnabled(bool bEnabled)
{
	if (bBuildModeEnabled == bEnabled || !GetWorld())
	{
		return;
	}

	if (bEnabled)
	{
		GameplayPawn = GetPawn();
		SetAllBuildPieceVisualsForBuildMode(true);

		if (AParkourGameMode* ParkourGameMode = GetWorld()->GetAuthGameMode<AParkourGameMode>())
		{
			ParkourGameMode->SetRunTimerPaused(true);
		}

		FTransform SpawnTransform = FTransform::Identity;
		if (GameplayPawn)
		{
			SpawnTransform = GameplayPawn->GetActorTransform();
			SpawnTransform.AddToTranslation(FVector(0.0f, 0.0f, 300.0f));

			if (ACharacter* Character = Cast<ACharacter>(GameplayPawn))
			{
				if (UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement())
				{
					MovementComponent->StopMovementImmediately();
					MovementComponent->DisableMovement();
				}
			}
		}

		TSubclassOf<AParkourBuildCameraPawn> CameraClass = BuildCameraPawnClass;
		if (!CameraClass)
		{
			CameraClass = AParkourBuildCameraPawn::StaticClass();
		}
		BuildCameraPawn = GetWorld()->SpawnActor<AParkourBuildCameraPawn>(CameraClass, SpawnTransform);
		if (BuildCameraPawn)
		{
			Possess(BuildCameraPawn);
		}

		TransformGizmo = GetWorld()->SpawnActor<AParkourTransformGizmo>(AParkourTransformGizmo::StaticClass(), SpawnTransform);

		if (BuildWidgetClass && !BuildWidget)
		{
			BuildWidget = CreateWidget<UUserWidget>(this, BuildWidgetClass);
			if (BuildWidget)
			{
				BuildWidget->AddToViewport();
			}
		}

		bBuildModeEnabled = true;
		SetBuildInputMode();
	}
	else
	{
		AParkourBuildCameraPawn* CameraToDestroy = BuildCameraPawn;
		if (AParkourBuildManager* BuildManager = FindBuildManager())
		{
			BuildManager->ClearSelection();
		}
		ClearAllBuildSelectionVisuals();
		SetAllBuildPieceVisualsForBuildMode(false);
		ResetBuildViewArtifacts();

		if (GameplayPawn)
		{
			Possess(GameplayPawn);

			if (ACharacter* Character = Cast<ACharacter>(GameplayPawn))
			{
				if (UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement())
				{
					MovementComponent->SetMovementMode(MOVE_Walking);
				}
			}
		}

		if (CameraToDestroy)
		{
			CameraToDestroy->Destroy();
		}

		if (TransformGizmo)
		{
			TransformGizmo->Destroy();
		}

		BuildCameraPawn = nullptr;
		TransformGizmo = nullptr;
		GameplayPawn = nullptr;
		bDraggingGizmo = false;
		bBuildLookActive = false;
		RemoveWidget(BuildWidget);

		if (AParkourGameMode* ParkourGameMode = GetWorld()->GetAuthGameMode<AParkourGameMode>())
		{
			ParkourGameMode->SetRunTimerPaused(false);
		}

		bBuildModeEnabled = false;
		SetGameplayInputMode();
	}
}

void AParkourPlayerController::HandleRunFinished(float FinalTime)
{
	SetMenuInputMode();
	OnRunFinished(FinalTime);
}

void AParkourPlayerController::SetMenuInputMode()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
}

void AParkourPlayerController::SetBuildInputMode()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
}

void AParkourPlayerController::SetGameplayInputMode()
{
	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
	ResetBuildViewArtifacts();

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
}

void AParkourPlayerController::CreateHUD()
{
	if (HUDWidgetClass && !HUDWidget)
	{
		HUDWidget = CreateWidget<UUserWidget>(this, HUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->AddToViewport();
		}
	}
}

void AParkourPlayerController::RemoveWidget(TObjectPtr<UUserWidget>& Widget)
{
	if (Widget)
	{
		Widget->RemoveFromParent();
		Widget = nullptr;
	}
}

AParkourBuildManager* AParkourPlayerController::FindBuildManager() const
{
	return Cast<AParkourBuildManager>(UGameplayStatics::GetActorOfClass(this, AParkourBuildManager::StaticClass()));
}

void AParkourPlayerController::HandleBuildPrimaryPressed()
{
	if (!bBuildModeEnabled || bBuildLookActive)
	{
		return;
	}

	FVector RayOrigin = FVector::ZeroVector;
	FVector RayDirection = FVector::ForwardVector;
	DeprojectMousePositionToWorld(RayOrigin, RayDirection);

	if (TransformGizmo)
	{
		const EParkourGizmoAxis Axis = TransformGizmo->HitTestAxis(RayOrigin, RayDirection);
		if (Axis != EParkourGizmoAxis::None)
		{
			bDraggingGizmo = TransformGizmo->BeginDrag(Axis, RayOrigin, RayDirection, PlayerCameraManager ? PlayerCameraManager->GetCameraRotation().Vector() : FVector::ForwardVector);
			return;
		}
	}

	FHitResult Hit;
	if (!GetHitResultUnderCursor(ECC_Visibility, false, Hit))
	{
		if (AParkourBuildManager* BuildManager = FindBuildManager())
		{
			BuildManager->ClearSelection();
		}
		if (TransformGizmo)
		{
			TransformGizmo->DetachFromTarget();
		}
		return;
	}

	if (TransformGizmo && Hit.GetActor() == TransformGizmo)
	{
		EParkourGizmoAxis Axis = EParkourGizmoAxis::None;
		if (Hit.GetComponent() == TransformGizmo->XHandle)
		{
			Axis = EParkourGizmoAxis::X;
		}
		else if (Hit.GetComponent() == TransformGizmo->YHandle)
		{
			Axis = EParkourGizmoAxis::Y;
		}
		else if (Hit.GetComponent() == TransformGizmo->ZHandle)
		{
			Axis = EParkourGizmoAxis::Z;
		}
		else if (Hit.GetComponent() == TransformGizmo->XYHandle)
		{
			Axis = EParkourGizmoAxis::XY;
		}

		bDraggingGizmo = Axis != EParkourGizmoAxis::None && TransformGizmo->BeginDrag(Axis, RayOrigin, RayDirection, PlayerCameraManager ? PlayerCameraManager->GetCameraRotation().Vector() : FVector::ForwardVector);
		return;
	}

	if (AParkourBuildPiece* BuildPiece = Cast<AParkourBuildPiece>(Hit.GetActor()))
	{
		if (AParkourBuildManager* BuildManager = FindBuildManager())
		{
			BuildManager->SelectPiece(BuildPiece);
		}
		if (TransformGizmo)
		{
			TransformGizmo->AttachToTarget(BuildPiece);
		}
	}
}

void AParkourPlayerController::HandleBuildPrimaryReleased()
{
	bDraggingGizmo = false;
	if (TransformGizmo)
	{
		TransformGizmo->EndDrag();
	}
}

void AParkourPlayerController::HandleBuildSecondaryPressed()
{
	if (!bBuildModeEnabled)
	{
		return;
	}

	bBuildLookActive = true;
	bShowMouseCursor = false;

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
}

void AParkourPlayerController::HandleBuildSecondaryReleased()
{
	if (!bBuildModeEnabled)
	{
		return;
	}

	bBuildLookActive = false;
	SetBuildInputMode();
}

void AParkourPlayerController::UpdateBuildDrag()
{
	if (!bDraggingGizmo || !TransformGizmo)
	{
		return;
	}

	FVector RayOrigin = FVector::ZeroVector;
	FVector RayDirection = FVector::ForwardVector;
	if (DeprojectMousePositionToWorld(RayOrigin, RayDirection))
	{
		TransformGizmo->UpdateDrag(RayOrigin, RayDirection);
	}
}

void AParkourPlayerController::ClearAllBuildSelectionVisuals() const
{
	if (!GetWorld())
	{
		return;
	}

	for (TActorIterator<AParkourBuildPiece> It(GetWorld()); It; ++It)
	{
		It->SetSelected(false);
	}
}

void AParkourPlayerController::SetAllBuildPieceVisualsForBuildMode(bool bBuildModeActive) const
{
	if (!GetWorld())
	{
		return;
	}

	for (TActorIterator<AParkourBuildPiece> It(GetWorld()); It; ++It)
	{
		It->SetBuildModeVisuals(bBuildModeActive);
	}
}

void AParkourPlayerController::ResetBuildViewArtifacts()
{
	ConsoleCommand(TEXT("viewmode lit"), false);
	ConsoleCommand(TEXT("showflag.Wireframe 0"), false);
	ConsoleCommand(TEXT("showflag.Collision 0"), false);
	ConsoleCommand(TEXT("showflag.Bounds 0"), false);
	ConsoleCommand(TEXT("showflag.Grid 0"), false);
	ConsoleCommand(TEXT("showflag.SelectionOutline 0"), false);
}

void AParkourPlayerController::SyncGizmoToSelection()
{
	if (!TransformGizmo || TransformGizmo->IsDragging())
	{
		return;
	}

	const AParkourBuildManager* BuildManager = FindBuildManager();
	AParkourBuildPiece* SelectedPiece = BuildManager ? BuildManager->GetSelectedPiece() : nullptr;
	if (SelectedPiece && TransformGizmo->GetTargetActor() != SelectedPiece)
	{
		TransformGizmo->AttachToTarget(SelectedPiece);
	}
	else if (!SelectedPiece && TransformGizmo->GetTargetActor())
	{
		TransformGizmo->DetachFromTarget();
	}
}
