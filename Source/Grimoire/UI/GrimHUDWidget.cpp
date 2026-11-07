#include "UI/GrimHUDWidget.h"
#include "UI/GrimUIData.h"
#include "UI/GrimUIMath.h"
#include "UI/GrimUISettings.h"
#include "Character/GrimPlayerCharacter.h"
#include "Magic/GrimBasicAttackAbility.h"
#include "Magic/GrimBasicAttackTypes.h"
#include "Magic/GrimCastFailure.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "Magic/GrimProficiencySettings.h"
#include "Magic/GrimSpellSlotInput.h"
#include "Magic/GrimProgressionComponent.h"
#include "Magic/GrimSkillbook.h"
#include "Magic/GrimSpellDefinition.h"
#include "Core/GrimAbilitySystemComponent.h"
#include "Core/GrimPlayerState.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "GrimHUD"

namespace GrimHUDLocal
{
	constexpr float SlotSize = 68.f;
	const FLinearColor PanelColor(0.02f, 0.02f, 0.03f, 0.55f);
	const FLinearColor SelectedColor(1.f, 0.95f, 0.8f, 1.f);

	UCanvasPanelSlot* Place(UCanvasPanel* Canvas, UWidget* Widget, const FVector2D& Anchor, const FVector2D& Alignment, const FVector2D& Position)
	{
		UCanvasPanelSlot* SlotW = Canvas->AddChildToCanvas(Widget);
		SlotW->SetAnchors(FAnchors(Anchor.X, Anchor.Y));
		SlotW->SetAlignment(Alignment);
		SlotW->SetPosition(Position);
		SlotW->SetAutoSize(true);
		return SlotW;
	}

	FLinearColor Darken(const FLinearColor& Color, float Scale, float Alpha)
	{
		return FLinearColor(Color.R * Scale, Color.G * Scale, Color.B * Scale, Alpha);
	}
}

TSharedRef<SWidget> UGrimHUDWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

