#include "ParkourTransformGizmo.h"

#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AParkourTransformGizmo::AParkourTransformGizmo()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	XHandle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("XHandle"));
	YHandle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("YHandle"));
	ZHandle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ZHandle"));
	XYHandle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("XYHandle"));

	UStaticMeshComponent* Handles[] = { XHandle, YHandle, ZHandle, XYHandle };

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	for (UStaticMeshComponent* Handle : Handles)
	{
		Handle->SetupAttachment(SceneRoot);
		Handle->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Handle->SetCollisionResponseToAllChannels(ECR_Ignore);
		Handle->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		if (CubeMesh.Succeeded())
		{
			Handle->SetStaticMesh(CubeMesh.Object);
		}
	}

	XHandle->SetRelativeLocation(FVector(120.0f, 0.0f, 0.0f));
	XHandle->SetRelativeScale3D(FVector(2.4f, 0.12f, 0.12f));

	YHandle->SetRelativeLocation(FVector(0.0f, 120.0f, 0.0f));
	YHandle->SetRelativeScale3D(FVector(0.12f, 2.4f, 0.12f));

	ZHandle->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
	ZHandle->SetRelativeScale3D(FVector(0.12f, 0.12f, 2.4f));

	XYHandle->SetRelativeLocation(FVector(55.0f, 55.0f, 0.0f));
	XYHandle->SetRelativeScale3D(FVector(1.1f, 1.1f, 0.04f));

	SetGizmoVisible(false);
}

void AParkourTransformGizmo::AttachToTarget(AActor* NewTarget)
{
	TargetActor = NewTarget;
	if (TargetActor)
	{
		SetActorLocation(TargetActor->GetActorLocation());
		SetGizmoVisible(true);
	}
	else
	{
		SetGizmoVisible(false);
	}
}

void AParkourTransformGizmo::DetachFromTarget()
{
	EndDrag();
	TargetActor = nullptr;
	SetGizmoVisible(false);
}

bool AParkourTransformGizmo::BeginDrag(EParkourGizmoAxis Axis, const FVector& RayOrigin, const FVector& RayDirection, const FVector& CameraForward)
{
	if (!TargetActor || Axis == EParkourGizmoAxis::None)
	{
		return false;
	}

	DragAxis = Axis;
	DragStartTargetLocation = TargetActor->GetActorLocation();

	const FVector AxisDirection = GetAxisDirection(Axis);
	if (Axis == EParkourGizmoAxis::XY)
	{
		DragPlane = FPlane(DragStartTargetLocation, FVector::UpVector);
	}
	else
	{
		FVector PlaneNormal = FVector::VectorPlaneProject(CameraForward, AxisDirection).GetSafeNormal();
		if (PlaneNormal.IsNearlyZero())
		{
			PlaneNormal = FVector::UpVector;
		}
		DragPlane = FPlane(DragStartTargetLocation, PlaneNormal);
	}

	if (!IntersectDragPlane(RayOrigin, RayDirection, DragStartPoint))
	{
		EndDrag();
		return false;
	}

	return true;
}

bool AParkourTransformGizmo::UpdateDrag(const FVector& RayOrigin, const FVector& RayDirection)
{
	if (!TargetActor || DragAxis == EParkourGizmoAxis::None)
	{
		return false;
	}

	FVector CurrentPoint = FVector::ZeroVector;
	if (!IntersectDragPlane(RayOrigin, RayDirection, CurrentPoint))
	{
		return false;
	}

	const FVector RawDelta = CurrentPoint - DragStartPoint;
	FVector NewLocation = DragStartTargetLocation;

	if (DragAxis == EParkourGizmoAxis::XY)
	{
		NewLocation += FVector(RawDelta.X, RawDelta.Y, 0.0f);
	}
	else
	{
		const FVector AxisDirection = GetAxisDirection(DragAxis);
		NewLocation += AxisDirection * FVector::DotProduct(RawDelta, AxisDirection);
	}

	NewLocation = SnapLocation(NewLocation);
	TargetActor->SetActorLocation(NewLocation);
	SetActorLocation(NewLocation);
	return true;
}

void AParkourTransformGizmo::EndDrag()
{
	DragAxis = EParkourGizmoAxis::None;
}

void AParkourTransformGizmo::SetSnapSize(float NewSnapSize)
{
	SnapSize = FMath::Max(0.0f, NewSnapSize);
}

FVector AParkourTransformGizmo::GetAxisDirection(EParkourGizmoAxis Axis) const
{
	switch (Axis)
	{
	case EParkourGizmoAxis::X:
		return FVector::ForwardVector;
	case EParkourGizmoAxis::Y:
		return FVector::RightVector;
	case EParkourGizmoAxis::Z:
		return FVector::UpVector;
	default:
		return FVector::ZeroVector;
	}
}

bool AParkourTransformGizmo::IntersectDragPlane(const FVector& RayOrigin, const FVector& RayDirection, FVector& OutPoint) const
{
	const FVector RayEnd = RayOrigin + RayDirection.GetSafeNormal() * 100000.0f;
	if (FMath::IsNearlyZero(FVector::DotProduct(DragPlane.GetNormal(), RayDirection.GetSafeNormal())))
	{
		return false;
	}

	OutPoint = FMath::LinePlaneIntersection(RayOrigin, RayEnd, DragPlane);
	return true;
}

FVector AParkourTransformGizmo::SnapLocation(const FVector& Location) const
{
	if (SnapSize <= 0.0f)
	{
		return Location;
	}

	return FVector(
		FMath::GridSnap(Location.X, SnapSize),
		FMath::GridSnap(Location.Y, SnapSize),
		FMath::GridSnap(Location.Z, SnapSize)
	);
}

void AParkourTransformGizmo::SetGizmoVisible(bool bVisible)
{
	SetActorHiddenInGame(!bVisible);
	SetActorEnableCollision(bVisible);
}
