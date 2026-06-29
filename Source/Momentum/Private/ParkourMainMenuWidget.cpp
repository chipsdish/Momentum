#include "ParkourMainMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/GameplayStatics.h"
#include "ParkourPlayerController.h"
#include "Sound/SoundBase.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FLinearColor BackgroundColor(0.006f, 0.008f, 0.012f, 1.0f);
	const FLinearColor PanelColor(0.025f, 0.033f, 0.045f, 0.82f);
	const FLinearColor ButtonIdleColor(0.06f, 0.075f, 0.095f, 0.72f);
	const FLinearColor ButtonHoverColor(0.10f, 0.16f, 0.20f, 0.92f);
	const FLinearColor TextPrimary(0.88f, 0.95f, 1.0f, 1.0f);
	const FLinearColor TextSecondary(0.42f, 0.58f, 0.68f, 1.0f);

	FButtonStyle MakeTransparentButtonStyle()
	{
		return FButtonStyle()
			.SetNormal(FSlateNoResource())
			.SetHovered(FSlateNoResource())
			.SetPressed(FSlateNoResource())
			.SetDisabled(FSlateNoResource())
			.SetNormalPadding(FMargin(0.0f))
			.SetPressedPadding(FMargin(0.0f));
	}

	void AddCanvasChild(UCanvasPanel* Root, UWidget* Widget, const FAnchors& Anchors, const FMargin& Offsets, const FVector2D& Alignment = FVector2D::ZeroVector)
	{
		if (!Root || !Widget)
		{
			return;
		}

		UCanvasPanelSlot* Slot = Root->AddChildToCanvas(Widget);
		Slot->SetAnchors(Anchors);
		Slot->SetOffsets(Offsets);
		Slot->SetAlignment(Alignment);
	}

	UBorder* MakeBorder(UWidgetTree* WidgetTree, const FName Name, const FLinearColor& Color)
	{
		UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
		Border->SetBrushColor(Color);
		return Border;
	}

	UTextBlock* MakeText(UWidgetTree* WidgetTree, const FName Name, const FString& Text, const int32 Size, const FLinearColor& Color)
	{
		UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		TextBlock->SetText(FText::FromString(Text));
		TextBlock->SetColorAndOpacity(FSlateColor(Color));
		TextBlock->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), Size));
		return TextBlock;
	}
}

UParkourMainMenuWidget::UParkourMainMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<USoundBase> HoverSoundFinder(TEXT("/Engine/VREditor/Sounds/VR_click1_Cue.VR_click1_Cue"));
	if (HoverSoundFinder.Succeeded())
	{
		HoverSound = HoverSoundFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> ClickSoundFinder(TEXT("/Engine/VREditor/Sounds/UI/Click_on_Button_Cue.Click_on_Button_Cue"));
	if (ClickSoundFinder.Succeeded())
	{
		ClickSound = ClickSoundFinder.Object;
	}
}