void UGrimHUDWidget::BuildTree()
{
	using namespace GrimHUDLocal;
	const UGrimUISettings* Settings = UGrimUISettings::Get();

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Canvas;

	// ── 왼쪽 아래: 서클 + 막대 ──
	UBorder* VitalPanel = GrimUIData::MakeBorder(WidgetTree, PanelColor, FMargin(10.f, 8.f));
	UVerticalBox* VitalBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	VitalPanel->SetContent(VitalBox);
	CircleText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 13, FLinearColor(0.95f, 0.88f, 0.7f), true);
	VitalBox->AddChildToVerticalBox(CircleText)->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));

	for (const FGrimVitalBarConfig& Config : Settings->VitalBars)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Row->AddChildToHorizontalBox(GrimUIData::MakeSizeBox(WidgetTree, GrimUIData::MakeText(WidgetTree, Config.Label, 11), 56.f, 0.f))->SetVerticalAlignment(VAlign_Center);
		UProgressBar* Bar = GrimUIData::MakeBar(WidgetTree, Config.Color);
		UHorizontalBoxSlot* BarSlot = Row->AddChildToHorizontalBox(GrimUIData::MakeSizeBox(WidgetTree, Bar, 200.f, 12.f));
		BarSlot->SetVerticalAlignment(VAlign_Center);
		UTextBlock* Value = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 11);
		Row->AddChildToHorizontalBox(Value)->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
		VitalBox->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, 2.f));
		Row->SetVisibility(ESlateVisibility::Collapsed);

		FBarWidgets& Widgets = Bars.AddDefaulted_GetRef();
		Widgets.Row = Row;
		Widgets.Bar = Bar;
		Widgets.Value = Value;
	}
	Place(Canvas, VitalPanel, FVector2D(0.f, 1.f), FVector2D(0.f, 1.f), FVector2D(24.f, -24.f));

	// ── 가운데 아래: 기본 공격 자리 + 슬롯 ──
	UVerticalBox* SlotColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	SetText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 10, FLinearColor(0.8f, 0.8f, 0.8f));
	SlotColumn->AddChildToVerticalBox(SetText)->SetHorizontalAlignment(HAlign_Right);
	UHorizontalBox* SlotRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	SlotColumn->AddChildToVerticalBox(SlotRow);

	// 키 글자(호그와트 배치 — 키보드 · 패드, 아이콘 대신 글자): UGrimSpellInputSettings
	const UGrimSpellInputSettings* InputSettings = UGrimSpellInputSettings::Get();
	if (Settings->bSeparateBasicAttackSlot)
	{
		BasicSlot = MakeSlot(FText::FromString(InputSettings->GetBasicAttackLabel()));
		SlotRow->AddChildToHorizontalBox(BasicSlot.Root)->SetPadding(FMargin(0.f, 0.f, 14.f, 0.f));
		BasicSlot.Root->SetVisibility(ESlateVisibility::Collapsed);
	}
	for (int32 Index = 0; Index < UGrimSpellbookSettings::Get()->SpellSlotsPerSet; ++Index)
	{
		FSlotWidgets SlotW = MakeSlot(FText::FromString(InputSettings->GetSlotLabel(Index)));
		SlotRow->AddChildToHorizontalBox(SlotW.Root)->SetPadding(FMargin(3.f, 0.f));
		Slots.Add(SlotW);
	}
	Place(Canvas, SlotColumn, FVector2D(0.5f, 1.f), FVector2D(0.5f, 1.f), FVector2D(0.f, -24.f));

	// ── 알림 · 안내 ──
	ToastText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 20, FLinearColor::White, true);
	ToastText->SetJustification(ETextJustify::Center);
	ToastText->SetVisibility(ESlateVisibility::Collapsed);
	Place(Canvas, ToastText, FVector2D(0.5f, 0.f), FVector2D(0.5f, 0.f), FVector2D(0.f, 90.f));

	PromptText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 16, FLinearColor(1.f, 0.95f, 0.8f), true);
	PromptText->SetJustification(ETextJustify::Center);
	PromptText->SetVisibility(ESlateVisibility::Collapsed);
	Place(Canvas, PromptText, FVector2D(0.5f, 1.f), FVector2D(0.5f, 1.f), FVector2D(0.f, -140.f));

	SetVisibility(ESlateVisibility::HitTestInvisible);
}

UGrimHUDWidget::FSlotWidgets UGrimHUDWidget::MakeSlot(const FText& KeyLabel)
{
	using namespace GrimHUDLocal;
	FSlotWidgets SlotW;
	SlotW.Frame = GrimUIData::MakeBorder(WidgetTree, PanelColor, FMargin(2.f));
	SlotW.Inner = GrimUIData::MakeBorder(WidgetTree, PanelColor, FMargin(0.f));
	SlotW.Frame->SetContent(SlotW.Inner);

	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	SlotW.Inner->SetContent(Overlay);

	SlotW.Name = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 12, FLinearColor::White, true);
	SlotW.Name->SetJustification(ETextJustify::Center);
	UOverlaySlot* NameSlot = Overlay->AddChildToOverlay(SlotW.Name);
	NameSlot->SetHorizontalAlignment(HAlign_Center);
	NameSlot->SetVerticalAlignment(VAlign_Center);

	SlotW.Key = GrimUIData::MakeText(WidgetTree, KeyLabel, 8, FLinearColor(0.85f, 0.85f, 0.85f));
	UOverlaySlot* KeySlot = Overlay->AddChildToOverlay(SlotW.Key);
	KeySlot->SetHorizontalAlignment(HAlign_Left);
	KeySlot->SetVerticalAlignment(VAlign_Top);
	KeySlot->SetPadding(FMargin(3.f, 1.f));

	SlotW.Grade = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 13, FLinearColor::White, true);
	UOverlaySlot* GradeSlot = Overlay->AddChildToOverlay(SlotW.Grade);
	GradeSlot->SetHorizontalAlignment(HAlign_Right);
	GradeSlot->SetVerticalAlignment(VAlign_Bottom);
	GradeSlot->SetPadding(FMargin(4.f, 1.f));

	// 쿨다운: 아래에서 위로 채워진 어두운 막(남은 비율).
	SlotW.Cooldown = GrimUIData::MakeBar(WidgetTree, FLinearColor(0.f, 0.f, 0.f, 0.65f));
	SlotW.Cooldown->SetBarFillType(EProgressBarFillType::BottomToTop);
	FProgressBarStyle Style = SlotW.Cooldown->GetWidgetStyle();
	Style.BackgroundImage.TintColor = FSlateColor(FLinearColor::Transparent);
	SlotW.Cooldown->SetWidgetStyle(Style);
	UOverlaySlot* CooldownSlot = Overlay->AddChildToOverlay(SlotW.Cooldown);
	CooldownSlot->SetHorizontalAlignment(HAlign_Fill);
	CooldownSlot->SetVerticalAlignment(VAlign_Fill);

	SlotW.CooldownText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 16, FLinearColor::White, true);
	UOverlaySlot* CooldownTextSlot = Overlay->AddChildToOverlay(SlotW.CooldownText);
	CooldownTextSlot->SetHorizontalAlignment(HAlign_Center);
	CooldownTextSlot->SetVerticalAlignment(VAlign_Center);

	SlotW.Root = GrimUIData::MakeSizeBox(WidgetTree, SlotW.Frame, SlotSize, SlotSize);
	return SlotW;
}

