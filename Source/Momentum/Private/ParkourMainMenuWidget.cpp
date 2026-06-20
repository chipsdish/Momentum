#include "ParkourMainMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "ParkourPlayerController.h"
#include "Styling/CoreStyle.h"

void UParkourMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetTree)
	{
		return;
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DarkBackground"));
	Background->SetBrushColor(FLinearColor(0.015f, 0.018f, 0.025f, 0.96f));
	UCanvasPanelSlot* BackgroundSlot = Root->AddChildToCanvas(Background);
	BackgroundSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	BackgroundSlot->SetOffsets(FMargin(0.0f));

	UVerticalBox* MenuBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuBox"));
	UCanvasPanelSlot* MenuSlot = Root->AddChildToCanvas(MenuBox);
	MenuSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	MenuSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	MenuSlot->SetSize(FVector2D(520.0f, 360.0f));

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
	Title->SetText(FText::FromString(TEXT("Momentum")));
	Title->SetJustification(ETextJustify::Center);
	Title->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.95f, 1.0f, 1.0f)));
	Title->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 72));
	UVerticalBoxSlot* TitleSlot = MenuBox->AddChildToVerticalBox(Title);
	TitleSlot->SetHorizontalAlignment(HAlign_Fill);
	TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 28.0f));

	UTextBlock* Subtitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Subtitle"));
	Subtitle->SetText(FText::FromString(TEXT("丝滑惯性跑酷原型")));
	Subtitle->SetJustification(ETextJustify::Center);
	Subtitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.45f, 0.70f, 0.85f, 1.0f)));
	Subtitle->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 24));
	UVerticalBoxSlot* SubtitleSlot = MenuBox->AddChildToVerticalBox(Subtitle);
	SubtitleSlot->SetHorizontalAlignment(HAlign_Fill);
	SubtitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 42.0f));

	TestLevelButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("TestLevelButton"));
	UTextBlock* TestText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TestLevelButtonText"));
	TestText->SetText(FText::FromString(TEXT("测试关卡")));
	TestText->SetJustification(ETextJustify::Center);
	TestText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 28));
	TestLevelButton->AddChild(TestText);
	UVerticalBoxSlot* TestButtonSlot = MenuBox->AddChildToVerticalBox(TestLevelButton);
	TestButtonSlot->SetHorizontalAlignment(HAlign_Fill);
	TestButtonSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));

	QuitButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("QuitButton"));
	UTextBlock* QuitText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuitButtonText"));
	QuitText->SetText(FText::FromString(TEXT("退出游戏")));
	QuitText->SetJustification(ETextJustify::Center);
	QuitText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 24));
	QuitButton->AddChild(QuitText);
	UVerticalBoxSlot* QuitButtonSlot = MenuBox->AddChildToVerticalBox(QuitButton);
	QuitButtonSlot->SetHorizontalAlignment(HAlign_Fill);

	TestLevelButton->OnClicked.AddDynamic(this, &UParkourMainMenuWidget::HandleTestLevelClicked);
	QuitButton->OnClicked.AddDynamic(this, &UParkourMainMenuWidget::HandleQuitClicked);
	TestLevelButton->OnHovered.AddDynamic(this, &UParkourMainMenuWidget::HandleButtonHovered);
	QuitButton->OnHovered.AddDynamic(this, &UParkourMainMenuWidget::HandleButtonHovered);
	TestLevelButton->OnUnhovered.AddDynamic(this, &UParkourMainMenuWidget::HandleButtonUnhovered);
	QuitButton->OnUnhovered.AddDynamic(this, &UParkourMainMenuWidget::HandleButtonUnhovered);
}

void UParkourMainMenuWidget::HandleTestLevelClicked()
{
	if (AParkourPlayerController* Controller = Cast<AParkourPlayerController>(GetOwningPlayer()))
	{
		Controller->StartTestLevel();
	}
}

void UParkourMainMenuWidget::HandleQuitClicked()
{
	if (AParkourPlayerController* Controller = Cast<AParkourPlayerController>(GetOwningPlayer()))
	{
		Controller->QuitGame();
	}
}

void UParkourMainMenuWidget::HandleButtonHovered()
{
	SetRenderScale(FVector2D(1.015f, 1.015f));
}

void UParkourMainMenuWidget::HandleButtonUnhovered()
{
	SetRenderScale(FVector2D(1.0f, 1.0f));
}
