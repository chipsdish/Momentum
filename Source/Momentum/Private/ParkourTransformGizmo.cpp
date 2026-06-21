#include "ParkourTransformGizmo.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
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
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	for (UStaticMeshComponent* Handle : Handles)
	{
		Handle->SetupAttachment(SceneRoot);
		Handle->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Handle->SetCollisionResponseToAllChannels(ECR_Ignore);
		Handle->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		Handle->SetDepthPriorityGroup(SDPG_Foreground);
		Handle->TranslucencySortPriority = 1000;
		if (CubeMesh.Succeeded())
		{
			Handle->SetStaticMesh(CubeMesh.Object);
		}
		if (BasicShapeMaterial.Succeeded())
		{
			Handle->SetMaterial(0, BasicShapeMaterial.Object);
		}
	}

	XHandle->SetRelativeLocation(FVector(160.0f, 0.0f, 0.0f));
	XHandle->SetRelativeScale3D(FVector(3.2f, 0.28f, 0.28f));

	YHandle->SetRelativeLocation(FVector(0.0f, 160.0f, 0.0f));
	YHandle->SetRelativeScale3D(FVector(0.28f, 3.2f, 0.28f));

	ZHandle->SetRelativeLocation(FVector(0.0f, 0.0f, 160.0f));
	ZHandle->SetRelativeScale3D(FVector(0.28f, 0.28f, 3.2f));

	XYHandle->SetRelativeLocation(FVector(80.0f, 80.0f, 0.0f));
	XYHandle->SetRelativeScale3D(FVector(1.6f, 1.6f, 0.06f));

	SetHandleColor(XHandle, FLinearColor(1.0f, 0.05f, 0.03f, 1.0f));
	SetHandleColor(YHandle, FLinearColor(0.05f, 0.9f, 0.05f, 1.0f));
	SetHandleColor(ZHandle, FLinearColor(0.08f, 0.35f, 1.0f, 1.0f));
	SetHandleColor(XYHandle, FLinearColor(1.0f, 0.85f, 0.05f, 1.0f));

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

EParkourGizmoAxis AParkourTransformGizmo::HitTestAxis(const FVector& RayOrigin, const FVector& RayDirection) const
{
	if (!TargetActor || IsHidden())
	{
		return EParkourGizmoAxis::None;
	}

	EParkourGizmoAxis BestAxis = EParkourGizmoAxis::None;
	float BestDistance = AxisPickRadius;
	float BestRayParameter = TNumericLimits<float>::Max();
	const FVector Origin = GetActorLocation();

	const EParkourGizmoAxis Axes[] = { EParkourGizmoAxis::X, EParkourGizmoAxis::Y, EParkourGizmoAxis::Z };
	for (EParkourGizmoAxis Axis : Axes)
	{
		const FVector AxisDirection = GetAxisDirection(Axis);
		const FVector SegmentEnd = Origin + AxisDirection * HandleLength;
		float Distance = 0.0f;
		float RayParameter = 0.0f;
		if (ComputeRaySegmentDistance(RayOrigin, RayDirection, Origin, SegmentEnd, Distance, RayParameter) && Distance <= BestDistance)
		{
			if (Distance < BestDistance || RayParameter < BestRayParameter)
			{
				BestDistance = Distance;
				BestRayParameter = RayParameter;
				BestAxis = Axis;
			}
		}
	}

	FVector PlanePoint = FVector::ZeroVector;
	const FPlane XYPlane(Origin, FVector::UpVector);
	const FVector SafeRayDirection = RayDirection.GetSafeNormal();
	if (!SafeRayDirection.IsNearlyZero() && !FMath::IsNearlyZero(FVector::DotProduct(XYPlane.GetNormal(), SafeRayDirection)))
	{
		const FVector RayEnd = RayOrigin + SafeRayDirection * 100000.0f;
		PlanePoint = FMath::LinePlaneIntersection(RayOrigin, RayEnd, XYPlane);
		const FVector LocalPoint = PlanePoint - Origin;
		if (LocalPoint.X >= 0.0f && LocalPoint.Y >= 0.0f && LocalPoint.X <= PlanePickSize && LocalPoint.Y <= PlanePickSize)
		{
			const float RayParameter = FVector::DotProduct(PlanePoint - RayOrigin, SafeRayDirection);
			if (BestAxis == EParkourGizmoAxis::None || RayParameter < BestRayParameter)
			{
				BestAxis = EParkourGizmoAxis::XY;
			}
		}
	}

	return BestAxis;
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

	if (Axis == EParkourGizmoAxis::XY)
	{
		if (!IntersectDragPlane(RayOrigin, RayDirection, DragStartPoint))
		{
			EndDrag();
			return false;
		}
	}
	else if (!ComputeAxisDragParameter(RayOrigin, RayDirection, DragStartAxisParameter))
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

	FVector NewLocation = DragStartTargetLocation;

	if (DragAxis == EParkourGizmoAxis::XY)
	{
		FVector CurrentPoint = FVector::ZeroVector;
		if (!IntersectDragPlane(RayOrigin, RayDirection, CurrentPoint))
		{
			return false;
		}

		const FVector RawDelta = CurrentPoint - DragStartPoint;
		NewLocation += FVector(RawDelta.X, RawDelta.Y, 0.0f);
	}
	else
	{
		float CurrentAxisParameter = 0.0f;
		if (!ComputeAxisDragParameter(RayOrigin, RayDirection, CurrentAxisParameter))
		{
			return false;
		}

		const FVector AxisDirection = GetAxisDirection(DragAxis);
		NewLocation += AxisDirection * (CurrentAxisParameter - DragStartAxisParameter);
	}

	NewLocation = SnapLocationForDrag(NewLocation);
	TargetActor->SetActorLocation(NewLocation);
	SetActorLocation(NewLocation);
	return true;
}

