#include "Base/GrimBaseTabs.h"
#include "Base/GrimBaseCore.h"
#include "Base/GrimBaseSettings.h"
#include "Base/GrimBaseStations.h"
#include "Base/GrimBaseSubsystem.h"
#include "Base/GrimBuilderComponent.h"
#include "Base/GrimEquipmentComponent.h"
#include "Base/GrimItemContainerComponent.h"
#include "Base/GrimStructureComponent.h"
#include "Items/GrimInventoryComponent.h"
#include "Items/GrimInventoryWidget.h"
#include "Items/GrimItemSettings.h"
#include "Items/GrimItemStatics.h"
#include "Survival/GrimVitalAttributeSet.h"
#include "Core/GrimPlayerState.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "GrimBaseTabs"

namespace GrimBaseTabsLocal // 유니티 빌드에서 이름이 부딪히지 않게
{
	const FLinearColor BodyColor(0.92f, 0.92f, 0.92f);
	const FLinearColor DimColor(0.65f, 0.65f, 0.65f);
	const FLinearColor ButtonColor(0.25f, 0.22f, 0.18f, 1.f);
	const FLinearColor SlotColor(0.18f, 0.16f, 0.14f, 0.95f);
	constexpr int32 StorageColumns = 6;

	FString ItemLabel(const FGrimItemInstance& Item)
	{
		if (Item.IsEmpty())
		{
			return FString();
		}
		return UGrimInventoryWidget::DescribeItem(Item);
	}

	FText SlotName(int32 SlotIndex)
	{
		switch (static_cast<EGrimEquipSlot>(SlotIndex))
		{
		case EGrimEquipSlot::Wand: return LOCTEXT("SlotWand", "완드");
		case EGrimEquipSlot::Head: return LOCTEXT("SlotHead", "머리");
		case EGrimEquipSlot::Body: return LOCTEXT("SlotBody", "몸");
		case EGrimEquipSlot::Cloak: return LOCTEXT("SlotCloak", "망토");
		case EGrimEquipSlot::Hands: return LOCTEXT("SlotHands", "손");
		case EGrimEquipSlot::Feet: return LOCTEXT("SlotFeet", "발");
		case EGrimEquipSlot::Ring: return LOCTEXT("SlotRing", "반지");
		case EGrimEquipSlot::Necklace: return LOCTEXT("SlotNecklace", "목걸이");
		default: return FText::GetEmpty();
		}
	}
}

// ───────────────────────── 공통 ─────────────────────────

UGrimBuilderComponent* UGrimBaseTabBase::GetBuilder() const
{
	return UGrimBuilderComponent::FindForActor(GetOwningPlayer());
}

template <typename T>
T* UGrimBaseTabBase::GetOpenStation() const
{
	const UGrimBuilderComponent* Builder = GetBuilder();
	return Builder ? Cast<T>(Builder->GetOpenStation()) : nullptr;
}

UGrimIdButton* UGrimBaseTabBase::MakeButton(UWidgetTree* Tree, int32 Id, const FText& Label, float Width, float Height, UTextBlock** OutLabel)
{
	using namespace GrimBaseTabsLocal;
	UGrimIdButton* Button = Tree->ConstructWidget<UGrimItemButton>(UGrimItemButton::StaticClass());
	Button->Id = Id;
	Button->Bind();
	Button->SetBackgroundColor(ButtonColor);
	UTextBlock* Text = GrimUIData::MakeText(Tree, Label, 12, BodyColor);
	Text->SetJustification(ETextJustify::Center);
	Text->SetAutoWrapText(true);
	Button->SetContent(Text);
	if (OutLabel)
	{
		*OutLabel = Text;
	}
	return Button;
}

// ───────────────────────── 장비 ─────────────────────────

FName UGrimEquipmentTab::GetTabId() const
{
	return UGrimInventoryWidget::EquipmentTabId;
}

FText UGrimEquipmentTab::GetLabel() const
{
	return LOCTEXT("EquipmentTab", "장비");
}

