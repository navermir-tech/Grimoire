#include "Appearance/GrimCharacterCreatorWidget.h"
#include "Appearance/GrimAppearanceComponent.h"
#include "Appearance/GrimAppearanceSettings.h"
#include "Appearance/GrimPlayerAppearanceComponent.h"
#include "UI/GrimUIData.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

#define LOCTEXT_NAMESPACE "GrimCharacterCreator"

namespace GrimCharacterCreatorLocal // 유니티 빌드에서 이름이 부딪히지 않게
{
	const FLinearColor PanelColor(0.08f, 0.07f, 0.05f, 0.94f);
	const FLinearColor Ink(0.95f, 0.90f, 0.78f, 1.f);
	const FLinearColor Faint(0.72f, 0.68f, 0.60f, 1.f);
	const FLinearColor Gold(1.f, 0.85f, 0.45f, 1.f);
	const FLinearColor ButtonColor(0.30f, 0.26f, 0.20f, 1.f);

	enum : int32
	{
		B_Random = 1, B_Confirm, B_Cancel,
		B_RowBase = 100 // 줄 i: ◀ = 100 + 2i, ▶ = 101 + 2i
	};

	TWeakObjectPtr<UGrimCharacterCreatorWidget> GOpenWidget;

	UGrimIdButton* MakeButton(UWidgetTree* Tree, const FText& Text, int32 Id, int32 FontSize = 14)
	{
		UGrimIdButton* Button = Tree->ConstructWidget<UGrimIdButton>(UGrimIdButton::StaticClass());
		Button->Id = Id;
		Button->Bind();
		Button->SetBackgroundColor(ButtonColor);
		UTextBlock* Label = GrimUIData::MakeText(Tree, Text, FontSize, Ink);
		Label->SetJustification(ETextJustify::Center);
		Button->SetContent(Label);
		return Button;
	}

	UHorizontalBox* MakeLabeled(UWidgetTree* Tree, const FText& Label, UWidget* Widget, float LabelWidth = 110.f)
	{
		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UTextBlock* Text = GrimUIData::MakeText(Tree, Label, 13, Faint);
		Row->AddChildToHorizontalBox(GrimUIData::MakeSizeBox(Tree, Text, LabelWidth, 0.f))->SetVerticalAlignment(VAlign_Center);
		UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(Widget);
		Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		Slot->SetVerticalAlignment(VAlign_Center);
		return Row;
	}

	void AddRow(UVerticalBox* Box, UWidget* Widget, float Top = 4.f)
	{
		UVerticalBoxSlot* Slot = Box->AddChildToVerticalBox(Widget);
		Slot->SetPadding(FMargin(0.f, Top, 0.f, 0.f));
		Slot->SetHorizontalAlignment(HAlign_Fill);
	}

	FText ColorName(FName Palette, uint8 Index)
	{
		const TArray<FGrimAppearanceColor>& Colors = UGrimAppearanceSettings::GetCatalog().GetPalette(Palette);
		return Colors.IsValidIndex(Index) ? FText::FromString(Colors[Index].DisplayName) : FText::AsNumber(Index);
	}
}

UGrimCharacterCreatorWidget* UGrimCharacterCreatorWidget::Open(APlayerController* Controller)
{
	using namespace GrimCharacterCreatorLocal;
	if (!Controller || !Controller->IsLocalController())
	{
		return nullptr;
	}
	if (GOpenWidget.IsValid() && GOpenWidget->IsInViewport())
	{
		return GOpenWidget.Get();
	}
	UGrimCharacterCreatorWidget* Widget = CreateWidget<UGrimCharacterCreatorWidget>(Controller, UGrimCharacterCreatorWidget::StaticClass());
	if (!Widget)
	{
		return nullptr;
	}
	Widget->SetIsFocusable(true);
	Widget->AddToViewport(50);
	GOpenWidget = Widget;

	FInputModeGameAndUI Mode;
	Mode.SetWidgetToFocus(Widget->TakeWidget());
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Mode.SetHideCursorDuringCapture(false);
	Controller->SetInputMode(Mode);
	Controller->SetShowMouseCursor(true);
	Controller->SetIgnoreLookInput(true);
	Controller->SetIgnoreMoveInput(true);
	return Widget;
}

