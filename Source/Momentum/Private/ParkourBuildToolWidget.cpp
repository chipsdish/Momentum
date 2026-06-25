#include "ParkourBuildToolWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/DefaultValueHelper.h"
#include "ParkourBuildManager.h"
#include "ParkourBuildPiece.h"
#include "Styling/CoreStyle.h"

namespace
{
	const FLinearColor PanelColor(0.015f, 0.020f, 0.028f, 0.88f);
	const FLinearColor ToolbarColor(0.012f, 0.016f, 0.022f, 0.92f);
	const FLinearColor AccentColor(0.16f, 0.75f, 1.0f, 1.0f);
	const FLinearColor TextColor(0.86f, 0.94f, 1.0f, 1.0f);
	const FLinearColor MutedTextColor(0.48f, 0.62f, 0.72f, 1.0f);

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

	UTextBlock* MakeText(UWidgetTree* WidgetTree, const FString& Text, int32 FontSize, const FLinearColor& Color = TextColor)
	{
		UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		TextBlock->SetText(FText::FromString(Text));
		TextBlock->SetColorAndOpacity(FSlateColor(Color));
		TextBlock->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), FontSize));
		return TextBlock;
	}

	float ClampSliderValue(USlider* Slider, float Value)
	{
		if (!Slider)
		{
			return Value;
		}

		return FMath::Clamp(Value, Slider->GetMinValue(), Slider->GetMaxValue());
	}
}

