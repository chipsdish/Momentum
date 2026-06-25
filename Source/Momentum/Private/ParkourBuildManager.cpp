#include "ParkourBuildManager.h"

#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "ParkourBuildPiece.h"
#include "ParkourBuildSaveGame.h"

namespace
{
	bool IsRampLikePieceType(EParkourBuildPieceType PieceType)
	{
		return PieceType == EParkourBuildPieceType::Ramp
			|| PieceType == EParkourBuildPieceType::JumpRamp
			|| PieceType == EParkourBuildPieceType::AccelerationRamp;
	}

	float GetRampRunForData(const FParkourBuildPieceData& PieceData)
	{
		const FVector SafeDimensions = PieceData.Dimensions.ComponentMax(FVector(10.0f));
		return PieceData.bUseInwardBank ? SafeDimensions.Y : SafeDimensions.X;
	}

	float GetRampRiseFromSlope(const FParkourBuildPieceData& PieceData)
	{
		const float AngleRadians = FMath::DegreesToRadians(FMath::Clamp(PieceData.SlopeAngle, 0.0f, 85.0f));
		return FMath::Max(FMath::Tan(AngleRadians) * GetRampRunForData(PieceData), 1.0f);
	}

	float GetRampSlopeFromRise(const FParkourBuildPieceData& PieceData)
	{
		const float Run = FMath::Max(GetRampRunForData(PieceData), 1.0f);
		const float Rise = FMath::Max(PieceData.Dimensions.Z, 1.0f);
		return FMath::Clamp(FMath::RadiansToDegrees(FMath::Atan(Rise / Run)), 0.0f, 85.0f);
	}

	void SyncRampHeightToSlope(FParkourBuildPieceData& PieceData)
	{
		if (IsRampLikePieceType(PieceData.PieceType))
		{
			PieceData.Dimensions.Z = GetRampRiseFromSlope(PieceData);
		}
	}
}

AParkourBuildManager::AParkourBuildManager()
{
	PrimaryActorTick.bCanEverTick = false;
	BuildPieceClass = AParkourBuildPiece::StaticClass();
}

void AParkourBuildManager::BeginPlay()
{
	Super::BeginPlay();
	RegisterExistingBuildPieces();
	TryAutoLoadSavedLayout();

	for (AParkourBuildPiece* Piece : RuntimePieces)
	{
		if (Piece)
		{
			Piece->SetBuildModeVisuals(false);
		}
	}
}

AParkourBuildPiece* AParkourBuildManager::AddDefaultPiece(EParkourBuildPieceType PieceType)
{
	return AddPieceFromData(MakeDefaultPieceData(PieceType));
}

bool AParkourBuildManager::BeginPreviewPlacement(EParkourBuildPieceType PieceType)
{
	if (!GetWorld())
	{
		return false;
	}

	CancelPreviewPlacement();
	ClearSelection();

	PreviewPieceData = MakeDefaultPieceData(PieceType);
	PreviewPieceData.PieceId = FGuid::NewGuid();
	PreviewPieceData.Transform.SetLocation(SnapLocation(PreviewPieceData.Transform.GetLocation()));

	TSubclassOf<AParkourBuildPiece> SpawnClass = BuildPieceClass;
	if (!SpawnClass)
	{
		SpawnClass = AParkourBuildPiece::StaticClass();
	}
	PreviewPiece = GetWorld()->SpawnActor<AParkourBuildPiece>(SpawnClass, PreviewPieceData.Transform);
	if (!PreviewPiece)
	{
		return false;
	}

	PreviewPiece->ConfigureFromData(PreviewPieceData);
	PreviewPiece->SetPreviewMode(true);
	return true;
}

bool AParkourBuildManager::ConfirmPreviewPlacement()
{
	if (!PreviewPiece)
	{
		return false;
	}

	FParkourBuildPieceData PlacedData = PreviewPiece->ToData();
	PlacedData.PieceId = FGuid::NewGuid();

	CancelPreviewPlacement();
	return AddPieceFromData(PlacedData) != nullptr;
}

