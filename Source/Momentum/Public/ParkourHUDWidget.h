#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ParkourHUDWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;

UCLASS()
class MOMENTUM_API UParkourHUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION()
	void HandleRestartClicked();

	UFUNCTION()
	void HandleMainMenuClicked();

	FString ResolveMovementStateText() const;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SpeedText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StateText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SlopeText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TimerText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FinishedTimeText;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> FinishedPanel;
};