FString UGrimEquipmentTab::DescribeSlot(int32 SlotIndex, const FGrimItemInstance& Item)
{
	const FString Name = GrimBaseTabsLocal::SlotName(SlotIndex).ToString();
	if (Item.IsEmpty())
	{
		return FString::Printf(TEXT("%s: —"), *Name);
	}
	const float Durability = UGrimEquipmentComponent::GetDurability(Item);
	const float Max = UGrimEquipmentComponent::GetMaxDurability(Item);
	FString Text = FString::Printf(TEXT("%s: %s   내구도 %.0f/%.0f"), *Name, *GrimBaseTabsLocal::ItemLabel(Item), Durability, Max);
	if (Durability <= 0.f && static_cast<EGrimEquipSlot>(SlotIndex) == EGrimEquipSlot::Wand)
	{
		Text += TEXT("  — 부러짐(수리로 되살린다)");
	}
	else if (Max > 0.f && Durability / Max <= 0.1f)
	{
		Text += TEXT("  — 곧 부서진다!");
	}
	else if (Max > 0.f && Durability / Max <= 0.25f)
	{
		Text += TEXT("  — 많이 닳았다");
	}
	if (Item.RepairCount > 0)
	{
		Text += FString::Printf(TEXT("  (수리 %d번)"), Item.RepairCount);
	}
	return Text;
}

UWidget* UGrimEquipmentTab::BuildContent(UWidgetTree* Tree)
{
	using namespace GrimBaseTabsLocal;
	UVerticalBox* Box = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	for (int32 Index = 0; Index < static_cast<int32>(EGrimEquipSlot::Count); ++Index)
	{
		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UTextBlock* Text = GrimUIData::MakeText(Tree, FText::GetEmpty(), 13, BodyColor);
		Row->AddChildToHorizontalBox(GrimUIData::MakeSizeBox(Tree, Text, 560.f, 30.f))->SetVerticalAlignment(VAlign_Center);
		UGrimIdButton* Button = MakeButton(Tree, Index, LOCTEXT("Unequip", "벗기"), 80.f, 28.f);
		Button->OnIdClicked.BindUObject(this, &UGrimEquipmentTab::HandleUnequip);
		Row->AddChildToHorizontalBox(GrimUIData::MakeSizeBox(Tree, Button, 80.f, 28.f));
		Box->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, 2.f));
		SlotTexts.Add(Text);
	}
	SummaryText = GrimUIData::MakeText(Tree, FText::GetEmpty(), 13, DimColor);
	SummaryText->SetAutoWrapText(true);
	Box->AddChildToVerticalBox(GrimUIData::MakeSizeBox(Tree, SummaryText, 660.f, 60.f))->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
	return Box;
}

void UGrimEquipmentTab::Refresh()
{
	const UGrimEquipmentComponent* Equipment = UGrimEquipmentComponent::FindForActor(GetOwningPlayer());
	for (int32 Index = 0; Index < SlotTexts.Num(); ++Index)
	{
		const FGrimItemInstance Item = Equipment && Equipment->GetSlots().IsValidIndex(Index) ? Equipment->GetSlots()[Index] : FGrimItemInstance();
		SlotTexts[Index]->SetText(FText::FromString(DescribeSlot(Index, Item)));
	}
	const APlayerController* PC = GetOwningPlayer();
	const AGrimPlayerState* PlayerState = PC ? PC->GetPlayerState<AGrimPlayerState>() : nullptr;
	const UAbilitySystemComponent* ASC = PlayerState ? PlayerState->GetAbilitySystemComponent() : nullptr;
	const float Cold = ASC ? ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetColdResistAttribute()) : 0.f;
	const float Heat = ASC ? ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetHeatResistAttribute()) : 0.f;
	SummaryText->SetText(FText::Format(LOCTEXT("Summary", "체온 보정 합: 추위 저항 +{0} · 더위 저항 +{1}\n입기: 가방 탭에서 장비를 고르고 [사용]. 수리: 작업대에 「수리」 마법(재료 1개 = 25%, 고칠 때마다 최대치 −10%)."),
		FText::AsNumber(FMath::RoundToInt(Cold)), FText::AsNumber(FMath::RoundToInt(Heat))));
}

void UGrimEquipmentTab::HandleUnequip(int32 SlotIndex)
{
	if (UGrimEquipmentComponent* Equipment = UGrimEquipmentComponent::FindForActor(GetOwningPlayer()))
	{
		Equipment->Server_Unequip(static_cast<uint8>(SlotIndex));
	}
}

// ───────────────────────── 저장고 ─────────────────────────

FName UGrimStorageTab::GetTabId() const
{
	return AGrimStorageChest::TabId;
}

FText UGrimStorageTab::GetLabel() const
{
	return LOCTEXT("StorageTab", "저장고");
}

bool UGrimStorageTab::IsAvailable() const
{
	return GetOpenStation<AGrimStorageChest>() != nullptr;
}