void UGrimCharacterCreatorWidget::Close(bool bConfirm)
{
	if (bClosed)
	{
		return;
	}
	bClosed = true;
	APlayerController* Controller = GetOwningPlayer();
	UGrimAppearanceComponent* Pawn = GetPawnAppearance();
	if (bConfirm)
	{
		if (Pawn)
		{
			Pawn->EndPreviewOnNextReplication();
		}
		if (UGrimPlayerAppearanceComponent* Player = UGrimPlayerAppearanceComponent::FindForController(Controller))
		{
			Player->SubmitAppearance(Working);
		}
	}
	else if (Pawn)
	{
		Pawn->ClearPreview();
	}
	if (Controller)
	{
		Controller->SetInputMode(FInputModeGameOnly());
		Controller->SetShowMouseCursor(false);
		Controller->SetIgnoreLookInput(false);
		Controller->SetIgnoreMoveInput(false);
	}
	RemoveFromParent();
}

UGrimAppearanceComponent* UGrimCharacterCreatorWidget::GetPawnAppearance() const
{
	const APlayerController* Controller = GetOwningPlayer();
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	return Pawn ? Pawn->FindComponentByClass<UGrimAppearanceComponent>() : nullptr;
}

TSharedRef<SWidget> UGrimCharacterCreatorWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

void UGrimCharacterCreatorWidget::NativeConstruct()
{
	Super::NativeConstruct();
	const FGrimAppearanceCatalog& Catalog = UGrimAppearanceSettings::GetCatalog();
	const UGrimAppearanceComponent* Pawn = GetPawnAppearance();
	Working = Pawn ? Pawn->GetDisplayedAppearance() : FGrimAppearance();
	if (!Working.IsSet())
	{
		FGrimAppearanceMath::Sanitize(Working, Catalog, true);
		FGrimAppearanceMath::ApplyFaceDefaults(Working, Catalog);
	}
	ApplyWorking();
}

void UGrimCharacterCreatorWidget::NativeDestruct()
{
	if (!bClosed)
	{
		if (UGrimAppearanceComponent* Pawn = GetPawnAppearance())
		{
			Pawn->ClearPreview();
		}
	}
	Super::NativeDestruct();
}

FReply UGrimCharacterCreatorWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape || InKeyEvent.GetKey() == EKeys::Gamepad_FaceButton_Right)
	{
		Close(false);
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UGrimCharacterCreatorWidget::BuildTree()
{
	using namespace GrimCharacterCreatorLocal;
	const FGrimAppearanceCatalog& Catalog = UGrimAppearanceSettings::GetCatalog();

	UHorizontalBox* Root = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;
	UBorder* Panel = GrimUIData::MakeBorder(WidgetTree, PanelColor, FMargin(16.f));
	UHorizontalBoxSlot* PanelSlot = Root->AddChildToHorizontalBox(GrimUIData::MakeSizeBox(WidgetTree, Panel, 460.f, 0.f));
	PanelSlot->SetPadding(FMargin(24.f));
	PanelSlot->SetVerticalAlignment(VAlign_Fill);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->SetContent(Column);
	AddRow(Column, GrimUIData::MakeText(WidgetTree, LOCTEXT("Title", "캐릭터 만들기"), 20, Gold, true), 0.f);
	AddRow(Column, GrimUIData::MakeText(WidgetTree, LOCTEXT("Hint", "고르는 동안은 내 화면에만 보인다. 확인하면 모두에게. 서클 고리는 서클을 따라 저절로 붙는다."), 11, Faint));

	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	UVerticalBoxSlot* ScrollSlot = Column->AddChildToVerticalBox(Scroll);
	ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	ScrollSlot->SetPadding(FMargin(0.f, 10.f, 0.f, 10.f));
	UVerticalBox* List = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Scroll->AddChild(List);

	auto AddCycle = [this, List](const FText& Label, ERow Kind, EGrimAppearanceSlot PartSlot = EGrimAppearanceSlot::Face)
	{
		const int32 RowIndex = Rows.Num();
		Rows.Add({ Kind, PartSlot });
		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Line->AddChildToHorizontalBox(GrimUIData::MakeSizeBox(WidgetTree, MakeButton(WidgetTree, LOCTEXT("Prev", "◀"), B_RowBase + RowIndex * 2), 34.f, 0.f));
		UTextBlock* Value = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 13, Ink);
		Value->SetJustification(ETextJustify::Center);
		UHorizontalBoxSlot* ValueSlot = Line->AddChildToHorizontalBox(Value);
		ValueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ValueSlot->SetVerticalAlignment(VAlign_Center);
		Line->AddChildToHorizontalBox(GrimUIData::MakeSizeBox(WidgetTree, MakeButton(WidgetTree, LOCTEXT("Next", "▶"), B_RowBase + RowIndex * 2 + 1), 34.f, 0.f));
		RowValues.Add(Value);
		AddRow(List, MakeLabeled(WidgetTree, Label, Line));
	};
	auto AddSlider = [this, List](const FText& Label, float Min, float Max) -> USlider*
	{
		USlider* Slider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass());
		Slider->SetMinValue(Min);
		Slider->SetMaxValue(Max);
		Slider->SetStepSize(0.01f);
		Slider->OnValueChanged.AddDynamic(this, &UGrimCharacterCreatorWidget::HandleSliderChanged);
		AddRow(List, MakeLabeled(WidgetTree, Label, Slider));
		return Slider;
	};
	auto AddSection = [this, List](const FText& Label)
	{
		AddRow(List, GrimUIData::MakeText(WidgetTree, Label, 14, Gold, true), 12.f);
	};

	AddSection(LOCTEXT("SecBody", "몸"));
	AddCycle(LOCTEXT("Body", "체형"), ERow::Body);
	HeightSlider = AddSlider(LOCTEXT("Height", "키"), 0.f, 1.f);

	AddSection(LOCTEXT("SecFace", "얼굴"));
	AddCycle(FGrimAppearanceMath::SlotDisplayName(EGrimAppearanceSlot::Face), ERow::Slot, EGrimAppearanceSlot::Face);
	for (const FGrimAppearanceMorph& Morph : Catalog.Morphs)
	{
		MorphSliders.Add(AddSlider(FText::FromString(Morph.Row.DisplayName.IsEmpty() ? Morph.Id.ToString() : Morph.Row.DisplayName), -1.f, 1.f));
	}
	AddCycle(LOCTEXT("Skin", "피부색"), ERow::Skin);
	AddCycle(LOCTEXT("Eye", "눈 색"), ERow::Eye);

	AddSection(LOCTEXT("SecHair", "머리"));
	AddCycle(FGrimAppearanceMath::SlotDisplayName(EGrimAppearanceSlot::Hair), ERow::Slot, EGrimAppearanceSlot::Hair);
	AddCycle(LOCTEXT("HairColor", "머리색"), ERow::HairColor);
	AddCycle(FGrimAppearanceMath::SlotDisplayName(EGrimAppearanceSlot::Beard), ERow::Slot, EGrimAppearanceSlot::Beard);

	AddSection(LOCTEXT("SecOutfit", "옷 — 시안 A 「잉크와 양피지」"));
	for (EGrimAppearanceSlot OutfitSlot : { EGrimAppearanceSlot::Hat, EGrimAppearanceSlot::Robe, EGrimAppearanceSlot::Stole, EGrimAppearanceSlot::Belt,
		EGrimAppearanceSlot::Cloak, EGrimAppearanceSlot::Gloves, EGrimAppearanceSlot::Boots, EGrimAppearanceSlot::Legs })
	{
		AddCycle(FGrimAppearanceMath::SlotDisplayName(OutfitSlot), ERow::Slot, OutfitSlot);
	}
	AddCycle(LOCTEXT("ClothPrimary", "옷 주색"), ERow::ClothPrimary);
	AddCycle(LOCTEXT("ClothSecondary", "옷 보조색"), ERow::ClothSecondary);
	AddCycle(LOCTEXT("Element", "안감 속성"), ERow::Element);

	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	for (const TPair<FText, int32>& Button : { TPair<FText, int32>(LOCTEXT("Random", "무작위"), B_Random),
		TPair<FText, int32>(LOCTEXT("Cancel", "취소"), B_Cancel), TPair<FText, int32>(LOCTEXT("Confirm", "확인"), B_Confirm) })
	{
		UHorizontalBoxSlot* ButtonSlot = Buttons->AddChildToHorizontalBox(MakeButton(WidgetTree, Button.Key, Button.Value, 15));
		ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ButtonSlot->SetPadding(FMargin(4.f, 0.f));
	}
	AddRow(Column, Buttons, 6.f);

	// 버튼 번호로 한 곳에서 받는다
	TArray<UWidget*> All;
	WidgetTree->GetAllWidgets(All);
	for (UWidget* Widget : All)
	{
		if (UGrimIdButton* Button = Cast<UGrimIdButton>(Widget))
		{
			Button->OnIdClicked.BindUObject(this, &UGrimCharacterCreatorWidget::HandleButton);
		}
	}
}

