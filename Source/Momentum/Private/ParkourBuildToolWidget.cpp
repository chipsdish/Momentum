#include "ParkourBuildToolWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Kismet/GameplayStatics.h"
#include "ParkourBuildManager.h"
#include "ParkourBuildPiece.h"
#include "Styling/CoreStyle.h"

TSharedRef<SWidget> UParkourBuildToolWidget::RebuildWidget()
{
	const TSharedRef<SWidget> FallbackWidget = Super::RebuildWidget();

	if (!WidgetTree)
	{
		return FallbackWidget;
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("BuildToolRoot"));
	WidgetTree->RootWidget = Root;

	UVerticalBox* Panel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BuildPanel"));
	UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Panel);
	PanelSlot->SetAnchors(FAnchors(1.0f, 0.0f));
	PanelSlot->SetAlignment(FVector2D(1.0f, 0.0f));
	PanelSlot->SetPosition(FVector2D(-24.0f, 24.0f));
	PanelSlot->SetSize(FVector2D(360.0f, 840.0f));

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BuildTitle"));
	Title->SetText(FText::FromString(TEXT("搭建工具")));
	Title->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 32));
	Panel->AddChildToVerticalBox(Title);

	SelectionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SelectionText"));
	SelectionText->SetText(FText::FromString(TEXT("选中: 无")));
	SelectionText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 18));
	Panel->AddChildToVerticalBox(SelectionText);

	UTextBlock* HelpText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BuildHelpText"));
	HelpText->SetText(FText::FromString(TEXT("左键选择 / 拖 Gizmo 改位置\n右键按住环视, WASD/QE 移动相机\nDelete/退格删除, Cmd/Ctrl+C 复制")));
	HelpText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 15));
	Panel->AddChildToVerticalBox(HelpText);

	AddButton(Panel, TEXT("添加平台"))->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::AddPlatform);
	AddButton(Panel, TEXT("添加普通斜坡"))->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::AddRamp);
	AddButton(Panel, TEXT("添加跳台"))->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::AddJumpRamp);
	AddButton(Panel, TEXT("添加真实加速坡"))->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::AddAccelerationRamp);
	AddButton(Panel, TEXT("添加空中平台"))->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::AddAirPlatform);
	AddButton(Panel, TEXT("添加墙边平台"))->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::AddWallPlatform);
	AddButton(Panel, TEXT("添加终点线"))->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::AddFinishGate);
	AddButton(Panel, TEXT("添加 BoostPad"))->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::AddBoostPad);
	AddButton(Panel, TEXT("坡度 -5"))->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::DecreaseSelectedSlope);
	AddButton(Panel, TEXT("坡度 +5"))->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::IncreaseSelectedSlope);
	AddButton(Panel, TEXT("长度 -100"))->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::DecreaseSelectedLength);
	AddButton(Panel, TEXT("长度 +100"))->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::IncreaseSelectedLength);
	AddButton(Panel, TEXT("宽度 -100"))->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::DecreaseSelectedWidth);
	AddButton(Panel, TEXT("宽度 +100"))->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::IncreaseSelectedWidth);
	AddButton(Panel, TEXT("高度 -50"))->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::DecreaseSelectedHeight);
	AddButton(Panel, TEXT("高度 +50"))->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::IncreaseSelectedHeight);
	AddButton(Panel, TEXT("复制选中"))->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::DuplicateSelected);
	AddButton(Panel, TEXT("删除选中"))->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::DeleteSelected);
	AddButton(Panel, TEXT("保存"))->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::SaveDefaultLayout);
	AddButton(Panel, TEXT("加载"))->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::LoadDefaultLayout);
	AddButton(Panel, TEXT("重置测试块"))->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::ResetRuntimeLayout);

	return Root->TakeWidget();
}

void UParkourBuildToolWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!SelectionText)
	{
		return;
	}

	const AParkourBuildManager* BuildManager = FindBuildManager();
	const AParkourBuildPiece* SelectedPiece = BuildManager ? BuildManager->GetSelectedPiece() : nullptr;
	if (SelectedPiece)
	{
		const FVector Dimensions = SelectedPiece->GetDimensions();
		SelectionText->SetText(FText::FromString(FString::Printf(TEXT("选中: %s\n尺寸: %.0f / %.0f / %.0f\n坡度: %.0f"), *SelectedPiece->GetName(), Dimensions.X, Dimensions.Y, Dimensions.Z, SelectedPiece->GetSlopeAngle())));
	}
	else
	{
		SelectionText->SetText(FText::FromString(TEXT("选中: 无")));
	}
}