UWidget* UGrimStorageTab::BuildContent(UWidgetTree* InTree)
{
	using namespace GrimBaseTabsLocal;
	Tree = InTree;
	UVerticalBox* Box = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Box->AddChildToVerticalBox(GrimUIData::MakeText(Tree, LOCTEXT("StorageHelp", "칸을 누르면 통째로 옮긴다 — 위: 가방 → 저장고, 아래: 저장고 → 가방"), 13, DimColor));
	BagGrid = Tree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
	BagGrid->SetSlotPadding(FMargin(2.f));
	Box->AddChildToVerticalBox(BagGrid)->SetPadding(FMargin(0.f, 6.f));
	Box->AddChildToVerticalBox(GrimUIData::MakeText(Tree, LOCTEXT("StorageChest", "저장고"), 14, BodyColor, true));
	ChestGrid = Tree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass());
	ChestGrid->SetSlotPadding(FMargin(2.f));
	Box->AddChildToVerticalBox(ChestGrid)->SetPadding(FMargin(0.f, 6.f));
	return Box;
}

void UGrimStorageTab::EnsureButtons(UUniformGridPanel* Grid, TArray<TObjectPtr<UTextBlock>>& Texts, int32 Count, bool bChest)
{
	using namespace GrimBaseTabsLocal;
	while (Texts.Num() < Count)
	{
		const int32 Index = Texts.Num();
		UTextBlock* Label = nullptr;
		UGrimIdButton* Button = MakeButton(Tree, Index, FText::GetEmpty(), 100.f, 40.f, &Label);
		Button->SetBackgroundColor(SlotColor);
		if (bChest)
		{
			Button->OnIdClicked.BindUObject(this, &UGrimStorageTab::HandleChestClicked);
		}
		else
		{
			Button->OnIdClicked.BindUObject(this, &UGrimStorageTab::HandleBagClicked);
		}
		Grid->AddChildToUniformGrid(GrimUIData::MakeSizeBox(Tree, Button, 100.f, 40.f), Index / StorageColumns, Index % StorageColumns);
		Texts.Add(Label);
	}
}

void UGrimStorageTab::Refresh()
{
	const UGrimInventoryComponent* Bag = GetInventory();
	const AGrimStorageChest* Chest = GetOpenStation<AGrimStorageChest>();
	const UGrimItemContainerComponent* Container = Chest ? Chest->GetContainer() : nullptr;
	if (!Bag || !Container || !Tree)
	{
		return;
	}
	EnsureButtons(BagGrid, BagTexts, Bag->GetSlotCount(), false);
	EnsureButtons(ChestGrid, ChestTexts, Container->GetSlotCount(), true);
	for (int32 Index = 0; Index < BagTexts.Num(); ++Index)
	{
		BagTexts[Index]->SetText(FText::FromString(Bag->GetSlots().IsValidIndex(Index) ? GrimBaseTabsLocal::ItemLabel(Bag->GetSlots()[Index]) : FString()));
	}
	for (int32 Index = 0; Index < ChestTexts.Num(); ++Index)
	{
		ChestTexts[Index]->SetText(FText::FromString(Container->GetSlots().IsValidIndex(Index) ? GrimBaseTabsLocal::ItemLabel(Container->GetSlots()[Index]) : FString()));
	}
}

void UGrimStorageTab::HandleBagClicked(int32 SlotIndex)
{
	UGrimBuilderComponent* Builder = GetBuilder();
	AGrimStorageChest* Chest = GetOpenStation<AGrimStorageChest>();
	if (Builder && Chest)
	{
		Builder->Server_TransferStorage(Chest, true, SlotIndex, -1);
		MarkDirty();
	}
}

void UGrimStorageTab::HandleChestClicked(int32 SlotIndex)
{
	UGrimBuilderComponent* Builder = GetBuilder();
	AGrimStorageChest* Chest = GetOpenStation<AGrimStorageChest>();
	if (Builder && Chest)
	{
		Builder->Server_TransferStorage(Chest, false, SlotIndex, -1);
		MarkDirty();
	}
}

// ───────────────────────── 작업대 ─────────────────────────

FName UGrimWorkbenchTab::GetTabId() const
{
	return AGrimWorkbench::TabId;
}

FText UGrimWorkbenchTab::GetLabel() const
{
	return LOCTEXT("WorkbenchTab", "작업대");
}

bool UGrimWorkbenchTab::IsAvailable() const
{
	return GetOpenStation<AGrimWorkbench>() != nullptr;
}