void AParkourBuildManager::CancelPreviewPlacement()
{
	if (PreviewPiece)
	{
		PreviewPiece->Destroy();
		PreviewPiece = nullptr;
	}
}

void AParkourBuildManager::UpdatePreviewFromController(APlayerController* Controller)
{
	if (!PreviewPiece || !Controller || !GetWorld())
	{
		return;
	}

	FVector RayOrigin = FVector::ZeroVector;
	FVector RayDirection = FVector::ForwardVector;
	if (!Controller->DeprojectMousePositionToWorld(RayOrigin, RayDirection))
	{
		if (Controller->PlayerCameraManager)
		{
			RayOrigin = Controller->PlayerCameraManager->GetCameraLocation();
			RayDirection = Controller->PlayerCameraManager->GetCameraRotation().Vector();
		}
	}

	RayDirection = RayDirection.GetSafeNormal();
	const FVector RayEnd = RayOrigin + RayDirection * PlacementTraceDistance;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ParkourBuildPreviewTrace), false);
	QueryParams.AddIgnoredActor(PreviewPiece);
	if (APawn* Pawn = Controller->GetPawn())
	{
		QueryParams.AddIgnoredActor(Pawn);
	}

	FHitResult Hit;
	FVector TargetLocation = FVector::ZeroVector;
	if (GetWorld()->LineTraceSingleByChannel(Hit, RayOrigin, RayEnd, ECC_Visibility, QueryParams))
	{
		TargetLocation = Hit.Location;
	}
	else
	{
		const FPlane GroundPlane(FVector::ZeroVector, FVector::UpVector);
		const FVector PlaneHit = FMath::LinePlaneIntersection(RayOrigin, RayEnd, GroundPlane);
		TargetLocation = PlaneHit.ContainsNaN() ? GetActorLocation() : PlaneHit;
	}

	TargetLocation += GetPlacementOffset(PreviewPieceData);
	PreviewPieceData.Transform.SetLocation(SnapLocation(TargetLocation));
	ApplyPreviewData();
}

void AParkourBuildManager::SetPreviewDimensions(const FVector& NewDimensions)
{
	if (!PreviewPiece)
	{
		return;
	}

	const FVector OldDimensions = PreviewPieceData.Dimensions;
	PreviewPieceData.Dimensions = NewDimensions.ComponentMax(FVector(10.0f));
	if (IsRampLikePieceType(PreviewPieceData.PieceType))
	{
		const bool bHeightChanged = !FMath::IsNearlyEqual(PreviewPieceData.Dimensions.Z, OldDimensions.Z, 0.1f);
		if (bHeightChanged)
		{
			PreviewPieceData.SlopeAngle = GetRampSlopeFromRise(PreviewPieceData);
		}
		else
		{
			SyncRampHeightToSlope(PreviewPieceData);
		}
	}
	ApplyPreviewData();
}

void AParkourBuildManager::SetPreviewSlopeAngle(float NewSlopeAngle)
{
	if (!PreviewPiece)
	{
		return;
	}

	PreviewPieceData.SlopeAngle = FMath::Clamp(FMath::Abs(NewSlopeAngle), 0.0f, 85.0f);
	SyncRampHeightToSlope(PreviewPieceData);
	ApplyPreviewData();
}

void AParkourBuildManager::SetPreviewRotationYaw(float NewYaw)
{
	if (!PreviewPiece)
	{
		return;
	}

	FRotator Rotation = PreviewPieceData.Transform.Rotator();
	Rotation.Pitch = 0.0f;
	Rotation.Roll = 0.0f;
	Rotation.Yaw = NewYaw;
	PreviewPieceData.Transform.SetRotation(Rotation.Quaternion());
	ApplyPreviewData();
}

void AParkourBuildManager::SetGridSnapSize(float NewGridSnapSize)
{
	GridSnapSize = FMath::Clamp(NewGridSnapSize, 1.0f, 1000.0f);
	if (PreviewPiece)
	{
		PreviewPieceData.Transform.SetLocation(SnapLocation(PreviewPieceData.Transform.GetLocation()));
		ApplyPreviewData();
	}
}