void UGrimCharacterCreatorWidget::HandleButton(int32 Id)
{
	using namespace GrimCharacterCreatorLocal;
	switch (Id)
	{
	case B_Random: Randomize(); return;
	case B_Confirm: Close(true); return;
	case B_Cancel: Close(false); return;
	default: break;
	}
	if (Id >= B_RowBase)
	{
		const int32 Offset = Id - B_RowBase;
		Cycle(Offset / 2, Offset % 2 == 0 ? -1 : 1);
	}
}

void UGrimCharacterCreatorWidget::Cycle(int32 RowIndex, int32 Step)
{
	if (!Rows.IsValidIndex(RowIndex))
	{
		return;
	}
	const FGrimAppearanceCatalog& Catalog = UGrimAppearanceSettings::GetCatalog();
	const FCycleRow& Row = Rows[RowIndex];
	auto Wrap = [Step](int32 Current, int32 Count) { return Count <= 0 ? 0 : ((Current + Step) % Count + Count) % Count; };
	auto CycleColor = [&Catalog, &Wrap](uint8& Value, FName Palette)
	{
		const TArray<uint8> Choices = Catalog.GetColorChoices(Palette, true);
		const int32 Current = Choices.IndexOfByKey(Value);
		Value = Choices.Num() > 0 ? Choices[Wrap(FMath::Max(Current, 0), Choices.Num())] : 0;
	};

	bool bFaceChanged = false;
	switch (Row.Kind)
	{
	case ERow::Body:
		Working.BodyType = Working.BodyType == 0 ? 1 : 0;
		break;
	case ERow::Slot:
	{
		TArray<FName> Options = Catalog.GetPartsForSlot(Row.Slot, Working.BodyType, true);
		if (Row.Slot != EGrimAppearanceSlot::Face)
		{
			Options.Insert(NAME_None, 0);
		}
		const int32 Current = Options.IndexOfByKey(Working.GetPart(Row.Slot));
		Working.SetPart(Row.Slot, Options.Num() > 0 ? Options[Wrap(FMath::Max(Current, 0), Options.Num())] : NAME_None);
		bFaceChanged = Row.Slot == EGrimAppearanceSlot::Face;
		break;
	}
	case ERow::Skin: CycleColor(Working.SkinColor, GrimAppearancePalettes::Skin); break;
	case ERow::Eye: CycleColor(Working.EyeColor, GrimAppearancePalettes::Eye); break;
	case ERow::HairColor: CycleColor(Working.HairColor, GrimAppearancePalettes::Hair); break;
	case ERow::ClothPrimary: CycleColor(Working.ClothPrimary, GrimAppearancePalettes::Cloth); break;
	case ERow::ClothSecondary: CycleColor(Working.ClothSecondary, GrimAppearancePalettes::Cloth); break;
	case ERow::Element: Working.Element = static_cast<uint8>(Wrap(Working.Element, FGrimAppearance::ElementCount + 1)); break;
	}
	ApplyWorking(bFaceChanged);
}