UWidget* UGrimWorkbenchTab::BuildContent(UWidgetTree* Tree)
{
	using namespace GrimBaseTabsLocal;
	UVerticalBox* Box = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Box->AddChildToVerticalBox(GrimUIData::MakeText(Tree, LOCTEXT("WorkbenchHelp", "레시피를 누르면 가방의 재료로 바로 만든다"), 13, DimColor))->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	for (const FGrimRecipeDefinition& Recipe : UGrimItemSettings::GetDatabase().Recipes)
	{
		if (Recipe.Row.Station != AGrimWorkbench::StationId)
		{
			continue;
		}
		const int32 Index = RecipeIds.Add(Recipe.Id);
		UTextBlock* Label = nullptr;
		UGrimIdButton* Button = MakeButton(Tree, Index, FText::GetEmpty(), 660.f, 36.f, &Label);
		Button->OnIdClicked.BindUObject(this, &UGrimWorkbenchTab::HandleRecipeClicked);
		Box->AddChildToVerticalBox(GrimUIData::MakeSizeBox(Tree, Button, 660.f, 36.f))->SetPadding(FMargin(0.f, 2.f));
		RecipeTexts.Add(Label);
	}
	const int32 RepairIndex = -1;
	UGrimIdButton* RepairButton = MakeButton(Tree, RepairIndex, LOCTEXT("RepairEquipment", "「수리」 걸기 — 닳은 장비를 가방의 재료로 고친다"), 660.f, 36.f);
	RepairButton->OnIdClicked.BindUObject(this, &UGrimWorkbenchTab::HandleRecipeClicked);
	Box->AddChildToVerticalBox(GrimUIData::MakeSizeBox(Tree, RepairButton, 660.f, 36.f))->SetPadding(FMargin(0.f, 10.f, 0.f, 2.f));
	InfoText = GrimUIData::MakeText(Tree, FText::GetEmpty(), 13, DimColor);
	InfoText->SetAutoWrapText(true);
	Box->AddChildToVerticalBox(GrimUIData::MakeSizeBox(Tree, InfoText, 660.f, 60.f));
	return Box;
}

void UGrimWorkbenchTab::Refresh()
{
	const UGrimInventoryComponent* Bag = GetInventory();
	const FGrimItemDatabase& Items = UGrimItemSettings::GetDatabase();
	for (int32 Index = 0; Index < RecipeIds.Num(); ++Index)
	{
		const FGrimRecipeDefinition* Recipe = Items.Recipes.FindByPredicate([&](const FGrimRecipeDefinition& Candidate) { return Candidate.Id == RecipeIds[Index]; });
		if (!Recipe || !RecipeTexts.IsValidIndex(Index))
		{
			continue;
		}
		TArray<FString> Parts;
		for (const FGrimRecipeInput& Input : Recipe->Inputs)
		{
			const FGrimItemDefinition* Item = Items.Find(Input.Id);
			const FString Name = Input.bGroup ? UGrimItemSettings::GetGroupDisplayName(Input.Id) : (Item ? Item->GetDisplayName().ToString() : Input.Id.ToString());
			Parts.Add(FString::Printf(TEXT("%s %d/%d"), *Name, Bag && !Input.bGroup ? Bag->CountOf(Input.Id) : 0, Input.Count));
		}
		const bool bCan = Bag && FGrimSlotTransferMath::HasInputs(Bag->GetSlots(), Items, Recipe->Inputs);
		RecipeTexts[Index]->SetText(FText::FromString(FString::Printf(TEXT("%s%s ← %s"), bCan ? TEXT("") : TEXT("(재료 모자람) "),
			*UGrimItemStatics::GetItemDisplayName(Recipe->Row.Output).ToString(), *FString::Join(Parts, TEXT(" · ")))));
	}
	const UGrimBuilderComponent* Builder = GetBuilder();
	InfoText->SetText(Builder && Builder->KnowsRepairSpell()
		? LOCTEXT("RepairKnown", "「수리」 마법을 작업대에 쏘아도 같다(마나 15).")
		: LOCTEXT("RepairUnknown", "「수리」 마법을 아직 모른다 — 스킬북으로 배운다."));
}

void UGrimWorkbenchTab::HandleRecipeClicked(int32 Index)
{
	UGrimBuilderComponent* Builder = GetBuilder();
	AGrimWorkbench* Workbench = GetOpenStation<AGrimWorkbench>();
	if (!Builder || !Workbench)
	{
		return;
	}
	if (Index < 0)
	{
		Builder->Server_Repair(Workbench);
	}
	else if (RecipeIds.IsValidIndex(Index))
	{
		Builder->Server_Craft(Workbench, RecipeIds[Index]);
	}
	MarkDirty();
}

