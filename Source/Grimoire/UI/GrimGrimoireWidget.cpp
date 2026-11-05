#include "UI/GrimGrimoireWidget.h"
#include "UI/GrimUIData.h"
#include "UI/GrimUIMath.h"
#include "UI/GrimUISettings.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "Magic/GrimMagicMath.h"
#include "Magic/GrimProficiencySettings.h"
#include "Magic/GrimProgressionComponent.h"
#include "Magic/GrimSpellAbility.h"
#include "Magic/GrimSpellbookComponent.h"
#include "Magic/GrimSpellDefinition.h"
#include "Magic/GrimWand.h"
#include "Core/GrimPlayerState.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "GrimGrimoire"

namespace GrimGrimoireLocal
{
	const FLinearColor PageColor(0.13f, 0.10f, 0.07f, 0.96f);
	const FLinearColor RowColor(0.20f, 0.16f, 0.11f, 1.f);
	const FLinearColor InkColor(0.95f, 0.90f, 0.78f, 1.f);
	const FLinearColor FaintInk(0.75f, 0.70f, 0.60f, 1.f);
	const FLinearColor ActiveTab(0.85f, 0.7f, 0.4f, 1.f);
	const FLinearColor IdleTab(0.5f, 0.45f, 0.35f, 0.35f);

	FText ElementName(const FGameplayTag& Element)
	{
		if (const FGrimElementDisplay* Display = UGrimUISettings::Get()->FindElementDisplay(Element))
		{
			return Display->Name;
		}
		return Element.IsValid() ? FText::FromName(Element.GetTagName()) : LOCTEXT("NoElement", "무속성");
	}

	FText FormatNumber(float Value)
	{
		FNumberFormattingOptions Options;
		Options.SetMaximumFractionalDigits(Value < 10.f ? 2 : 1);
		return FText::AsNumber(Value, &Options);
	}
}

TSharedRef<SWidget> UGrimGrimoireWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

void UGrimGrimoireWidget::BuildTree()
{
	using namespace GrimGrimoireLocal;

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Canvas;

	// 뒤를 어둡게
	UBorder* Dim = GrimUIData::MakeBorder(WidgetTree, FLinearColor(0.f, 0.f, 0.f, 0.35f), FMargin(0.f));
	UCanvasPanelSlot* DimSlot = Canvas->AddChildToCanvas(Dim);
	DimSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	DimSlot->SetOffsets(FMargin(0.f));
	Dim->SetVisibility(ESlateVisibility::HitTestInvisible);

	// 책 한 쪽
	UBorder* Page = GrimUIData::MakeBorder(WidgetTree, PageColor, FMargin(22.f, 18.f));
	UCanvasPanelSlot* PageSlot = Canvas->AddChildToCanvas(GrimUIData::MakeSizeBox(WidgetTree, Page, 940.f, 660.f));
	PageSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	PageSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	PageSlot->SetAutoSize(true);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Page->SetContent(Column);

	Column->AddChildToVerticalBox(GrimUIData::MakeText(WidgetTree, LOCTEXT("Title", "그리모어"), 26, InkColor, true));
	CircleText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 16, InkColor, true);
	Column->AddChildToVerticalBox(CircleText)->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	RequirementText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 12, FaintInk);
	RequirementText->SetAutoWrapText(true);
	Column->AddChildToVerticalBox(RequirementText)->SetPadding(FMargin(0.f, 2.f, 0.f, 8.f));

	// 탭
	UHorizontalBox* TabRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Column->AddChildToVerticalBox(TabRow)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	const FText TabNames[] = { LOCTEXT("TabMagic", "마법"), LOCTEXT("TabTree", "성장 트리"), LOCTEXT("TabCodex", "도감") };
	TabButtons.Reset();
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(TabNames); ++Index)
	{
		UGrimIdButton* Button = WidgetTree->ConstructWidget<UGrimIdButton>(UGrimIdButton::StaticClass());
		Button->Id = Index;
		Button->Bind();
		Button->OnIdClicked.BindUObject(this, &ThisClass::HandleTabClicked);
		Button->SetContent(GrimUIData::MakeText(WidgetTree, TabNames[Index], 15, InkColor, true));
		TabRow->AddChildToHorizontalBox(Button)->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
		TabButtons.Add(Button);
	}

	Pages = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass());
	Column->AddChildToVerticalBox(Pages)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	Pages->AddChild(BuildMagicPage());
	Pages->AddChild(BuildPlaceholderPage(LOCTEXT("TreeTitle", "성장 트리"),
		LOCTEXT("TreeBody", "속성 성장 트리 — 노말 · 4원소(물 · 불 · 바람 · 땅) · 빛 · 어둠.\n레벨이 오르면 받는 포인트로 연다. 다음 묶음에서 채워진다.")));
	Pages->AddChild(BuildPlaceholderPage(LOCTEXT("CodexTitle", "도감"),
		LOCTEXT("CodexBody", "발견한 상호작용 규칙(불 + 마른 풀 → 번짐 같은 것)이 여기에 기록된다.\n아직 기록된 것이 없다 — 다음 묶음에서 채워진다.")));

	Column->AddChildToVerticalBox(GrimUIData::MakeText(WidgetTree, LOCTEXT("Footer", "그리모어 키로 닫는다"), 11, FaintInk))
		->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));

	SetTab(TabIndex);
	RefreshFilterButtons();
}

