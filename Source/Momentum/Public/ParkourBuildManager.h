#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ParkourTypes.h"
#include "ParkourBuildManager.generated.h"

class AParkourBuildPiece;
class UParkourBuildSaveGame;

UCLASS()
class MOMENTUM_API AParkourBuildManager : public AActor
{
	GENERATED_BODY()

public:
	AParkourBuildManager();

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	AParkourBuildPiece* AddDefaultPiece(EParkourBuildPieceType PieceType);

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
	void ClearRuntimePieces();

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	bool SaveLayout(const FString& LayoutName);

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	bool LoadLayout(const FString& LayoutName);

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	bool DeleteLayout(const FString& LayoutName);

	UFUNCTION(BlueprintPure, Category = "Parkour|Build")
	TArray<FString> GetSavedLayoutNames() const;

	UFUNCTION(BlueprintCallable, Category = "Parkour|Build")
	void ResetDefaultRuntimeLayout();

	UFUNCTION(BlueprintPure, Category = "Parkour|Build")
	AParkourBuildPiece* GetSelectedPiece() const { return SelectedPiece; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Build")
	TSubclassOf<AParkourBuildPiece> BuildPieceClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Build")
	FString SaveSlotName = TEXT("ParkourRuntimeLayouts");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Build")
	int32 SaveUserIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Build")
	float DuplicateOffset = 150.0f;

protected:
	UParkourBuildSaveGame* LoadOrCreateSaveGame() const;
	FParkourBuildPieceData MakeDefaultPieceData(EParkourBuildPieceType PieceType) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Build")
	TArray<TObjectPtr<AParkourBuildPiece>> RuntimePieces;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Build")
	TObjectPtr<AParkourBuildPiece> SelectedPiece;
};
