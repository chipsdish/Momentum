#include "ParkourBuildPiece.h"

#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInterface.h"
#include "ParkourGameMode.h"
#include "ProceduralMeshComponent.h"
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
	Mesh->SetGenerateOverlapEvents(true);

	RampMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("RampMesh"));
	RampMesh->SetupAttachment(SceneRoot);
	RampMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RampMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	RampMesh->SetGenerateOverlapEvents(false);
	RampMesh->SetHiddenInGame(true);
	RampMesh->SetVisibility(false);
	RampMesh->bUseComplexAsSimpleCollision = false;

	SelectionBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("SelectionBounds"));
	SelectionBounds->SetupAttachment(SceneRoot);
	SelectionBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SelectionBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	SelectionBounds->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SelectionBounds->SetHiddenInGame(true);
	SelectionBounds->SetVisibility(false);

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	TriggerVolume->SetupAttachment(SceneRoot);
	TriggerVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerVolume->SetGenerateOverlapEvents(false);
	TriggerVolume->SetHiddenInGame(true);
	TriggerVolume->SetVisibility(false);

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

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BasicShapeMaterial.Succeeded())
	{
		Mesh->SetMaterial(0, BasicShapeMaterial.Object);
		RampMesh->SetMaterial(0, BasicShapeMaterial.Object);
	}

	ApplyPieceVisuals();
}

void AParkourBuildPiece::BeginPlay()
{
	Super::BeginPlay();

	if (IsRampLikePiece() && PieceData.SlopeAngle > KINDA_SMALL_NUMBER)
	{
		ApplySlopeRotation();
	}
	ApplyPieceVisuals();

	if (TriggerVolume)
	{
		TriggerVolume->OnComponentBeginOverlap.AddUniqueDynamic(this, &AParkourBuildPiece::OnBuildPieceOverlap);
	}
}

void AParkourBuildPiece::ConfigureFromData(const FParkourBuildPieceData& NewPieceData)
{
	PieceData = NewPieceData;
	if (!PieceData.PieceId.IsValid())
	{
		PieceData.PieceId = FGuid::NewGuid();
	}

	if (PieceData.PieceType == EParkourBuildPieceType::BoostPad)
	{
		PieceData.bBoostPadEnabled = true;
		if (PieceData.BoostStrength <= 0.0f)
		{
			PieceData.BoostStrength = 1400.0f;
		}
	}

	SetActorTransform(PieceData.Transform);
	if (IsRampLikePiece() && PieceData.SlopeAngle > KINDA_SMALL_NUMBER)
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
		Mesh->SetRenderCustomDepth(false);
		Mesh->SetCustomDepthStencilValue(0);
		Mesh->MarkRenderStateDirty();
	}

	if (RampMesh)
	{
		RampMesh->SetRenderCustomDepth(false);
		RampMesh->SetCustomDepthStencilValue(0);
		RampMesh->MarkRenderStateDirty();
	}

	if (LabelText)
	{
		LabelText->SetHiddenInGame(!bSelected && PieceData.PieceType != EParkourBuildPieceType::Sign);
	}
}

