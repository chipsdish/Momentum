#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ParkourMainMenuWidget.generated.h"

class UButton;

UCLASS()
class MOMENTUM_API UParkourMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void HandleTestLevelClicked();

	UFUNCTION()
	void HandleQuitClicked();

	UFUNCTION()
	void HandleButtonHovered();

	UFUNCTION()
	void HandleButtonUnhovered();

	UPROPERTY(Transient)
	TObjectPtr<UButton> TestLevelButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> QuitButton;
};