UWidget* UGrimGrimoireWidget::BuildPlaceholderPage(const FText& Title, const FText& Body)
{
	using namespace GrimGrimoireLocal;
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Box->AddChildToVerticalBox(GrimUIData::MakeText(WidgetTree, Title, 18, InkColor, true))->SetPadding(FMargin(0.f, 8.f));
	UTextBlock* BodyText = GrimUIData::MakeText(WidgetTree, Body, 13, FaintInk);
	BodyText->SetAutoWrapText(true);
	Box->AddChildToVerticalBox(BodyText);
	return Box;
}

UWidget* UGrimGrimoireWidget::BuildMagicPage()
{
	using namespace GrimGrimoireLocal;
	const UGrimUISettings* Settings = UGrimUISettings::Get();
	const UGrimSpellbookSettings* SlotSettings = UGrimSpellbookSettings::Get();

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

	// 완드 보정
	WandText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 12, FaintInk);
	Box->AddChildToVerticalBox(WandText)->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));

	// 단축 슬롯 배치: 세트 고르기 + 지금 세트의 칸
	UHorizontalBox* SlotRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Box->AddChildToVerticalBox(SlotRow)->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
	SlotRow->AddChildToHorizontalBox(GrimUIData::MakeText(WidgetTree, LOCTEXT("SlotSets", "단축 슬롯 세트"), 12, InkColor, true))->SetVerticalAlignment(VAlign_Center);
	SetButtons.Reset();
	for (int32 Index = 0; Index < SlotSettings->SpellSlotSets; ++Index)
	{
		UGrimIdButton* Button = WidgetTree->ConstructWidget<UGrimIdButton>(UGrimIdButton::StaticClass());
		Button->Id = Index;
		Button->Bind();
		Button->OnIdClicked.BindUObject(this, &ThisClass::HandleSetClicked);
		Button->SetContent(GrimUIData::MakeText(WidgetTree, FText::AsNumber(Index + 1), 12, InkColor, true));
		SlotRow->AddChildToHorizontalBox(Button)->SetPadding(FMargin(6.f, 0.f, 0.f, 0.f));
		SetButtons.Add(Button);
	}
	SlotRow->AddChildToHorizontalBox(GrimUIData::MakeSizeBox(WidgetTree, nullptr, 24.f, 0.f));
	SlotButtons.Reset();
	SlotLabels.Reset();
	for (int32 Index = 0; Index < SlotSettings->SpellSlotsPerSet; ++Index)
	{
		UGrimIdButton* Button = WidgetTree->ConstructWidget<UGrimIdButton>(UGrimIdButton::StaticClass());
		Button->Id = Index;
		Button->Bind();
		Button->OnIdClicked.BindUObject(this, &ThisClass::HandleSlotClicked);
		UTextBlock* Label = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 12, InkColor);
		Label->SetJustification(ETextJustify::Center);
		Button->SetContent(GrimUIData::MakeSizeBox(WidgetTree, Label, 96.f, 0.f));
		SlotRow->AddChildToHorizontalBox(Button)->SetPadding(FMargin(4.f, 0.f, 0.f, 0.f));
		SlotButtons.Add(Button);
		SlotLabels.Add(Label);
	}
	Box->AddChildToVerticalBox(GrimUIData::MakeText(WidgetTree, LOCTEXT("SlotHint", "마법을 누르면 고르고, 칸을 누르면 그 칸에 놓는다(같은 마법이 든 칸을 누르면 비운다)."), 11, FaintInk))
		->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));

	// 속성 필터: 전체 + 설정의 속성들
	FilterRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Box->AddChildToVerticalBox(FilterRow)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	FilterTags.Reset();
	FilterButtons.Reset();
	FilterTags.Add(FGameplayTag());
	for (const FGrimElementDisplay& Display : Settings->Elements)
	{
		FilterTags.Add(Display.Element);
	}
	for (int32 Index = 0; Index < FilterTags.Num(); ++Index)
	{
		UGrimIdButton* Button = WidgetTree->ConstructWidget<UGrimIdButton>(UGrimIdButton::StaticClass());
		Button->Id = Index;
		Button->Bind();
		Button->OnIdClicked.BindUObject(this, &ThisClass::HandleFilterClicked);
		const FText Label = Index == 0 ? LOCTEXT("FilterAll", "전체") : ElementName(FilterTags[Index]);
		Button->SetContent(GrimUIData::MakeText(WidgetTree, Label, 12, InkColor));
		FilterRow->AddChildToHorizontalBox(Button)->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
		FilterButtons.Add(Button);
	}

	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	Box->AddChildToVerticalBox(Scroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	List = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Scroll->AddChild(List);
	return Box;
}