TSharedRef<SWidget> UParkourMainMenuWidget::RebuildWidget()
{
	const TSharedRef<SWidget> FallbackWidget = Super::RebuildWidget();

	if (!WidgetTree)
	{
		return FallbackWidget;
	}

	SpeedLines.Reset();
	PreviewBlocks.Reset();
	ComingSoonPill = nullptr;
	ComingSoonText = nullptr;
	TestLevelState = FMainMenuButtonAnimState();
	SettingsState = FMainMenuButtonAnimState();
	QuitState = FMainMenuButtonAnimState();
	MenuTime = 0.0f;
	ComingSoonTimer = 0.0f;

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	UBorder* Background = MakeBorder(WidgetTree, TEXT("Background"), BackgroundColor);
	AddCanvasChild(Root, Background, FAnchors(0.0f, 0.0f, 1.0f, 1.0f), FMargin(0.0f));

	for (int32 LineIndex = 0; LineIndex < 14; ++LineIndex)
	{
		const float Alpha = LineIndex % 3 == 0 ? 0.16f : 0.075f;
		UBorder* Line = MakeBorder(WidgetTree, *FString::Printf(TEXT("SpeedLine_%02d"), LineIndex), FLinearColor(0.16f, 0.67f, 0.86f, Alpha));
		Line->SetRenderTransformAngle(-13.0f);
		const float Top = 48.0f + LineIndex * 62.0f;
		const float Left = 560.0f + (LineIndex % 5) * 116.0f;
		AddCanvasChild(Root, Line, FAnchors(0.0f, 0.0f), FMargin(Left, Top, 420.0f + (LineIndex % 4) * 160.0f, 2.0f));
		SpeedLines.Add(Line);
	}

	for (int32 GridIndex = 0; GridIndex < 8; ++GridIndex)
	{
		UBorder* GridLine = MakeBorder(WidgetTree, *FString::Printf(TEXT("GridLine_%02d"), GridIndex), FLinearColor(0.45f, 0.60f, 0.72f, 0.045f));
		AddCanvasChild(Root, GridLine, FAnchors(0.0f, 0.0f, 1.0f, 0.0f), FMargin(0.0f, 110.0f + GridIndex * 92.0f, 0.0f, 1.0f));
	}

	UBorder* LeftPanel = MakeBorder(WidgetTree, TEXT("LeftPanel"), PanelColor);
	AddCanvasChild(Root, LeftPanel, FAnchors(0.0f, 0.0f, 0.48f, 1.0f), FMargin(0.0f));

	UBorder* PanelEdge = MakeBorder(WidgetTree, TEXT("PanelEdge"), FLinearColor(0.16f, 0.75f, 0.95f, 0.24f));
	AddCanvasChild(Root, PanelEdge, FAnchors(0.48f, 0.0f, 0.48f, 1.0f), FMargin(-2.0f, 0.0f, 2.0f, 0.0f));

	UVerticalBox* MenuBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuBox"));
	AddCanvasChild(Root, MenuBox, FAnchors(0.0f, 0.5f), FMargin(88.0f, -285.0f, 460.0f, 570.0f));

	UTextBlock* Eyebrow = MakeText(WidgetTree, TEXT("Eyebrow"), TEXT("B H O P   /   K Z   /   S U R F   P R O T O T Y P E"), 16, FLinearColor(0.38f, 0.78f, 0.96f, 1.0f));
	UVerticalBoxSlot* EyebrowSlot = MenuBox->AddChildToVerticalBox(Eyebrow);
	EyebrowSlot->SetPadding(FMargin(2.0f, 0.0f, 0.0f, 14.0f));

	UTextBlock* Title = MakeText(WidgetTree, TEXT("Title"), TEXT("MOMENTUM"), 86, TextPrimary);
	Title->SetShadowColorAndOpacity(FLinearColor(0.04f, 0.55f, 0.78f, 0.65f));
	Title->SetShadowOffset(FVector2D(0.0f, 0.0f));
	UVerticalBoxSlot* TitleSlot = MenuBox->AddChildToVerticalBox(Title);
	TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));

	UTextBlock* Subtitle = MakeText(WidgetTree, TEXT("Subtitle"), TEXT("A greybox movement lab for speed, flow, and clean lines."), 18, TextSecondary);
	UVerticalBoxSlot* SubtitleSlot = MenuBox->AddChildToVerticalBox(Subtitle);
	SubtitleSlot->SetPadding(FMargin(3.0f, 0.0f, 0.0f, 54.0f));

	auto BuildMenuButton = [this, MenuBox](const FName ButtonName, const FName FillName, const FName AccentName, const FName TextName, const FString& Label, TObjectPtr<UButton>& OutButton, TObjectPtr<UBorder>& OutFill, TObjectPtr<UBorder>& OutAccent, TObjectPtr<UTextBlock>& OutLabel)
	{
		OutButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
		OutButton->SetStyle(MakeTransparentButtonStyle());

		UOverlay* ButtonOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), *FString::Printf(TEXT("%sOverlay"), *ButtonName.ToString()));
		OutButton->AddChild(ButtonOverlay);

		OutFill = MakeBorder(WidgetTree, FillName, ButtonIdleColor);
		UOverlaySlot* FillSlot = ButtonOverlay->AddChildToOverlay(OutFill);
		FillSlot->SetHorizontalAlignment(HAlign_Fill);
		FillSlot->SetVerticalAlignment(VAlign_Fill);

		UHorizontalBox* ContentRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), *FString::Printf(TEXT("%sContent"), *ButtonName.ToString()));
		UOverlaySlot* RowSlot = ButtonOverlay->AddChildToOverlay(ContentRow);
		RowSlot->SetHorizontalAlignment(HAlign_Fill);
		RowSlot->SetVerticalAlignment(VAlign_Center);
		RowSlot->SetPadding(FMargin(0.0f));

		OutAccent = MakeBorder(WidgetTree, AccentName, FLinearColor(0.18f, 0.82f, 1.0f, 0.0f));
		UHorizontalBoxSlot* AccentSlot = ContentRow->AddChildToHorizontalBox(OutAccent);
		AccentSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		AccentSlot->SetPadding(FMargin(0.0f, 10.0f, 18.0f, 10.0f));
		AccentSlot->SetHorizontalAlignment(HAlign_Left);
		AccentSlot->SetVerticalAlignment(VAlign_Fill);
		USpacer* AccentSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass(), *FString::Printf(TEXT("%sAccentSpacer"), *ButtonName.ToString()));
		AccentSpacer->SetSize(FVector2D(4.0f, 1.0f));
		OutAccent->SetContent(AccentSpacer);

		OutLabel = MakeText(WidgetTree, TextName, Label, 23, TextPrimary);
		UHorizontalBoxSlot* TextSlot = ContentRow->AddChildToHorizontalBox(OutLabel);
		TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TextSlot->SetPadding(FMargin(0.0f, 0.0f, 18.0f, 0.0f));
		TextSlot->SetVerticalAlignment(VAlign_Center);

		UTextBlock* Chevron = MakeText(WidgetTree, *FString::Printf(TEXT("%sChevron"), *ButtonName.ToString()), TEXT(">"), 22, FLinearColor(0.18f, 0.82f, 1.0f, 0.42f));
		UHorizontalBoxSlot* ChevronSlot = ContentRow->AddChildToHorizontalBox(Chevron);
		ChevronSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		ChevronSlot->SetPadding(FMargin(0.0f, 0.0f, 20.0f, 0.0f));
		ChevronSlot->SetVerticalAlignment(VAlign_Center);

		UVerticalBoxSlot* ButtonSlot = MenuBox->AddChildToVerticalBox(OutButton);
		ButtonSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
		ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
	};

	BuildMenuButton(TEXT("TestLevelButton"), TEXT("TestLevelFill"), TEXT("TestLevelAccentBar"), TEXT("TestLevelLabel"), TEXT("TEST LEVEL"), TestLevelButton, TestLevelFill, TestLevelAccentBar, TestLevelLabel);
	BuildMenuButton(TEXT("SettingsButton"), TEXT("SettingsFill"), TEXT("SettingsAccentBar"), TEXT("SettingsLabel"), TEXT("SETTINGS"), SettingsButton, SettingsFill, SettingsAccentBar, SettingsLabel);
	BuildMenuButton(TEXT("QuitButton"), TEXT("QuitFill"), TEXT("QuitAccentBar"), TEXT("QuitLabel"), TEXT("QUIT"), QuitButton, QuitFill, QuitAccentBar, QuitLabel);

	ComingSoonPill = MakeBorder(WidgetTree, TEXT("ComingSoonPill"), FLinearColor(0.08f, 0.16f, 0.20f, 0.0f));
	AddCanvasChild(Root, ComingSoonPill, FAnchors(0.0f, 0.5f), FMargin(88.0f, 244.0f, 300.0f, 38.0f));
	ComingSoonText = MakeText(WidgetTree, TEXT("ComingSoonText"), TEXT("SETTINGS: COMING SOON"), 16, FLinearColor(0.78f, 0.94f, 1.0f, 0.0f));
	ComingSoonPill->SetContent(ComingSoonText);

	UTextBlock* VersionText = MakeText(WidgetTree, TEXT("VersionText"), TEXT("PROTOTYPE v0.1.0"), 15, FLinearColor(0.52f, 0.64f, 0.72f, 0.78f));
	VersionText->SetJustification(ETextJustify::Right);
	AddCanvasChild(Root, VersionText, FAnchors(1.0f, 1.0f), FMargin(-36.0f, -24.0f, 260.0f, 32.0f), FVector2D(1.0f, 1.0f));

	UTextBlock* PreviewLabel = MakeText(WidgetTree, TEXT("PreviewLabel"), TEXT("T E S T   C O U R S E   P R E V I E W"), 16, FLinearColor(0.45f, 0.78f, 0.95f, 0.68f));
	AddCanvasChild(Root, PreviewLabel, FAnchors(1.0f, 0.0f), FMargin(-540.0f, 90.0f, 430.0f, 30.0f));

	const TArray<FVector4> BlockSpecs = {
		FVector4(-510.0f, 410.0f, 360.0f, 34.0f),
		FVector4(-410.0f, 344.0f, 520.0f, 32.0f),
		FVector4(-338.0f, 282.0f, 420.0f, 30.0f),
		FVector4(-248.0f, 214.0f, 510.0f, 28.0f),
		FVector4(-174.0f, 154.0f, 300.0f, 26.0f)
	};

	for (int32 BlockIndex = 0; BlockIndex < BlockSpecs.Num(); ++BlockIndex)
	{
		const FVector4 Spec = BlockSpecs[BlockIndex];
		UBorder* Block = MakeBorder(WidgetTree, *FString::Printf(TEXT("PreviewBlock_%02d"), BlockIndex), FLinearColor(0.22f, 0.28f, 0.32f, 0.56f));
		Block->SetRenderTransformAngle(BlockIndex % 2 == 0 ? -8.0f : 8.0f);
		AddCanvasChild(Root, Block, FAnchors(1.0f, 0.5f), FMargin(Spec.X, Spec.Y - 430.0f, Spec.Z, Spec.W));
		PreviewBlocks.Add(Block);
	}

	UBorder* PreviewGlow = MakeBorder(WidgetTree, TEXT("PreviewGlow"), FLinearColor(0.08f, 0.42f, 0.56f, 0.16f));
	PreviewGlow->SetRenderTransformAngle(-8.0f);
	AddCanvasChild(Root, PreviewGlow, FAnchors(1.0f, 0.5f), FMargin(-536.0f, 164.0f, 560.0f, 4.0f));

	if (TestLevelButton)
	{
		TestLevelButton->OnClicked.AddDynamic(this, &UParkourMainMenuWidget::HandleTestLevelClicked);
		TestLevelButton->OnHovered.AddDynamic(this, &UParkourMainMenuWidget::HandleTestLevelHovered);
		TestLevelButton->OnUnhovered.AddDynamic(this, &UParkourMainMenuWidget::HandleTestLevelUnhovered);
		TestLevelButton->OnPressed.AddDynamic(this, &UParkourMainMenuWidget::HandleTestLevelPressed);
	}
	if (SettingsButton)
	{
		SettingsButton->OnClicked.AddDynamic(this, &UParkourMainMenuWidget::HandleSettingsClicked);
		SettingsButton->OnHovered.AddDynamic(this, &UParkourMainMenuWidget::HandleSettingsHovered);
		SettingsButton->OnUnhovered.AddDynamic(this, &UParkourMainMenuWidget::HandleSettingsUnhovered);
		SettingsButton->OnPressed.AddDynamic(this, &UParkourMainMenuWidget::HandleSettingsPressed);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.AddDynamic(this, &UParkourMainMenuWidget::HandleQuitClicked);
		QuitButton->OnHovered.AddDynamic(this, &UParkourMainMenuWidget::HandleQuitHovered);
		QuitButton->OnUnhovered.AddDynamic(this, &UParkourMainMenuWidget::HandleQuitUnhovered);
		QuitButton->OnPressed.AddDynamic(this, &UParkourMainMenuWidget::HandleQuitPressed);
	}

	return Root->TakeWidget();
}

void UParkourMainMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	MenuTime += InDeltaTime;
	ComingSoonTimer = FMath::Max(ComingSoonTimer - InDeltaTime, 0.0f);

	for (int32 LineIndex = 0; LineIndex < SpeedLines.Num(); ++LineIndex)
	{
		if (UBorder* Line = SpeedLines[LineIndex])
		{
			const float Phase = MenuTime * (18.0f + LineIndex * 1.7f) + LineIndex * 47.0f;
			const float OffsetX = FMath::Fmod(Phase, 240.0f) - 120.0f;
			Line->SetRenderTranslation(FVector2D(OffsetX, FMath::Sin(MenuTime * 0.7f + LineIndex) * 3.0f));
		}
	}

	for (int32 BlockIndex = 0; BlockIndex < PreviewBlocks.Num(); ++BlockIndex)
	{
		if (UBorder* Block = PreviewBlocks[BlockIndex])
		{
			Block->SetRenderTranslation(FVector2D(FMath::Sin(MenuTime * 0.48f + BlockIndex) * 8.0f, FMath::Cos(MenuTime * 0.36f + BlockIndex * 0.7f) * 4.0f));
		}
	}

	UpdateButtonVisual(TestLevelButton, TestLevelAccentBar, TestLevelFill, TestLevelLabel, TestLevelState, InDeltaTime);
	UpdateButtonVisual(SettingsButton, SettingsAccentBar, SettingsFill, SettingsLabel, SettingsState, InDeltaTime);
	UpdateButtonVisual(QuitButton, QuitAccentBar, QuitFill, QuitLabel, QuitState, InDeltaTime);

	const float ComingSoonAlpha = ComingSoonTimer > 0.0f ? FMath::Clamp(ComingSoonTimer * 3.0f, 0.0f, 1.0f) : 0.0f;
	if (ComingSoonPill)
	{
		ComingSoonPill->SetBrushColor(FLinearColor(0.08f, 0.16f, 0.20f, ComingSoonAlpha * 0.86f));
	}
	if (ComingSoonText)
	{
		ComingSoonText->SetColorAndOpacity(FSlateColor(FLinearColor(0.78f, 0.94f, 1.0f, ComingSoonAlpha)));
	}
}