void UGrimCharacterCreatorWidget::Randomize()
{
	const uint8 Circle = Working.Circle;
	Working = FGrimAppearanceMath::GenerateFromRule(UGrimAppearanceSettings::GetCatalog(), UGrimAppearanceSettings::Get()->PlayerRule, FMath::Rand());
	Working.Circle = Circle;
	ApplyWorking();
}

void UGrimCharacterCreatorWidget::ApplyWorking(bool bResetFaceMorphs)
{
	const FGrimAppearanceCatalog& Catalog = UGrimAppearanceSettings::GetCatalog();
	const uint8 Circle = Working.Circle;
	FGrimAppearanceMath::Sanitize(Working, Catalog, true);
	Working.Circle = Circle; // 고리는 서버가 서클로 채운다 — 미리보기는 지금 보이는 수 그대로
	if (bResetFaceMorphs)
	{
		FGrimAppearanceMath::ApplyFaceDefaults(Working, Catalog);
	}
	if (UGrimAppearanceComponent* Pawn = GetPawnAppearance())
	{
		Pawn->PreviewAppearance(Working);
	}

	bSyncingSliders = true;
	if (HeightSlider)
	{
		HeightSlider->SetValue(Working.Height / 255.f);
	}
	for (int32 Index = 0; Index < MorphSliders.Num(); ++Index)
	{
		MorphSliders[Index]->SetValue(Working.GetMorph(Index));
	}
	bSyncingSliders = false;
	RefreshTexts();
}

void UGrimCharacterCreatorWidget::HandleSliderChanged(float Value)
{
	if (bSyncingSliders)
	{
		return;
	}
	if (HeightSlider)
	{
		Working.Height = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(HeightSlider->GetValue() * 255.f), 0, 255));
	}
	for (int32 Index = 0; Index < MorphSliders.Num(); ++Index)
	{
		Working.SetMorph(Index, MorphSliders[Index]->GetValue());
	}
	if (UGrimAppearanceComponent* Pawn = GetPawnAppearance())
	{
		Pawn->PreviewAppearance(Working);
	}
}

void UGrimCharacterCreatorWidget::RefreshTexts()
{
	for (int32 Index = 0; Index < Rows.Num() && Index < RowValues.Num(); ++Index)
	{
		if (RowValues[Index])
		{
			RowValues[Index]->SetText(DescribeRow(Rows[Index]));
		}
	}
}

FText UGrimCharacterCreatorWidget::DescribeRow(const FCycleRow& Row) const
{
	using namespace GrimCharacterCreatorLocal;
	switch (Row.Kind)
	{
	case ERow::Body:
		return Working.BodyType == 0 ? LOCTEXT("BodyA", "체형 A") : LOCTEXT("BodyB", "체형 B");
	case ERow::Slot:
	{
		const FGrimAppearancePart* Part = UGrimAppearanceSettings::GetCatalog().FindPart(Working.GetPart(Row.Slot));
		if (!Part)
		{
			return LOCTEXT("None", "없음");
		}
		return FText::FromString(Part->Row.DisplayName.IsEmpty() ? Part->Id.ToString() : Part->Row.DisplayName);
	}
	case ERow::Skin: return ColorName(GrimAppearancePalettes::Skin, Working.SkinColor);
	case ERow::Eye: return ColorName(GrimAppearancePalettes::Eye, Working.EyeColor);
	case ERow::HairColor: return ColorName(GrimAppearancePalettes::Hair, Working.HairColor);
	case ERow::ClothPrimary: return ColorName(GrimAppearancePalettes::Cloth, Working.ClothPrimary);
	case ERow::ClothSecondary: return ColorName(GrimAppearancePalettes::Cloth, Working.ClothSecondary);
	case ERow::Element: return FGrimAppearanceMath::ElementDisplayName(Working.Element);
	}
	return FText::GetEmpty();
}

#undef LOCTEXT_NAMESPACE
