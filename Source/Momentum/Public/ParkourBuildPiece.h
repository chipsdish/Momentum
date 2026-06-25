#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ParkourTypes.h"
#include "ParkourBuildPiece.generated.h"

class UBoxComponent;
class APawn;
class UMaterialInterface;
class UProceduralMeshComponent;
class UPrimitiveComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class MOMENTUM_API AParkourBuildPiece : public AActor
{
	GENERATED_BODY()

public:
	AParkourBuildPiece();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	void ConfigureFromData(const FParkourBuildPieceData& NewPieceData);

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	void SetPreviewMode(bool bNewPreviewMode);

	UFUNCTION(BlueprintPure, Category = "Parkour|Build")
	FParkourBuildPieceData ToData() const;

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	void SetSelected(bool bSelected);

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	void SetBuildModeVisuals(bool bBuildModeActive);

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	void SetDimensions(const FVector& NewDimensions);

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	void AdjustDimensions(const FVector& DeltaDimensions);

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	void SetLabelText(const FString& NewLabel);

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	void SetPieceType(EParkourBuildPieceType NewPieceType);

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	void SetPieceTypeName(const FString& NewPieceTypeName);

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	void SetSlopeAngle(float NewSlopeAngle);

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	void AdjustSlopeAngle(float DeltaAngle);

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	void SetUseInwardBank(bool bNewUseInwardBank);

	UFUNCTION(BlueprintPure, Category = "Parkour|Build")
	float GetSlopeAngle() const { return PieceData.SlopeAngle; }

	UFUNCTION(BlueprintPure, Category = "Parkour|Build")
	FGuid GetPieceId() const { return PieceData.PieceId; }

	UFUNCTION(BlueprintPure, Category = "Parkour|Build")
	EParkourBuildPieceType GetPieceType() const { return PieceData.PieceType; }

	UFUNCTION(BlueprintPure, Category = "Parkour|Build")
	FVector GetDimensions() const { return PieceData.Dimensions; }

	UFUNCTION(BlueprintPure, Category = "Parkour|Build")
	bool IsPreviewMode() const { return bPreviewMode; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Build")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Build")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Build")
	TObjectPtr<UProceduralMeshComponent> RampMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Build")
	TObjectPtr<UBoxComponent> SelectionBounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Build")
	TObjectPtr<UBoxComponent> TriggerVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Build")
	TObjectPtr<UTextRenderComponent> LabelText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Build")
	FParkourBuildPieceData PieceData;

protected:
	void ApplyPieceVisuals();
	void ApplySlopeRotation();
	void RebuildRampMesh(const FVector& SafeDimensions);
	bool IsRampLikePiece() const;
	float GetRampRise(const FVector& SafeDimensions) const;
	float GetInwardSideSign() const;
	void ApplyBoostPad(APawn* Pawn);
	bool CanBoostActor(AActor* Actor) const;
	void RecordBoostActor(AActor* Actor);

	UFUNCTION()
	void OnBuildPieceOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Build")
	float BoostPadReuseCooldown = 0.25f;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> DefaultBuildMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> PreviewBuildMaterial;

	TMap<TWeakObjectPtr<AActor>, float> LastBoostTimes;

	bool bPreviewMode = false;
};
