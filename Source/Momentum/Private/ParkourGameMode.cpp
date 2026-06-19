#include "ParkourGameMode.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "ParkourCharacter.h"
#include "ParkourPlayerController.h"

AParkourGameMode::AParkourGameMode()
{
	DefaultPawnClass = AParkourCharacter::StaticClass();
	PlayerControllerClass = AParkourPlayerController::StaticClass();
}

void AParkourGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoStartRunTimer)
	{
		StartRunTimer();
	}
}

void AParkourGameMode::StartRunTimer()
{
	if (!GetWorld())
	{
		return;
	}

	RunStartTime = GetWorld()->GetTimeSeconds();
	PauseStartTime = 0.0f;
	AccumulatedPauseTime = 0.0f;
	FinalRunTime = 0.0f;
	bRunActive = true;
	bRunPaused = false;
	bRunFinished = false;
}

void AParkourGameMode::ResetRunTimer()
{
	StartRunTimer();
}

void AParkourGameMode::SetRunTimerPaused(bool bPaused)
{
	if (!GetWorld() || !bRunActive || bRunFinished || bRunPaused == bPaused)
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (bPaused)
	{
		PauseStartTime = CurrentTime;
		bRunPaused = true;
	}
	else
	{
		AccumulatedPauseTime += CurrentTime - PauseStartTime;
		PauseStartTime = 0.0f;
		bRunPaused = false;
	}
}

float AParkourGameMode::GetElapsedRunTime() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.0f;
	}

	if (bRunFinished)
	{
		return FinalRunTime;
	}

	if (!bRunActive)
	{
		return 0.0f;
	}

	const float EndTime = bRunPaused ? PauseStartTime : World->GetTimeSeconds();
	return FMath::Max(EndTime - RunStartTime - AccumulatedPauseTime, 0.0f);
}

void AParkourGameMode::FinishRun(APawn* FinishingPawn)
{
	if (!bRunActive || bRunFinished)
	{
		return;
	}

	FinalRunTime = GetElapsedRunTime();
	bRunFinished = true;
	bRunActive = false;
	bRunPaused = false;

	OnRunFinished.Broadcast(FinalRunTime);

	if (FinishingPawn)
	{
		if (AParkourPlayerController* ParkourController = Cast<AParkourPlayerController>(FinishingPawn->GetController()))
		{
			ParkourController->HandleRunFinished(FinalRunTime);
		}
	}
}

void AParkourGameMode::SetRespawnTransform(const FTransform& NewRespawnTransform)
{
	CurrentRespawnTransform = NewRespawnTransform;
	bHasRespawnTransform = true;
}

void AParkourGameMode::RespawnPlayer(AController* Controller)
{
	if (!Controller)
	{
		return;
	}

	APawn* Pawn = Controller->GetPawn();
	if (!Pawn)
	{
		RestartPlayer(Controller);
		return;
	}

	const FTransform RespawnTransform = ResolveRespawnTransform(Controller);
	Pawn->TeleportTo(RespawnTransform.GetLocation(), RespawnTransform.Rotator(), false, true);
	Controller->SetControlRotation(RespawnTransform.Rotator());

	if (ACharacter* Character = Cast<ACharacter>(Pawn))
	{
		if (UCharacterMovementComponent* MovementComponent = Character->GetCharacterMovement())
		{
			MovementComponent->StopMovementImmediately();
			MovementComponent->SetMovementMode(MOVE_Walking);
		}
	}
}

FTransform AParkourGameMode::ResolveRespawnTransform(AController* Controller)
{
	if (bHasRespawnTransform)
	{
		return CurrentRespawnTransform;
	}

	if (Controller)
	{
		if (AActor* PlayerStart = FindPlayerStart(Controller))
		{
			return PlayerStart->GetActorTransform();
		}
	}

	return FTransform::Identity;
}
