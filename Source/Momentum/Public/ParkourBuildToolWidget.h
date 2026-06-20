#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ParkourBuildToolWidget.generated.h"

class AParkourBuildManager;
class UTextBlock;

UCLASS()
class MOMENTUM_API UParkourBuildToolWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION()
	void AddPlatform();

	UFUNCTION()
	void AddRamp();

	UFUNCTION()
	void AddJumpRamp();

	UFUNCTION()
	void AddAccelerationRamp();

	UFUNCTION()
	void AddAirPlatform();

	UFUNCTION()
	void AddWallPlatform();

	UFUNCTION()
	void DuplicateSelected();

	UFUNCTION()
	void DeleteSelected();

	UFUNCTION()
	void SaveDefaultLayout();

	UFUNCTION()
	void LoadDefaultLayout();

	UFUNCTION()
	void ResetRuntimeLayout();

	AParkourBuildManager* FindBuildManager() const;
	class UButton* AddButton(class UVerticalBox* Parent, const FString& Label);

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SelectionText;
};
