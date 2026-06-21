#include "ParkourMovementComponent.h"

#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "ParkourGameMode.h"

UParkourMovementComponent::UParkourMovementComponent()
{
	MaxWalkSpeed = MaxSpeed;
	MaxAcceleration = GroundAcceleration;
	JumpZVelocity = JumpVelocity;
	AirControl = 0.0f;
	FallingLateralFriction = 0.0f;
	BrakingFrictionFactor = 0.0f;
	BrakingDecelerationWalking = 0.0f;
	BrakingDecelerationFalling = 0.0f;
	GroundFriction = 0.08f;
	bUseSeparateBrakingFriction = true;
	BrakingFriction = 0.0f;
	bMaintainHorizontalGroundVelocity = false;

	// Keep Unreal's floor solver permissive; parkour state thresholds decide behavior.
	SetWalkableFloorAngle(89.0f);
}

void UParkourMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	MaxWalkSpeed = MaxSpeed;
	JumpZVelocity = JumpVelocity;
}

void UParkourMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateMovementState(DeltaTime);

	if (UpdatedComponent && UpdatedComponent->GetComponentLocation().Z < RespawnZThreshold)
	{
		RespawnPlayer();
	}
}

void UParkourMovementComponent::SetMoveInput(const FVector2D& InMoveInput)
{
	MoveInput = InMoveInput.GetClampedToMaxSize(1.0f);
}

void UParkourMovementComponent::RequestJumpPressed()
{
	if (!GetWorld())
	{
		return;
	}

	LastJumpPressedTime = GetWorld()->GetTimeSeconds();
	bJumpRequestConsumed = false;
	bJumpHeld = true;
}

void UParkourMovementComponent::RequestJumpReleased()
{
	bJumpHeld = false;
}

void UParkourMovementComponent::ClearJumpBuffer()
{
	bJumpRequestConsumed = true;
}

float UParkourMovementComponent::GetHorizontalSpeed() const
{
	return FVector(Velocity.X, Velocity.Y, 0.0f).Size();
}

void UParkourMovementComponent::CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration)
{
	if (!HasValidData() || DeltaTime < MIN_TICK_TIME)
	{
		return;
	}

	if (HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocity())
	{
		Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
		return;
	}

	UpdateMovementState(DeltaTime);

	if (IsMovingOnGround())
	{
		MaxWalkSpeed = MaxSpeed;
		MaxAcceleration = GroundAcceleration;

		HandleBunnyHop(DeltaTime);
		if (!IsMovingOnGround())
		{
			ClampVelocity();
			return;
		}

		if (ParkourMovementState == EParkourMovementState::Surfing || ParkourMovementState == EParkourMovementState::Sliding)
		{
			ApplySurfMovement(DeltaTime);
		}
		else
		{
			ApplyGroundMovement(DeltaTime);
		}

		ClampVelocity();
		return;
	}

	Super::CalcVelocity(DeltaTime, 0.0f, bFluid, 0.0f);
	ClampVelocity();
}

void UParkourMovementComponent::PhysFalling(float DeltaTime, int32 Iterations)
{
	// Disable default lateral air control; CS/KZ-style acceleration is applied after falling physics.
	Acceleration = FVector::ZeroVector;
	Super::PhysFalling(DeltaTime, Iterations);

	if (DeltaTime >= MIN_TICK_TIME && HasValidData() && MovementMode == MOVE_Falling)
	{
		UpdateMovementState(DeltaTime);
		ApplyAirMovement(DeltaTime);
		ClampVelocity();
	}
}

void UParkourMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	const bool bWasSurfingOrSliding = ParkourMovementState == EParkourMovementState::Surfing || ParkourMovementState == EParkourMovementState::Sliding;
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);

	if (PreviousMovementMode == MOVE_Walking && MovementMode == MOVE_Falling && bWasSurfingOrSliding && bHasLastSurfSurfaceVelocity)
	{
		if (!bSkipNextSurfExitVelocityRestore)
		{
			Velocity = LastSurfSurfaceVelocity;
		}

		bHasLastSurfSurfaceVelocity = false;
		bSkipNextSurfExitVelocityRestore = false;
	}

	if (MovementMode == MOVE_Walking)
	{
		UpdateMovementState(0.0f);
		HandleBunnyHop(0.0f);
	}
}

