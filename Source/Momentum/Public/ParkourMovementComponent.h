#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ParkourTypes.h"
#include "ParkourMovementComponent.generated.h"

UCLASS(ClassGroup = (Movement), meta = (BlueprintSpawnableComponent))
class MOMENTUM_API UParkourMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UParkourMovementComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void PhysFalling(float DeltaTime, int32 Iterations) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
	virtual bool ShouldCatchAir(const FFindFloorResult& OldFloor, const FFindFloorResult& NewFloor) override;

	void SetMoveInput(const FVector2D& InMoveInput);
	void RequestJumpPressed();
	void RequestJumpReleased();

	UFUNCTION(BlueprintPure, Category = "Parkour|Movement")
	EParkourMovementState GetParkourMovementState() const { return ParkourMovementState; }

	UFUNCTION(BlueprintPure, Category = "Parkour|Movement")
	float GetCurrentSlopeAngle() const { return CurrentSlopeAngle; }

	UFUNCTION(BlueprintPure, Category = "Parkour|Movement")
	float GetHorizontalSpeed() const;

	UFUNCTION(BlueprintPure, Category = "Parkour|Movement")
	bool IsSurfing() const { return ParkourMovementState == EParkourMovementState::Surfing; }

	UFUNCTION(BlueprintPure, Category = "Parkour|Movement")
	bool IsSliding() const { return ParkourMovementState == EParkourMovementState::Sliding; }

	UFUNCTION(BlueprintCallable, Category = "Parkour|Movement")
	void ClearJumpBuffer();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Speed")
	float MaxSpeed = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Speed")
	float MaxTestSpeed = 4500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Speed")
	float MaxFallSpeed = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Acceleration")
	float GroundAcceleration = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Acceleration")
	float AirAcceleration = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Acceleration")
	float AirWishSpeed = 1600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Acceleration")
	float AirTurnRateDegrees = 540.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Acceleration")
	float AirAimFollowResponsiveness = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Acceleration")
	float AirControlStrength = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Acceleration")
	float AirForwardControlScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Acceleration")
	float AirSideControlScale = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Acceleration")
	float AirBackwardControlScale = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Acceleration")
	float AirBrakeDeceleration = 2200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Jump")
	float JumpVelocity = 520.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Jump")
	float BunnyHopBufferTime = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Jump")
	float SurfJumpGraceTime = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Slope")
	float ParkourWalkableSlopeAngle = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Slope")
	float SurfSlopeAngle = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Slope")
	float SlideSlopeAngle = 55.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Slope")
	float SurfFriction = 0.02f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Slope")
	float SlideFriction = 0.01f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Slope")
	float SurfAcceleration = 980.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Slope")
	float SurfControlAcceleration = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Slope")
	float SurfUphillControlScale = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Slope")
	float SurfUphillSpeedLoss = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Slope")
	float SurfUphillDetachSpeed = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Slope")
	float SurfReattachBlockTime = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Slope")
	float MinSurfSurfaceNormalZ = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Slope")
	float MaxSurfVerticalSpeed = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Respawn")
	float RespawnZThreshold = -2000.0f;

protected:
	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;

	void UpdateMovementState(float DeltaTime);
	void ApplyGroundMovement(float DeltaTime);
	void ApplyAirMovement(float DeltaTime);
	void ApplySurfMovement(float DeltaTime);
	bool CheckSlope();
	void HandleBunnyHop(float DeltaTime);
	void ClampVelocity();
	void RespawnPlayer();

	FVector ComputeWishDirection() const;
	FVector ComputeSurfaceWishDirection(const FVector& SurfaceNormal) const;
	FVector ComputeSurfaceVelocity(const FVector& CurrentVelocity, const FVector& SurfaceNormal, bool bPreserveExistingUpwardVelocity) const;
	FVector ClampSurfaceVerticalSpeed(const FVector& CurrentVelocity) const;
	FVector GetFloorNormal() const;
	void ApplyHorizontalFriction(float DeltaTime, float Friction);
	void Accelerate(const FVector& WishDirection, float WishSpeed, float AccelerationAmount, float DeltaTime);
	bool ShouldDetachFromSurfUphill(const FVector& FloorNormal, const FVector& DownSlopeDirection) const;
	void DetachFromSurf(const FVector& ExitVelocity);
	void BeginSurfReattachBlock();
	bool IsSurfReattachBlocked() const;
	bool IsJumpBuffered() const;
	bool IsSurfJumpGraceActive() const;
	bool TryConsumeSurfJump();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Debug")
	EParkourMovementState ParkourMovementState = EParkourMovementState::Airborne;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Debug")
	float CurrentSlopeAngle = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Debug")
	FVector LastFloorNormal = FVector::UpVector;

	FVector LastSurfSurfaceVelocity = FVector::ZeroVector;
	float LastSurfContactTime = -10000.0f;
	float SurfReattachBlockedUntilTime = -10000.0f;
	bool bHasLastSurfSurfaceVelocity = false;
	bool bSkipNextSurfExitVelocityRestore = false;

	FVector2D MoveInput = FVector2D::ZeroVector;
	float LastJumpPressedTime = -10000.0f;
	bool bJumpRequestConsumed = true;
	bool bJumpHeld = false;
};