TSharedRef<SWidget> UParkourBuildToolWidget::RebuildWidget()
{
	const TSharedRef<SWidget> FallbackWidget = Super::RebuildWidget();

	if (!WidgetTree)
	{
		return FallbackWidget;
	}

	LastPreviewPiece = nullptr;
	bSyncingControls = false;

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("EditModeRoot"));
	WidgetTree->RootWidget = Root;

	UBorder* ToolbarBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BuildToolbarBackground"));
	ToolbarBackground->SetBrushColor(ToolbarColor);
	AddCanvasChild(Root, ToolbarBackground, FAnchors(0.0f, 1.0f, 1.0f, 1.0f), FMargin(0.0f, -112.0f, 0.0f, 112.0f));

	UHorizontalBox* Toolbar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BuildToolbar"));
	ToolbarBackground->SetContent(Toolbar);

	UTextBlock* ToolbarTitle = MakeText(WidgetTree, TEXT("BUILD"), 18, AccentColor);
	UHorizontalBoxSlot* ToolbarTitleSlot = Toolbar->AddChildToHorizontalBox(ToolbarTitle);
	ToolbarTitleSlot->SetPadding(FMargin(28.0f, 0.0f, 20.0f, 0.0f));
	ToolbarTitleSlot->SetVerticalAlignment(VAlign_Center);

	UButton* GroundButton = MakeTextButton(TEXT("Ground / 地面"));
	GroundButton->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::SelectGround);
	AddToolbarButton(Toolbar, GroundButton);

	UButton* RampButton = MakeTextButton(TEXT("Ramp / 斜坡"));
	RampButton->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::SelectRamp);
	AddToolbarButton(Toolbar, RampButton);

	UButton* ConfirmButton = MakeTextButton(TEXT("Confirm / 放置"));
	ConfirmButton->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::ConfirmPlacement);
	AddToolbarButton(Toolbar, ConfirmButton);

	UButton* CancelButton = MakeTextButton(TEXT("Cancel / 取消"));
	CancelButton->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::CancelPlacement);
	AddToolbarButton(Toolbar, CancelButton);

	SelectionText = MakeText(WidgetTree, TEXT("编辑模式: 选择 Ground 或 Ramp 后会先生成预览虚影"), 15, MutedTextColor);
	UHorizontalBoxSlot* SelectionSlot = Toolbar->AddChildToHorizontalBox(SelectionText);
	SelectionSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	SelectionSlot->SetPadding(FMargin(22.0f, 0.0f, 24.0f, 0.0f));
	SelectionSlot->SetVerticalAlignment(VAlign_Center);

	ParameterPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BuildParametersPanel"));
	ParameterPanel->SetBrushColor(PanelColor);
	AddCanvasChild(Root, ParameterPanel, FAnchors(0.0f, 0.5f), FMargin(24.0f, -330.0f, 360.0f, 560.0f));

	UVerticalBox* ParameterBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ParameterBox"));
	ParameterPanel->SetContent(ParameterBox);

	ParameterTitleText = MakeText(WidgetTree, TEXT("未选择建造物"), 24, TextColor);
	UVerticalBoxSlot* TitleSlot = ParameterBox->AddChildToVerticalBox(ParameterTitleText);
	TitleSlot->SetPadding(FMargin(18.0f, 18.0f, 18.0f, 10.0f));

	HelpText = MakeText(WidgetTree, TEXT("鼠标指向地面或已有物体\n参数改变会实时更新虚影\nEsc 取消预览 / 再按退出编辑模式"), 14, MutedTextColor);
	UVerticalBoxSlot* HelpSlot = ParameterBox->AddChildToVerticalBox(HelpText);
	HelpSlot->SetPadding(FMargin(18.0f, 0.0f, 18.0f, 16.0f));

	AddParameterRow(ParameterBox, TEXT("Length / 长度"), LengthSlider, LengthTextBox, 100.0f, 3000.0f);
	AddParameterRow(ParameterBox, TEXT("Width / 宽度"), WidthSlider, WidthTextBox, 100.0f, 2000.0f);
	AddParameterRow(ParameterBox, TEXT("Height / 高度"), HeightSlider, HeightTextBox, 20.0f, 1000.0f);
	AddParameterRow(ParameterBox, TEXT("Slope Angle / 斜度"), SlopeSlider, SlopeTextBox, 0.0f, 75.0f);
	AddParameterRow(ParameterBox, TEXT("Rotation / 旋转"), RotationSlider, RotationTextBox, -180.0f, 180.0f);
	AddParameterRow(ParameterBox, TEXT("Grid Snap / 网格"), GridSnapSlider, GridSnapTextBox, 10.0f, 400.0f);

	LengthSlider->OnValueChanged.AddDynamic(this, &UParkourBuildToolWidget::OnLengthSliderChanged);
	WidthSlider->OnValueChanged.AddDynamic(this, &UParkourBuildToolWidget::OnWidthSliderChanged);
	HeightSlider->OnValueChanged.AddDynamic(this, &UParkourBuildToolWidget::OnHeightSliderChanged);
	SlopeSlider->OnValueChanged.AddDynamic(this, &UParkourBuildToolWidget::OnSlopeSliderChanged);
	RotationSlider->OnValueChanged.AddDynamic(this, &UParkourBuildToolWidget::OnRotationSliderChanged);
	GridSnapSlider->OnValueChanged.AddDynamic(this, &UParkourBuildToolWidget::OnGridSnapSliderChanged);

	LengthTextBox->OnTextCommitted.AddDynamic(this, &UParkourBuildToolWidget::OnLengthTextCommitted);
	WidthTextBox->OnTextCommitted.AddDynamic(this, &UParkourBuildToolWidget::OnWidthTextCommitted);
	HeightTextBox->OnTextCommitted.AddDynamic(this, &UParkourBuildToolWidget::OnHeightTextCommitted);
	SlopeTextBox->OnTextCommitted.AddDynamic(this, &UParkourBuildToolWidget::OnSlopeTextCommitted);
	RotationTextBox->OnTextCommitted.AddDynamic(this, &UParkourBuildToolWidget::OnRotationTextCommitted);
	GridSnapTextBox->OnTextCommitted.AddDynamic(this, &UParkourBuildToolWidget::OnGridSnapTextCommitted);

	UHorizontalBox* PanelButtons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ParameterButtons"));
	UVerticalBoxSlot* PanelButtonSlot = ParameterBox->AddChildToVerticalBox(PanelButtons);
	PanelButtonSlot->SetPadding(FMargin(18.0f, 14.0f, 18.0f, 18.0f));

	UButton* PanelConfirmButton = MakeTextButton(TEXT("Confirm"));
	PanelConfirmButton->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::ConfirmPlacement);
	AddToolbarButton(PanelButtons, PanelConfirmButton);

	UButton* PanelCancelButton = MakeTextButton(TEXT("Cancel"));
	PanelCancelButton->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::CancelPlacement);
	AddToolbarButton(PanelButtons, PanelCancelButton);

	SetParameterPanelVisible(false);
	return Root->TakeWidget();
}

void UParkourBuildToolWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	AParkourBuildManager* BuildManager = FindBuildManager();
	AParkourBuildPiece* PreviewPiece = BuildManager ? BuildManager->GetPreviewPiece() : nullptr;

	SetParameterPanelVisible(PreviewPiece != nullptr);
	if (PreviewPiece && PreviewPiece != LastPreviewPiece)
	{
		LastPreviewPiece = PreviewPiece;
		SyncControlsFromPreview(true);
	}
	else if (!PreviewPiece)
	{
		LastPreviewPiece = nullptr;
	}

	if (!SelectionText)
	{
		return;
	}

	if (PreviewPiece)
	{
		const FParkourBuildPieceData PreviewData = BuildManager->GetPreviewData();
		const FString TypeLabel = PreviewData.PieceType == EParkourBuildPieceType::Ramp ? TEXT("Ramp / 斜坡") : TEXT("Ground / 地面");
		SelectionText->SetText(FText::FromString(FString::Printf(TEXT("预览: %s | Confirm 后才会生成正式物体"), *TypeLabel)));
		if (ParameterTitleText)
		{
			ParameterTitleText->SetText(FText::FromString(TypeLabel));
		}
	}
	else if (const AParkourBuildPiece* SelectedPiece = BuildManager ? BuildManager->GetSelectedPiece() : nullptr)
	{
		const FVector Dimensions = SelectedPiece->GetDimensions();
		SelectionText->SetText(FText::FromString(FString::Printf(TEXT("选中: %s | %.0f / %.0f / %.0f | 坡度 %.0f"), *SelectedPiece->GetName(), Dimensions.X, Dimensions.Y, Dimensions.Z, SelectedPiece->GetSlopeAngle())));
	}
	else
	{
		SelectionText->SetText(FText::FromString(TEXT("编辑模式: 点击 Ground 或 Ramp 生成半透明预览虚影")));
	}
}

void UParkourBuildToolWidget::SelectGround()
{
	if (AParkourBuildManager* BuildManager = FindBuildManager())
	{
		BuildManager->BeginPreviewPlacement(EParkourBuildPieceType::Platform);
		SyncControlsFromPreview(true);
	}
}

void UParkourBuildToolWidget::SelectRamp()
{
	if (AParkourBuildManager* BuildManager = FindBuildManager())
	{
		BuildManager->BeginPreviewPlacement(EParkourBuildPieceType::Ramp);
		SyncControlsFromPreview(true);
	}
}

void UParkourBuildToolWidget::ConfirmPlacement()
{
	if (AParkourBuildManager* BuildManager = FindBuildManager())
	{
		BuildManager->ConfirmPreviewPlacement();
	}
}

void UParkourBuildToolWidget::CancelPlacement()
{
	if (AParkourBuildManager* BuildManager = FindBuildManager())
	{
		BuildManager->CancelPreviewPlacement();
	}
}

void UParkourBuildToolWidget::OnLengthSliderChanged(float Value)
{
	if (bSyncingControls)
	{
		return;
	}

	ApplyTextBoxValue(LengthTextBox, Value);
	ApplyPreviewDimensions();
	SyncControlsFromPreview(true);
}

void UParkourBuildToolWidget::OnWidthSliderChanged(float Value)
{
	if (bSyncingControls)
	{
		return;
	}

	ApplyTextBoxValue(WidthTextBox, Value);
	ApplyPreviewDimensions();
	SyncControlsFromPreview(true);
}

void UParkourBuildToolWidget::OnHeightSliderChanged(float Value)
{
	if (bSyncingControls)
	{
		return;
	}

	ApplyTextBoxValue(HeightTextBox, Value);
	ApplyPreviewDimensions();
	SyncControlsFromPreview(true);
}

void UParkourBuildToolWidget::OnSlopeSliderChanged(float Value)
{
	if (bSyncingControls)
	{
		return;
	}

	ApplyTextBoxValue(SlopeTextBox, Value);
	if (AParkourBuildManager* BuildManager = FindBuildManager())
	{
		BuildManager->SetPreviewSlopeAngle(Value);
	}
	SyncControlsFromPreview(true);
}

void UParkourBuildToolWidget::OnRotationSliderChanged(float Value)
{
	if (bSyncingControls)
	{
		return;
	}

	ApplyTextBoxValue(RotationTextBox, Value);
	if (AParkourBuildManager* BuildManager = FindBuildManager())
	{
		BuildManager->SetPreviewRotationYaw(Value);
	}
}