// ───────────────────────── 거점 ─────────────────────────

FName UGrimBaseCoreTab::GetTabId() const
{
	return AGrimBaseCore::TabId;
}

FText UGrimBaseCoreTab::GetLabel() const
{
	return LOCTEXT("BaseTab", "거점");
}

bool UGrimBaseCoreTab::IsAvailable() const
{
	return GetOpenStation<AGrimBaseCore>() != nullptr;
}

UWidget* UGrimBaseCoreTab::BuildContent(UWidgetTree* Tree)
{
	using namespace GrimBaseTabsLocal;
	UVerticalBox* Box = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	InfoText = GrimUIData::MakeText(Tree, FText::GetEmpty(), 13, BodyColor);
	InfoText->SetAutoWrapText(true);
	Box->AddChildToVerticalBox(GrimUIData::MakeSizeBox(Tree, InfoText, 660.f, 200.f));
	UGrimIdButton* Button = MakeButton(Tree, 0, FText::Format(LOCTEXT("BaseRepair", "거점 수리 — 저장고의 재료로 닳은 것을 모두 고친다(마나 {0})"),
		FText::AsNumber(FMath::RoundToInt(UGrimBaseSettings::Get()->BaseRepairManaCost))), 660.f, 36.f);
	Button->OnIdClicked.BindUObject(this, &UGrimBaseCoreTab::HandleRepairClicked);
	Box->AddChildToVerticalBox(GrimUIData::MakeSizeBox(Tree, Button, 660.f, 36.f))->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
	return Box;
}

void UGrimBaseCoreTab::Refresh()
{
	const AGrimBaseCore* Core = GetOpenStation<AGrimBaseCore>();
	const UGrimBaseSubsystem* Subsystem = UGrimBaseSubsystem::Get(GetOwningPlayer());
	if (!Core || !Subsystem)
	{
		return;
	}
	int32 Count = 0;
	int32 Damaged = 0;
	TMap<FName, int32> Needed;
	for (const UGrimStructureComponent* Structure : Subsystem->GetStructures())
	{
		if (Structure->FindBaseCore() != Core)
		{
			continue;
		}
		++Count;
		if (Structure->IsDamaged())
		{
			++Damaged;
			if (!Structure->GetRepairItem().IsNone())
			{
				Needed.FindOrAdd(Structure->GetRepairItem()) += Structure->GetMaterialsNeeded();
			}
		}
	}
	TMap<FName, int32> Stored;
	for (TActorIterator<AGrimStorageChest> It(Core->GetWorld()); It; ++It)
	{
		if (Core->IsInside(It->GetActorLocation()) && It->GetContainer())
		{
			for (const FGrimItemInstance& Item : It->GetContainer()->GetSlots())
			{
				if (!Item.IsEmpty())
				{
					Stored.FindOrAdd(Item.ItemId) += Item.Count;
				}
			}
		}
	}
	TArray<FString> Needs;
	for (const TPair<FName, int32>& Pair : Needed)
	{
		Needs.Add(FString::Printf(TEXT("%s %d (저장고 %d)"), *UGrimItemStatics::GetItemDisplayName(Pair.Key).ToString(), Pair.Value, Stored.FindRef(Pair.Key)));
	}
	const FString Text = FString::Printf(TEXT("%s 의 거점 — 반경 %.0f m\n지은 것 %d · 닳은 것 %d\n수리 재료: %s\n\n거점 안은 아주 느리게, 밖은 빠르게 닳는다. 나무는 불에 탄다."),
		Core->GetOwnerName().IsEmpty() ? TEXT("주인 없는") : *Core->GetOwnerName(), UGrimBaseSettings::Get()->BaseRadius / 100.f, Count, Damaged,
		Needs.Num() > 0 ? *FString::Join(Needs, TEXT(" · ")) : TEXT("없음"));
	InfoText->SetText(FText::FromString(Text));
}

void UGrimBaseCoreTab::HandleRepairClicked(int32 Unused)
{
	UGrimBuilderComponent* Builder = GetBuilder();
	AGrimBaseCore* Core = GetOpenStation<AGrimBaseCore>();
	if (Builder && Core)
	{
		Builder->Server_Repair(Core);
	}
}

#undef LOCTEXT_NAMESPACE