void UGrimGrimoireWidget::SetTab(int32 InTabIndex)
{
	TabIndex = FMath::Clamp(InTabIndex, 0, FMath::Max(TabButtons.Num() - 1, 0));
	if (Pages)
	{
		Pages->SetActiveWidgetIndex(TabIndex);
	}
	RefreshTabButtons();
}

void UGrimGrimoireWidget::HandleTabClicked(int32 InTabIndex)
{
	SetTab(InTabIndex);
}

void UGrimGrimoireWidget::RefreshTabButtons()
{
	for (int32 Index = 0; Index < TabButtons.Num(); ++Index)
	{
		TabButtons[Index]->SetBackgroundColor(Index == TabIndex ? GrimGrimoireLocal::ActiveTab : GrimGrimoireLocal::IdleTab);
	}
}

void UGrimGrimoireWidget::HandleFilterClicked(int32 InFilterIndex)
{
	if (FilterTags.IsValidIndex(InFilterIndex) && InFilterIndex != FilterIndex)
	{
		FilterIndex = InFilterIndex;
		bListDirty = true;
		RefreshFilterButtons();
	}
}

void UGrimGrimoireWidget::RefreshFilterButtons()
{
	for (int32 Index = 0; Index < FilterButtons.Num(); ++Index)
	{
		FLinearColor Color(0.6f, 0.55f, 0.45f);
		if (Index > 0)
		{
			if (const FGrimElementDisplay* Display = UGrimUISettings::Get()->FindElementDisplay(FilterTags[Index]))
			{
				Color = Display->Color;
			}
		}
		Color.A = Index == FilterIndex ? 1.f : 0.35f;
		FilterButtons[Index]->SetBackgroundColor(Color);
	}
}

UGrimSpellbookComponent* UGrimGrimoireWidget::GetSpellbook() const
{
	const APlayerController* PC = GetOwningPlayer();
	const AGrimPlayerState* PlayerState = PC ? PC->GetPlayerState<AGrimPlayerState>() : nullptr;
	return PlayerState ? PlayerState->GetSpellbook() : nullptr;
}

void UGrimGrimoireWidget::HandleRowClicked(int32 KnownIndex)
{
	PickedKnownIndex = KnownIndex;
	UGrimSpellbookComponent* Spellbook = GetSpellbook();
	if (!Spellbook)
	{
		return;
	}
	if (Spellbook->GetOwner()->HasAuthority())
	{
		Spellbook->SelectSpell(KnownIndex);
	}
	else
	{
		Spellbook->Server_SelectSpell(KnownIndex);
	}
}