void UParkourBuildToolWidget::OnGridSnapSliderChanged(float Value)
{
	if (bSyncingControls)
	{
		return;
	}

	ApplyTextBoxValue(GridSnapTextBox, Value);
	if (AParkourBuildManager* BuildManager = FindBuildManager())
	{
		BuildManager->SetGridSnapSize(Value);
	}
}

void UParkourBuildToolWidget::OnLengthTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	float Value = 0.0f;
	if (TryParseFloat(Text, Value) && LengthSlider)
	{
		LengthSlider->SetValue(ClampSliderValue(LengthSlider, Value));
		OnLengthSliderChanged(LengthSlider->GetValue());
	}
}

void UParkourBuildToolWidget::OnWidthTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	float Value = 0.0f;
	if (TryParseFloat(Text, Value) && WidthSlider)
	{
		WidthSlider->SetValue(ClampSliderValue(WidthSlider, Value));
		OnWidthSliderChanged(WidthSlider->GetValue());
	}
}

void UParkourBuildToolWidget::OnHeightTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	float Value = 0.0f;
	if (TryParseFloat(Text, Value) && HeightSlider)
	{
		HeightSlider->SetValue(ClampSliderValue(HeightSlider, Value));
		OnHeightSliderChanged(HeightSlider->GetValue());
	}
}

void UParkourBuildToolWidget::OnSlopeTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	float Value = 0.0f;
	if (TryParseFloat(Text, Value) && SlopeSlider)
	{
		SlopeSlider->SetValue(ClampSliderValue(SlopeSlider, Value));
		OnSlopeSliderChanged(SlopeSlider->GetValue());
	}
}

void UParkourBuildToolWidget::OnRotationTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	float Value = 0.0f;
	if (TryParseFloat(Text, Value) && RotationSlider)
	{
		RotationSlider->SetValue(ClampSliderValue(RotationSlider, Value));
		OnRotationSliderChanged(RotationSlider->GetValue());
	}
}

void UParkourBuildToolWidget::OnGridSnapTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	float Value = 0.0f;
	if (TryParseFloat(Text, Value) && GridSnapSlider)
	{
		GridSnapSlider->SetValue(ClampSliderValue(GridSnapSlider, Value));
		OnGridSnapSliderChanged(GridSnapSlider->GetValue());
	}
}

void UParkourBuildToolWidget::DeleteSelected()
{
	if (AParkourBuildManager* BuildManager = FindBuildManager())
	{
		BuildManager->DeleteSelectedPiece();
	}
}

void UParkourBuildToolWidget::SaveDefaultLayout()
{
	if (AParkourBuildManager* BuildManager = FindBuildManager())
	{
		BuildManager->SaveLayout(BuildManager->AutoLoadLayoutName);
	}
}

void UParkourBuildToolWidget::LoadDefaultLayout()
{
	if (AParkourBuildManager* BuildManager = FindBuildManager())
	{
		BuildManager->LoadLayout(BuildManager->AutoLoadLayoutName);
	}
}

void UParkourBuildToolWidget::ResetRuntimeLayout()
{
	if (AParkourBuildManager* BuildManager = FindBuildManager())
	{
		BuildManager->ResetDefaultRuntimeLayout();
	}
}

AParkourBuildManager* UParkourBuildToolWidget::FindBuildManager() const
{
	return Cast<AParkourBuildManager>(UGameplayStatics::GetActorOfClass(this, AParkourBuildManager::StaticClass()));
}

UButton* UParkourBuildToolWidget::MakeTextButton(const FString& Label)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	UTextBlock* Text = MakeText(WidgetTree, Label, 17, TextColor);
	Text->SetJustification(ETextJustify::Center);
	Button->AddChild(Text);
	return Button;
}

void UParkourBuildToolWidget::AddToolbarButton(UHorizontalBox* Parent, UButton* Button)
{
	if (!Parent || !Button)
	{
		return;
	}

	UHorizontalBoxSlot* ButtonSlot = Parent->AddChildToHorizontalBox(Button);
	ButtonSlot->SetPadding(FMargin(8.0f, 18.0f, 8.0f, 18.0f));
	ButtonSlot->SetVerticalAlignment(VAlign_Center);
}

