#include "Items/GrimInventoryWidget.h"
#include "Items/GrimCookingPot.h"
#include "Items/GrimItemPickup.h"
#include "Items/GrimItemSettings.h"
#include "UI/GrimUIData.h"
#include "UI/GrimUISubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "GrimInventory"

namespace GrimInventoryWidgetLocal // 유니티 빌드에서 이름이 부딪히지 않게
{
	const FLinearColor TitleColor(1.f, 0.85f, 0.45f);
	const FLinearColor BodyColor(0.92f, 0.92f, 0.92f);
	const FLinearColor DimColor(0.6f, 0.6f, 0.6f);
	const FLinearColor PanelColor(0.f, 0.f, 0.f, 0.6f);
	const FLinearColor PromptPanelColor(0.f, 0.f, 0.f, 0.45f);
	const FLinearColor SlotColor(0.18f, 0.16f, 0.14f, 0.95f);
	const FLinearColor SelectedColor(0.65f, 0.5f, 0.2f, 1.f);
	const FLinearColor ActionColor(0.25f, 0.22f, 0.18f, 1.f);
	constexpr int32 Columns = 6;
	constexpr float SlotWidth = 118.f;
	constexpr float SlotHeight = 52.f;

	enum EAction : int32 { ActionUse = 0, ActionDropOne = 1, ActionDropAll = 2, ActionMove = 3 };

	FString InputName(const FGrimItemDatabase& Database, const FGrimRecipeInput& Input)
	{
		if (Input.bGroup)
		{
			return UGrimItemSettings::GetGroupDisplayName(Input.Id);
		}
		const FGrimItemDefinition* Def = Database.Find(Input.Id);
		return Def ? Def->GetDisplayName().ToString() : Input.Id.ToString();
	}

	FText KindName(EGrimItemKind Kind)
	{
		switch (Kind)
		{
		case EGrimItemKind::Food: return LOCTEXT("KindFood", "음식");
		case EGrimItemKind::Skillbook: return LOCTEXT("KindSkillbook", "스킬북");
		case EGrimItemKind::Tool: return LOCTEXT("KindTool", "도구");
		default: return LOCTEXT("KindMaterial", "재료");
		}
	}
}

void UGrimInventoryWidget::SetInventory(UGrimInventoryComponent* InInventory)
{
	if (UGrimInventoryComponent* Old = Inventory.Get())
	{
		Old->OnInventoryChanged.Remove(ChangedHandle);
		Old->OnNotice.Remove(NoticeHandle);
	}
	Inventory = InInventory;
	if (InInventory)
	{
		ChangedHandle = InInventory->OnInventoryChanged.AddUObject(this, &UGrimInventoryWidget::HandleInventoryChanged);
		NoticeHandle = InInventory->OnNotice.AddUObject(this, &UGrimInventoryWidget::HandleNotice);
	}
	bBagDirty = true;
}

void UGrimInventoryWidget::NativeDestruct()
{
	if (UGrimInventoryComponent* Old = Inventory.Get())
	{
		Old->OnInventoryChanged.Remove(ChangedHandle);
		Old->OnNotice.Remove(NoticeHandle);
	}
	Super::NativeDestruct();
}

TSharedRef<SWidget> UGrimInventoryWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

