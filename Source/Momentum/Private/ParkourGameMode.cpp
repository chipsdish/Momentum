#include "ParkourGameMode.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerStart.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextRenderActor.h"
#include "Kismet/GameplayStatics.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "ParkourBoostPad.h"
#include "ParkourBuildManager.h"
#include "ParkourCharacter.h"
#include "ParkourFinishVolume.h"
#include "ParkourPlayerController.h"
#include "ParkourRespawnVolume.h"

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

bool AParkourGameMode::HasPlacedGreyboxCourse() const
{
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

	SpawnGreyboxBlock(TEXT("Start Platform"), FVector(0.0f, 0.0f, 0.0f), FRotator::ZeroRotator, FVector(600.0f, 600.0f, 80.0f));
	SpawnCourseLabel(TEXT("Start / 起点"), FVector(0.0f, -360.0f, 160.0f));

	SpawnGreyboxBlock(TEXT("Walkable Slope"), FVector(900.0f, 0.0f, 60.0f), FRotator(18.0f, 0.0f, 0.0f), FVector(900.0f, 420.0f, 70.0f));
	SpawnCourseLabel(TEXT("小坡: 正常行走"), FVector(900.0f, -360.0f, 240.0f));

	SpawnGreyboxBlock(TEXT("Bunny Hop 1"), FVector(1700.0f, 0.0f, 70.0f), FRotator::ZeroRotator, FVector(250.0f, 250.0f, 70.0f));
	SpawnGreyboxBlock(TEXT("Bunny Hop 2"), FVector(2150.0f, 0.0f, 70.0f), FRotator::ZeroRotator, FVector(250.0f, 250.0f, 70.0f));
	SpawnGreyboxBlock(TEXT("Bunny Hop 3"), FVector(2600.0f, 0.0f, 70.0f), FRotator::ZeroRotator, FVector(250.0f, 250.0f, 70.0f));
	SpawnCourseLabel(TEXT("Bunny Hop / 手动连跳"), FVector(2150.0f, -330.0f, 220.0f));

	SpawnGreyboxBlock(TEXT("Surf Ramp"), FVector(3500.0f, 0.0f, 170.0f), FRotator(44.0f, 0.0f, 0.0f), FVector(1200.0f, 500.0f, 70.0f));
	SpawnCourseLabel(TEXT("大斜坡: Surf"), FVector(3500.0f, -420.0f, 420.0f));

	SpawnGreyboxBlock(TEXT("Acceleration Ramp"), FVector(4700.0f, 0.0f, 260.0f), FRotator(50.0f, 0.0f, 0.0f), FVector(1400.0f, 500.0f, 70.0f));
	SpawnCourseLabel(TEXT("真实加速坡: 无 AddImpulse"), FVector(4700.0f, -440.0f, 540.0f));

	SpawnGreyboxBlock(TEXT("Curved Slide 1"), FVector(5900.0f, 0.0f, 380.0f), FRotator(43.0f, 0.0f, 0.0f), FVector(900.0f, 430.0f, 65.0f));
	SpawnGreyboxBlock(TEXT("Curved Slide 2"), FVector(6600.0f, 170.0f, 470.0f), FRotator(43.0f, 20.0f, 0.0f), FVector(900.0f, 430.0f, 65.0f));
	SpawnGreyboxBlock(TEXT("Curved Slide 3"), FVector(7250.0f, 520.0f, 560.0f), FRotator(43.0f, 40.0f, 0.0f), FVector(900.0f, 430.0f, 65.0f));
	SpawnCourseLabel(TEXT("弯曲滑坡: 分段拼接"), FVector(6650.0f, -300.0f, 760.0f));

	SpawnGreyboxBlock(TEXT("Jump Ramp"), FVector(7900.0f, 820.0f, 580.0f), FRotator(28.0f, 40.0f, 0.0f), FVector(550.0f, 360.0f, 75.0f));
	SpawnCourseLabel(TEXT("跳台斜坡: Bunny Hop / 空中转向"), FVector(7900.0f, 420.0f, 820.0f));

	SpawnGreyboxBlock(TEXT("Air Platform"), FVector(8750.0f, 1350.0f, 750.0f), FRotator::ZeroRotator, FVector(620.0f, 520.0f, 70.0f));
	SpawnCourseLabel(TEXT("空中平台"), FVector(8750.0f, 1000.0f, 920.0f));

	AParkourBoostPad* BoostPad = World->SpawnActor<AParkourBoostPad>(AParkourBoostPad::StaticClass(), FVector(8350.0f, -780.0f, 120.0f), FRotator(0.0f, 0.0f, 0.0f));
#if WITH_EDITOR
	if (BoostPad)
	{
		BoostPad->SetActorLabel(TEXT("Boost Pad - Optional Trigger"));
	}
#endif
	SpawnCourseLabel(TEXT("Boost Pad: 独立触发测试模块"), FVector(8350.0f, -1120.0f, 300.0f));

	AParkourFinishVolume* FinishVolume = World->SpawnActor<AParkourFinishVolume>(AParkourFinishVolume::StaticClass(), FVector(9650.0f, 1350.0f, 890.0f), FRotator::ZeroRotator);
	if (FinishVolume && FinishVolume->TriggerVolume)
	{
		FinishVolume->TriggerVolume->SetBoxExtent(FVector(160.0f, 240.0f, 240.0f));
	}
	SpawnCourseLabel(TEXT("Finish / 终点"), FVector(9650.0f, 1000.0f, 1130.0f));

	AParkourRespawnVolume* RespawnVolume = World->SpawnActor<AParkourRespawnVolume>(AParkourRespawnVolume::StaticClass(), FVector(4800.0f, 300.0f, -900.0f), FRotator::ZeroRotator);
	if (RespawnVolume && RespawnVolume->TriggerVolume)
	{
		RespawnVolume->TriggerVolume->SetBoxExtent(FVector(11500.0f, 4500.0f, 160.0f));
	}

	World->SpawnActor<AParkourBuildManager>(AParkourBuildManager::StaticClass(), FVector(0.0f, 900.0f, 120.0f), FRotator::ZeroRotator);
}

AActor* AParkourGameMode::SpawnGreyboxBlock(const FString& Name, const FVector& Location, const FRotator& Rotation, const FVector& Dimensions)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	static UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	AStaticMeshActor* Block = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Location, Rotation);
	if (!Block)
	{
		return nullptr;
	}

#if WITH_EDITOR
	Block->SetActorLabel(Name);
#endif

	UStaticMeshComponent* MeshComponent = Block->GetStaticMeshComponent();
	if (MeshComponent)
	{
		MeshComponent->SetStaticMesh(CubeMesh);
		MeshComponent->SetMobility(EComponentMobility::Static);
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
	}

	Block->SetActorScale3D(Dimensions / 100.0f);
	return Block;
}

void AParkourGameMode::SpawnCourseLabel(const FString& Text, const FVector& Location)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ATextRenderActor* Label = World->SpawnActor<ATextRenderActor>(ATextRenderActor::StaticClass(), Location, FRotator(0.0f, 180.0f, 0.0f));
	if (!Label || !Label->GetTextRender())
	{
		return;
	}

	Label->GetTextRender()->SetText(FText::FromString(Text));
	Label->GetTextRender()->SetWorldSize(72.0f);
	Label->GetTextRender()->SetHorizontalAlignment(EHTA_Center);
}