void UParkourBuildToolWidget::AddParameterRow(UVerticalBox* Parent, const FString& Label, TObjectPtr<USlider>& OutSlider, TObjectPtr<UEditableTextBox>& OutTextBox, float MinValue, float MaxValue)
{
	if (!Parent)
	{
		return;
	}

	UTextBlock* LabelText = MakeText(WidgetTree, Label, 14, MutedTextColor);
	UVerticalBoxSlot* LabelSlot = Parent->AddChildToVerticalBox(LabelText);
	LabelSlot->SetPadding(FMargin(18.0f, 5.0f, 18.0f, 2.0f));

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UVerticalBoxSlot* RowSlot = Parent->AddChildToVerticalBox(Row);
	RowSlot->SetPadding(FMargin(18.0f, 0.0f, 18.0f, 8.0f));

	OutSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass());
	OutSlider->SetMinValue(MinValue);
	OutSlider->SetMaxValue(MaxValue);
	OutSlider->SetStepSize(1.0f);
	UHorizontalBoxSlot* SliderSlot = Row->AddChildToHorizontalBox(OutSlider);
	SliderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	SliderSlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
	SliderSlot->SetVerticalAlignment(VAlign_Center);

	OutTextBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
	OutTextBox->SetMinDesiredWidth(82.0f);
	UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(OutTextBox);
	TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	TextSlot->SetVerticalAlignment(VAlign_Center);
}

void UParkourBuildToolWidget::SyncControlsFromPreview(bool bForceTextUpdate)
{
	AParkourBuildManager* BuildManager = FindBuildManager();
	if (!BuildManager || !BuildManager->HasActivePreview())
	{
		return;
	}

	const FParkourBuildPieceData PreviewData = BuildManager->GetPreviewData();
	bSyncingControls = true;
	if (LengthSlider) { LengthSlider->SetValue(ClampSliderValue(LengthSlider, PreviewData.Dimensions.X)); }
	if (WidthSlider) { WidthSlider->SetValue(ClampSliderValue(WidthSlider, PreviewData.Dimensions.Y)); }
	if (HeightSlider) { HeightSlider->SetValue(ClampSliderValue(HeightSlider, PreviewData.Dimensions.Z)); }
	if (SlopeSlider) { SlopeSlider->SetValue(ClampSliderValue(SlopeSlider, PreviewData.SlopeAngle)); }
	if (RotationSlider) { RotationSlider->SetValue(ClampSliderValue(RotationSlider, PreviewData.Transform.Rotator().Yaw)); }
	if (GridSnapSlider) { GridSnapSlider->SetValue(ClampSliderValue(GridSnapSlider, BuildManager->GetGridSnapSize())); }
	bSyncingControls = false;

	if (bForceTextUpdate)
	{
		ApplyTextBoxValue(LengthTextBox, PreviewData.Dimensions.X);
		ApplyTextBoxValue(WidthTextBox, PreviewData.Dimensions.Y);
		ApplyTextBoxValue(HeightTextBox, PreviewData.Dimensions.Z);
		ApplyTextBoxValue(SlopeTextBox, PreviewData.SlopeAngle);
		ApplyTextBoxValue(RotationTextBox, PreviewData.Transform.Rotator().Yaw);
		ApplyTextBoxValue(GridSnapTextBox, BuildManager->GetGridSnapSize());
	}
}

void UParkourBuildToolWidget::ApplyPreviewDimensions()
{
	if (AParkourBuildManager* BuildManager = FindBuildManager())
	{
		BuildManager->SetPreviewDimensions(FVector(
			GetSliderValue(LengthSlider, 400.0f),
			GetSliderValue(WidthSlider, 400.0f),
			GetSliderValue(HeightSlider, 80.0f)));
	}
}

void UParkourBuildToolWidget::ApplyTextBoxValue(UEditableTextBox* TextBox, float Value) const
{
	if (TextBox)
	{
		TextBox->SetText(FText::FromString(FString::Printf(TEXT("%.0f"), Value)));
	}
}

bool UParkourBuildToolWidget::TryParseFloat(const FText& Text, float& OutValue) const
{
	const FString StringValue = Text.ToString().TrimStartAndEnd();
	return FDefaultValueHelper::ParseFloat(StringValue, OutValue);
}

float UParkourBuildToolWidget::GetSliderValue(USlider* Slider, float Fallback) const
{
	return Slider ? Slider->GetValue() : Fallback;
}

void UParkourBuildToolWidget::SetParameterPanelVisible(bool bVisible)
{
	if (ParameterPanel)
	{
		ParameterPanel->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}