void UGrimInventoryWidget::BuildTree()
{
	using namespace GrimInventoryWidgetLocal;

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Canvas;
	// 뿌리는 숨기지 않는다(숨기면 틱이 멈춰 키를 못 본다).
	Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	// ── 아래 가운데: 알림 + 안내 ──
	UVerticalBox* Bottom = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Bottom->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* BottomSlot = Canvas->AddChildToCanvas(Bottom);
	BottomSlot->SetAnchors(FAnchors(0.5f, 1.f));
	BottomSlot->SetAlignment(FVector2D(0.5f, 1.f));
	BottomSlot->SetPosition(FVector2D(0.f, -170.f));
	BottomSlot->SetAutoSize(true);

	NoticeText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 16, TitleColor, true);
	NoticeText->SetJustification(ETextJustify::Center);
	NoticeText->SetVisibility(ESlateVisibility::Collapsed);
	UVerticalBoxSlot* NoticeSlot = Bottom->AddChildToVerticalBox(NoticeText);
	NoticeSlot->SetHorizontalAlignment(HAlign_Center);
	NoticeSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));

	UBorder* PromptPanel = GrimUIData::MakeBorder(WidgetTree, PromptPanelColor, FMargin(14.f, 6.f));
	PromptText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 15, BodyColor);
	PromptText->SetJustification(ETextJustify::Center);
	PromptPanel->SetContent(PromptText);
	PromptRoot = PromptPanel;
	PromptRoot->SetVisibility(ESlateVisibility::Collapsed);
	Bottom->AddChildToVerticalBox(PromptPanel)->SetHorizontalAlignment(HAlign_Center);

	// ── 가운데: 가방 ──
	UBorder* BagPanel = GrimUIData::MakeBorder(WidgetTree, PanelColor, FMargin(16.f, 12.f));
	BagRoot = BagPanel;
	BagRoot->SetVisibility(ESlateVisibility::Collapsed);
	UCanvasPanelSlot* BagSlot = Canvas->AddChildToCanvas(BagPanel);
	BagSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	BagSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	BagSlot->SetAutoSize(true);

	UVerticalBox* BagBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	BagPanel->SetContent(BagBox);

	const FKey& BagKey = UGrimItemSettings::Get()->BagKey;
	BagBox->AddChildToVerticalBox(GrimUIData::MakeText(WidgetTree,
		FText::Format(LOCTEXT("BagTitle", "가방  ({0} 닫기)"), BagKey.GetDisplayName()), 18, TitleColor, true))->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));

	SlotGrid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
	SlotGrid->SetSlotPadding(FMargin(3.f));
	BagBox->AddChildToVerticalBox(SlotGrid);

	DetailText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 13, BodyColor);
	DetailText->SetAutoWrapText(true);
	USizeBox* DetailBox = GrimUIData::MakeSizeBox(WidgetTree, DetailText, Columns * (SlotWidth + 6.f), 96.f);
	BagBox->AddChildToVerticalBox(DetailBox)->SetPadding(FMargin(0.f, 10.f, 0.f, 6.f));

	UHorizontalBox* Actions = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	BagBox->AddChildToVerticalBox(Actions);
	const FText ActionLabels[] = {
		LOCTEXT("ActionUse", "사용 (먹기 · 읽기)"),
		LOCTEXT("ActionDropOne", "하나 버리기"),
		LOCTEXT("ActionDropAll", "모두 버리기"),
		LOCTEXT("ActionMove", "옮기기"),
	};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(ActionLabels); ++Index)
	{
		UGrimIdButton* Button = WidgetTree->ConstructWidget<UGrimItemButton>(UGrimItemButton::StaticClass());
		Button->Id = Index;
		Button->Bind();
		Button->OnIdClicked.BindUObject(this, &UGrimInventoryWidget::HandleActionClicked);
		Button->SetBackgroundColor(ActionColor);
		UTextBlock* Label = GrimUIData::MakeText(WidgetTree, ActionLabels[Index], 13, BodyColor);
		Label->SetJustification(ETextJustify::Center);
		Button->SetContent(Label);
		if (Index == ActionMove)
		{
			MoveButtonText = Label;
		}
		Actions->AddChildToHorizontalBox(GrimUIData::MakeSizeBox(WidgetTree, Button, 160.f, 38.f))->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
	}
}

// ───── 틱: 키 · 안내 ─────

void UGrimInventoryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	HandleKeys();

	if (NoticeRemaining > 0.f)
	{
		NoticeRemaining -= InDeltaTime;
		if (NoticeRemaining <= 0.f && NoticeText)
		{
			NoticeText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	PromptAccumulator += InDeltaTime;
	if (PromptAccumulator >= 0.1f)
	{
		PromptAccumulator = 0.f;
		RefreshPrompt();
	}
	if (bBagOpen && bBagDirty)
	{
		RefreshBag();
	}
}

void UGrimInventoryWidget::HandleKeys()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}
	const UGrimItemSettings* Settings = UGrimItemSettings::Get();
	if (PC->WasInputKeyJustPressed(Settings->BagKey))
	{
		SetBagOpen(!bBagOpen);
	}
	else if (!bBagOpen && PC->WasInputKeyJustPressed(Settings->InteractKey))
	{
		Interact();
	}
}

void UGrimInventoryWidget::Interact()
{
	UGrimInventoryComponent* InventoryComponent = Inventory.Get();
	if (!InventoryComponent)
	{
		return;
	}
	if (AGrimItemPickup* Pickup = TargetPickup.Get())
	{
		InventoryComponent->Server_PickUp(Pickup);
	}
	else if (AGrimCookingPot* Pot = TargetPot.Get())
	{
		InventoryComponent->Server_InteractPot(Pot);
	}
}