AParkourBuildPiece* AParkourBuildManager::AddPieceFromData(const FParkourBuildPieceData& PieceData)
{
	if (!GetWorld())
	{
		return nullptr;
	}

	TSubclassOf<AParkourBuildPiece> SpawnClass = BuildPieceClass;
	if (!SpawnClass)
	{
		SpawnClass = AParkourBuildPiece::StaticClass();
	}
	AParkourBuildPiece* Piece = GetWorld()->SpawnActor<AParkourBuildPiece>(SpawnClass, PieceData.Transform);
	if (!Piece)
	{
		return nullptr;
	}

	Piece->ConfigureFromData(PieceData);
	RuntimePieces.AddUnique(Piece);
	SelectPiece(Piece);
	return Piece;
}

void AParkourBuildManager::SelectPiece(AParkourBuildPiece* Piece)
{
	if (SelectedPiece)
	{
		SelectedPiece->SetSelected(false);
	}

	SelectedPiece = Piece;
	if (SelectedPiece)
	{
		RuntimePieces.AddUnique(SelectedPiece);
		SelectedPiece->SetSelected(true);
	}
}

void AParkourBuildManager::ClearSelection()
{
	SelectPiece(nullptr);
}

void AParkourBuildManager::DeleteSelectedPiece()
{
	if (!SelectedPiece)
	{
		return;
	}

	AParkourBuildPiece* PieceToDelete = SelectedPiece;
	RuntimePieces.Remove(PieceToDelete);
	SelectedPiece = nullptr;
	PieceToDelete->Destroy();
}

AParkourBuildPiece* AParkourBuildManager::DuplicateSelectedPiece()
{
	if (!SelectedPiece)
	{
		return nullptr;
	}

	FParkourBuildPieceData DuplicatedData = SelectedPiece->ToData();
	DuplicatedData.PieceId = FGuid::NewGuid();
	DuplicatedData.Transform.AddToTranslation(FVector(DuplicateOffset, DuplicateOffset, 0.0f));
	return AddPieceFromData(DuplicatedData);
}

void AParkourBuildManager::AdjustSelectedSlopeAngle(float DeltaAngle)
{
	if (SelectedPiece)
	{
		SelectedPiece->AdjustSlopeAngle(DeltaAngle);
	}
}

void AParkourBuildManager::SetSelectedSlopeAngle(float NewSlopeAngle)
{
	if (SelectedPiece)
	{
		SelectedPiece->SetSlopeAngle(NewSlopeAngle);
	}
}

void AParkourBuildManager::AdjustSelectedDimensions(const FVector& DeltaDimensions)
{
	if (SelectedPiece)
	{
		SelectedPiece->AdjustDimensions(DeltaDimensions);
	}
}

void AParkourBuildManager::ClearRuntimePieces()
{
	CancelPreviewPlacement();
	ClearSelection();

	for (AParkourBuildPiece* Piece : RuntimePieces)
	{
		if (Piece)
		{
			Piece->Destroy();
		}
	}

	RuntimePieces.Empty();
}

bool AParkourBuildManager::SaveLayout(const FString& LayoutName)
{
	if (LayoutName.IsEmpty())
	{
		return false;
	}

	UParkourBuildSaveGame* SaveGame = LoadOrCreateSaveGame();
	if (!SaveGame)
	{
		return false;
	}

	FParkourBuildLayoutData NewLayout;
	NewLayout.LayoutName = LayoutName;

	for (AParkourBuildPiece* Piece : RuntimePieces)
	{
		if (Piece)
		{
			NewLayout.Pieces.Add(Piece->ToData());
		}
	}

	const int32 ExistingIndex = SaveGame->Layouts.IndexOfByPredicate([&LayoutName](const FParkourBuildLayoutData& Layout)
	{
		return Layout.LayoutName == LayoutName;
	});

	if (ExistingIndex == INDEX_NONE)
	{
		SaveGame->Layouts.Add(NewLayout);
	}
	else
	{
		SaveGame->Layouts[ExistingIndex] = NewLayout;
	}

	return UGameplayStatics::SaveGameToSlot(SaveGame, SaveSlotName, SaveUserIndex);
}