void UParkourMainMenuWidget::HandleTestLevelClicked()
{
	PlayClickSound();
	if (AParkourPlayerController* Controller = Cast<AParkourPlayerController>(GetOwningPlayer()))
	{
		Controller->StartTestLevel();
	}
}

void UParkourMainMenuWidget::HandleSettingsClicked()
{
	PlayClickSound();
	ShowComingSoon();
}

void UParkourMainMenuWidget::HandleQuitClicked()
{
	PlayClickSound();
	if (AParkourPlayerController* Controller = Cast<AParkourPlayerController>(GetOwningPlayer()))
	{
		Controller->QuitGame();
	}
}

void UParkourMainMenuWidget::HandleTestLevelHovered()
{
	SetButtonHovered(TestLevelState, TestLevelAccentBar, TestLevelFill, TestLevelLabel, true);
}

void UParkourMainMenuWidget::HandleSettingsHovered()
{
	SetButtonHovered(SettingsState, SettingsAccentBar, SettingsFill, SettingsLabel, true);
}

void UParkourMainMenuWidget::HandleQuitHovered()
{
	SetButtonHovered(QuitState, QuitAccentBar, QuitFill, QuitLabel, true);
}

void UParkourMainMenuWidget::HandleTestLevelUnhovered()
{
	SetButtonHovered(TestLevelState, TestLevelAccentBar, TestLevelFill, TestLevelLabel, false);
}

