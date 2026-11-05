#include "Base/GrimBaseWidget.h"
#include "Base/GrimBaseInteractable.h"
#include "Base/GrimBaseSettings.h"
#include "Items/GrimInventoryComponent.h"
#include "Items/GrimInventoryWidget.h"
#include "Items/GrimItemSettings.h"
#include "UI/GrimUIData.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "GrimBaseWidget"

namespace GrimBaseWidgetLocal // 유니티 빌드에서 이름이 부딪히지 않게
{
	const FLinearColor TitleColor(1.f, 0.85f, 0.45f);
	const FLinearColor BodyColor(0.92f, 0.92f, 0.92f);
	const FLinearColor GoodColor(0.55f, 1.f, 0.55f);
	const FLinearColor PanelColor(0.f, 0.f, 0.f, 0.55f);
}

void UGrimBaseWidget::SetBuilder(UGrimBuilderComponent* InBuilder)
{
	if (UGrimBuilderComponent* Old = Builder.Get())
	{
		Old->OnNotice.Remove(NoticeHandle);
	}
	Builder = InBuilder;
	if (InBuilder)
	{
		NoticeHandle = InBuilder->OnNotice.AddUObject(this, &UGrimBaseWidget::HandleNotice);
	}
}

void UGrimBaseWidget::NativeDestruct()
{
	if (UGrimBuilderComponent* Old = Builder.Get())
	{
		Old->OnNotice.Remove(NoticeHandle);
	}
	Super::NativeDestruct();
}

TSharedRef<SWidget> UGrimBaseWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

void UGrimBaseWidget::BuildTree()
{
	using namespace GrimBaseWidgetLocal;
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Canvas;
	Canvas->SetVisibility(ESlateVisibility::HitTestInvisible);

	// 오른쪽 가운데: 건축 패널
	UBorder* Panel = GrimUIData::MakeBorder(WidgetTree, PanelColor, FMargin(14.f, 10.f));
	UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Panel);
	PanelSlot->SetAnchors(FAnchors(1.f, 0.5f));
	PanelSlot->SetAlignment(FVector2D(1.f, 0.5f));
	PanelSlot->SetPosition(FVector2D(-24.f, 0.f));
	PanelSlot->SetAutoSize(true);
	BuildPanel = Panel;
	BuildPanel->SetVisibility(ESlateVisibility::Collapsed);

	UVerticalBox* PanelBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->SetContent(PanelBox);
	const UGrimBaseSettings* Settings = UGrimBaseSettings::Get();
	PanelBox->AddChildToVerticalBox(GrimUIData::MakeText(WidgetTree,
		FText::Format(LOCTEXT("BuildTitle", "건축  ({0} 닫기)"), Settings->BuildKey.GetDisplayName()), 17, TitleColor, true))->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	PieceListText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 13, BodyColor);
	PanelBox->AddChildToVerticalBox(PieceListText);
	PanelBox->AddChildToVerticalBox(GrimUIData::MakeText(WidgetTree,
		FText::Format(LOCTEXT("BuildKeys", "{0} · {1} 고르기   {2} 돌리기   {3} 놓기   {4} 헐기"),
			Settings->PrevPieceKey.GetDisplayName(), Settings->NextPieceKey.GetDisplayName(), Settings->RotateKey.GetDisplayName(),
			Settings->PlaceKey.GetDisplayName(), Settings->DemolishKey.GetDisplayName()), 12, BodyColor))->SetPadding(FMargin(0.f, 8.f, 0.f, 2.f));
	StatusText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 14, GoodColor, true);
	PanelBox->AddChildToVerticalBox(StatusText);

	// 아래 가운데: 알림 + 안내(가방 안내보다 위)
	UVerticalBox* Bottom = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UCanvasPanelSlot* BottomSlot = Canvas->AddChildToCanvas(Bottom);
	BottomSlot->SetAnchors(FAnchors(0.5f, 1.f));
	BottomSlot->SetAlignment(FVector2D(0.5f, 1.f));
	BottomSlot->SetPosition(FVector2D(0.f, -250.f));
	BottomSlot->SetAutoSize(true);

	NoticeText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 16, TitleColor, true);
	NoticeText->SetJustification(ETextJustify::Center);
	NoticeText->SetVisibility(ESlateVisibility::Collapsed);
	Bottom->AddChildToVerticalBox(NoticeText)->SetHorizontalAlignment(HAlign_Center);

	UBorder* PromptPanel = GrimUIData::MakeBorder(WidgetTree, PanelColor, FMargin(14.f, 6.f));
	PromptText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 15, BodyColor);
	PromptText->SetJustification(ETextJustify::Center);
	PromptPanel->SetContent(PromptText);
	PromptRoot = PromptPanel;
	PromptRoot->SetVisibility(ESlateVisibility::Collapsed);
	Bottom->AddChildToVerticalBox(PromptPanel)->SetHorizontalAlignment(HAlign_Center);
}

void UGrimBaseWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (NoticeRemaining > 0.f)
	{
		NoticeRemaining -= InDeltaTime;
		if (NoticeRemaining <= 0.f && NoticeText)
		{
			NoticeText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	RefreshAccumulator += InDeltaTime;
	if (RefreshAccumulator >= 0.1f)
	{
		RefreshAccumulator = 0.f;
		Refresh();
	}
}

FString UGrimBaseWidget::DescribePieceList(const UGrimBuilderComponent* Builder)
{
	const FGrimBaseDatabase& Database = UGrimBaseSettings::GetDatabase();
	const FGrimItemDatabase& Items = UGrimItemSettings::GetDatabase();
	const UGrimInventoryComponent* Bag = Builder ? Builder->GetBag() : nullptr;
	TArray<FString> Lines;
	for (int32 Index = 0; Index < Database.PieceOrder.Num(); ++Index)
	{
		const FGrimBuildPieceDefinition* Def = Database.FindPiece(Database.PieceOrder[Index]);
		if (!Def)
		{
			continue;
		}
		TArray<FString> Costs;
		for (const FGrimRecipeInput& Input : Def->Cost)
		{
			const FGrimItemDefinition* Item = Items.Find(Input.Id);
			Costs.Add(FString::Printf(TEXT("%s %d/%d"), Item ? *Item->GetDisplayName().ToString() : *Input.Id.ToString(), Bag ? Bag->CountOf(Input.Id) : 0, Input.Count));
		}
		const bool bSelected = Builder && Builder->GetSelectedIndex() == Index;
		Lines.Add(FString::Printf(TEXT("%s %s   %s"), bSelected ? TEXT("▶") : TEXT("  "), *Def->GetDisplayName().ToString(), *FString::Join(Costs, TEXT(" · "))));
	}
	return FString::Join(Lines, TEXT("\n"));
}

void UGrimBaseWidget::Refresh()
{
	using namespace GrimBaseWidgetLocal;
	const UGrimBuilderComponent* BuilderComponent = Builder.Get();
	if (!BuilderComponent || !BuildPanel)
	{
		return;
	}
	const bool bBuildMode = BuilderComponent->IsBuildMode();
	BuildPanel->SetVisibility(bBuildMode ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (bBuildMode)
	{
		PieceListText->SetText(FText::FromString(DescribePieceList(BuilderComponent)));
		const FGrimBuildPreview& Preview = BuilderComponent->GetPreview();
		const bool bOk = Preview.Result == EGrimBuildResult::Ok;
		StatusText->SetColorAndOpacity(FSlateColor(bOk ? GoodColor : FLinearColor(1.f, 0.45f, 0.4f)));
		StatusText->SetText(UGrimBuilderComponent::DescribeResult(Preview.Result));
	}

	// 상호작용 안내 — 가방 화면이 열리면 숨긴다.
	const UGrimInventoryComponent* Bag = BuilderComponent->GetBag();
	const bool bBagOpen = Bag && Bag->GetWidget() && Bag->GetWidget()->IsBagOpen();
	const AActor* Target = BuilderComponent->GetInteractTarget();
	const IGrimBaseInteractable* Interactable = Cast<IGrimBaseInteractable>(Target);
	if (Interactable && !bBagOpen)
	{
		PromptText->SetText(FText::Format(LOCTEXT("Prompt", "[{0}] {1}"), UGrimBaseSettings::Get()->InteractKey.GetDisplayName(), Interactable->GetInteractPrompt(BuilderComponent->GetPawn())));
		PromptRoot->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		PromptRoot->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UGrimBaseWidget::HandleNotice(UGrimBuilderComponent* InBuilder, EGrimBaseNotice Notice, FName Item, int32 CountOrResult)
{
	if (!NoticeText)
	{
		return;
	}
	NoticeText->SetText(UGrimBuilderComponent::DescribeNotice(Notice, Item, CountOrResult));
	NoticeText->SetVisibility(ESlateVisibility::HitTestInvisible);
	NoticeRemaining = UGrimBaseSettings::Get()->NoticeSeconds;
}

#undef LOCTEXT_NAMESPACE