void UGrimGrimoireWidget::HandleSetClicked(int32 SetIndex)
{
	UGrimSpellbookComponent* Spellbook = GetSpellbook();
	if (!Spellbook)
	{
		return;
	}
	if (Spellbook->GetOwner()->HasAuthority())
	{
		Spellbook->SetActiveSlotSet(SetIndex);
	}
	else
	{
		Spellbook->Server_SetActiveSlotSet(SetIndex);
	}
}

void UGrimGrimoireWidget::HandleSlotClicked(int32 SlotInSet)
{
	UGrimSpellbookComponent* Spellbook = GetSpellbook();
	if (!Spellbook)
	{
		return;
	}
	const int32 SlotIndex = Spellbook->GetActiveSlotSet() * UGrimSpellbookSettings::Get()->SpellSlotsPerSet + SlotInSet;
	const TArray<TObjectPtr<UGrimSpellDefinition>>& Known = Spellbook->GetKnownSpells();
	UGrimSpellDefinition* Picked = Known.IsValidIndex(PickedKnownIndex) ? Known[PickedKnownIndex].Get() : nullptr;
	if (!Picked || !Picked->bTracksProficiency)
	{
		return;
	}
	const TArray<TObjectPtr<UGrimSpellDefinition>>& SlotSpells = Spellbook->GetSlotSpells();
	UGrimSpellDefinition* NewSpell = (SlotSpells.IsValidIndex(SlotIndex) && SlotSpells[SlotIndex] == Picked) ? nullptr : Picked;
	if (Spellbook->GetOwner()->HasAuthority())
	{
		Spellbook->AssignSlot(SlotIndex, NewSpell);
	}
	else
	{
		Spellbook->Server_AssignSlot(SlotIndex, NewSpell);
	}
}

void UGrimGrimoireWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const APlayerController* PC = GetOwningPlayer();
	const AGrimPlayerState* PlayerState = PC ? PC->GetPlayerState<AGrimPlayerState>() : nullptr;
	const UGrimSpellbookComponent* Spellbook = PlayerState ? PlayerState->GetSpellbook() : nullptr;
	const int32 KnownCount = Spellbook ? Spellbook->GetKnownSpells().Num() : INDEX_NONE;
	if (bListDirty || KnownCount != LastKnownCount)
	{
		bListDirty = false;
		LastKnownCount = KnownCount;
		RebuildList(PlayerState);
		RefreshTimer = 0.f;
	}

	RefreshTimer -= InDeltaTime;
	if (RefreshTimer <= 0.f)
	{
		RefreshTimer = 0.1f;
		RefreshHeader(PlayerState);
		RefreshSlotEditor(PlayerState);
		RefreshRows(PlayerState);
	}
}

