#include "ParkourGameMode.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerStart.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Kismet/GameplayStatics.h"
#include "Components/BoxComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "ParkourBuildManager.h"
#include "ParkourBuildPiece.h"
#include "ParkourCharacter.h"
#include "ParkourPlayerController.h"
#include "ParkourRespawnVolume.h"
#include "ParkourWorldLabel.h"

AParkourGameMode::AParkourGameMode()
{
	DefaultPawnClass = AParkourCharacter::StaticClass();
	PlayerControllerClass = AParkourPlayerController::StaticClass();
}

void AParkourGameMode::BeginPlay()
{
	Super::BeginPlay();

	const FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(this, true);
	const bool bIsMainMenu = CurrentLevelName == TEXT("MainMenu");

	EnsureBasicLighting();

	if (bSpawnDefaultGreyboxCourse && !bIsMainMenu && !HasPlacedGreyboxCourse())
	{
		SpawnDefaultGreyboxCourse();
	}

	if (bAutoStartRunTimer && !bIsMainMenu)
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

void AParkourGameMode::EnsureBasicLighting()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!UGameplayStatics::GetActorOfClass(this, ADirectionalLight::StaticClass()))
	{
		ADirectionalLight* DirectionalLight = World->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), FVector::ZeroVector, FRotator(-45.0f, -35.0f, 0.0f));
		if (DirectionalLight && DirectionalLight->GetLightComponent())
		{
			DirectionalLight->GetLightComponent()->SetMobility(EComponentMobility::Movable);
			DirectionalLight->GetLightComponent()->SetIntensity(6.0f);
			if (UDirectionalLightComponent* DirectionalLightComponent = Cast<UDirectionalLightComponent>(DirectionalLight->GetLightComponent()))
			{
				DirectionalLightComponent->SetAtmosphereSunLight(true);
				DirectionalLightComponent->SetAtmosphereSunLightIndex(0);
			}
		}
#if WITH_EDITOR
		if (DirectionalLight)
		{
			DirectionalLight->SetActorLabel(TEXT("Runtime Safety Directional Light"));
		}