void UParkourBuildToolWidget::AddPlatform()
{
	if (AParkourBuildManager* BuildManager = FindBuildManager())
	{
		BuildManager->AddDefaultPiece(EParkourBuildPieceType::Platform);
	}
}

void UParkourBuildToolWidget::AddRamp()
{
	if (AParkourBuildManager* BuildManager = FindBuildManager())
	{
		BuildManager->AddDefaultPiece(EParkourBuildPieceType::Ramp);
	}
}

void UParkourBuildToolWidget::AddJumpRamp()
{
	if (AParkourBuildManager* BuildManager = FindBuildManager())
	{
		BuildManager->AddDefaultPiece(EParkourBuildPieceType::JumpRamp);
	}
}

void UParkourBuildToolWidget::AddAccelerationRamp()
{
	if (AParkourBuildManager* BuildManager = FindBuildManager())
	{
		BuildManager->AddDefaultPiece(EParkourBuildPieceType::AccelerationRamp);
	}
}

void UParkourBuildToolWidget::AddAirPlatform()
{
	if (AParkourBuildManager* BuildManager = FindBuildManager())
	{
		BuildManager->AddDefaultPiece(EParkourBuildPieceType::AirPlatform);
	}
}

void UParkourBuildToolWidget::AddWallPlatform()
{
	if (AParkourBuildManager* BuildManager = FindBuildManager())
	{
		BuildManager->AddDefaultPiece(EParkourBuildPieceType::WallPlatform);
	}
}

void UParkourBuildToolWidget::AddFinishGate()
{
	if (AParkourBuildManager* BuildManager = FindBuildManager())
	{
		BuildManager->AddDefaultPiece(EParkourBuildPieceType::FinishGate);
	}
}

void UParkourBuildToolWidget::AddBoostPad()
{
	if (AParkourBuildManager* BuildManager = FindBuildManager())
	{
		BuildManager->AddDefaultPiece(EParkourBuildPieceType::BoostPad);
	}
}

void UParkourBuildToolWidget::DuplicateSelected()
{
	if (AParkourBuildManager* BuildManager = FindBuildManager())
	{
		BuildManager->DuplicateSelectedPiece();
	}
}

void UParkourBuildToolWidget::DecreaseSelectedSlope()
{
	if (AParkourBuildManager* BuildManager = FindBuildManager())
	{
		BuildManager->AdjustSelectedSlopeAngle(-5.0f);
	}
}

void UParkourBuildToolWidget::IncreaseSelectedSlope()
{
	if (AParkourBuildManager* BuildManager = FindBuildManager())
	{
		BuildManager->AdjustSelectedSlopeAngle(5.0f);
	}
}

void UParkourBuildToolWidget::IncreaseSelectedLength()
{
	if (AParkourBuildManager* BuildManager = FindBuildManager())
	{
		BuildManager->AdjustSelectedDimensions(FVector(100.0f, 0.0f, 0.0f));
	}
}

void UParkourBuildToolWidget::DecreaseSelectedLength()
{
	if (AParkourBuildManager* BuildManager = FindBuildManager())
	{
		BuildManager->AdjustSelectedDimensions(FVector(-100.0f, 0.0f, 0.0f));
	}
}

void UParkourBuildToolWidget::IncreaseSelectedWidth()
{
	if (AParkourBuildManager* BuildManager = FindBuildManager())
	{
		BuildManager->AdjustSelectedDimensions(FVector(0.0f, 100.0f, 0.0f));
	}
}

void UParkourBuildToolWidget::DecreaseSelectedWidth()
{
	if (AParkourBuildManager* BuildManager = FindBuildManager())
	{
		BuildManager->AdjustSelectedDimensions(FVector(0.0f, -100.0f, 0.0f));
	}
}

void UParkourBuildToolWidget::IncreaseSelectedHeight()
{
	if (AParkourBuildManager* BuildManager = FindBuildManager())
	{
		BuildManager->AdjustSelectedDimensions(FVector(0.0f, 0.0f, 50.0f));
	}
}

void UParkourBuildToolWidget::DecreaseSelectedHeight()
{
	if (AParkourBuildManager* BuildManager = FindBuildManager())
	{
		BuildManager->AdjustSelectedDimensions(FVector(0.0f, 0.0f, -50.0f));
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

UButton* UParkourBuildToolWidget::AddButton(UVerticalBox* Parent, const FString& Label)
{
	if (!Parent || !WidgetTree)
	{
		return nullptr;
	}

	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Text->SetText(FText::FromString(Label));
	Text->SetJustification(ETextJustify::Center);
	Text->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 18));
	Button->AddChild(Text);
	Parent->AddChildToVerticalBox(Button);
	return Button;
}