void UGrimGrimoireWidget::RebuildList(const AGrimPlayerState* PlayerState)
{
	using namespace GrimGrimoireLocal;
	List->ClearChildren();
	Rows.Reset();

	const UGrimSpellbookComponent* Spellbook = PlayerState ? PlayerState->GetSpellbook() : nullptr;
	if (!Spellbook)
	{
		return;
	}
	const TArray<TObjectPtr<UGrimSpellDefinition>>& Known = Spellbook->GetKnownSpells();

	TArray<FGrimGrimoireEntryKey> Keys;
	for (int32 Index = 0; Index < Known.Num(); ++Index)
	{
		FGrimGrimoireEntryKey& Key = Keys.AddDefaulted_GetRef();
		Key.KnownIndex = Index;
		Key.Circle = Known[Index] ? Known[Index]->Circle : 1;
		Key.bTracked = Known[Index] ? Known[Index]->bTracksProficiency : true;
		Key.Element = Known[Index] ? Known[Index]->Element : FGameplayTag();
	}
	const TArray<int32> Order = FGrimUIMath::BuildGrimoireOrder(Keys, FilterTags.IsValidIndex(FilterIndex) ? FilterTags[FilterIndex] : FGameplayTag());

	if (Order.IsEmpty())
	{
		List->AddChildToVerticalBox(GrimUIData::MakeText(WidgetTree, LOCTEXT("Empty", "이 속성의 마법은 아직 없다"), 13, FaintInk));
		return;
	}

	int32 LastGroup = MIN_int32;
	for (const int32 Index : Order)
	{
		const UGrimSpellDefinition* Spell = Known[Index];
		if (!Spell)
		{
			continue;
		}

		// 묶음 머리: 숙련도 없는 항목은 「기본」, 나머지는 서클별
		const int32 Group = Spell->bTracksProficiency ? Spell->Circle : 0;
		if (Group != LastGroup)
		{
			LastGroup = Group;
			const FText Title = UGrimUISettings::Get()->GetCircleTitle(Group);
			const FText Header = Group == 0 ? LOCTEXT("GroupBasic", "기본")
				: (Title.IsEmpty() ? FText::Format(LOCTEXT("GroupCircle", "{0}서클"), Group) : FText::Format(LOCTEXT("GroupCircleTitle", "{0}서클 — {1}"), Group, Title));
			List->AddChildToVerticalBox(GrimUIData::MakeText(WidgetTree, Header, 14, FaintInk, true))->SetPadding(FMargin(0.f, 8.f, 0.f, 4.f));
		}

		FRowWidgets& Row = Rows.AddDefaulted_GetRef();
		Row.KnownIndex = Index;
		Row.Button = WidgetTree->ConstructWidget<UGrimIdButton>(UGrimIdButton::StaticClass());
		Row.Button->Id = Index;
		Row.Button->Bind();
		Row.Button->OnIdClicked.BindUObject(this, &ThisClass::HandleRowClicked);
		Row.Button->SetBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.12f));

		Row.Frame = GrimUIData::MakeBorder(WidgetTree, RowColor, FMargin(2.f));
		Row.Button->SetContent(Row.Frame);
		UBorder* Inner = GrimUIData::MakeBorder(WidgetTree, RowColor, FMargin(8.f, 6.f));
		Row.Frame->SetContent(Inner);
		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Inner->SetContent(Line);

		// 속성색 띠
		Line->AddChildToHorizontalBox(GrimUIData::MakeSizeBox(WidgetTree, GrimUIData::MakeBorder(WidgetTree, GrimUIData::GetElementColor(Spell), FMargin(0.f)), 6.f, 40.f))
			->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));

		// 이름 · 속성 · 서클
		UVerticalBox* NameBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		NameBox->AddChildToVerticalBox(GrimUIData::MakeText(WidgetTree, Spell->DisplayName, 15, InkColor, true));
		NameBox->AddChildToVerticalBox(GrimUIData::MakeText(WidgetTree,
			FText::Format(LOCTEXT("ElementCircle", "{0} · {1}서클"), ElementName(Spell->Element), Spell->Circle), 11, FaintInk));
		Line->AddChildToHorizontalBox(GrimUIData::MakeSizeBox(WidgetTree, NameBox, 170.f, 0.f))->SetVerticalAlignment(VAlign_Center);

		// 등급 글자
		Row.Grade = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 24, InkColor, true);
		Row.Grade->SetJustification(ETextJustify::Center);
		Line->AddChildToHorizontalBox(GrimUIData::MakeSizeBox(WidgetTree, Row.Grade, 46.f, 0.f))->SetVerticalAlignment(VAlign_Center);

		// 등급 이름 · 다음 등급 막대 · 경험
		UVerticalBox* GradeBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Row.GradeName = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 12, InkColor);
		GradeBox->AddChildToVerticalBox(Row.GradeName);
		Row.Bar = GrimUIData::MakeBar(WidgetTree, FLinearColor(0.85f, 0.7f, 0.35f));
		GradeBox->AddChildToVerticalBox(GrimUIData::MakeSizeBox(WidgetTree, Row.Bar, 200.f, 8.f))->SetPadding(FMargin(0.f, 2.f));
		Row.XP = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 10, FaintInk);
		GradeBox->AddChildToVerticalBox(Row.XP);
		Line->AddChildToHorizontalBox(GrimUIData::MakeSizeBox(WidgetTree, GradeBox, 230.f, 0.f))->SetVerticalAlignment(VAlign_Center);

		// 실제 마나 · 쿨타임(완드 · 숙련도 반영)
		Row.Cost = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 12, InkColor);
		Row.Cost->SetAutoWrapText(true);
		Line->AddChildToHorizontalBox(GrimUIData::MakeSizeBox(WidgetTree, Row.Cost, 210.f, 0.f))->SetVerticalAlignment(VAlign_Center);

		Row.Selected = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 12, FLinearColor(1.f, 0.95f, 0.7f), true);
		Line->AddChildToHorizontalBox(Row.Selected)->SetVerticalAlignment(VAlign_Center);

		List->AddChildToVerticalBox(Row.Button)->SetPadding(FMargin(0.f, 2.f));
	}
}

