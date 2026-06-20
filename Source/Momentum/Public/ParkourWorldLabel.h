#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Actor.h"
#include "ParkourWorldLabel.generated.h"

class UTextBlock;
class UWidgetComponent;

UCLASS()
class MOMENTUM_API UParkourWorldLabelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Parkour|Label")
	void SetLabelText(const FString& NewText, float NewFontSize);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LabelTextBlock;

	FString CurrentText;
	float CurrentFontSize = 28.0f;
};

UCLASS()
class MOMENTUM_API AParkourWorldLabel : public AActor
{
	GENERATED_BODY()

public:
	AParkourWorldLabel();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Parkour|Label")
	void SetLabelText(const FString& NewText, float NewFontSize);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Label")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Parkour|Label")
	TObjectPtr<UWidgetComponent> WidgetComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Label")
	FString LabelText = TEXT("Label");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Label")
	float FontSize = 28.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Label")
	FVector2D DrawSize = FVector2D(760.0f, 180.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Label")
	float WorldScale = 0.55f;

	UFUNCTION(BlueprintCallable, Category = "Parkour|Label")
	void UpdateLabelWidget();
};
