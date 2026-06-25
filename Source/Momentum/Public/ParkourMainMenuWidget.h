#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ParkourMainMenuWidget.generated.h"

class UBorder;
class UButton;
class UTextBlock;
class USoundBase;

struct FMainMenuButtonAnimState
{
	bool bHovered = false;
	float CurrentScale = 1.0f;
	float TargetScale = 1.0f;
	float PressedTimer = 0.0f;
};

UCLASS()
class MOMENTUM_API UParkourMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UParkourMainMenuWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION()
	void HandleTestLevelClicked();

	UFUNCTION()
	void HandleSettingsClicked();

	UFUNCTION()
	void HandleQuitClicked();

	UFUNCTION()
	void HandleTestLevelHovered();

	UFUNCTION()
	void HandleSettingsHovered();

	UFUNCTION()
	void HandleQuitHovered();

	UFUNCTION()
	void HandleTestLevelUnhovered();

	UFUNCTION()
	void HandleSettingsUnhovered();

	UFUNCTION()
	void HandleQuitUnhovered();

	UFUNCTION()
	void HandleTestLevelPressed();

	UFUNCTION()
	void HandleSettingsPressed();

	UFUNCTION()
	void HandleQuitPressed();

	void SetButtonHovered(FMainMenuButtonAnimState& State, UBorder* AccentBar, UBorder* Fill, UTextBlock* Label, bool bHovered);
	void PressButton(FMainMenuButtonAnimState& State);
	void UpdateButtonVisual(UButton* Button, UBorder* AccentBar, UBorder* Fill, UTextBlock* Label, FMainMenuButtonAnimState& State, float DeltaTime);
	void ShowComingSoon();
	void PlayHoverSound() const;
	void PlayClickSound() const;

	UPROPERTY(Transient)
	TObjectPtr<UButton> TestLevelButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> SettingsButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> QuitButton;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> TestLevelAccentBar;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> SettingsAccentBar;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> QuitAccentBar;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> TestLevelFill;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> SettingsFill;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> QuitFill;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TestLevelLabel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SettingsLabel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> QuitLabel;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> ComingSoonPill;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ComingSoonText;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> SpeedLines;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> PreviewBlocks;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> HoverSound;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> ClickSound;

	FMainMenuButtonAnimState TestLevelState;
	FMainMenuButtonAnimState SettingsState;
	FMainMenuButtonAnimState QuitState;
	float MenuTime = 0.0f;
	float ComingSoonTimer = 0.0f;
};
