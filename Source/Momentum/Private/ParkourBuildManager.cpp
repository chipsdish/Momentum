#include "ParkourBuildManager.h"

#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "ParkourBuildPiece.h"
#include "ParkourBuildSaveGame.h"

AParkourBuildManager::AParkourBuildManager()
{
	PrimaryActorTick.bCanEverTick = false;
	BuildPieceClass = AParkourBuildPiece::StaticClass();
}

void AParkourBuildManager::BeginPlay()
{
	Super::BeginPlay();
	RegisterExistingBuildPieces();
}

AParkourBuildPiece* AParkourBuildManager::AddDefaultPiece(EParkourBuildPieceType PieceType)
{
	return AddPieceFromData(MakeDefaultPieceData(PieceType));
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

void AParkourBuildManager::ClearRuntimePieces()
{
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
	for (const FParkourBuildPieceData& PieceData : Layout->Pieces)
	{
		AddPieceFromData(PieceData);
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
		PieceData.Transform.SetRotation(FRotator(PieceData.SlopeAngle, 0.0f, 0.0f).Quaternion());
		break;
	case EParkourBuildPieceType::JumpRamp:
		PieceData.Dimensions = FVector(350.0f, 250.0f, 80.0f);
		PieceData.SlopeAngle = 25.0f;
		PieceData.Transform.SetRotation(FRotator(PieceData.SlopeAngle, 0.0f, 0.0f).Quaternion());
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
		PieceData.Transform.SetRotation(FRotator(0.0f, 0.0f, PieceData.SlopeAngle).Quaternion());
		break;
	case EParkourBuildPieceType::RespawnVolume:
		PieceData.Dimensions = FVector(1000.0f, 1000.0f, 120.0f);
		break;
	case EParkourBuildPieceType::FinishGate:
		PieceData.Transform.AddToTranslation(FVector(1200.0f, 0.0f, 120.0f));
		PieceData.Dimensions = FVector(200.0f, 60.0f, 300.0f);
		break;
	case EParkourBuildPieceType::Sign:
		PieceData.Dimensions = FVector(160.0f, 20.0f, 100.0f);
		PieceData.Label = TEXT("Sign");
		break;
	case EParkourBuildPieceType::BoostPad:
		PieceData.Dimensions = FVector(260.0f, 180.0f, 20.0f);
		PieceData.bBoostPadEnabled = false;
		break;
	default:
		PieceData.Dimensions = FVector(400.0f, 400.0f, 80.0f);
		break;
	}

	return PieceData;
}