void UGrimGrimoireWidget::RefreshRows(const AGrimPlayerState* PlayerState)
{
	using namespace GrimGrimoireLocal;
	const UGrimSpellbookComponent* Spellbook = PlayerState ? PlayerState->GetSpellbook() : nullptr;
	if (!Spellbook)
	{
		return;
	}
	const AGrimWand* Wand = Spellbook->GetWand();
	const FLinearColor Master = UGrimUISettings::Get()->MasterColor;

	for (FRowWidgets& Row : Rows)
	{
		const UGrimSpellDefinition* Spell = Spellbook->GetKnownSpells().IsValidIndex(Row.KnownIndex) ? Spellbook->GetKnownSpells()[Row.KnownIndex].Get() : nullptr;
		if (!Spell)
		{
			continue;
		}
		const FGrimProficiencyView View = Spellbook->GetProficiency(Spell);

		Row.Grade->SetText(FGrimUIMath::GradeLabel(View.bTracked, View.Grade));
		Row.Grade->SetColorAndOpacity(FSlateColor(View.bMaxGrade ? Master : InkColor));
		Row.Frame->SetBrushColor(View.bMaxGrade ? Master : RowColor);

		if (View.bTracked)
		{
			Row.GradeName->SetText(View.GradeName);
			Row.Bar->SetPercent(View.Progress);
			Row.Bar->SetVisibility(ESlateVisibility::HitTestInvisible);
			Row.XP->SetText(View.bMaxGrade
				? FText::Format(LOCTEXT("XPMax", "경험 {0} — 마스터"), FText::AsNumber(FMath::FloorToInt(View.XP)))
				: FText::Format(LOCTEXT("XPNext", "경험 {0} / {1}"), FText::AsNumber(FMath::FloorToInt(View.XP)), FText::AsNumber(FMath::CeilToInt(View.NextGradeXP))));
		}
		else
		{
			Row.GradeName->SetText(LOCTEXT("NoProficiency", "숙련도 없음"));
			Row.Bar->SetVisibility(ESlateVisibility::Collapsed);
			Row.XP->SetText(FText::GetEmpty());
		}

		const FGrimProficiencyMultipliers Caster = UGrimSpellAbility::GetCasterMultipliers(Spell, PlayerState->GetAbilitySystemComponent());
		const float Mana = UGrimSpellAbility::GetEffectiveManaCost(Spell, Wand, Caster.ManaCost);
		const float Cooldown = UGrimSpellAbility::GetEffectiveCooldown(Spell, Wand, Caster.Cooldown);
		FText CostText = FText::Format(LOCTEXT("Cost", "마나 {0} · 쿨타임 {1}초"), FormatNumber(Mana), FormatNumber(Cooldown));
		if (Spell->bChanneled && Spell->ChannelManaPerSecond > 0.f)
		{
			const float PerSecond = GrimMagicMath::CalcManaCost(Spell->ChannelManaPerSecond, Wand ? Wand->GetManaEfficiency() : 0.f) * Caster.ManaCost;
			CostText = FText::Format(LOCTEXT("CostChannel", "마나 {0} + 초당 {1} · 쿨타임 {2}초"), FormatNumber(Mana), FormatNumber(PerSecond), FormatNumber(Cooldown));
		}
		Row.Cost->SetText(CostText);

		Row.Selected->SetText(Row.KnownIndex == Spellbook->GetSelectedIndex() ? LOCTEXT("SelectedMark", "◀ 지금 마법") : FText::GetEmpty());
		Row.Button->SetBackgroundColor(Row.KnownIndex == PickedKnownIndex ? FLinearColor(1.f, 0.9f, 0.6f, 0.45f) : FLinearColor(1.f, 1.f, 1.f, 0.12f));
	}
}

