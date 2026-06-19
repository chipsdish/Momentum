#include "ParkourPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ParkourBuildCameraPawn.h"
#include "ParkourGameMode.h"

AParkourPlayerController::AParkourPlayerController()
{
	bShowMouseCursor = false;
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

		if (BuildWidgetClass && !BuildWidget)
		{
			BuildWidget = CreateWidget<UUserWidget>(this, BuildWidgetClass);
			if (BuildWidget)
			{
				BuildWidget->AddToViewport();
			}
		}

		bBuildModeEnabled = true;
		SetMenuInputMode();
	}
	else
	{
		AParkourBuildCameraPawn* CameraToDestroy = BuildCameraPawn;

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

		BuildCameraPawn = nullptr;
		GameplayPawn = nullptr;
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

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
}

void AParkourPlayerController::SetGameplayInputMode()
{
	bShowMouseCursor = false;

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