void UParkourMainMenuWidget::HandleSettingsUnhovered()
{
	SetButtonHovered(SettingsState, SettingsAccentBar, SettingsFill, SettingsLabel, false);
}

void UParkourMainMenuWidget::HandleQuitUnhovered()
{
	SetButtonHovered(QuitState, QuitAccentBar, QuitFill, QuitLabel, false);
}

void UParkourMainMenuWidget::HandleTestLevelPressed()
{
	PressButton(TestLevelState);
}

void UParkourMainMenuWidget::HandleSettingsPressed()
{
	PressButton(SettingsState);
}

void UParkourMainMenuWidget::HandleQuitPressed()
{
	PressButton(QuitState);
}

void UParkourMainMenuWidget::SetButtonHovered(FMainMenuButtonAnimState& State, UBorder* AccentBar, UBorder* Fill, UTextBlock* Label, bool bHovered)
{
	State.bHovered = bHovered;
	State.TargetScale = bHovered ? 1.035f : 1.0f;
	if (bHovered)
	{
		PlayHoverSound();
	}

	if (AccentBar)
	{
		AccentBar->SetBrushColor(FLinearColor(0.18f, 0.82f, 1.0f, bHovered ? 1.0f : 0.0f));
	}
	if (Fill)
	{
		Fill->SetBrushColor(bHovered ? ButtonHoverColor : ButtonIdleColor);
	}
	if (Label)
	{
		Label->SetColorAndOpacity(FSlateColor(bHovered ? FLinearColor::White : TextPrimary));
	}
}

