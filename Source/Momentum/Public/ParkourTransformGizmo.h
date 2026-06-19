#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ParkourTypes.h"
#include "ParkourTransformGizmo.generated.h"

class UStaticMeshComponent;

UCLASS()
class MOMENTUM_API AParkourTransformGizmo : public AActor
{
	GENERATED_BODY()

public:
	AParkourTransformGizmo();

	UFUNCTION(BlueprintCallable, Category = "Parkour|Gizmo")
	void AttachToTarget(AActor* NewTarget);

	UFUNCTION(BlueprintCallable, Category = "Parkour|Gizmo")
	void DetachFromTarget();

	UFUNCTION(BlueprintCallable, Category = "Parkour|Gizmo")
	bool BeginDrag(EParkourGizmoAxis Axis, const FVector& RayOrigin, const FVector& RayDirection, const FVector& CameraForward);

	UFUNCTION(BlueprintCallable, Category = "Parkour|Gizmo")
	bool UpdateDrag(const FVector& RayOrigin, const FVector& RayDirection);

	UFUNCTION(BlueprintCallable, Category = "Parkour|Gizmo")
	void EndDrag();

	UFUNCTION(BlueprintCallable, Category = "Parkour|Gizmo")
	void SetSnapSize(float NewSnapSize);

	UFUNCTION(BlueprintPure, Category = "Parkour|Gizmo")
	AActor* GetTargetActor() const { return TargetActor; }

	UFUNCTION(BlueprintPure, Category = "Parkour|Gizmo")
	bool IsDragging() const { return DragAxis != EParkourGizmoAxis::None; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Gizmo")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Gizmo")
	TObjectPtr<UStaticMeshComponent> XHandle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Gizmo")
	TObjectPtr<UStaticMeshComponent> YHandle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Gizmo")
	TObjectPtr<UStaticMeshComponent> ZHandle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Gizmo")
	TObjectPtr<UStaticMeshComponent> XYHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Gizmo")
	float SnapSize = 50.0f;

protected:
	FVector GetAxisDirection(EParkourGizmoAxis Axis) const;
	bool IntersectDragPlane(const FVector& RayOrigin, const FVector& RayDirection, FVector& OutPoint) const;
	FVector SnapLocation(const FVector& Location) const;
	void SetGizmoVisible(bool bVisible);

	UPROPERTY(Transient)
	TObjectPtr<AActor> TargetActor;

	EParkourGizmoAxis DragAxis = EParkourGizmoAxis::None;
	FPlane DragPlane;
	FVector DragStartPoint = FVector::ZeroVector;
	FVector DragStartTargetLocation = FVector::ZeroVector;
};
