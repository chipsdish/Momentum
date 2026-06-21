#include "ParkourBuildPiece.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"

AParkourBuildPiece::AParkourBuildPiece()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(SceneRoot);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionResponseToAllChannels(ECR_Block);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	SelectionBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("SelectionBounds"));
	SelectionBounds->SetupAttachment(SceneRoot);
	SelectionBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SelectionBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	SelectionBounds->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SelectionBounds->SetHiddenInGame(true);
	SelectionBounds->SetVisibility(false);

	LabelText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("LabelText"));
	LabelText->SetupAttachment(SceneRoot);
	LabelText->SetHorizontalAlignment(EHTA_Center);
	LabelText->SetVerticalAlignment(EVRTA_TextCenter);
	LabelText->SetWorldSize(64.0f);
	LabelText->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
	LabelText->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	LabelText->SetHiddenInGame(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
	}

	ApplyPieceVisuals();
}

void AParkourBuildPiece::ConfigureFromData(const FParkourBuildPieceData& NewPieceData)
{
	PieceData = NewPieceData;
	if (!PieceData.PieceId.IsValid())
	{
		PieceData.PieceId = FGuid::NewGuid();
	}

	SetActorTransform(PieceData.Transform);
	if (PieceData.SlopeAngle > KINDA_SMALL_NUMBER)
	{
		ApplySlopeRotation();
	}
	ApplyPieceVisuals();
}

FParkourBuildPieceData AParkourBuildPiece::ToData() const
{
	FParkourBuildPieceData Result = PieceData;
	Result.Transform = GetActorTransform();
	return Result;
}

void AParkourBuildPiece::SetSelected(bool bSelected)
{
	if (Mesh)
	{
		Mesh->SetRenderCustomDepth(bSelected);
		Mesh->MarkRenderStateDirty();
	}

	if (LabelText)
	{
		LabelText->SetHiddenInGame(!bSelected && PieceData.PieceType != EParkourBuildPieceType::Sign);
	}
}

