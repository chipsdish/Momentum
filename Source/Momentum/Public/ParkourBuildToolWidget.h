#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ParkourBuildToolWidget.generated.h"

class AParkourBuildManager;
class AParkourBuildPiece;
class UBorder;
class UButton;
class UEditableTextBox;
class USlider;
class UTextBlock;
class UVerticalBox;

UCLASS()
class MOMENTUM_API UParkourBuildToolWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION()
	void SelectGround();

	UFUNCTION()
	void SelectRamp();

	UFUNCTION()
	void ConfirmPlacement();

	UFUNCTION()
	void CancelPlacement();

	UFUNCTION()
	void OnLengthSliderChanged(float Value);

	UFUNCTION()
	void OnWidthSliderChanged(float Value);

	UFUNCTION()
	void OnHeightSliderChanged(float Value);

	UFUNCTION()
	void OnSlopeSliderChanged(float Value);

	UFUNCTION()
	void OnRotationSliderChanged(float Value);

	UFUNCTION()
	void OnGridSnapSliderChanged(float Value);

	UFUNCTION()
	void OnLengthTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void OnWidthTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void OnHeightTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void OnSlopeTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void OnRotationTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void OnGridSnapTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void DeleteSelected();

	UFUNCTION()
	void SaveDefaultLayout();

	UFUNCTION()
	void LoadDefaultLayout();

	UFUNCTION()
	void ResetRuntimeLayout();

	AParkourBuildManager* FindBuildManager() const;
	UButton* MakeTextButton(const FString& Label);
	void AddToolbarButton(class UHorizontalBox* Parent, UButton* Button);
	void AddParameterRow(UVerticalBox* Parent, const FString& Label, TObjectPtr<USlider>& OutSlider, TObjectPtr<UEditableTextBox>& OutTextBox, float MinValue, float MaxValue);
	void SyncControlsFromPreview(bool bForceTextUpdate);
	void ApplyPreviewDimensions();
	void ApplyTextBoxValue(UEditableTextBox* TextBox, float Value) const;
	bool TryParseFloat(const FText& Text, float& OutValue) const;
	float GetSliderValue(USlider* Slider, float Fallback) const;
	void SetParameterPanelVisible(bool bVisible);

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SelectionText;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> ParameterPanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ParameterTitleText;

	UPROPERTY(Transient)
	TObjectPtr<USlider> LengthSlider;

	UPROPERTY(Transient)
	TObjectPtr<USlider> WidthSlider;

	UPROPERTY(Transient)
	TObjectPtr<USlider> HeightSlider;

	UPROPERTY(Transient)
	TObjectPtr<USlider> SlopeSlider;

	UPROPERTY(Transient)
	TObjectPtr<USlider> RotationSlider;

	UPROPERTY(Transient)
	TObjectPtr<USlider> GridSnapSlider;

	UPROPERTY(Transient)
	TObjectPtr<UEditableTextBox> LengthTextBox;

	UPROPERTY(Transient)
	TObjectPtr<UEditableTextBox> WidthTextBox;

	UPROPERTY(Transient)
	TObjectPtr<UEditableTextBox> HeightTextBox;

	UPROPERTY(Transient)
	TObjectPtr<UEditableTextBox> SlopeTextBox;

	UPROPERTY(Transient)
	TObjectPtr<UEditableTextBox> RotationTextBox;

	UPROPERTY(Transient)
	TObjectPtr<UEditableTextBox> GridSnapTextBox;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HelpText;

	UPROPERTY(Transient)
	TObjectPtr<AParkourBuildPiece> LastPreviewPiece;

	bool bSyncingControls = false;
};
