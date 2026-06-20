#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ParkourTypes.h"
#include "ParkourBuildPiece.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class MOMENTUM_API AParkourBuildPiece : public AActor
{
	GENERATED_BODY()

public:
	AParkourBuildPiece();

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	void ConfigureFromData(const FParkourBuildPieceData& NewPieceData);

	UFUNCTION(BlueprintPure, Category = "Parkour|Build")
	FParkourBuildPieceData ToData() const;

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	void SetSelected(bool bSelected);

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	void SetDimensions(const FVector& NewDimensions);

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Build")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Build")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Build")
	TObjectPtr<UBoxComponent> SelectionBounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Build")
	TObjectPtr<UTextRenderComponent> LabelText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Build")
	FParkourBuildPieceData PieceData;

protected:
	void ApplyPieceVisuals();
	void ApplySlopeRotation();
	float GetInwardBankRoll(float SlopeAngle) const;
};