void UGrimInventoryWidget::RefreshPrompt()
{
	const APlayerController* PC = GetOwningPlayer();
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	// 주울 것이 먼저(냄비 앞에 놓인 스튜를 집게), 그다음 냄비.
	AGrimItemPickup* Pickup = Pawn ? AGrimItemPickup::FindNearestReachable(Pawn) : nullptr;
	AGrimCookingPot* Pot = (Pawn && !Pickup) ? AGrimCookingPot::FindNearestReachable(Pawn) : nullptr;
	TargetPickup = Pickup;
	TargetPot = Pot;

	FString Text;
	if (Pickup)
	{
		Text = DescribePickup(Pickup);
	}
	else if (Pot)
	{
		Text = DescribePot(Pot);
	}
	if (PromptRoot)
	{
		PromptRoot->SetVisibility(Text.IsEmpty() || bBagOpen ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (PromptText && !Text.IsEmpty())
	{
		PromptText->SetText(FText::FromString(Text));
	}
}

FString UGrimInventoryWidget::DescribeItem(const FGrimItemInstance& Item)
{
	const FGrimItemDefinition* Def = UGrimItemSettings::GetDatabase().Find(Item.ItemId);
	const FString Name = Def ? Def->GetDisplayName().ToString() : Item.ItemId.ToString();
	return Item.Count > 1 ? FString::Printf(TEXT("%s ×%d"), *Name, Item.Count) : Name;
}

FString UGrimInventoryWidget::DescribePickup(const AGrimItemPickup* Pickup)
{
	if (!Pickup)
	{
		return FString();
	}
	const FString KeyName = UGrimItemSettings::Get()->InteractKey.GetDisplayName().ToString();
	FString Text = FString::Printf(TEXT("[%s] 줍기 — %s"), *KeyName, *DescribeItem(Pickup->GetItem()));
	const float CookSeconds = Pickup->GetCookSeconds();
	if (CookSeconds > 0.f && (Pickup->IsHeated() || Pickup->GetCookProgress() > 0.f))
	{
		const int32 Percent = FMath::Clamp(FMath::RoundToInt(100.f * Pickup->GetCookProgress() / CookSeconds), 0, 100);
		Text += Pickup->IsHeated() ? FString::Printf(TEXT("\n익는 중 %d%%"), Percent) : FString::Printf(TEXT("\n반쯤 익음 %d%% — 불 곁에 두거나 불 마법으로"), Percent);
	}
	return Text;
}

FString UGrimInventoryWidget::DescribePot(const AGrimCookingPot* Pot)
{
	using namespace GrimInventoryWidgetLocal;
	if (!Pot)
	{
		return FString();
	}
	const FGrimItemDatabase& Database = UGrimItemSettings::GetDatabase();
	const FString KeyName = UGrimItemSettings::Get()->InteractKey.GetDisplayName().ToString();
	const FGrimRecipeProgress Progress = Pot->DescribeBestRecipe();

	FString Text = FString::Printf(TEXT("[%s] 냄비에 재료 넣기"), *KeyName);
	if (Progress.Recipe)
	{
		TArray<FString> Parts;
		for (int32 Index = 0; Index < Progress.Recipe->Inputs.Num(); ++Index)
		{
			const FGrimRecipeInput& Input = Progress.Recipe->Inputs[Index];
			Parts.Add(FString::Printf(TEXT("%s %d/%d"), *InputName(Database, Input), Progress.Filled.IsValidIndex(Index) ? Progress.Filled[Index] : 0, Input.Count));
		}
		const FString RecipeName = Progress.Recipe->Row.DisplayName.IsEmpty() ? Progress.Recipe->Id.ToString() : Progress.Recipe->Row.DisplayName;
		Text += FString::Printf(TEXT("\n%s: %s"), *RecipeName, *FString::Join(Parts, TEXT(" · ")));
	}

	const FGrimRecipeDefinition* Matching = Pot->GetMatchingRecipe();
	if (Pot->GetContents().Num() == 0)
	{
		return Text;
	}
	if (Matching)
	{
		const int32 Percent = FMath::Clamp(FMath::RoundToInt(100.f * Pot->GetProgress() / FMath::Max(0.01f, Matching->Row.CookSeconds)), 0, 100);
		Text += Pot->IsHeated() ? FString::Printf(TEXT("\n끓는 중 %d%%"), Percent) : TEXT("\n불이 필요하다 — 아궁이에 불을 붙이자");
	}
	else if (Progress.bHasExtra)
	{
		Text += FString::Printf(TEXT("\n맞지 않는 재료가 들어 있다 — [%s] 로 꺼낸다"), *KeyName);
	}
	else
	{
		Text += TEXT("\n재료가 모자라다");
	}
	return Text;
}

// ───── 가방 ─────

void UGrimInventoryWidget::SetBagOpen(bool bOpen)
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC || bOpen == bBagOpen)
	{
		return;
	}
	bBagOpen = bOpen;
	bMoveArmed = false;
	bBagDirty = true;
	if (BagRoot)
	{
		BagRoot->SetVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (bOpen)
	{
		if (UGrimInventoryComponent* Owned = Inventory.Get())
		{
			Owned->Server_NotifyBagOpened();
		}
		if (UGrimUISubsystem* UI = PC->GetLocalPlayer() ? PC->GetLocalPlayer()->GetSubsystem<UGrimUISubsystem>() : nullptr)
		{
			UI->SetGrimoireOpen(false);
		}
		FInputModeGameAndUI Mode;
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Mode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(Mode);
		PC->SetShowMouseCursor(true);
		PC->SetIgnoreLookInput(true);
		RefreshBag();
	}
	else
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
		PC->SetIgnoreLookInput(false);
	}
}