#endif
	}

	if (!UGameplayStatics::GetActorOfClass(this, ASkyLight::StaticClass()))
	{
		ASkyLight* SkyLight = World->SpawnActor<ASkyLight>(ASkyLight::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
		if (SkyLight && SkyLight->GetLightComponent())
		{
			SkyLight->GetLightComponent()->SetMobility(EComponentMobility::Movable);
			SkyLight->GetLightComponent()->SetIntensity(1.6f);
		}
#if WITH_EDITOR
		if (SkyLight)
		{
			SkyLight->SetActorLabel(TEXT("Runtime Safety Sky Light"));
		}
#endif
	}

	if (!UGameplayStatics::GetActorOfClass(this, ASkyAtmosphere::StaticClass()))
	{
		ASkyAtmosphere* SkyAtmosphere = World->SpawnActor<ASkyAtmosphere>(ASkyAtmosphere::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
#if WITH_EDITOR
		if (SkyAtmosphere)
		{
			SkyAtmosphere->SetActorLabel(TEXT("Runtime Safety Sky Atmosphere"));
		}
#endif
	}
}

bool AParkourGameMode::HasPlacedGreyboxCourse() const
{
	TArray<AActor*> BuildPieces;
	UGameplayStatics::GetAllActorsOfClass(this, AParkourBuildPiece::StaticClass(), BuildPieces);
	if (BuildPieces.Num() > 0)
	{
		return true;
	}

	TArray<AActor*> TaggedActors;
	UGameplayStatics::GetAllActorsWithTag(this, PlacedCourseTag, TaggedActors);
	return TaggedActors.Num() > 0;
}

void AParkourGameMode::SpawnDefaultGreyboxCourse()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	SetRespawnTransform(FTransform(FRotator(0.0f, 0.0f, 0.0f), FVector(0.0f, 0.0f, 220.0f)));

	World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), FVector(0.0f, 0.0f, 180.0f), FRotator::ZeroRotator);

	SpawnGreyboxBlock(TEXT("Start Platform"), FVector(0.0f, 0.0f, 0.0f), FRotator::ZeroRotator, FVector(1000.0f, 1000.0f, 80.0f));
	SpawnCourseLabel(TEXT("Start / 起点"), FVector(0.0f, -620.0f, 160.0f));

	SpawnGreyboxBlock(TEXT("Walkable Slope"), FVector(1300.0f, 0.0f, 60.0f), FRotator::ZeroRotator, FVector(900.0f, 420.0f, 70.0f), EParkourBuildPieceType::Ramp, 18.0f, false);
	SpawnCourseLabel(TEXT("小坡: 正常行走"), FVector(1300.0f, -520.0f, 260.0f));

	SpawnGreyboxBlock(TEXT("Bunny Hop 1"), FVector(2500.0f, 0.0f, 70.0f), FRotator::ZeroRotator, FVector(250.0f, 250.0f, 70.0f));
	SpawnGreyboxBlock(TEXT("Bunny Hop 2"), FVector(3300.0f, 0.0f, 70.0f), FRotator::ZeroRotator, FVector(250.0f, 250.0f, 70.0f));
	SpawnGreyboxBlock(TEXT("Bunny Hop 3"), FVector(4100.0f, 0.0f, 70.0f), FRotator::ZeroRotator, FVector(250.0f, 250.0f, 70.0f));
	SpawnCourseLabel(TEXT("Bunny Hop / 手动连跳"), FVector(3300.0f, -520.0f, 240.0f));

	SpawnGreyboxBlock(TEXT("Surf Ramp Left"), FVector(5600.0f, -560.0f, 170.0f), FRotator::ZeroRotator, FVector(1200.0f, 500.0f, 70.0f), EParkourBuildPieceType::Ramp, 44.0f, true);
	SpawnCourseLabel(TEXT("左侧大斜坡: Surf"), FVector(5600.0f, -1120.0f, 430.0f));

	SpawnGreyboxBlock(TEXT("Acceleration Ramp Right"), FVector(7400.0f, 560.0f, 260.0f), FRotator::ZeroRotator, FVector(1400.0f, 500.0f, 70.0f), EParkourBuildPieceType::AccelerationRamp, 50.0f, true);
	SpawnCourseLabel(TEXT("右侧真实加速坡: 无 AddImpulse"), FVector(7400.0f, 1120.0f, 560.0f));

	SpawnGreyboxBlock(TEXT("Curved Slide Left 1"), FVector(9200.0f, -560.0f, 390.0f), FRotator::ZeroRotator, FVector(1000.0f, 430.0f, 65.0f), EParkourBuildPieceType::Ramp, 43.0f, true);
	SpawnGreyboxBlock(TEXT("Curved Slide Right 2"), FVector(10800.0f, 560.0f, 500.0f), FRotator::ZeroRotator, FVector(1000.0f, 430.0f, 65.0f), EParkourBuildPieceType::Ramp, 43.0f, true);
	SpawnGreyboxBlock(TEXT("Curved Slide Left 3"), FVector(12400.0f, -560.0f, 610.0f), FRotator::ZeroRotator, FVector(1000.0f, 430.0f, 65.0f), EParkourBuildPieceType::Ramp, 43.0f, true);
	SpawnCourseLabel(TEXT("弯曲滑坡: 左右交替分段"), FVector(10800.0f, 0.0f, 820.0f));

	SpawnGreyboxBlock(TEXT("Jump Ramp"), FVector(14200.0f, 560.0f, 660.0f), FRotator(0.0f, 18.0f, 0.0f), FVector(650.0f, 380.0f, 75.0f), EParkourBuildPieceType::JumpRamp, 28.0f, false);
	SpawnCourseLabel(TEXT("跳台斜坡: Bunny Hop / 空中转向"), FVector(14200.0f, 1060.0f, 900.0f));

	SpawnGreyboxBlock(TEXT("Air Platform"), FVector(15600.0f, 560.0f, 800.0f), FRotator::ZeroRotator, FVector(720.0f, 560.0f, 70.0f));
	SpawnCourseLabel(TEXT("空中平台"), FVector(15600.0f, 1060.0f, 980.0f));

	SpawnGreyboxBlock(TEXT("Boost Pad - Optional Trigger"), FVector(7400.0f, -1120.0f, 120.0f), FRotator::ZeroRotator, FVector(260.0f, 180.0f, 24.0f), EParkourBuildPieceType::BoostPad);
	SpawnCourseLabel(TEXT("Boost Pad: 独立触发测试模块"), FVector(7400.0f, -1540.0f, 320.0f));

	SpawnGreyboxBlock(TEXT("Finish Gate"), FVector(17000.0f, 560.0f, 940.0f), FRotator::ZeroRotator, FVector(320.0f, 480.0f, 480.0f), EParkourBuildPieceType::FinishGate);
	SpawnCourseLabel(TEXT("Finish / 终点"), FVector(17000.0f, 1060.0f, 1180.0f));

	AParkourRespawnVolume* RespawnVolume = World->SpawnActor<AParkourRespawnVolume>(AParkourRespawnVolume::StaticClass(), FVector(8500.0f, 0.0f, -900.0f), FRotator::ZeroRotator);
	if (RespawnVolume && RespawnVolume->TriggerVolume)
	{
		RespawnVolume->TriggerVolume->SetBoxExtent(FVector(19000.0f, 5200.0f, 160.0f));
	}

	World->SpawnActor<AParkourBuildManager>(AParkourBuildManager::StaticClass(), FVector(0.0f, 900.0f, 120.0f), FRotator::ZeroRotator);
}

AActor* AParkourGameMode::SpawnGreyboxBlock(const FString& Name, const FVector& Location, const FRotator& Rotation, const FVector& Dimensions, EParkourBuildPieceType PieceType, float SlopeAngle, bool bUseInwardBank)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	AParkourBuildPiece* Block = World->SpawnActor<AParkourBuildPiece>(AParkourBuildPiece::StaticClass(), Location, Rotation);
	if (!Block)
	{
		return nullptr;
	}

#if WITH_EDITOR
	Block->SetActorLabel(Name);
#endif

	FParkourBuildPieceData PieceData;
	PieceData.PieceType = PieceType;
	PieceData.Transform = FTransform(Rotation, Location);
	PieceData.Dimensions = Dimensions;
	PieceData.SlopeAngle = FMath::Max(0.0f, SlopeAngle);
	PieceData.bUseInwardBank = bUseInwardBank;
	PieceData.Label = Name;
	if (PieceType == EParkourBuildPieceType::BoostPad)
	{
		PieceData.bBoostPadEnabled = true;
		PieceData.BoostStrength = 1400.0f;
	}
	Block->ConfigureFromData(PieceData);
	if (SlopeAngle > KINDA_SMALL_NUMBER)
	{
		Block->SetUseInwardBank(bUseInwardBank);
		Block->SetSlopeAngle(SlopeAngle);
	}
	Block->Tags.AddUnique(PlacedCourseTag);
	return Block;
}

void AParkourGameMode::SpawnCourseLabel(const FString& Text, const FVector& Location)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	AParkourWorldLabel* Label = World->SpawnActor<AParkourWorldLabel>(AParkourWorldLabel::StaticClass(), Location, FRotator::ZeroRotator);
	if (!Label)
	{
		return;
	}

	Label->SetLabelText(Text, 30.0f);
}
