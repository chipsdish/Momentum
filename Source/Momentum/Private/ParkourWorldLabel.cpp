#include "ParkourWorldLabel.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/PlayerController.h"
#include "Styling/CoreStyle.h"

void UParkourWorldLabelWidget::SetLabelText(const FString& NewText, float NewFontSize)
{
	CurrentText = NewText;
	CurrentFontSize = FMath::Max(NewFontSize, 8.0f);

	if (LabelTextBlock)
	{
		LabelTextBlock->SetText(FText::FromString(CurrentText));
		LabelTextBlock->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), CurrentFontSize));
	}
}

TSharedRef<SWidget> UParkourWorldLabelWidget::RebuildWidget()
{
	const TSharedRef<SWidget> FallbackWidget = Super::RebuildWidget();
	if (!WidgetTree)
	{
		return FallbackWidget;
	}

	UBorder* Root = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LabelRoot"));
	Root->SetBrushColor(FLinearColor(0.02f, 0.025f, 0.035f, 0.88f));
	Root->SetPadding(FMargin(14.0f, 10.0f));
	WidgetTree->RootWidget = Root;

	LabelTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LabelText"));
	LabelTextBlock->SetText(FText::FromString(CurrentText));
	LabelTextBlock->SetAutoWrapText(true);
	LabelTextBlock->SetJustification(ETextJustify::Center);
	LabelTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.96f, 1.0f, 1.0f)));
	LabelTextBlock->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), CurrentFontSize));
	LabelTextBlock->SetShadowOffset(FVector2D(2.0f, 2.0f));
	LabelTextBlock->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
	Root->SetContent(LabelTextBlock);

	return Root->TakeWidget();
}

AParkourWorldLabel::AParkourWorldLabel()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
	WidgetComponent->SetupAttachment(SceneRoot);
	WidgetComponent->SetWidgetClass(UParkourWorldLabelWidget::StaticClass());
	WidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	WidgetComponent->SetDrawAtDesiredSize(false);
	WidgetComponent->SetDrawSize(DrawSize);
	WidgetComponent->SetPivot(FVector2D(0.5f, 0.5f));
	WidgetComponent->SetTwoSided(true);
	WidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WidgetComponent->SetRelativeScale3D(FVector(WorldScale));
}

void AParkourWorldLabel::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateFacing();
}

void AParkourWorldLabel::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateLabelWidget();
	UpdateFacing();
}

void AParkourWorldLabel::BeginPlay()
{
	Super::BeginPlay();
	UpdateLabelWidget();
	UpdateFacing();
}

void AParkourWorldLabel::SetLabelText(const FString& NewText, float NewFontSize)
{
	LabelText = NewText;
	FontSize = FMath::Max(NewFontSize, 8.0f);
	UpdateLabelWidget();
}

void AParkourWorldLabel::UpdateLabelWidget()
{
	if (!WidgetComponent)
	{
		return;
	}

	WidgetComponent->SetWidgetClass(UParkourWorldLabelWidget::StaticClass());
	WidgetComponent->SetDrawSize(DrawSize);
	WidgetComponent->SetRelativeScale3D(FVector(WorldScale));
	WidgetComponent->InitWidget();

	if (UParkourWorldLabelWidget* LabelWidget = Cast<UParkourWorldLabelWidget>(WidgetComponent->GetWidget()))
	{
		LabelWidget->SetLabelText(LabelText, FontSize);
	}
}

void AParkourWorldLabel::UpdateFacing()
{
	if (!bFacePlayerCamera || !GetWorld())
	{
		return;
	}

	const APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!PlayerController || !PlayerController->PlayerCameraManager)
	{
		return;
	}

	const FVector ToCamera = PlayerController->PlayerCameraManager->GetCameraLocation() - GetActorLocation();
	if (ToCamera.IsNearlyZero())
	{
		return;
	}

	FRotator FacingRotation = ToCamera.Rotation();
	FacingRotation.Pitch = 0.0f;
	FacingRotation.Roll = 0.0f;
	FacingRotation.Yaw += FacingYawOffset;
	SetActorRotation(FacingRotation);
}
