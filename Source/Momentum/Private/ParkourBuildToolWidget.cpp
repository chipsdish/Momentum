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

void UParkourBuildToolWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetTree)
	{
		return;
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("BuildToolRoot"));
	WidgetTree->RootWidget = Root;

	UVerticalBox* Panel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BuildPanel"));
	UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Panel);
	PanelSlot->SetAnchors(FAnchors(1.0f, 0.0f));
	PanelSlot->SetAlignment(FVector2D(1.0f, 0.0f));
	PanelSlot->SetPosition(FVector2D(-24.0f, 24.0f));
	PanelSlot->SetSize(FVector2D(340.0f, 680.0f));

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BuildTitle"));
	Title->SetText(FText::FromString(TEXT("搭建工具")));
	Title->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 32));
	Panel->AddChildToVerticalBox(Title);

	SelectionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SelectionText"));
	SelectionText->SetText(FText::FromString(TEXT("选中: 无")));
	SelectionText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 18));
	Panel->AddChildToVerticalBox(SelectionText);

	UTextBlock* HelpText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BuildHelpText"));
	HelpText->SetText(FText::FromString(TEXT("左键选择 / 拖 Gizmo 改位置\nQ/E 升降相机, Shift 加速\nDelete/退格删除, Cmd/Ctrl+C 复制")));
	HelpText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 15));
	Panel->AddChildToVerticalBox(HelpText);

	AddButton(Panel, TEXT("添加平台"))->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::AddPlatform);
	AddButton(Panel, TEXT("添加普通斜坡"))->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::AddRamp);
	AddButton(Panel, TEXT("添加跳台"))->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::AddJumpRamp);
	AddButton(Panel, TEXT("添加真实加速坡"))->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::AddAccelerationRamp);
	AddButton(Panel, TEXT("添加空中平台"))->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::AddAirPlatform);
	AddButton(Panel, TEXT("添加墙边平台"))->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::AddWallPlatform);
	AddButton(Panel, TEXT("复制选中"))->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::DuplicateSelected);
	AddButton(Panel, TEXT("删除选中"))->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::DeleteSelected);
	AddButton(Panel, TEXT("保存布局 01"))->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::SaveDefaultLayout);
	AddButton(Panel, TEXT("加载布局 01"))->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::LoadDefaultLayout);
	AddButton(Panel, TEXT("重置测试块"))->OnClicked.AddDynamic(this, &UParkourBuildToolWidget::ResetRuntimeLayout);
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
	SelectionText->SetText(FText::FromString(SelectedPiece ? FString::Printf(TEXT("选中: %s"), *SelectedPiece->GetName()) : TEXT("选中: 无")));
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

void UParkourBuildToolWidget::DuplicateSelected()
{
	if (AParkourBuildManager* BuildManager = FindBuildManager())
	{
		BuildManager->DuplicateSelectedPiece();
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
		BuildManager->SaveLayout(TEXT("布局 01"));
	}
}

void UParkourBuildToolWidget::LoadDefaultLayout()
{
	if (AParkourBuildManager* BuildManager = FindBuildManager())
	{
		BuildManager->LoadLayout(TEXT("布局 01"));
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