void UGrimHUDWidget::NativeDestruct()
{
	BindPlayerState(nullptr);
	Super::NativeDestruct();
}

void UGrimHUDWidget::BindPlayerState(AGrimPlayerState* PlayerState)
{
	if (BoundPlayerState.Get() == PlayerState)
	{
		return;
	}
	if (AGrimPlayerState* Old = BoundPlayerState.Get())
	{
		if (UGrimSpellbookComponent* Spellbook = Old->GetSpellbook())
		{
			Spellbook->OnProficiencyGradeUp.RemoveDynamic(this, &ThisClass::HandleGradeUp);
			Spellbook->OnLearnResult.RemoveDynamic(this, &ThisClass::HandleLearnResult);
		}
		if (UGrimProgressionComponent* Progression = Old->GetProgression())
		{
			Progression->OnCircleUpResult.RemoveDynamic(this, &ThisClass::HandleCircleUpResult);
		}
		if (UGrimAbilitySystemComponent* OldASC = Cast<UGrimAbilitySystemComponent>(Old->GetAbilitySystemComponent()))
		{
			OldASC->OnCastFailedNotice.Remove(CastFailedHandle);
		}
		CastFailedHandle.Reset();
	}
	BoundPlayerState = PlayerState;
	ResolvedASC = nullptr;
	if (PlayerState)
	{
		if (UGrimSpellbookComponent* Spellbook = PlayerState->GetSpellbook())
		{
			Spellbook->OnProficiencyGradeUp.AddUniqueDynamic(this, &ThisClass::HandleGradeUp);
			Spellbook->OnLearnResult.AddUniqueDynamic(this, &ThisClass::HandleLearnResult);
		}
		if (UGrimProgressionComponent* Progression = PlayerState->GetProgression())
		{
			Progression->OnCircleUpResult.AddUniqueDynamic(this, &ThisClass::HandleCircleUpResult);
		}
		if (UGrimAbilitySystemComponent* GrimASC = Cast<UGrimAbilitySystemComponent>(PlayerState->GetAbilitySystemComponent()))
		{
			CastFailedHandle = GrimASC->OnCastFailedNotice.AddUObject(this, &ThisClass::HandleCastFailed);
		}
	}
}