void AParkourBuildPiece::SetBuildModeVisuals(bool bBuildModeActive)
{
	SetSelected(false);

	TArray<UPrimitiveComponent*> PrimitiveComponents;
	GetComponents<UPrimitiveComponent>(PrimitiveComponents);
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (PrimitiveComponent)
		{
			PrimitiveComponent->SetRenderCustomDepth(false);
			PrimitiveComponent->SetCustomDepthStencilValue(0);
			PrimitiveComponent->MarkRenderStateDirty();
		}
	}

	if (Mesh)
	{
		const bool bUseRampMesh = IsRampLikePiece();
		Mesh->SetHiddenInGame(bUseRampMesh);
		Mesh->SetVisibility(!bUseRampMesh, true);
		Mesh->SetRenderCustomDepth(false);
		Mesh->SetCustomDepthStencilValue(0);
		Mesh->MarkRenderStateDirty();
	}

	if (RampMesh)
	{
		const bool bUseRampMesh = IsRampLikePiece();
		RampMesh->SetHiddenInGame(!bUseRampMesh);
		RampMesh->SetVisibility(bUseRampMesh, true);
		RampMesh->SetRenderCustomDepth(false);
		RampMesh->SetCustomDepthStencilValue(0);
		RampMesh->MarkRenderStateDirty();
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

void AParkourBuildPiece::AdjustDimensions(const FVector& DeltaDimensions)
{
	SetDimensions(PieceData.Dimensions + DeltaDimensions);
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
	if (IsRampLikePiece())
	{
		ApplySlopeRotation();
	}
	ApplyPieceVisuals();
}

void AParkourBuildPiece::AdjustSlopeAngle(float DeltaAngle)
{
	SetSlopeAngle(PieceData.SlopeAngle + DeltaAngle);
}

void AParkourBuildPiece::SetUseInwardBank(bool bNewUseInwardBank)
{
	PieceData.bUseInwardBank = bNewUseInwardBank;
	if (IsRampLikePiece() && PieceData.SlopeAngle > KINDA_SMALL_NUMBER)
	{
		ApplySlopeRotation();
	}
}

void AParkourBuildPiece::ApplyPieceVisuals()
{
	const FVector SafeDimensions = PieceData.Dimensions.ComponentMax(FVector(10.0f));
	const bool bUseRampMesh = IsRampLikePiece();
	const float VisualHeight = bUseRampMesh ? GetRampRise(SafeDimensions) : SafeDimensions.Z;

	if (Mesh)
	{
		Mesh->SetRelativeScale3D(SafeDimensions / 100.0f);
		Mesh->SetHiddenInGame(bUseRampMesh);
		Mesh->SetVisibility(!bUseRampMesh, true);

		if (bUseRampMesh)
		{
			Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
			Mesh->SetGenerateOverlapEvents(false);
		}
		else if (PieceData.PieceType == EParkourBuildPieceType::FinishGate)
		{
			Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
			Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
			Mesh->SetGenerateOverlapEvents(false);
		}
		else
		{
			Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Mesh->SetCollisionResponseToAllChannels(ECR_Block);
			Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
			Mesh->SetGenerateOverlapEvents(false);
		}
	}

	if (RampMesh)
	{
		RampMesh->ClearAllMeshSections();
		RampMesh->ClearCollisionConvexMeshes();
		RampMesh->SetHiddenInGame(!bUseRampMesh);
		RampMesh->SetVisibility(bUseRampMesh, true);

		if (bUseRampMesh)
		{
			RebuildRampMesh(SafeDimensions);
			if (Mesh && Mesh->GetMaterial(0))
			{
				RampMesh->SetMaterial(0, Mesh->GetMaterial(0));
			}
			RampMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			RampMesh->SetCollisionResponseToAllChannels(ECR_Block);
			RampMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
			RampMesh->SetGenerateOverlapEvents(false);
		}
		else
		{
			RampMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			RampMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
			RampMesh->SetGenerateOverlapEvents(false);
		}
	}

	if (TriggerVolume)
	{
		TriggerVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		TriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
		TriggerVolume->SetGenerateOverlapEvents(false);
		TriggerVolume->SetHiddenInGame(true);
		TriggerVolume->SetVisibility(false);
		TriggerVolume->SetRelativeLocation(FVector::ZeroVector);
		TriggerVolume->SetBoxExtent(SafeDimensions * 0.5f);

		if (PieceData.PieceType == EParkourBuildPieceType::FinishGate)
		{
			TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			TriggerVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
			TriggerVolume->SetGenerateOverlapEvents(true);
		}
		else if (PieceData.PieceType == EParkourBuildPieceType::BoostPad && PieceData.bBoostPadEnabled)
		{
			const FVector TriggerExtent(SafeDimensions.X * 0.5f, SafeDimensions.Y * 0.5f, FMath::Max(80.0f, SafeDimensions.Z));
			TriggerVolume->SetBoxExtent(TriggerExtent);
			TriggerVolume->SetRelativeLocation(FVector(0.0f, 0.0f, SafeDimensions.Z * 0.5f + TriggerExtent.Z));
			TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			TriggerVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
			TriggerVolume->SetGenerateOverlapEvents(true);
		}
	}

	if (SelectionBounds)
	{
		SelectionBounds->SetRelativeLocation(bUseRampMesh ? FVector(0.0f, 0.0f, VisualHeight * 0.5f) : FVector::ZeroVector);
		SelectionBounds->SetBoxExtent(bUseRampMesh ? FVector(SafeDimensions.X * 0.5f, SafeDimensions.Y * 0.5f, VisualHeight * 0.5f) : SafeDimensions * 0.5f);
		SelectionBounds->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		SelectionBounds->SetHiddenInGame(true);
		SelectionBounds->SetVisibility(false);
	}

	if (LabelText)
	{
		const FString Label = PieceData.Label.IsEmpty() ? UEnum::GetValueAsString(PieceData.PieceType) : PieceData.Label;
		LabelText->SetText(FText::FromString(Label));
		LabelText->SetRelativeLocation(FVector(0.0f, 0.0f, VisualHeight + 80.0f));
		LabelText->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
		LabelText->SetHiddenInGame(PieceData.PieceType != EParkourBuildPieceType::Sign);
	}
}

void AParkourBuildPiece::ApplySlopeRotation()
{
	FRotator NewRotation = GetActorRotation();
	// Ramp geometry now owns the slope. Keep actor pitch/roll flat so hidden side faces cannot become floor candidates.
	NewRotation.Pitch = 0.0f;
	NewRotation.Roll = 0.0f;

	SetActorRotation(NewRotation);
	PieceData.Transform = GetActorTransform();
}

void AParkourBuildPiece::RebuildRampMesh(const FVector& SafeDimensions)
{
	if (!RampMesh)
	{
		return;
	}

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	auto AddTriangle = [&Vertices, &Triangles, &Normals, &UV0, &VertexColors, &Tangents](FVector A, FVector B, FVector C, const FVector& DesiredNormal)
	{
		FVector Normal = FVector::CrossProduct(B - A, C - A).GetSafeNormal();
		if (Normal.IsNearlyZero())
		{
			Normal = DesiredNormal.GetSafeNormal();
		}
		if (FVector::DotProduct(Normal, DesiredNormal) < 0.0f)
		{
			Swap(B, C);
			Normal *= -1.0f;
		}

		FVector TangentDirection = FVector::VectorPlaneProject(FVector::ForwardVector, Normal).GetSafeNormal();
		if (TangentDirection.IsNearlyZero())
		{
			TangentDirection = FVector::VectorPlaneProject(FVector::RightVector, Normal).GetSafeNormal();
		}

		const int32 BaseIndex = Vertices.Num();
		Vertices.Add(A);
		Vertices.Add(B);
		Vertices.Add(C);
		Triangles.Add(BaseIndex);
		Triangles.Add(BaseIndex + 1);
		Triangles.Add(BaseIndex + 2);
		Normals.Add(Normal);
		Normals.Add(Normal);
		Normals.Add(Normal);
		VertexColors.Add(FColor::White);
		VertexColors.Add(FColor::White);
		VertexColors.Add(FColor::White);
		Tangents.Add(FProcMeshTangent(TangentDirection, false));
		Tangents.Add(FProcMeshTangent(TangentDirection, false));
		Tangents.Add(FProcMeshTangent(TangentDirection, false));
		UV0.Add(FVector2D(0.0f, 0.0f));
		UV0.Add(FVector2D(1.0f, 0.0f));
		UV0.Add(FVector2D(1.0f, 1.0f));
	};

	auto AddQuad = [&AddTriangle](const FVector& A, const FVector& B, const FVector& C, const FVector& D, const FVector& DesiredNormal)
	{
		AddTriangle(A, B, C, DesiredNormal);
		AddTriangle(A, C, D, DesiredNormal);
	};

	const float HalfX = SafeDimensions.X * 0.5f;
	const float HalfY = SafeDimensions.Y * 0.5f;
	const float Rise = GetRampRise(SafeDimensions);
	TArray<FVector> ConvexVertices;

	if (PieceData.bUseInwardBank)
	{
		const float InwardSign = GetInwardSideSign();
		const float LowY = InwardSign * HalfY;
		const float HighY = -InwardSign * HalfY;

		const FVector LowA(-HalfX, LowY, 0.0f);
		const FVector LowB(HalfX, LowY, 0.0f);
		const FVector HighBaseA(-HalfX, HighY, 0.0f);
		const FVector HighBaseB(HalfX, HighY, 0.0f);
		const FVector HighTopA(-HalfX, HighY, Rise);
		const FVector HighTopB(HalfX, HighY, Rise);

		ConvexVertices = { LowA, LowB, HighBaseA, HighBaseB, HighTopA, HighTopB };
		const FVector TopNormal(0.0f, InwardSign * Rise / SafeDimensions.Y, 1.0f);

		AddQuad(LowA, LowB, HighBaseB, HighBaseA, FVector::DownVector);
		AddQuad(LowA, HighTopA, HighTopB, LowB, TopNormal.GetSafeNormal());
		AddQuad(HighBaseA, HighBaseB, HighTopB, HighTopA, FVector(0.0f, -InwardSign, 0.0f));
		AddTriangle(LowA, HighBaseA, HighTopA, FVector(-1.0f, 0.0f, 0.0f));
		AddTriangle(LowB, HighTopB, HighBaseB, FVector(1.0f, 0.0f, 0.0f));
	}
	else
	{
		const FVector LowLeft(-HalfX, -HalfY, 0.0f);
		const FVector LowRight(-HalfX, HalfY, 0.0f);
		const FVector HighBaseLeft(HalfX, -HalfY, 0.0f);
		const FVector HighBaseRight(HalfX, HalfY, 0.0f);
		const FVector HighTopLeft(HalfX, -HalfY, Rise);
		const FVector HighTopRight(HalfX, HalfY, Rise);

		ConvexVertices = { LowLeft, LowRight, HighBaseLeft, HighBaseRight, HighTopLeft, HighTopRight };
		const FVector TopNormal(-Rise / SafeDimensions.X, 0.0f, 1.0f);

		AddQuad(LowLeft, HighBaseLeft, HighBaseRight, LowRight, FVector::DownVector);
		AddQuad(LowLeft, LowRight, HighTopRight, HighTopLeft, TopNormal.GetSafeNormal());
		AddQuad(HighBaseLeft, HighTopLeft, HighTopRight, HighBaseRight, FVector::ForwardVector);
		AddTriangle(LowLeft, HighTopLeft, HighBaseLeft, FVector::LeftVector);
		AddTriangle(LowRight, HighBaseRight, HighTopRight, FVector::RightVector);
	}

	RampMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UV0, VertexColors, Tangents, false);

	TArray<TArray<FVector>> ConvexMeshes;
	ConvexMeshes.Add(ConvexVertices);
	RampMesh->SetCollisionConvexMeshes(ConvexMeshes);
}