void UGrimInventoryWidget::RefreshBag()
{
	using namespace GrimInventoryWidgetLocal;
	bBagDirty = false;
	const UGrimInventoryComponent* InventoryComponent = Inventory.Get();
	if (!InventoryComponent || !SlotGrid)
	{
		return;
	}
	const TArray<FGrimItemInstance>& Slots = InventoryComponent->GetSlots();
	while (SlotButtons.Num() < Slots.Num())
	{
		const int32 Index = SlotButtons.Num();
		UGrimIdButton* Button = WidgetTree->ConstructWidget<UGrimItemButton>(UGrimItemButton::StaticClass());
		Button->Id = Index;
		Button->Bind();
		Button->OnIdClicked.BindUObject(this, &UGrimInventoryWidget::HandleSlotClicked);
		UTextBlock* Label = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 12, BodyColor);
		Label->SetJustification(ETextJustify::Center);
		Label->SetAutoWrapText(true);
		Button->SetContent(Label);
		UUniformGridSlot* GridSlot = SlotGrid->AddChildToUniformGrid(GrimUIData::MakeSizeBox(WidgetTree, Button, SlotWidth, SlotHeight), Index / Columns, Index % Columns);
		GridSlot->SetHorizontalAlignment(HAlign_Fill);
		SlotButtons.Add(Button);
		SlotTexts.Add(Label);
	}

	const FGrimItemDatabase& Database = UGrimItemSettings::GetDatabase();
	for (int32 Index = 0; Index < SlotButtons.Num(); ++Index)
	{
		const bool bUsed = Slots.IsValidIndex(Index);
		const FGrimItemInstance Item = bUsed ? Slots[Index] : FGrimItemInstance();
		SlotButtons[Index]->SetBackgroundColor(Index == SelectedSlot ? SelectedColor : SlotColor);
		SlotTexts[Index]->SetText(Item.IsEmpty() ? FText::GetEmpty() : FText::FromString(DescribeItem(Item)));
	}

	// 고른 칸 설명
	FString Detail;
	if (Slots.IsValidIndex(SelectedSlot) && !Slots[SelectedSlot].IsEmpty())
	{
		const FGrimItemInstance& Item = Slots[SelectedSlot];
		if (const FGrimItemDefinition* Def = Database.Find(Item.ItemId))
		{
			Detail = FString::Printf(TEXT("%s  (%s)\n%s"), *DescribeItem(Item), *KindName(Def->Row.Kind).ToString(), *Def->Row.Description);
			if (Def->IsFood())
			{
				Detail += FString::Printf(TEXT("\n배고픔 +%.0f"), Def->Row.HungerRestore);
				if (Def->Row.bRaw)
				{
					Detail += TEXT(" · 날것 — 배탈이 날 수 있다");
				}
				if (Def->Row.BuffSeconds > 0.f)
				{
					Detail += FString::Printf(TEXT(" · 버프 %.0f초"), Def->Row.BuffSeconds);
				}
			}
			if (Def->IsCookable())
			{
				Detail += TEXT("\n불 곁에 두거나 불 마법으로 익힌다");
			}
		}
	}
	else
	{
		Detail = LOCTEXT("PickSlot", "칸을 골라 쓰거나 버린다. 옮기기: 칸을 고르고 [옮기기] 뒤 놓을 칸을 누른다.").ToString();
	}
	if (bMoveArmed)
	{
		Detail += TEXT("\n→ 놓을 칸을 누른다");
	}
	DetailText->SetText(FText::FromString(Detail));
	if (MoveButtonText)
	{
		MoveButtonText->SetText(bMoveArmed ? LOCTEXT("ActionMoveArmed", "옮기는 중…") : LOCTEXT("ActionMove", "옮기기"));
	}
}