void AParkourTransformGizmo::EndDrag()
{
	DragAxis = EParkourGizmoAxis::None;
	DragStartAxisParameter = 0.0f;
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

bool AParkourTransformGizmo::ComputeAxisDragParameter(const FVector& RayOrigin, const FVector& RayDirection, float& OutParameter) const
{
	const FVector AxisDirection = GetAxisDirection(DragAxis).GetSafeNormal();
	const FVector SafeRayDirection = RayDirection.GetSafeNormal();
	if (AxisDirection.IsNearlyZero() || SafeRayDirection.IsNearlyZero())
	{
		return false;
	}

	const float AxisRayDot = FVector::DotProduct(AxisDirection, SafeRayDirection);
	const float Denominator = 1.0f - AxisRayDot * AxisRayDot;
	if (FMath::IsNearlyZero(Denominator, 0.0001f))
	{
		return false;
	}

	const FVector AxisToRay = DragStartTargetLocation - RayOrigin;
	const float AxisProjection = FVector::DotProduct(AxisDirection, AxisToRay);
	const float RayProjection = FVector::DotProduct(SafeRayDirection, AxisToRay);
	OutParameter = (AxisRayDot * RayProjection - AxisProjection) / Denominator;
	return true;
}

bool AParkourTransformGizmo::ComputeRaySegmentDistance(const FVector& RayOrigin, const FVector& RayDirection, const FVector& SegmentStart, const FVector& SegmentEnd, float& OutDistance, float& OutRayParameter) const
{
	const FVector SafeRayDirection = RayDirection.GetSafeNormal();
	const FVector SegmentDirection = SegmentEnd - SegmentStart;
	const float SegmentLength = SegmentDirection.Size();
	if (SafeRayDirection.IsNearlyZero() || SegmentLength <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const FVector SafeSegmentDirection = SegmentDirection / SegmentLength;
	const FVector RayToSegment = RayOrigin - SegmentStart;
	const float RaySegmentDot = FVector::DotProduct(SafeRayDirection, SafeSegmentDirection);
	const float Denominator = 1.0f - RaySegmentDot * RaySegmentDot;

	float RayParameter = 0.0f;
	float SegmentParameter = 0.0f;
	if (FMath::IsNearlyZero(Denominator, 0.0001f))
	{
		SegmentParameter = FMath::Clamp(FVector::DotProduct(RayOrigin - SegmentStart, SafeSegmentDirection), 0.0f, SegmentLength);
		const FVector SegmentPoint = SegmentStart + SafeSegmentDirection * SegmentParameter;
		RayParameter = FMath::Max(FVector::DotProduct(SegmentPoint - RayOrigin, SafeRayDirection), 0.0f);
	}
	else
	{
		const float RayToSegmentOnRay = FVector::DotProduct(RayToSegment, SafeRayDirection);
		const float RayToSegmentOnSegment = FVector::DotProduct(RayToSegment, SafeSegmentDirection);
		RayParameter = (RaySegmentDot * RayToSegmentOnSegment - RayToSegmentOnRay) / Denominator;
		SegmentParameter = (RayToSegmentOnSegment - RaySegmentDot * RayToSegmentOnRay) / Denominator;
		RayParameter = FMath::Max(RayParameter, 0.0f);
		SegmentParameter = FMath::Clamp(SegmentParameter, 0.0f, SegmentLength);
	}

	const FVector ClosestRayPoint = RayOrigin + SafeRayDirection * RayParameter;
	const FVector ClosestSegmentPoint = SegmentStart + SafeSegmentDirection * SegmentParameter;
	OutDistance = FVector::Dist(ClosestRayPoint, ClosestSegmentPoint);
	OutRayParameter = RayParameter;
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

FVector AParkourTransformGizmo::SnapLocationForDrag(const FVector& Location) const
{
	if (SnapSize <= 0.0f)
	{
		return Location;
	}

	FVector SnappedLocation = DragStartTargetLocation;
	switch (DragAxis)
	{
	case EParkourGizmoAxis::X:
		SnappedLocation.X = FMath::GridSnap(Location.X, SnapSize);
		break;
	case EParkourGizmoAxis::Y:
		SnappedLocation.Y = FMath::GridSnap(Location.Y, SnapSize);
		break;
	case EParkourGizmoAxis::Z:
		SnappedLocation.Z = FMath::GridSnap(Location.Z, SnapSize);
		break;
	case EParkourGizmoAxis::XY:
		SnappedLocation.X = FMath::GridSnap(Location.X, SnapSize);
		SnappedLocation.Y = FMath::GridSnap(Location.Y, SnapSize);
		break;
	default:
		SnappedLocation = SnapLocation(Location);
		break;
	}

	return SnappedLocation;
}

void AParkourTransformGizmo::SetGizmoVisible(bool bVisible)
{
	SetActorHiddenInGame(!bVisible);
	SetActorEnableCollision(bVisible);
}

void AParkourTransformGizmo::SetHandleColor(UStaticMeshComponent* Handle, const FLinearColor& Color)
{
	if (!Handle)
	{
		return;
	}

	UMaterialInterface* SourceMaterial = Handle->GetMaterial(0);
	if (!SourceMaterial)
	{
		return;
	}

	UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(SourceMaterial, this);
	if (!DynamicMaterial)
	{
		return;
	}

	DynamicMaterial->SetVectorParameterValue(TEXT("Color"), Color);
	DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), Color);
	Handle->SetMaterial(0, DynamicMaterial);
}