bool AParkourBuildManager::LoadLayout(const FString& LayoutName)
{
	UParkourBuildSaveGame* SaveGame = LoadOrCreateSaveGame();
	if (!SaveGame)
	{
		return false;
	}

	const FParkourBuildLayoutData* Layout = SaveGame->Layouts.FindByPredicate([&LayoutName](const FParkourBuildLayoutData& Candidate)
	{
		return Candidate.LayoutName == LayoutName;
	});

	if (!Layout)
	{
		return false;
	}

	ClearRuntimePieces();
	bool bLoadedFinishGate = false;
	FVector SuggestedFinishLocation = GetActorLocation() + FVector(1200.0f, 0.0f, 120.0f);
	for (const FParkourBuildPieceData& PieceData : Layout->Pieces)
	{
		const FVector PieceLocation = PieceData.Transform.GetLocation();
		if (PieceLocation.X >= SuggestedFinishLocation.X)
		{
			SuggestedFinishLocation = PieceLocation + FVector(800.0f, 0.0f, 180.0f);
		}

		bLoadedFinishGate = bLoadedFinishGate || PieceData.PieceType == EParkourBuildPieceType::FinishGate;
		AddPieceFromData(PieceData);
	}

	if (!bLoadedFinishGate)
	{
		FParkourBuildPieceData FinishGateData = MakeDefaultPieceData(EParkourBuildPieceType::FinishGate);
		FinishGateData.Transform.SetLocation(SuggestedFinishLocation);
		AddPieceFromData(FinishGateData);
	}

	ClearSelection();
	return true;
}

bool AParkourBuildManager::DeleteLayout(const FString& LayoutName)
{
	UParkourBuildSaveGame* SaveGame = LoadOrCreateSaveGame();
	if (!SaveGame)
	{
		return false;
	}

	const int32 RemovedCount = SaveGame->Layouts.RemoveAll([&LayoutName](const FParkourBuildLayoutData& Layout)
	{
		return Layout.LayoutName == LayoutName;
	});

	return RemovedCount > 0 && UGameplayStatics::SaveGameToSlot(SaveGame, SaveSlotName, SaveUserIndex);
}

bool AParkourBuildManager::TryAutoLoadSavedLayout()
{
	if (!bAutoLoadSavedLayout || AutoLoadLayoutName.IsEmpty())
	{
		return false;
	}

	return LoadLayout(AutoLoadLayoutName);
}

TArray<FString> AParkourBuildManager::GetSavedLayoutNames() const
{
	TArray<FString> LayoutNames;

	const UParkourBuildSaveGame* SaveGame = LoadOrCreateSaveGame();
	if (!SaveGame)
	{
		return LayoutNames;
	}

	for (const FParkourBuildLayoutData& Layout : SaveGame->Layouts)
	{
		LayoutNames.Add(Layout.LayoutName);
	}

	return LayoutNames;
}

void AParkourBuildManager::ResetDefaultRuntimeLayout()
{
	ClearRuntimePieces();

	AddPieceFromData(MakeDefaultPieceData(EParkourBuildPieceType::AirPlatform));
	AddPieceFromData(MakeDefaultPieceData(EParkourBuildPieceType::WallPlatform));
	AddPieceFromData(MakeDefaultPieceData(EParkourBuildPieceType::BoostPad));
	AddPieceFromData(MakeDefaultPieceData(EParkourBuildPieceType::FinishGate));
	ClearSelection();
}

void AParkourBuildManager::RegisterExistingBuildPieces()
{
	RuntimePieces.RemoveAll([](const TObjectPtr<AParkourBuildPiece>& Piece)
	{
		return !IsValid(Piece);
	});

	if (!GetWorld())
	{
		return;
	}

	for (TActorIterator<AParkourBuildPiece> It(GetWorld()); It; ++It)
	{
		RuntimePieces.AddUnique(*It);
	}
}