void UGrimInventoryWidget::HandleSlotClicked(int32 SlotIndex)
{
	UGrimInventoryComponent* InventoryComponent = Inventory.Get();
	if (bMoveArmed && InventoryComponent && SelectedSlot != INDEX_NONE && SelectedSlot != SlotIndex)
	{
		InventoryComponent->Server_MoveItem(SelectedSlot, SlotIndex);
		bMoveArmed = false;
	}
	SelectedSlot = SlotIndex;
	bBagDirty = true;
}

void UGrimInventoryWidget::HandleActionClicked(int32 Action)
{
	using namespace GrimInventoryWidgetLocal;
	UGrimInventoryComponent* InventoryComponent = Inventory.Get();
	if (!InventoryComponent || !InventoryComponent->GetSlots().IsValidIndex(SelectedSlot) || InventoryComponent->GetSlots()[SelectedSlot].IsEmpty())
	{
		return;
	}
	switch (Action)
	{
	case ActionUse: InventoryComponent->Server_UseItem(SelectedSlot); break;
	case ActionDropOne: InventoryComponent->Server_DropItem(SelectedSlot, 1); break;
	case ActionDropAll: InventoryComponent->Server_DropItem(SelectedSlot, InventoryComponent->GetSlots()[SelectedSlot].Count); break;
	case ActionMove: bMoveArmed = !bMoveArmed; break;
	default: break;
	}
	bBagDirty = true;
}

void UGrimInventoryWidget::HandleInventoryChanged(UGrimInventoryComponent* InInventory)
{
	bBagDirty = true;
}

// ───── 알림 ─────

FText UGrimInventoryWidget::DescribeNotice(EGrimItemNotice Notice, FName ItemId, int32 Count)
{
	const FText Name = UGrimItemSettings::GetDatabase().Find(ItemId) ? UGrimItemSettings::GetDatabase().Find(ItemId)->GetDisplayName() : FText::FromName(ItemId);
	switch (Notice)
	{
	case EGrimItemNotice::Obtained: return FText::Format(LOCTEXT("Obtained", "{0} ×{1} 을(를) 얻었다"), Name, Count);
	case EGrimItemNotice::BagFull: return FText::Format(LOCTEXT("BagFull", "가방이 가득 찼다 — {0} ×{1} 을(를) 못 넣었다"), Name, Count);
	case EGrimItemNotice::Eaten: return FText::Format(LOCTEXT("Eaten", "{0} 을(를) 먹었다"), Name);
	case EGrimItemNotice::GotSick: return FText::Format(LOCTEXT("GotSick", "{0} 을(를) 먹었다 — 배탈이 났다!"), Name);
	case EGrimItemNotice::Learned: return FText::Format(LOCTEXT("Learned", "{0} 을(를) 읽었다"), Name);
	case EGrimItemNotice::CannotUse: return FText::Format(LOCTEXT("CannotUse", "{0} 은(는) 지금 쓸 수 없다"), Name);
	case EGrimItemNotice::Dropped: return FText::Format(LOCTEXT("Dropped", "{0} ×{1} 을(를) 버렸다"), Name, Count);
	case EGrimItemNotice::IngredientsAdded: return FText::Format(LOCTEXT("IngredientsAdded", "냄비에 재료 {0}개를 넣었다"), Count);
	case EGrimItemNotice::NothingToAdd: return LOCTEXT("NothingToAdd", "넣을 재료가 없다");
	case EGrimItemNotice::TookBack: return FText::Format(LOCTEXT("TookBack", "냄비에서 {0}개를 꺼냈다"), Count);
	default: return FText::GetEmpty();
	}
}

void UGrimInventoryWidget::HandleNotice(UGrimInventoryComponent* InInventory, EGrimItemNotice Notice, FName ItemId, int32 Count)
{
	if (!NoticeText)
	{
		return;
	}
	NoticeText->SetText(DescribeNotice(Notice, ItemId, Count));
	NoticeText->SetVisibility(ESlateVisibility::HitTestInvisible);
	NoticeRemaining = UGrimItemSettings::Get()->NoticeSeconds;
}

#undef LOCTEXT_NAMESPACE
