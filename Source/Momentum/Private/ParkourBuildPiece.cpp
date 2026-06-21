#include "ParkourBuildPiece.h"

#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "ParkourGameMode.h"
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

	ApplyPieceVisuals();
}

void AParkourBuildPiece::BeginPlay()
{
	Super::BeginPlay();

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
		Mesh->SetRenderCustomDepth(false);
		Mesh->SetCustomDepthStencilValue(0);
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
		Mesh->SetHiddenInGame(false);
		Mesh->SetVisibility(true, true);
		Mesh->SetRenderCustomDepth(false);
		Mesh->SetCustomDepthStencilValue(0);
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
	const FVector SafeDimensions = PieceData.Dimensions.ComponentMax(FVector(10.0f));

	if (Mesh)
	{
		Mesh->SetRelativeScale3D(SafeDimensions / 100.0f);

		if (PieceData.PieceType == EParkourBuildPieceType::FinishGate)
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
		SelectionBounds->SetBoxExtent(SafeDimensions * 0.5f);
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