void AParkourBuildPiece::SetBuildModeVisuals(bool bBuildModeActive)
{
	SetSelected(false);

	if (Mesh)
	{
		Mesh->SetHiddenInGame(false);
		Mesh->SetVisibility(true, true);
		Mesh->SetRenderCustomDepth(false);
		Mesh->MarkRenderStateDirty();
	}

	if (SelectionBounds)
	{
		SelectionBounds->SetCollisionEnabled(bBuildModeActive ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
		SelectionBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
		SelectionBounds->SetCollisionResponseToChannel(ECC_Visibility, bBuildModeActive ? ECR_Block : ECR_Ignore);
		SelectionBounds->SetHiddenInGame(true);
		SelectionBounds->SetVisibility(false, true);
	}

	if (LabelText)
	{
		LabelText->SetHiddenInGame(PieceData.PieceType != EParkourBuildPieceType::Sign);
	}
}

void AParkourBuildPiece::SetDimensions(const FVector& NewDimensions)
{
	PieceData.Dimensions = NewDimensions.ComponentMax(FVector(10.0f));
	ApplyPieceVisuals();
}

void AParkourBuildPiece::SetLabelText(const FString& NewLabel)
{
	PieceData.Label = NewLabel;
	ApplyPieceVisuals();
}

void AParkourBuildPiece::SetPieceType(EParkourBuildPieceType NewPieceType)
{
	PieceData.PieceType = NewPieceType;
	ApplyPieceVisuals();
}

void AParkourBuildPiece::SetPieceTypeName(const FString& NewPieceTypeName)
{
	if (NewPieceTypeName.Equals(TEXT("Ramp"), ESearchCase::IgnoreCase))
	{
		SetPieceType(EParkourBuildPieceType::Ramp);
	}
	else if (NewPieceTypeName.Equals(TEXT("JumpRamp"), ESearchCase::IgnoreCase))
	{
		SetPieceType(EParkourBuildPieceType::JumpRamp);
	}
	else if (NewPieceTypeName.Equals(TEXT("WallPlatform"), ESearchCase::IgnoreCase))
	{
		SetPieceType(EParkourBuildPieceType::WallPlatform);
	}
	else if (NewPieceTypeName.Equals(TEXT("AirPlatform"), ESearchCase::IgnoreCase))
	{
		SetPieceType(EParkourBuildPieceType::AirPlatform);
	}
	else if (NewPieceTypeName.Equals(TEXT("AccelerationRamp"), ESearchCase::IgnoreCase))
	{
		SetPieceType(EParkourBuildPieceType::AccelerationRamp);
	}
	else if (NewPieceTypeName.Equals(TEXT("RespawnVolume"), ESearchCase::IgnoreCase))
	{
		SetPieceType(EParkourBuildPieceType::RespawnVolume);
	}
	else if (NewPieceTypeName.Equals(TEXT("FinishGate"), ESearchCase::IgnoreCase))
	{
		SetPieceType(EParkourBuildPieceType::FinishGate);
	}
	else if (NewPieceTypeName.Equals(TEXT("Sign"), ESearchCase::IgnoreCase))
	{
		SetPieceType(EParkourBuildPieceType::Sign);
	}
	else if (NewPieceTypeName.Equals(TEXT("BoostPad"), ESearchCase::IgnoreCase))
	{
		SetPieceType(EParkourBuildPieceType::BoostPad);
	}
	else
	{
		SetPieceType(EParkourBuildPieceType::Platform);
	}
}

void AParkourBuildPiece::SetSlopeAngle(float NewSlopeAngle)
{
	PieceData.SlopeAngle = FMath::Clamp(FMath::Abs(NewSlopeAngle), 0.0f, 85.0f);
	ApplySlopeRotation();
	ApplyPieceVisuals();
}

void AParkourBuildPiece::AdjustSlopeAngle(float DeltaAngle)
{
	SetSlopeAngle(PieceData.SlopeAngle + DeltaAngle);
}

void AParkourBuildPiece::SetUseInwardBank(bool bNewUseInwardBank)
{
	PieceData.bUseInwardBank = bNewUseInwardBank;
	if (PieceData.SlopeAngle > KINDA_SMALL_NUMBER)
	{
		ApplySlopeRotation();
	}
}

void AParkourBuildPiece::ApplyPieceVisuals()
{
	if (Mesh)
	{
		const FVector SafeDimensions = PieceData.Dimensions.ComponentMax(FVector(10.0f));
		Mesh->SetRelativeScale3D(SafeDimensions / 100.0f);
	}

	if (SelectionBounds)
	{
		SelectionBounds->SetBoxExtent(PieceData.Dimensions.ComponentMax(FVector(10.0f)) * 0.5f);
		SelectionBounds->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		SelectionBounds->SetHiddenInGame(true);
		SelectionBounds->SetVisibility(false);
	}

	if (LabelText)
	{
		const FString Label = PieceData.Label.IsEmpty() ? UEnum::GetValueAsString(PieceData.PieceType) : PieceData.Label;
		LabelText->SetText(FText::FromString(Label));
		LabelText->SetRelativeLocation(FVector(0.0f, 0.0f, PieceData.Dimensions.Z * 0.5f + 80.0f));
		LabelText->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
		LabelText->SetHiddenInGame(PieceData.PieceType != EParkourBuildPieceType::Sign);
	}
}

void AParkourBuildPiece::ApplySlopeRotation()
{
	FRotator NewRotation = GetActorRotation();
	if (PieceData.bUseInwardBank)
	{
		NewRotation.Pitch = 0.0f;
		NewRotation.Roll = GetInwardBankRoll(PieceData.SlopeAngle);
	}
	else
	{
		NewRotation.Pitch = PieceData.SlopeAngle;
		NewRotation.Roll = 0.0f;
	}

	SetActorRotation(NewRotation);
	PieceData.Transform = GetActorTransform();
}

float AParkourBuildPiece::GetInwardBankRoll(float SlopeAngle) const
{
	const float SideSign = GetActorLocation().Y >= 0.0f ? -1.0f : 1.0f;
	return FMath::Abs(SlopeAngle) * SideSign;
}
