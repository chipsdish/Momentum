#include "ParkourBuildCameraPawn.h"

#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Kismet/GameplayStatics.h"
#include "ParkourBuildManager.h"

AParkourBuildCameraPawn::AParkourBuildCameraPawn()
{
	PrimaryActorTick.bCanEverTick = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	SetRootComponent(Camera);
	Camera->bUsePawnControlRotation = false;

	FloatingMovement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("FloatingMovement"));
	FloatingMovement->MaxSpeed = BaseFlySpeed;
	FloatingMovement->Acceleration = 8000.0f;
	FloatingMovement->Deceleration = 8000.0f;
}

void AParkourBuildCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	AddBuildMappingContext();

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInputComponent)
	{
		return;
	}

	if (MoveAction)
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AParkourBuildCameraPawn::Input_Move);
	}

	if (LookAction)
	{
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AParkourBuildCameraPawn::Input_Look);
	}

	if (ElevateAction)
	{
		EnhancedInputComponent->BindAction(ElevateAction, ETriggerEvent::Triggered, this, &AParkourBuildCameraPawn::Input_Elevate);
	}

	if (BoostAction)
	{
		EnhancedInputComponent->BindAction(BoostAction, ETriggerEvent::Started, this, &AParkourBuildCameraPawn::Input_BoostStarted);
		EnhancedInputComponent->BindAction(BoostAction, ETriggerEvent::Completed, this, &AParkourBuildCameraPawn::Input_BoostCompleted);
	}

	if (AdjustSpeedAction)
	{
		EnhancedInputComponent->BindAction(AdjustSpeedAction, ETriggerEvent::Triggered, this, &AParkourBuildCameraPawn::Input_AdjustSpeed);
	}

	if (DeleteSelectedAction)
	{
		EnhancedInputComponent->BindAction(DeleteSelectedAction, ETriggerEvent::Started, this, &AParkourBuildCameraPawn::Input_DeleteSelected);
	}

	if (DuplicateSelectedAction)
	{
		EnhancedInputComponent->BindAction(DuplicateSelectedAction, ETriggerEvent::Started, this, &AParkourBuildCameraPawn::Input_DuplicateSelected);
	}
}

void AParkourBuildCameraPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveBuildMappingContext();
	Super::EndPlay(EndPlayReason);
}

void AParkourBuildCameraPawn::AddBuildMappingContext() const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController || !BuildMappingContext)
	{
		return;
	}

	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
	{
		Subsystem->AddMappingContext(BuildMappingContext, 10);
	}
}

void AParkourBuildCameraPawn::RemoveBuildMappingContext() const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController || !BuildMappingContext)
	{
		return;
	}

	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
	{
		Subsystem->RemoveMappingContext(BuildMappingContext);
	}
}

AParkourBuildManager* AParkourBuildCameraPawn::FindBuildManager() const
{
	return Cast<AParkourBuildManager>(UGameplayStatics::GetActorOfClass(this, AParkourBuildManager::StaticClass()));
}

void AParkourBuildCameraPawn::Input_Move(const FInputActionValue& Value)
{
	const FVector2D MoveValue = Value.Get<FVector2D>();
	AddMovementInput(GetActorForwardVector(), MoveValue.Y);
	AddMovementInput(GetActorRightVector(), MoveValue.X);
}

void AParkourBuildCameraPawn::Input_Look(const FInputActionValue& Value)
{
	const FVector2D LookValue = Value.Get<FVector2D>();
	AddActorWorldRotation(FRotator(LookValue.Y, LookValue.X, 0.0f));
}

void AParkourBuildCameraPawn::Input_Elevate(const FInputActionValue& Value)
{
	AddMovementInput(FVector::UpVector, Value.Get<float>());
}

void AParkourBuildCameraPawn::Input_BoostStarted(const FInputActionValue& Value)
{
	if (FloatingMovement)
	{
		FloatingMovement->MaxSpeed = BoostFlySpeed;
	}
}

void AParkourBuildCameraPawn::Input_DeleteSelected(const FInputActionValue& Value)
{
	if (AParkourBuildManager* BuildManager = FindBuildManager())
	{
		BuildManager->DeleteSelectedPiece();
	}
}

void AParkourBuildCameraPawn::Input_DuplicateSelected(const FInputActionValue& Value)
{
	if (AParkourBuildManager* BuildManager = FindBuildManager())
	{
		BuildManager->DuplicateSelectedPiece();
	}
}

void AParkourBuildCameraPawn::Input_BoostCompleted(const FInputActionValue& Value)
{
	if (FloatingMovement)
	{
		FloatingMovement->MaxSpeed = BaseFlySpeed;
	}
}

void AParkourBuildCameraPawn::Input_AdjustSpeed(const FInputActionValue& Value)
{
	BaseFlySpeed = FMath::Max(250.0f, BaseFlySpeed + Value.Get<float>() * SpeedStep);
	if (FloatingMovement)
	{
		FloatingMovement->MaxSpeed = BaseFlySpeed;
	}
}
