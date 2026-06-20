#include "ParkourHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "ParkourGameMode.h"
#include "ParkourMovementComponent.h"
#include "ParkourPlayerController.h"
#include "Styling/CoreStyle.h"

void UParkourHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetTree)
	{
		return;
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("HUDRoot"));
	WidgetTree->RootWidget = Root;

	UVerticalBox* DebugBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DebugBox"));
	UCanvasPanelSlot* DebugSlot = Root->AddChildToCanvas(DebugBox);
	DebugSlot->SetAnchors(FAnchors(0.0f, 0.0f));
	DebugSlot->SetPosition(FVector2D(28.0f, 28.0f));
	DebugSlot->SetSize(FVector2D(420.0f, 180.0f));

	auto MakeDebugText = [this, DebugBox](const FString& Name)
	{
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *Name);
		Text->SetColorAndOpacity(FSlateColor(FLinearColor(0.78f, 0.95f, 1.0f, 1.0f)));
		Text->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 22));
		UVerticalBoxSlot* Slot = DebugBox->AddChildToVerticalBox(Text);
		Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
		return Text;
	};

	SpeedText = MakeDebugText(TEXT("SpeedText"));
	StateText = MakeDebugText(TEXT("StateText"));
	SlopeText = MakeDebugText(TEXT("SlopeText"));
	TimerText = MakeDebugText(TEXT("TimerText"));

	FinishedPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("FinishedPanel"));
	FinishedPanel->SetVisibility(ESlateVisibility::Collapsed);
	UCanvasPanelSlot* FinishedSlot = Root->AddChildToCanvas(FinishedPanel);
	FinishedSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	FinishedSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	FinishedSlot->SetSize(FVector2D(520.0f, 300.0f));

	UTextBlock* FinishedTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FinishedTitle"));
	FinishedTitle->SetText(FText::FromString(TEXT("完成")));
	FinishedTitle->SetJustification(ETextJustify::Center);
	FinishedTitle->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 64));
	FinishedTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 1.0f, 0.85f, 1.0f)));
	FinishedPanel->AddChildToVerticalBox(FinishedTitle);

	FinishedTimeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FinishedTimeText"));
	FinishedTimeText->SetJustification(ETextJustify::Center);
	FinishedTimeText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 30));
	FinishedPanel->AddChildToVerticalBox(FinishedTimeText);

	UButton* RestartButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RestartButton"));
	UTextBlock* RestartText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RestartText"));
	RestartText->SetText(FText::FromString(TEXT("重新开始")));
	RestartText->SetJustification(ETextJustify::Center);
	RestartButton->AddChild(RestartText);
	FinishedPanel->AddChildToVerticalBox(RestartButton);
	RestartButton->OnClicked.AddDynamic(this, &UParkourHUDWidget::HandleRestartClicked);

	UButton* MainMenuButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("MainMenuButton"));
	UTextBlock* MainMenuText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MainMenuText"));
	MainMenuText->SetText(FText::FromString(TEXT("返回主菜单")));
	MainMenuText->SetJustification(ETextJustify::Center);
	MainMenuButton->AddChild(MainMenuText);
	FinishedPanel->AddChildToVerticalBox(MainMenuButton);
	MainMenuButton->OnClicked.AddDynamic(this, &UParkourHUDWidget::HandleMainMenuClicked);
}

void UParkourHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const APawn* Pawn = GetOwningPlayerPawn();
	const UParkourMovementComponent* Movement = Pawn ? Pawn->FindComponentByClass<UParkourMovementComponent>() : nullptr;
	const AParkourGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AParkourGameMode>() : nullptr;

	if (SpeedText)
	{
		const float Speed = Movement ? Movement->GetHorizontalSpeed() : 0.0f;
		SpeedText->SetText(FText::FromString(FString::Printf(TEXT("速度: %.0f uu/s"), Speed)));
	}

	if (StateText)
	{
		StateText->SetText(FText::FromString(FString::Printf(TEXT("状态: %s"), *ResolveMovementStateText())));
	}

	if (SlopeText)
	{
		const float Slope = Movement ? Movement->GetCurrentSlopeAngle() : 0.0f;
		SlopeText->SetText(FText::FromString(FString::Printf(TEXT("坡度: %.1f°"), Slope)));
	}

	if (TimerText)
	{
		const float Time = GameMode ? GameMode->GetElapsedRunTime() : 0.0f;
		TimerText->SetText(FText::FromString(FString::Printf(TEXT("计时: %.2f"), Time)));
	}

	const bool bFinished = GameMode && GameMode->IsRunFinished();
	if (FinishedPanel)
	{
		FinishedPanel->SetVisibility(bFinished ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (FinishedTimeText && bFinished)
	{
		FinishedTimeText->SetText(FText::FromString(FString::Printf(TEXT("时间: %.2f"), GameMode->GetElapsedRunTime())));
	}
}

void UParkourHUDWidget::HandleRestartClicked()
{
	if (AParkourPlayerController* Controller = Cast<AParkourPlayerController>(GetOwningPlayer()))
	{
		Controller->RestartCurrentLevel();
	}
}

void UParkourHUDWidget::HandleMainMenuClicked()
{
	if (AParkourPlayerController* Controller = Cast<AParkourPlayerController>(GetOwningPlayer()))
	{
		Controller->ReturnToMainMenu();
	}
}

FString UParkourHUDWidget::ResolveMovementStateText() const
{
	const APawn* Pawn = GetOwningPlayerPawn();
	const UParkourMovementComponent* Movement = Pawn ? Pawn->FindComponentByClass<UParkourMovementComponent>() : nullptr;
	if (!Movement)
	{
		return TEXT("未知");
	}

	switch (Movement->GetParkourMovementState())
	{
	case EParkourMovementState::Grounded:
		return TEXT("Grounded / 地面");
	case EParkourMovementState::Airborne:
		return TEXT("Airborne / 空中");
	case EParkourMovementState::Surfing:
		return TEXT("Surfing / 斜面滑行");
	case EParkourMovementState::Sliding:
		return TEXT("Sliding / 陡坡滑落");
	default:
		return TEXT("未知");
	}
}
