#include "ParkourBoostPad.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "ParkourMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

AParkourBoostPad::AParkourBoostPad()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	PadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PadMesh"));
	PadMesh->SetupAttachment(SceneRoot);
	PadMesh->SetRelativeScale3D(FVector(2.6f, 1.8f, 0.12f));
	PadMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PadMesh->SetCollisionResponseToAllChannels(ECR_Block);

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	TriggerVolume->SetupAttachment(SceneRoot);
	TriggerVolume->SetBoxExtent(FVector(150.0f, 110.0f, 70.0f));
	TriggerVolume->SetRelativeLocation(FVector(0.0f, 0.0f, 80.0f));
	TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	LabelText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("LabelText"));
	LabelText->SetupAttachment(SceneRoot);
	LabelText->SetRelativeLocation(FVector(0.0f, 0.0f, 170.0f));
	LabelText->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	LabelText->SetHorizontalAlignment(EHTA_Center);
	LabelText->SetWorldSize(42.0f);
	LabelText->SetText(FText::FromString(TEXT("Boost Pad / 独立触发器")));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		PadMesh->SetStaticMesh(CubeMesh.Object);
	}
}

void AParkourBoostPad::BeginPlay()
{
	Super::BeginPlay();

	if (TriggerVolume)
	{
		TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AParkourBoostPad::OnBoostOverlap);
	}
}

void AParkourBoostPad::OnBoostOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
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

	if (bOverrideHorizontalSpeed)
	{
		const FVector HorizontalVelocity = FVector(Movement->Velocity.X, Movement->Velocity.Y, 0.0f);
		const float CurrentForwardSpeed = FVector::DotProduct(HorizontalVelocity, BoostDirection);
		if (CurrentForwardSpeed < BoostStrength)
		{
			const FVector SidewaysVelocity = HorizontalVelocity - BoostDirection * CurrentForwardSpeed;
			const FVector NewHorizontalVelocity = SidewaysVelocity + BoostDirection * BoostStrength;
			Movement->Velocity.X = NewHorizontalVelocity.X;
			Movement->Velocity.Y = NewHorizontalVelocity.Y;
		}
	}
	else
	{
		Movement->Velocity += BoostDirection * BoostStrength;
	}

	if (!FMath::IsNearlyZero(VerticalBoost))
	{
		Movement->Velocity.Z = FMath::Max(Movement->Velocity.Z, VerticalBoost);
	}

	RecordBoost(Pawn);
}

bool AParkourBoostPad::CanBoostActor(AActor* Actor) const
{
	if (!Actor || !GetWorld())
	{
		return false;
	}

	const float* LastTime = LastBoostTimes.Find(Actor);
	if (!LastTime)
	{
		return true;
	}

	return GetWorld()->GetTimeSeconds() - *LastTime >= ReuseCooldown;
}

void AParkourBoostPad::RecordBoost(AActor* Actor)
{
	if (Actor && GetWorld())
	{
		LastBoostTimes.Add(Actor, GetWorld()->GetTimeSeconds());
	}
}
