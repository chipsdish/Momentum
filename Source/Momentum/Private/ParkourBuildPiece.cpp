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

	LabelText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("LabelText"));
	LabelText->SetupAttachment(SceneRoot);
	LabelText->SetHorizontalAlignment(EHTA_Center);
	LabelText->SetVerticalAlignment(EVRTA_TextCenter);
	LabelText->SetWorldSize(48.0f);
	LabelText->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
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
	}

	if (LabelText)
	{
		LabelText->SetHiddenInGame(!bSelected && PieceData.PieceType != EParkourBuildPieceType::Sign);
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
	}

	if (LabelText)
	{
		const FString Label = PieceData.Label.IsEmpty() ? UEnum::GetValueAsString(PieceData.PieceType) : PieceData.Label;
		LabelText->SetText(FText::FromString(Label));
		LabelText->SetRelativeLocation(FVector(0.0f, 0.0f, PieceData.Dimensions.Z * 0.5f + 80.0f));
		LabelText->SetHiddenInGame(PieceData.PieceType != EParkourBuildPieceType::Sign);
	}
}
