#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ParkourTypes.h"
#include "ParkourBuildManager.generated.h"

class AParkourBuildPiece;
class APlayerController;
class UParkourBuildSaveGame;

UCLASS()
class MOMENTUM_API AParkourBuildManager : public AActor
{
	GENERATED_BODY()

public:
	AParkourBuildManager();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	AParkourBuildPiece* AddDefaultPiece(EParkourBuildPieceType PieceType);

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	bool BeginPreviewPlacement(EParkourBuildPieceType PieceType);

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	bool ConfirmPreviewPlacement();

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	void CancelPreviewPlacement();

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	void UpdatePreviewFromController(APlayerController* Controller);

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	void SetPreviewDimensions(const FVector& NewDimensions);

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	void SetPreviewSlopeAngle(float NewSlopeAngle);

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	void SetPreviewRotationYaw(float NewYaw);

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	void SetGridSnapSize(float NewGridSnapSize);

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	AParkourBuildPiece* AddPieceFromData(const FParkourBuildPieceData& PieceData);

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	void SelectPiece(AParkourBuildPiece* Piece);

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	void ClearSelection();

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	void DeleteSelectedPiece();

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	AParkourBuildPiece* DuplicateSelectedPiece();

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	void AdjustSelectedSlopeAngle(float DeltaAngle);

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	void SetSelectedSlopeAngle(float NewSlopeAngle);

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	void AdjustSelectedDimensions(const FVector& DeltaDimensions);

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	void ClearRuntimePieces();

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	bool SaveLayout(const FString& LayoutName);

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	bool LoadLayout(const FString& LayoutName);

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	bool DeleteLayout(const FString& LayoutName);

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	bool TryAutoLoadSavedLayout();

	UFUNCTION(BlueprintPure, Category = "Parkour|Build")
	TArray<FString> GetSavedLayoutNames() const;

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	void ResetDefaultRuntimeLayout();

	UFUNCTION(BlueprintPure, Category = "Parkour|Build")
	AParkourBuildPiece* GetSelectedPiece() const { return SelectedPiece; }

	UFUNCTION(BlueprintPure, Category = "Parkour|Build")
	AParkourBuildPiece* GetPreviewPiece() const { return PreviewPiece; }

	UFUNCTION(BlueprintPure, Category = "Parkour|Build")
	bool HasActivePreview() const { return PreviewPiece != nullptr; }

	UFUNCTION(BlueprintPure, Category = "Parkour|Build")
	FParkourBuildPieceData GetPreviewData() const { return PreviewPieceData; }

	UFUNCTION(BlueprintPure, Category = "Parkour|Build")
	float GetGridSnapSize() const { return GridSnapSize; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Build")
	TSubclassOf<AParkourBuildPiece> BuildPieceClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Build")
	FString SaveSlotName = TEXT("ParkourRuntimeLayouts");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Build")
	int32 SaveUserIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Build")
	bool bAutoLoadSavedLayout = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Build")
	FString AutoLoadLayoutName = TEXT("布局 01");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Build")
	float DuplicateOffset = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Build")
	float GridSnapSize = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Build")
	float PlacementTraceDistance = 30000.0f;

protected:
	void RegisterExistingBuildPieces();
	UParkourBuildSaveGame* LoadOrCreateSaveGame() const;
	FParkourBuildPieceData MakeDefaultPieceData(EParkourBuildPieceType PieceType) const;
	FVector SnapLocation(const FVector& Location) const;
	FVector GetPlacementOffset(const FParkourBuildPieceData& PieceData) const;
	void ApplyPreviewData();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Build")
	TArray<TObjectPtr<AParkourBuildPiece>> RuntimePieces;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Build")
	TObjectPtr<AParkourBuildPiece> SelectedPiece;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Build")
	TObjectPtr<AParkourBuildPiece> PreviewPiece;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Build")
	FParkourBuildPieceData PreviewPieceData;
};