void UGrimGrimoireWidget::RefreshSlotEditor(const AGrimPlayerState* PlayerState)
{
	using namespace GrimGrimoireLocal;
	const UGrimSpellbookComponent* Spellbook = PlayerState ? PlayerState->GetSpellbook() : nullptr;
	if (!Spellbook)
	{
		return;
	}
	const int32 ActiveSet = Spellbook->GetActiveSlotSet();
	for (int32 Index = 0; Index < SetButtons.Num(); ++Index)
	{
		SetButtons[Index]->SetBackgroundColor(Index == ActiveSet ? ActiveTab : IdleTab);
	}
	const int32 PerSet = UGrimSpellbookSettings::Get()->SpellSlotsPerSet;
	const TArray<TObjectPtr<UGrimSpellDefinition>>& SlotSpells = Spellbook->GetSlotSpells();
	for (int32 Index = 0; Index < SlotLabels.Num(); ++Index)
	{
		const int32 SlotIndex = ActiveSet * PerSet + Index;
		const UGrimSpellDefinition* Spell = SlotSpells.IsValidIndex(SlotIndex) ? SlotSpells[SlotIndex].Get() : nullptr;
		SlotLabels[Index]->SetText(Spell ? FText::Format(LOCTEXT("SlotLabel", "{0}. {1}"), Index + 1, Spell->DisplayName)
			: FText::Format(LOCTEXT("SlotEmpty", "{0}. —"), Index + 1));
		SlotButtons[Index]->SetBackgroundColor(Spell ? GrimUIData::GetElementColor(Spell) : FLinearColor(0.4f, 0.4f, 0.4f, 0.35f));
	}

	const AGrimWand* Wand = Spellbook->GetWand();
	WandText->SetText(Wand
		? FText::Format(LOCTEXT("Wand", "완드 — 마나 절약 {0}% · 시전 속도 ×{1}"), FText::AsNumber(FMath::RoundToInt(Wand->GetManaEfficiency() * 100.f)), FormatNumber(Wand->GetCastSpeed()))
		: LOCTEXT("NoWand", "완드 없음 — 기본 공격만 쓸 수 있다"));
}

void UGrimGrimoireWidget::RefreshHeader(const AGrimPlayerState* PlayerState)
{
	const UAbilitySystemComponent* ASC = PlayerState ? PlayerState->GetAbilitySystemComponent() : nullptr;
	const UGrimProgressionComponent* Progression = PlayerState ? PlayerState->GetProgression() : nullptr;
	if (!ASC || !Progression)
	{
		CircleText->SetText(FText::GetEmpty());
		RequirementText->SetText(FText::GetEmpty());
		return;
	}

	const FGrimCircleCheck Check = Progression->EvaluateNextCircle();
	const FText Title = UGrimUISettings::Get()->GetCircleTitle(Check.CurrentCircle);
	CircleText->SetText(FText::Format(LOCTEXT("Circle", "{0}서클 · {1} — 최대 마나 {2}"), Check.CurrentCircle, Title,
		FText::AsNumber(FMath::RoundToInt(ASC->GetNumericAttribute(UGrimMagicAttributeSet::GetMaxManaAttribute())))));

	if (Check.bAtMax)
	{
		RequirementText->SetText(LOCTEXT("AtMax", "지금 오를 수 있는 가장 높은 서클이다."));
	}
	else
	{
		const FText Progress = FText::Format(LOCTEXT("Requirement", "다음 {0}서클 — 배운 마법 {1}/{2} · 숙련도 등급 합 {3}/{4}"),
			Check.NextCircle, Check.KnownSpells, Check.RequiredKnownSpells, Check.GradeSum, Check.RequiredGradeSum);
		RequirementText->SetText(Check.bMet ? FText::Format(LOCTEXT("RequirementMet", "{0} — 조건을 채웠다, 승급 시험을 볼 수 있다"), Progress) : Progress);
	}
}

#undef LOCTEXT_NAMESPACE
