#pragma once

#include "CoreMinimal.h"
#include "ParkourTypes.generated.h"

UENUM(BlueprintType)
enum class EParkourMovementState : uint8
{
	Grounded UMETA(DisplayName = "Grounded"),
	Airborne UMETA(DisplayName = "Airborne"),
	Surfing UMETA(DisplayName = "Surfing"),
	Sliding UMETA(DisplayName = "Sliding")
};

UENUM(BlueprintType)
enum class EParkourBuildPieceType : uint8
{
	Platform UMETA(DisplayName = "Platform"),
	Ramp UMETA(DisplayName = "Ramp"),
	JumpRamp UMETA(DisplayName = "Jump Ramp"),
	WallPlatform UMETA(DisplayName = "Wall Platform"),
	AirPlatform UMETA(DisplayName = "Air Platform"),
	AccelerationRamp UMETA(DisplayName = "Acceleration Ramp"),
	RespawnVolume UMETA(DisplayName = "Respawn Volume"),
	FinishGate UMETA(DisplayName = "Finish Gate"),
	Sign UMETA(DisplayName = "Sign"),
	BoostPad UMETA(DisplayName = "Boost Pad")
};

UENUM(BlueprintType)
enum class EParkourGizmoAxis : uint8
{
	None UMETA(DisplayName = "None"),
	X UMETA(DisplayName = "X"),
	Y UMETA(DisplayName = "Y"),
	Z UMETA(DisplayName = "Z"),
	XY UMETA(DisplayName = "XY Plane")
};

USTRUCT(BlueprintType)
struct FParkourBuildPieceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Parkour|Build")
	FGuid PieceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Parkour|Build")
	EParkourBuildPieceType PieceType = EParkourBuildPieceType::Platform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Parkour|Build")
	FTransform Transform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Parkour|Build")
	FVector Dimensions = FVector(400.0f, 400.0f, 80.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Parkour|Build")
	float SlopeAngle = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Parkour|Build")
	FString Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Parkour|Build")
	bool bBoostPadEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Parkour|Build", meta = (EditCondition = "bBoostPadEnabled"))
	float BoostStrength = 1200.0f;

	FParkourBuildPieceData()
	{
		PieceId = FGuid::NewGuid();
	}
};

USTRUCT(BlueprintType)
struct FParkourBuildLayoutData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Parkour|Build")
	FString LayoutName = TEXT("Default Layout");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Parkour|Build")
	TArray<FParkourBuildPieceData> Pieces;
};