void UGrimHUDWidget::HandleCastFailed(uint8 Reason, int32 Param)
{
	// 마법 실패(Q64) — 서버가 정한 까닭을 위쪽 알림 한 줄로.
	const FText Text = GrimCastFailure::GetMessage(static_cast<EGrimCastFailReason>(Reason), Param);
	if (!Text.IsEmpty())
	{
		ShowToast(Text, UGrimUISettings::Get()->CastFailColor);
	}
}

void UGrimHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const APlayerController* PC = GetOwningPlayer();
	AGrimPlayerState* PlayerState = PC ? PC->GetPlayerState<AGrimPlayerState>() : nullptr;
	BindPlayerState(PlayerState);
	UAbilitySystemComponent* ASC = PlayerState ? PlayerState->GetAbilitySystemComponent() : nullptr;
	const UGrimSpellbookComponent* Spellbook = PlayerState ? PlayerState->GetSpellbook() : nullptr;

	RefreshVitals(ASC);
	RefreshSlots(ASC, Spellbook);

	PromptTimer -= InDeltaTime;
	if (PromptTimer <= 0.f)
	{
		PromptTimer = 0.2f;
		RefreshPrompt(Spellbook);
	}

	if (ToastRemaining > 0.f)
	{
		ToastRemaining -= InDeltaTime;
		const float Alpha = FMath::Clamp(ToastRemaining / 0.5f, 0.f, 1.f);
		ToastText->SetRenderOpacity(Alpha);
		if (ToastRemaining <= 0.f)
		{
			ToastText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UGrimHUDWidget::RefreshVitals(UAbilitySystemComponent* ASC)
{
	const UGrimUISettings* Settings = UGrimUISettings::Get();

	// 어트리뷰트는 ASC 가 바뀔 때, 못 찾은 것은 1초마다 다시 찾는다(다른 갈래가 셋을 더해도 따라간다).
	ResolveTimer -= GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.f;
	const bool bNewASC = ResolvedASC.Get() != ASC;
	if (bNewASC || ResolveTimer <= 0.f)
	{
		ResolveTimer = 1.f;
		ResolvedASC = ASC;
		for (int32 Index = 0; Index < Bars.Num() && Index < Settings->VitalBars.Num(); ++Index)
		{
			FBarWidgets& Widgets = Bars[Index];
			if (Widgets.bResolved && !bNewASC)
			{
				continue;
			}
			const FGrimVitalBarConfig& Config = Settings->VitalBars[Index];
			Widgets.bResolved = GrimUIData::FindAttribute(ASC, Config.Attribute, Widgets.Attribute);
			if (!Widgets.bResolved || Config.MaxAttribute.IsEmpty() || !GrimUIData::FindAttribute(ASC, Config.MaxAttribute, Widgets.MaxAttribute))
			{
				Widgets.MaxAttribute = FGameplayAttribute();
			}
			Widgets.Row->SetVisibility(Widgets.bResolved ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	}

	if (!ASC)
	{
		CircleText->SetText(FText::GetEmpty());
		return;
	}

	const int32 Circle = FMath::RoundToInt(ASC->GetNumericAttribute(UGrimMagicAttributeSet::GetCircleAttribute()));
	const FText Title = Settings->GetCircleTitle(Circle);
	CircleText->SetText(Title.IsEmpty()
		? FText::Format(LOCTEXT("CircleOnly", "{0}서클"), Circle)
		: FText::Format(LOCTEXT("CircleTitle", "{0}서클 · {1}"), Circle, Title));

	FNumberFormattingOptions Format;
	for (int32 Index = 0; Index < Bars.Num() && Index < Settings->VitalBars.Num(); ++Index)
	{
		FBarWidgets& Widgets = Bars[Index];
		if (!Widgets.bResolved)
		{
			continue;
		}
		const FGrimVitalBarConfig& Config = Settings->VitalBars[Index];
		const float Value = ASC->GetNumericAttribute(Widgets.Attribute);
		const float Max = Widgets.MaxAttribute.IsValid() ? ASC->GetNumericAttribute(Widgets.MaxAttribute) : Config.MaxValue;
		Widgets.Bar->SetPercent(FGrimUIMath::BarFraction(Value, Config.MinValue, Max));

		Format.SetMinimumFractionalDigits(Config.Decimals).SetMaximumFractionalDigits(Config.Decimals);
		const FText ValueText = FText::AsNumber(Value, &Format);
		Widgets.Value->SetText(Widgets.MaxAttribute.IsValid()
			? FText::Format(LOCTEXT("ValueMax", "{0} / {1}"), ValueText, FText::AsNumber(FMath::RoundToInt(Max)))
			: FText::Format(LOCTEXT("ValueUnit", "{0}{1}"), ValueText, FText::FromString(Config.Unit)));
	}
}

void UGrimHUDWidget::FillSlot(FSlotWidgets& SlotW, UAbilitySystemComponent* ASC, const UGrimSpellbookComponent* Spellbook, const UGrimSpellDefinition* Spell, bool bSelected)
{
	using namespace GrimHUDLocal;
	if (!Spell)
	{
		SlotW.Frame->SetBrushColor(PanelColor);
		SlotW.Inner->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.25f));
		SlotW.Name->SetText(FText::GetEmpty());
		SlotW.Grade->SetText(FText::GetEmpty());
		SlotW.Cooldown->SetPercent(0.f);
		SlotW.CooldownText->SetText(FText::GetEmpty());
		return;
	}

	const FLinearColor ElementColor = GrimUIData::GetElementColor(Spell);
	SlotW.Frame->SetBrushColor(bSelected ? SelectedColor : Darken(ElementColor, 0.6f, 0.9f));
	SlotW.Inner->SetBrushColor(Darken(ElementColor, bSelected ? 0.55f : 0.3f, 0.9f));
	SlotW.Name->SetText(Spell->DisplayName);

	const FGrimProficiencyView View = Spellbook ? Spellbook->GetProficiency(Spell) : FGrimProficiencyView();
	SlotW.Grade->SetText(FGrimUIMath::GradeLabel(View.bTracked, View.Grade));
	SlotW.Grade->SetColorAndOpacity(FSlateColor(View.bMaxGrade ? UGrimUISettings::Get()->MasterColor : FLinearColor::White));

	float Remaining = 0.f, Duration = 0.f;
	GrimUIData::GetSpellCooldown(ASC, Spell, Remaining, Duration);
	SlotW.Cooldown->SetPercent(FGrimUIMath::CooldownFraction(Remaining, Duration));
	SlotW.CooldownText->SetText(FGrimUIMath::FormatCooldown(Remaining));
}

void UGrimHUDWidget::RefreshSlots(UAbilitySystemComponent* ASC, const UGrimSpellbookComponent* Spellbook)
{
	const UGrimUISettings* Settings = UGrimUISettings::Get();
	const UGrimSpellbookSettings* SlotSettings = UGrimSpellbookSettings::Get();
	TArray<bool> Tracked;
	TArray<int32> SlotKnown;
	if (Spellbook)
	{
		const TArray<TObjectPtr<UGrimSpellDefinition>>& Known = Spellbook->GetKnownSpells();
		for (const UGrimSpellDefinition* Spell : Known)
		{
			Tracked.Add(Spell ? Spell->bTracksProficiency : true);
		}
		for (const UGrimSpellDefinition* Spell : Spellbook->GetSlotSpells())
		{
			SlotKnown.Add(Spell ? Known.IndexOfByKey(Spell) : INDEX_NONE);
		}
	}
	const FGrimSlotLayout Layout = FGrimUIMath::ComputeSlotLayout(SlotKnown, Tracked, Spellbook ? Spellbook->GetActiveSlotSet() : 0,
		SlotSettings->SpellSlotsPerSet, Spellbook ? Spellbook->GetUnlockedSlotSets() : SlotSettings->SpellSlotSets, Settings->bSeparateBasicAttackSlot);

	auto SpellAt = [Spellbook](int32 Index) -> const UGrimSpellDefinition*
	{
		return Spellbook && Spellbook->GetKnownSpells().IsValidIndex(Index) ? Spellbook->GetKnownSpells()[Index].Get() : nullptr;
	};

	// 고른 마법 하나만 시전하던 방식은 없어졌다(슬롯 바로 시전) — 패드에서 세트를 활성(R2/RT 누름)하면 칸 테두리가 밝아진다.
	const AGrimPlayerCharacter* Character = Cast<AGrimPlayerCharacter>(GetOwningPlayerPawn());
	const bool bSetHeld = Character && Character->IsSpellSetHeld();
	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		const int32 SpellIndex = Layout.Slots.IsValidIndex(SlotIndex) ? Layout.Slots[SlotIndex] : INDEX_NONE;
		FillSlot(Slots[SlotIndex], ASC, Spellbook, SpellAt(SpellIndex), bSetHeld && SpellIndex != INDEX_NONE);
	}

	if (BasicSlot.Root)
	{
		const UGrimSpellDefinition* Basic = SpellAt(Layout.BasicIndex);
		BasicSlot.Root->SetVisibility(Basic ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (Basic)
		{
			FillSlot(BasicSlot, ASC, Spellbook, Basic, false);
			// 연타(R26): 이어 친 번째 「2/4」 — 숙련도 글자 자리(기본 공격은 숙련도가 없다).
			const int32 Step = UGrimBasicAttackAbility::GetVisibleComboStep(ASC);
			BasicSlot.Grade->SetText(Step > 0
				? FText::Format(LOCTEXT("ComboStep", "{0}/{1}"), Step, UGrimBasicAttackSettings::Get()->ComboLength)
				: FText::GetEmpty());
		}
	}

	SetText->SetText(Layout.Sets > 1
		? FText::Format(LOCTEXT("SlotSet", "세트 {0}/{1}"), Layout.SetIndex + 1, Layout.Sets)
		: FText::GetEmpty());
}

void UGrimHUDWidget::RefreshPrompt(const UGrimSpellbookComponent* Spellbook)
{
	const APlayerController* PC = GetOwningPlayer();
	const AGrimSkillbook* Book = PC ? AGrimSkillbook::FindNearestReadable(PC->GetPawn()) : nullptr;
	const UGrimSpellDefinition* Spell = Book ? Book->GetTeachesSpell() : nullptr;
	if (!Spell || !Spellbook)
	{
		PromptText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	FText Text;
	switch (Spellbook->PreviewSkillbook(Spell))
	{
	case EGrimLearnResult::Learned:
		Text = FText::Format(LOCTEXT("PromptLearn", "스킬북 「{0}」 ({1}서클) — 집으면 배운다"), Spell->DisplayName, Spell->Circle);
		break;
	case EGrimLearnResult::ProficiencyGained:
		Text = FText::Format(LOCTEXT("PromptXP", "스킬북 「{0}」 — 이미 아는 마법, 집으면 숙련도가 오른다"), Spell->DisplayName);
		break;
	case EGrimLearnResult::AlreadyMastered:
		Text = FText::Format(LOCTEXT("PromptMastered", "스킬북 「{0}」 — 이미 통달한 마법"), Spell->DisplayName);
		break;
	case EGrimLearnResult::CircleTooLow:
		Text = Spellbook->IsSealed(Spell)
			? FText::Format(LOCTEXT("PromptCircle", "스킬북 「{0}」 — {1}서클이 되어야 읽을 수 있다(이미 봉인된 책으로 기록됨)"), Spell->DisplayName, Spell->Circle)
			: FText::Format(LOCTEXT("PromptSeal", "스킬북 「{0}」 ({1}서클) — 집으면 「봉인된 책」으로 그리모어에 기록한다"), Spell->DisplayName, Spell->Circle);
		break;
	default:
		Text = FText::Format(LOCTEXT("PromptKnown", "스킬북 「{0}」"), Spell->DisplayName);
		break;
	}
	PromptText->SetText(Text);
	PromptText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UGrimHUDWidget::ShowToast(const FText& Text, const FLinearColor& Color)
{
	if (!ToastText)
	{
		return;
	}
	ToastText->SetText(Text);
	ToastText->SetColorAndOpacity(FSlateColor(Color));
	ToastText->SetRenderOpacity(1.f);
	ToastText->SetVisibility(ESlateVisibility::HitTestInvisible);
	ToastRemaining = UGrimUISettings::Get()->ToastSeconds;
}

void UGrimHUDWidget::HandleGradeUp(UGrimSpellbookComponent* Spellbook, const UGrimSpellDefinition* Spell, int32 GradeIndex)
{
	if (!Spellbook || !Spell)
	{
		return;
	}
	const FGrimProficiencyView View = Spellbook->GetProficiency(Spell);
	ShowToast(FText::Format(LOCTEXT("GradeUp", "{0} 숙련도 {1} — {2}"), Spell->DisplayName, FText::FromName(View.Grade), View.GradeName),
		View.bMaxGrade ? UGrimUISettings::Get()->MasterColor : FLinearColor::White);
}

void UGrimHUDWidget::HandleLearnResult(UGrimSpellbookComponent* Spellbook, const UGrimSpellDefinition* Spell, EGrimLearnResult Result)
{
	const FText Name = Spell ? Spell->DisplayName : FText::GetEmpty();
	switch (Result)
	{
	case EGrimLearnResult::Learned:
		ShowToast(FText::Format(LOCTEXT("Learned", "새 마법 「{0}」 — 그리모어에 기록되었다"), Name), FLinearColor(0.75f, 0.95f, 1.f));
		break;
	case EGrimLearnResult::ProficiencyGained:
		ShowToast(FText::Format(LOCTEXT("SkillbookXP", "「{0}」 스킬북을 읽었다 — 숙련도가 올랐다"), Name), FLinearColor(0.75f, 0.95f, 1.f));
		break;
	case EGrimLearnResult::AlreadyMastered:
		ShowToast(FText::Format(LOCTEXT("AlreadyMastered", "「{0}」은(는) 이미 통달한 마법이다"), Name), UGrimUISettings::Get()->MasterColor);
		break;
	case EGrimLearnResult::AlreadyKnown:
		ShowToast(FText::Format(LOCTEXT("AlreadyKnown", "「{0}」은(는) 이미 아는 마법이다"), Name));
		break;
	case EGrimLearnResult::CircleTooLow:
		ShowToast(FText::Format(LOCTEXT("CircleTooLow", "{0}서클이 되어야 읽을 수 있다"), Spell ? Spell->Circle : 0), FLinearColor(1.f, 0.6f, 0.5f));
		break;
	case EGrimLearnResult::TooFar:
		ShowToast(LOCTEXT("TooFar", "책에서 너무 멀다"));
		break;
	case EGrimLearnResult::Sealed:
		ShowToast(FText::Format(LOCTEXT("Sealed", "봉인된 책 「{0}」 ({1}서클) — 그리모어에 기록했다. 그 서클에 오르면 저절로 읽는다"), Name, Spell ? Spell->Circle : 0),
			FLinearColor(0.85f, 0.8f, 1.f));
		break;
	default:
		break;
	}
}

void UGrimHUDWidget::HandleCircleUpResult(UGrimProgressionComponent* Progression, int32 Circle, bool bSuccess)
{
	if (bSuccess)
	{
		const FText Title = UGrimUISettings::Get()->GetCircleTitle(Circle);
		ShowToast(FText::Format(LOCTEXT("CircleUp", "{0}서클에 올랐다 · {1}"), Circle, Title), UGrimUISettings::Get()->MasterColor);
	}
	else
	{
		ShowToast(LOCTEXT("CircleUpFail", "아직 다음 서클의 자격이 모자라다 — 그리모어 「서클」 탭에서 확인"), FLinearColor(1.f, 0.6f, 0.5f));
	}
}

#undef LOCTEXT_NAMESPACE