void UParkourMovementComponent::UpdateMovementState(float DeltaTime)
{
	if (!HasValidData())
	{
		ParkourMovementState = EParkourMovementState::Airborne;
		CurrentSlopeAngle = 0.0f;
		LastFloorNormal = FVector::UpVector;
		return;
	}

	if (!IsMovingOnGround())
	{
		ParkourMovementState = EParkourMovementState::Airborne;
		CurrentSlopeAngle = 0.0f;
		LastFloorNormal = FVector::UpVector;
		return;
	}

	CheckSlope();

	if (CurrentSlopeAngle >= SlideSlopeAngle)
	{
		ParkourMovementState = EParkourMovementState::Sliding;
	}
	else if (CurrentSlopeAngle >= SurfSlopeAngle)
	{
		ParkourMovementState = EParkourMovementState::Surfing;
	}
	else
	{
		ParkourMovementState = EParkourMovementState::Grounded;
		bHasLastSurfSurfaceVelocity = false;
	}
}

void UParkourMovementComponent::ApplyGroundMovement(float DeltaTime)
{
	const float InputAmount = MoveInput.Size();
	const float FrictionToApply = IsJumpBuffered() ? 0.0f : GroundFriction;

	ApplyHorizontalFriction(DeltaTime, FrictionToApply);

	if (InputAmount <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FVector WishDirection = ComputeWishDirection();
	Accelerate(WishDirection, MaxSpeed * InputAmount, GroundAcceleration, DeltaTime);
}

void UParkourMovementComponent::ApplyAirMovement(float DeltaTime)
{
	const float InputAmount = MoveInput.Size();
	if (InputAmount <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FVector InitialHorizontalVelocity(Velocity.X, Velocity.Y, 0.0f);
	const float InitialHorizontalSpeed = InitialHorizontalVelocity.Size();
	if (InitialHorizontalSpeed <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FVector WishDirection = ComputeWishDirection();
	if (WishDirection.IsNearlyZero())
	{
		return;
	}

	const float ForwardAmount = FMath::Max(MoveInput.Y, 0.0f);
	const float SideAmount = FMath::Abs(MoveInput.X);
	const float BackwardAmount = FMath::Max(-MoveInput.Y, 0.0f);
	const float DirectionWeight = FMath::Max(ForwardAmount + SideAmount + BackwardAmount, KINDA_SMALL_NUMBER);
	const float DirectionalScale = (ForwardAmount * AirForwardControlScale + SideAmount * AirSideControlScale + BackwardAmount * AirBackwardControlScale) / DirectionWeight;

	const FVector CurrentDirection = InitialHorizontalVelocity / InitialHorizontalSpeed;
	const float DirectionDot = FMath::Clamp(FVector::DotProduct(CurrentDirection, WishDirection), -1.0f, 1.0f);
	const float AngleToWishDirection = FMath::Acos(DirectionDot);
	if (AngleToWishDirection <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float MaxTurnAngle = FMath::DegreesToRadians(AirTurnRateDegrees * AirControlStrength * DirectionalScale * DeltaTime);
	const float TurnAlpha = FMath::Clamp(MaxTurnAngle / AngleToWishDirection, 0.0f, 1.0f);
	const FVector NewDirection = FMath::Lerp(CurrentDirection, WishDirection, TurnAlpha).GetSafeNormal();
	const FVector NewHorizontalVelocity = NewDirection * InitialHorizontalSpeed;
	Velocity.X = NewHorizontalVelocity.X;
	Velocity.Y = NewHorizontalVelocity.Y;
}

void UParkourMovementComponent::ApplySurfMovement(float DeltaTime)
{
	const FVector FloorNormal = GetFloorNormal();
	const FVector DownSlopeDirection = FVector::VectorPlaneProject(FVector::DownVector, FloorNormal).GetSafeNormal();
	const bool bIsSliding = ParkourMovementState == EParkourMovementState::Sliding;

	ApplyHorizontalFriction(DeltaTime, bIsSliding ? SlideFriction : SurfFriction);

	if (!DownSlopeDirection.IsNearlyZero())
	{
		const float GravityScaleForSlope = bIsSliding ? 1.25f : 1.0f;
		Velocity += DownSlopeDirection * SurfAcceleration * GravityScaleForSlope * DeltaTime;
	}

	const float InputAmount = MoveInput.Size();
	if (InputAmount > KINDA_SMALL_NUMBER)
	{
		FVector WishDirection = ComputeSurfaceWishDirection(FloorNormal);
		const float UphillAmount = FVector::DotProduct(WishDirection, -DownSlopeDirection);
		float ControlAcceleration = SurfControlAcceleration;
		if (UphillAmount > 0.0f)
		{
			ControlAcceleration *= FMath::Lerp(1.0f, SurfUphillControlScale, UphillAmount);
		}

		Accelerate(WishDirection, MaxSpeed * InputAmount, ControlAcceleration, DeltaTime);
		if (UphillAmount > 0.0f && SurfUphillSpeedLoss > 0.0f)
		{
			Velocity *= FMath::Max(1.0f - SurfUphillSpeedLoss * UphillAmount * DeltaTime, 0.0f);
		}
	}

	Velocity = ComputeSurfaceVelocity(Velocity, FloorNormal, false);
	LastSurfSurfaceVelocity = Velocity;
	bHasLastSurfSurfaceVelocity = true;
}

bool UParkourMovementComponent::CheckSlope()
{
	if (!CurrentFloor.bBlockingHit)
	{
		CurrentSlopeAngle = 0.0f;
		LastFloorNormal = FVector::UpVector;
		return false;
	}

	LastFloorNormal = CurrentFloor.HitResult.ImpactNormal.GetSafeNormal();
	const float UpDot = FMath::Clamp(FVector::DotProduct(LastFloorNormal, FVector::UpVector), -1.0f, 1.0f);
	CurrentSlopeAngle = FMath::RadiansToDegrees(FMath::Acos(UpDot));
	return true;
}

void UParkourMovementComponent::HandleBunnyHop(float DeltaTime)
{
	if (!IsMovingOnGround() || !IsJumpBuffered())
	{
		return;
	}

	JumpZVelocity = JumpVelocity;
	bJumpRequestConsumed = true;

	const bool bWasSurfingOrSliding = ParkourMovementState == EParkourMovementState::Surfing || ParkourMovementState == EParkourMovementState::Sliding;
	bSkipNextSurfExitVelocityRestore = bWasSurfingOrSliding;
	if (!DoJump(false, DeltaTime))
	{
		bSkipNextSurfExitVelocityRestore = false;
	}
}

void UParkourMovementComponent::ClampVelocity()
{
	FVector HorizontalVelocity(Velocity.X, Velocity.Y, 0.0f);
	const float HorizontalSpeed = HorizontalVelocity.Size();

	if (HorizontalSpeed <= MaxTestSpeed || HorizontalSpeed <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	HorizontalVelocity = HorizontalVelocity.GetSafeNormal() * MaxTestSpeed;
	Velocity.X = HorizontalVelocity.X;
	Velocity.Y = HorizontalVelocity.Y;
}

void UParkourMovementComponent::RespawnPlayer()
{
	if (!CharacterOwner || !GetWorld())
	{
		return;
	}

	AParkourGameMode* ParkourGameMode = GetWorld()->GetAuthGameMode<AParkourGameMode>();
	if (!ParkourGameMode)
	{
		return;
	}

	ParkourGameMode->RespawnPlayer(CharacterOwner->GetController());
}

FVector UParkourMovementComponent::ComputeWishDirection() const
{
	if (!CharacterOwner)
	{
		return FVector::ZeroVector;
	}

	const AController* Controller = CharacterOwner->GetController();
	const FRotator ControlRotation = Controller ? Controller->GetControlRotation() : CharacterOwner->GetActorRotation();
	const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);

	const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	return (Forward * MoveInput.Y + Right * MoveInput.X).GetSafeNormal();
}

FVector UParkourMovementComponent::ComputeSurfaceWishDirection(const FVector& SurfaceNormal) const
{
	const FVector WishDirection = ComputeWishDirection();
	return FVector::VectorPlaneProject(WishDirection, SurfaceNormal).GetSafeNormal();
}

FVector UParkourMovementComponent::ComputeSurfaceVelocity(const FVector& CurrentVelocity, const FVector& SurfaceNormal, bool bPreserveExistingUpwardVelocity) const
{
	const FVector SafeNormal = SurfaceNormal.GetSafeNormal();
	if (SafeNormal.IsNearlyZero())
	{
		return CurrentVelocity;
	}

	FVector SurfaceVelocity(CurrentVelocity.X, CurrentVelocity.Y, 0.0f);
	if (FMath::Abs(SafeNormal.Z) > KINDA_SMALL_NUMBER)
	{
		SurfaceVelocity.Z = -FVector::DotProduct(FVector(SurfaceVelocity.X, SurfaceVelocity.Y, 0.0f), SafeNormal) / SafeNormal.Z;
	}
	else
	{
		SurfaceVelocity = FVector::VectorPlaneProject(CurrentVelocity, SafeNormal);
	}

	if (bPreserveExistingUpwardVelocity && CurrentVelocity.Z > SurfaceVelocity.Z)
	{
		SurfaceVelocity.Z = CurrentVelocity.Z;
	}

	return SurfaceVelocity;
}

FVector UParkourMovementComponent::GetFloorNormal() const
{
	return CurrentFloor.bBlockingHit ? CurrentFloor.HitResult.ImpactNormal.GetSafeNormal() : FVector::UpVector;
}

void UParkourMovementComponent::ApplyHorizontalFriction(float DeltaTime, float Friction)
{
	if (Friction <= 0.0f)
	{
		return;
	}

	FVector HorizontalVelocity(Velocity.X, Velocity.Y, 0.0f);
	const float Speed = HorizontalVelocity.Size();
	if (Speed <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float Drop = Speed * Friction * DeltaTime;
	const float NewSpeed = FMath::Max(Speed - Drop, 0.0f);
	HorizontalVelocity *= NewSpeed / Speed;

	Velocity.X = HorizontalVelocity.X;
	Velocity.Y = HorizontalVelocity.Y;
}

void UParkourMovementComponent::Accelerate(const FVector& WishDirection, float WishSpeed, float AccelerationAmount, float DeltaTime)
{
	const FVector NormalizedWishDirection = WishDirection.GetSafeNormal();
	if (NormalizedWishDirection.IsNearlyZero())
	{
		return;
	}

	const float ClampedWishSpeed = FMath::Min(WishSpeed, MaxTestSpeed);
	const float CurrentSpeedInWishDirection = FVector::DotProduct(Velocity, NormalizedWishDirection);
	const float AdditionalSpeedNeeded = ClampedWishSpeed - CurrentSpeedInWishDirection;
	if (AdditionalSpeedNeeded <= 0.0f)
	{
		return;
	}

	float AccelerationSpeed = AccelerationAmount * ClampedWishSpeed * DeltaTime;
	AccelerationSpeed = FMath::Min(AccelerationSpeed, AdditionalSpeedNeeded);
	Velocity += NormalizedWishDirection * AccelerationSpeed;
}

bool UParkourMovementComponent::IsJumpBuffered() const
{
	const UWorld* World = GetWorld();
	if (!World || bJumpRequestConsumed)
	{
		return false;
	}

	return World->GetTimeSeconds() - LastJumpPressedTime <= BunnyHopBufferTime;
}
