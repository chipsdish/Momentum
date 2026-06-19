#include "ParkourCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/SpringArmComponent.h"
#include "ParkourMovementComponent.h"
#include "ParkourPlayerController.h"

AParkourCharacter::AParkourCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UParkourMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.0f, 0.0f, 72.0f));
	FirstPersonCamera->bUsePawnControlRotation = true;

	ThirdPersonSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("ThirdPersonSpringArm"));
	ThirdPersonSpringArm->SetupAttachment(GetRootComponent());
	ThirdPersonSpringArm->TargetArmLength = 350.0f;
	ThirdPersonSpringArm->bUsePawnControlRotation = true;
	ThirdPersonSpringArm->bDoCollisionTest = true;
	ThirdPersonSpringArm->SetActive(false);

	ThirdPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ThirdPersonCamera"));
	ThirdPersonCamera->SetupAttachment(ThirdPersonSpringArm, USpringArmComponent::SocketName);
	ThirdPersonCamera->bUsePawnControlRotation = false;
	ThirdPersonCamera->SetActive(false);
}

void AParkourCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	AddPlayerMappingContext();

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInputComponent)
	{
		return;
	}

	if (MoveAction)
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AParkourCharacter::Input_Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &AParkourCharacter::Input_MoveCompleted);
	}

	if (LookAction)
	{
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AParkourCharacter::Input_Look);
	}

	if (JumpAction)
	{
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AParkourCharacter::Input_JumpStarted);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AParkourCharacter::Input_JumpCompleted);
	}

	if (CrouchAction)
	{
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &AParkourCharacter::Input_CrouchStarted);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &AParkourCharacter::Input_CrouchCompleted);
	}

	if (SprintAction)
	{
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AParkourCharacter::Input_SprintStarted);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AParkourCharacter::Input_SprintCompleted);
	}

	if (PauseAction)
	{
		EnhancedInputComponent->BindAction(PauseAction, ETriggerEvent::Started, this, &AParkourCharacter::Input_PauseStarted);
	}

	if (ToggleBuildModeAction)
	{
		EnhancedInputComponent->BindAction(ToggleBuildModeAction, ETriggerEvent::Started, this, &AParkourCharacter::Input_ToggleBuildModeStarted);
	}
}

UParkourMovementComponent* AParkourCharacter::GetParkourMovementComponent() const
{
	return Cast<UParkourMovementComponent>(GetCharacterMovement());
}

void AParkourCharacter::SetFirstPersonCameraActive(bool bActive)
{
	if (FirstPersonCamera)
	{
		FirstPersonCamera->SetActive(bActive);
	}

	if (ThirdPersonSpringArm)
	{
		ThirdPersonSpringArm->SetActive(!bActive);
	}

	if (ThirdPersonCamera)
	{
		ThirdPersonCamera->SetActive(!bActive);
	}
}

void AParkourCharacter::AddPlayerMappingContext() const
{
	const APlayerController* PlayerController = Cast<APlayerController>(Controller);
	if (!PlayerController || !PlayerMappingContext)
	{
		return;
	}

	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (Subsystem)
	{
		Subsystem->AddMappingContext(PlayerMappingContext, 0);
	}
}

void AParkourCharacter::Input_Move(const FInputActionValue& Value)
{
	if (UParkourMovementComponent* ParkourMovement = GetParkourMovementComponent())
	{
		ParkourMovement->SetMoveInput(Value.Get<FVector2D>());
	}
}

void AParkourCharacter::Input_MoveCompleted(const FInputActionValue& Value)
{
	if (UParkourMovementComponent* ParkourMovement = GetParkourMovementComponent())
	{
		ParkourMovement->SetMoveInput(FVector2D::ZeroVector);
	}
}

void AParkourCharacter::Input_Look(const FInputActionValue& Value)
{
	const FVector2D LookValue = Value.Get<FVector2D>();
	AddControllerYawInput(LookValue.X);
	AddControllerPitchInput(LookValue.Y);
}

void AParkourCharacter::Input_JumpStarted(const FInputActionValue& Value)
{
	if (UParkourMovementComponent* ParkourMovement = GetParkourMovementComponent())
	{
		ParkourMovement->RequestJumpPressed();
	}
}

void AParkourCharacter::Input_JumpCompleted(const FInputActionValue& Value)
{
	if (UParkourMovementComponent* ParkourMovement = GetParkourMovementComponent())
	{
		ParkourMovement->RequestJumpReleased();
	}
}

void AParkourCharacter::Input_CrouchStarted(const FInputActionValue& Value)
{
	Crouch();
}

void AParkourCharacter::Input_CrouchCompleted(const FInputActionValue& Value)
{
	UnCrouch();
}

void AParkourCharacter::Input_SprintStarted(const FInputActionValue& Value)
{
	// Reserved for a later sprint or slide modifier.
}

void AParkourCharacter::Input_SprintCompleted(const FInputActionValue& Value)
{
	// Reserved for a later sprint or slide modifier.
}

void AParkourCharacter::Input_PauseStarted(const FInputActionValue& Value)
{
	if (AParkourPlayerController* ParkourController = Cast<AParkourPlayerController>(GetController()))
	{
		ParkourController->TogglePauseMenu();
	}
}

void AParkourCharacter::Input_ToggleBuildModeStarted(const FInputActionValue& Value)
{
	if (AParkourPlayerController* ParkourController = Cast<AParkourPlayerController>(GetController()))
	{
		ParkourController->ToggleBuildMode();
	}
}