void UParkourMainMenuWidget::PressButton(FMainMenuButtonAnimState& State)
{
	State.PressedTimer = 0.11f;
	State.CurrentScale = FMath::Min(State.CurrentScale, 0.985f);
}

void UParkourMainMenuWidget::UpdateButtonVisual(UButton* Button, UBorder* AccentBar, UBorder* Fill, UTextBlock* Label, FMainMenuButtonAnimState& State, float DeltaTime)
{
	if (!Button)
	{
		return;
	}

	State.PressedTimer = FMath::Max(State.PressedTimer - DeltaTime, 0.0f);
	const float TargetScale = State.PressedTimer > 0.0f ? 0.985f : State.TargetScale;
	State.CurrentScale = FMath::FInterpTo(State.CurrentScale, TargetScale, DeltaTime, 16.0f);

	Button->SetRenderScale(FVector2D(State.CurrentScale, State.CurrentScale));
	Button->SetRenderTranslation(FVector2D(State.bHovered ? 8.0f : 0.0f, 0.0f));

	const float AccentAlpha = State.bHovered ? 1.0f : 0.0f;
	if (AccentBar)
	{
		AccentBar->SetBrushColor(FLinearColor(0.18f, 0.82f, 1.0f, AccentAlpha));
	}
	if (Fill)
	{
		Fill->SetBrushColor(State.bHovered ? ButtonHoverColor : ButtonIdleColor);
	}
	if (Label)
	{
		Label->SetColorAndOpacity(FSlateColor(State.bHovered ? FLinearColor::White : TextPrimary));
	}
}

void UParkourMainMenuWidget::ShowComingSoon()
{
	ComingSoonTimer = 1.8f;
}

void UParkourMainMenuWidget::PlayHoverSound() const
{
	if (HoverSound && GetWorld())
	{
		UGameplayStatics::PlaySound2D(GetWorld(), HoverSound, 0.35f);
	}
}

void UParkourMainMenuWidget::PlayClickSound() const
{
	if (ClickSound && GetWorld())
	{
		UGameplayStatics::PlaySound2D(GetWorld(), ClickSound, 0.55f);
	}
}