UParkourBuildSaveGame* AParkourBuildManager::LoadOrCreateSaveGame() const
{
	if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, SaveUserIndex))
	{
		if (UParkourBuildSaveGame* ExistingSave = Cast<UParkourBuildSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, SaveUserIndex)))
		{
			return ExistingSave;
		}
	}

	return Cast<UParkourBuildSaveGame>(UGameplayStatics::CreateSaveGameObject(UParkourBuildSaveGame::StaticClass()));
}

FParkourBuildPieceData AParkourBuildManager::MakeDefaultPieceData(EParkourBuildPieceType PieceType) const
{
	FParkourBuildPieceData PieceData;
	PieceData.PieceType = PieceType;
	PieceData.Transform = FTransform(GetActorRotation(), GetActorLocation() + FVector(0.0f, 0.0f, 120.0f));

	switch (PieceType)
	{
	case EParkourBuildPieceType::Ramp:
		PieceData.Dimensions = FVector(600.0f, 300.0f, 60.0f);
		PieceData.SlopeAngle = 35.0f;
		break;
	case EParkourBuildPieceType::JumpRamp:
		PieceData.Dimensions = FVector(350.0f, 250.0f, 80.0f);
		PieceData.SlopeAngle = 25.0f;
		break;
	case EParkourBuildPieceType::AirPlatform:
		PieceData.Transform.AddToTranslation(FVector(600.0f, 0.0f, 300.0f));
		PieceData.Dimensions = FVector(350.0f, 350.0f, 60.0f);
		break;
	case EParkourBuildPieceType::WallPlatform:
		PieceData.Transform.AddToTranslation(FVector(0.0f, 500.0f, 160.0f));
		PieceData.Dimensions = FVector(800.0f, 80.0f, 80.0f);
		break;
	case EParkourBuildPieceType::AccelerationRamp:
		PieceData.Dimensions = FVector(900.0f, 350.0f, 80.0f);
		PieceData.SlopeAngle = 45.0f;
		PieceData.bUseInwardBank = true;
		break;
	case EParkourBuildPieceType::RespawnVolume:
		PieceData.Dimensions = FVector(1000.0f, 1000.0f, 120.0f);
		break;
	case EParkourBuildPieceType::FinishGate:
		PieceData.Transform.AddToTranslation(FVector(1200.0f, 0.0f, 120.0f));
		PieceData.Dimensions = FVector(320.0f, 480.0f, 480.0f);
		PieceData.Label = TEXT("Finish Gate");
		break;
	case EParkourBuildPieceType::Sign:
		PieceData.Dimensions = FVector(160.0f, 20.0f, 100.0f);
		PieceData.Label = TEXT("Sign");
		break;
	case EParkourBuildPieceType::BoostPad:
		PieceData.Transform.AddToTranslation(FVector(450.0f, -450.0f, -80.0f));
		PieceData.Dimensions = FVector(260.0f, 180.0f, 24.0f);
		PieceData.bBoostPadEnabled = true;
		PieceData.BoostStrength = 1400.0f;
		PieceData.Label = TEXT("Boost Pad");
		break;
	default:
		PieceData.Dimensions = FVector(400.0f, 400.0f, 80.0f);
		break;
	}

	SyncRampHeightToSlope(PieceData);
	return PieceData;
}

FVector AParkourBuildManager::SnapLocation(const FVector& Location) const
{
	if (GridSnapSize <= KINDA_SMALL_NUMBER)
	{
		return Location;
	}

	return FVector(
		FMath::GridSnap(Location.X, GridSnapSize),
		FMath::GridSnap(Location.Y, GridSnapSize),
		FMath::GridSnap(Location.Z, GridSnapSize));
}

FVector AParkourBuildManager::GetPlacementOffset(const FParkourBuildPieceData& PieceData) const
{
	return IsRampLikePieceType(PieceData.PieceType) ? FVector::ZeroVector : FVector(0.0f, 0.0f, PieceData.Dimensions.Z * 0.5f);
}

void AParkourBuildManager::ApplyPreviewData()
{
	if (!PreviewPiece)
	{
		return;
	}

	PreviewPiece->ConfigureFromData(PreviewPieceData);
	PreviewPiece->SetPreviewMode(true);
}