bool AParkourBuildPiece::IsRampLikePiece() const
{
	return PieceData.PieceType == EParkourBuildPieceType::Ramp
		|| PieceData.PieceType == EParkourBuildPieceType::JumpRamp
		|| PieceData.PieceType == EParkourBuildPieceType::AccelerationRamp;
}

float AParkourBuildPiece::GetRampRise(const FVector& SafeDimensions) const
{
	const float AngleRadians = FMath::DegreesToRadians(FMath::Clamp(PieceData.SlopeAngle, 0.0f, 85.0f));
	const float Run = PieceData.bUseInwardBank ? SafeDimensions.Y : SafeDimensions.X;
	return FMath::Max(FMath::Tan(AngleRadians) * Run, 1.0f);
}

float AParkourBuildPiece::GetInwardSideSign() const
{
	return GetActorLocation().Y >= 0.0f ? -1.0f : 1.0f;
}

void AParkourBuildPiece::OnBuildPieceOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !GetWorld())
	{
		return;
	}

	if (PieceData.PieceType == EParkourBuildPieceType::FinishGate)
	{
		if (AParkourGameMode* ParkourGameMode = GetWorld()->GetAuthGameMode<AParkourGameMode>())
		{
			ParkourGameMode->FinishRun(Pawn);
		}
	}
	else if (PieceData.PieceType == EParkourBuildPieceType::BoostPad && PieceData.bBoostPadEnabled)
	{
		ApplyBoostPad(Pawn);
	}
}

void AParkourBuildPiece::ApplyBoostPad(APawn* Pawn)
{
	if (!Pawn || !CanBoostActor(Pawn))
	{
		return;
	}

	UCharacterMovementComponent* Movement = Pawn->FindComponentByClass<UCharacterMovementComponent>();
	if (!Movement)
	{
		return;
	}

	FVector BoostDirection = GetActorForwardVector();
	BoostDirection.Z = 0.0f;
	BoostDirection = BoostDirection.GetSafeNormal();
	if (BoostDirection.IsNearlyZero())
	{
		BoostDirection = FVector::ForwardVector;
	}

	const FVector HorizontalVelocity(Movement->Velocity.X, Movement->Velocity.Y, 0.0f);
	const float CurrentForwardSpeed = FVector::DotProduct(HorizontalVelocity, BoostDirection);
	if (CurrentForwardSpeed < PieceData.BoostStrength)
	{
		const FVector SidewaysVelocity = HorizontalVelocity - BoostDirection * CurrentForwardSpeed;
		const FVector NewHorizontalVelocity = SidewaysVelocity + BoostDirection * PieceData.BoostStrength;
		Movement->Velocity.X = NewHorizontalVelocity.X;
		Movement->Velocity.Y = NewHorizontalVelocity.Y;
	}

	RecordBoostActor(Pawn);
}

bool AParkourBuildPiece::CanBoostActor(AActor* Actor) const
{
	if (!Actor || !GetWorld())
	{
		return false;
	}

	const float* LastTime = LastBoostTimes.Find(Actor);
	return !LastTime || GetWorld()->GetTimeSeconds() - *LastTime >= BoostPadReuseCooldown;
}

void AParkourBuildPiece::RecordBoostActor(AActor* Actor)
{
	if (Actor && GetWorld())
	{
		LastBoostTimes.Add(Actor, GetWorld()->GetTimeSeconds());
	}
}
