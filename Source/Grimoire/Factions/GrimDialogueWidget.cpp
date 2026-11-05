#include "Factions/GrimDialogueWidget.h"
#include "Factions/GrimFactionTypes.h"
#include "Factions/GrimTowerNPC.h"
#include "Quest/GrimQuestComponent.h"
#include "UI/GrimUIData.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

#define LOCTEXT_NAMESPACE "GrimDialogue"

namespace GrimDialogueWidgetLocal
{
	const FLinearColor PanelColor(0.05f, 0.05f, 0.09f, 0.85f);
	const FLinearColor NameColor(1.f, 0.85f, 0.45f);
	const FLinearColor TextColor(0.95f, 0.95f, 0.95f);
	const FLinearColor FaintColor(0.7f, 0.7f, 0.75f);
	const FLinearColor ButtonColor(0.2f, 0.22f, 0.35f);
	constexpr float PanelWidth = 760.f;
}

TSharedRef<SWidget> UGrimDialogueWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

void UGrimDialogueWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(true);
}

void UGrimDialogueWidget::BuildTree()
{
	using namespace GrimDialogueWidgetLocal;

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Canvas;

	// 안내 한 줄(다가가면)
	PromptText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 16, TextColor, true);
	UCanvasPanelSlot* PromptSlot = Canvas->AddChildToCanvas(PromptText);
	PromptSlot->SetAnchors(FAnchors(0.5f, 1.f));
	PromptSlot->SetAlignment(FVector2D(0.5f, 1.f));
	PromptSlot->SetPosition(FVector2D(0.f, -230.f));
	PromptSlot->SetAutoSize(true);
	PromptText->SetVisibility(ESlateVisibility::Collapsed);

	// 대화 패널
	UBorder* Border = GrimUIData::MakeBorder(WidgetTree, PanelColor, FMargin(20.f, 14.f));
	Panel = GrimUIData::MakeSizeBox(WidgetTree, Border, PanelWidth, 0.f);
	UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Panel);
	PanelSlot->SetAnchors(FAnchors(0.5f, 1.f));
	PanelSlot->SetAlignment(FVector2D(0.5f, 1.f));
	PanelSlot->SetPosition(FVector2D(0.f, -40.f));
	PanelSlot->SetAutoSize(true);
	Panel->SetVisibility(ESlateVisibility::Collapsed);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Border->SetContent(Column);
	NameText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 19, NameColor, true);
	Column->AddChildToVerticalBox(NameText);
	TitleText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 12, FaintColor);
	Column->AddChildToVerticalBox(TitleText)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	LineText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 16, TextColor);
	LineText->SetAutoWrapText(true);
	Column->AddChildToVerticalBox(LineText)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	MenuBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Column->AddChildToVerticalBox(MenuBox);
	HintText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 11, FaintColor);
	Column->AddChildToVerticalBox(HintText)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
}

void UGrimDialogueWidget::NativeDestruct()
{
	if (UGrimQuestComponent* Quest = BoundQuest.Get())
	{
		Quest->OnNpcResponse.Remove(ResponseHandle);
	}
	Super::NativeDestruct();
}

void UGrimDialogueWidget::ShowPrompt(const FText& Text)
{
	if (PromptText && Phase == EPhase::Closed)
	{
		PromptText->SetText(Text);
		PromptText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UGrimDialogueWidget::HidePrompt()
{
	if (PromptText)
	{
		PromptText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

UGrimQuestComponent* UGrimDialogueWidget::GetQuest() const
{
	return UGrimQuestComponent::FindForController(GetOwningPlayer());
}

void UGrimDialogueWidget::OpenConversation(AGrimTowerNPC* InNpc)
{
	UGrimQuestComponent* Quest = GetQuest();
	if (!InNpc || !Quest || !Panel)
	{
		return;
	}
	Npc = InNpc;
	if (BoundQuest.Get() != Quest)
	{
		if (UGrimQuestComponent* Old = BoundQuest.Get())
		{
			Old->OnNpcResponse.Remove(ResponseHandle);
		}
		BoundQuest = Quest;
		ResponseHandle = Quest->OnNpcResponse.AddUObject(this, &UGrimDialogueWidget::HandleNpcResponse);
	}

	NameText->SetText(InNpc->GetDisplayName());
	const FGrimNpcDef* Def = InNpc->GetNpcDef();
	TitleText->SetText(Def ? FText::FromString(Def->Title) : FText::GetEmpty());
	HidePrompt();
	Panel->SetVisibility(ESlateVisibility::Visible);

	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		PlayerController->SetShowMouseCursor(true);
	}
	SetFocusToSelf();

	TArray<FString> Lines;
	InNpc->BuildLines(GrimDialogueContexts::Talk, Quest->GetLog(), Lines);
	ShowLines(Lines, EAfterLines::FinishTalk);
}

void UGrimDialogueWidget::SetFocusToSelf()
{
	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		FInputModeUIOnly Mode;
		Mode.SetWidgetToFocus(TakeWidget());
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(Mode);
	}
}

void UGrimDialogueWidget::CloseConversation()
{
	if (Phase == EPhase::Closed)
	{
		return;
	}
	Phase = EPhase::Closed;
	PendingLines.Reset();
	MenuServices.Reset();
	if (MenuBox)
	{
		MenuBox->ClearChildren();
	}
	if (Panel)
	{
		Panel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		PlayerController->SetInputMode(FInputModeGameOnly());
		PlayerController->SetShowMouseCursor(false);
	}
	if (AGrimTowerNPC* TowerNpc = Npc.Get())
	{
		TowerNpc->BlockReopenFor(0.4f);
	}
}

void UGrimDialogueWidget::ShowLines(const TArray<FString>& Lines, EAfterLines After)
{
	PendingLines = Lines;
	LineIndex = 0;
	AfterLines = After;
	Phase = EPhase::Lines;
	MenuBox->ClearChildren();
	ShowCurrentLine();
}

void UGrimDialogueWidget::ShowCurrentLine()
{
	LineText->SetText(PendingLines.IsValidIndex(LineIndex) ? FText::FromString(PendingLines[LineIndex]) : FText::GetEmpty());
	const bool bLast = LineIndex >= PendingLines.Num() - 1;
	HintText->SetText(bLast ? LOCTEXT("HintLast", "[F] 계속    [Esc] 닫기") : LOCTEXT("HintNext", "[F] 다음    [Esc] 닫기"));
}

void UGrimDialogueWidget::Advance()
{
	if (Phase == EPhase::Menu)
	{
		CloseConversation();
		return;
	}
	if (Phase != EPhase::Lines)
	{
		return;
	}
	if (LineIndex < PendingLines.Num() - 1)
	{
		++LineIndex;
		ShowCurrentLine();
		return;
	}

	AGrimTowerNPC* TowerNpc = Npc.Get();
	UGrimQuestComponent* Quest = GetQuest();
	if (!TowerNpc || !Quest)
	{
		CloseConversation();
		return;
	}
	if (AfterLines == EAfterLines::Close)
	{
		CloseConversation();
		return;
	}
	if (AfterLines == EAfterLines::FinishTalk)
	{
		Quest->RequestNpc(TowerNpc, GrimDialogueContexts::Talk);
	}
	const FGrimNpcDef* Def = TowerNpc->GetNpcDef();
	if (Def && Def->Services.Num() > 0)
	{
		ShowMenu();
	}
	else
	{
		CloseConversation();
	}
}

void UGrimDialogueWidget::ShowMenu()
{
	using namespace GrimDialogueWidgetLocal;
	AGrimTowerNPC* TowerNpc = Npc.Get();
	UGrimQuestComponent* Quest = GetQuest();
	const FGrimNpcDef* Def = TowerNpc ? TowerNpc->GetNpcDef() : nullptr;
	if (!Def || !Quest)
	{
		CloseConversation();
		return;
	}
	Phase = EPhase::Menu;
	TArray<FString> Header;
	TowerNpc->BuildLines(GrimDialogueContexts::Menu, Quest->GetLog(), Header);
	LineText->SetText(FText::FromString(Header.Num() > 0 ? Header[0] : FString()));

	MenuServices = Def->Services;
	MenuBox->ClearChildren();
	for (int32 Index = 0; Index < MenuServices.Num(); ++Index)
	{
		UGrimIdButton* Button = WidgetTree->ConstructWidget<UGrimIdButton>(UGrimIdButton::StaticClass());
		Button->Id = Index;
		Button->Bind();
		Button->OnIdClicked.BindUObject(this, &UGrimDialogueWidget::ChooseService);
		Button->SetBackgroundColor(ButtonColor);
		const FString Label = Def->ServiceLabels.IsValidIndex(Index) ? Def->ServiceLabels[Index] : MenuServices[Index].ToString();
		Button->SetContent(GrimUIData::MakeText(WidgetTree, FText::FromString(FString::Printf(TEXT("%d. %s"), Index + 1, *Label)), 15, TextColor));
		MenuBox->AddChildToVerticalBox(Button)->SetPadding(FMargin(0.f, 2.f));
	}
	HintText->SetText(LOCTEXT("HintMenu", "[1~9] 고르기    [F] · [Esc] 닫기"));
	SetFocusToSelf();
}

void UGrimDialogueWidget::ChooseService(int32 Index)
{
	if (Phase != EPhase::Menu || !MenuServices.IsValidIndex(Index))
	{
		return;
	}
	AGrimTowerNPC* TowerNpc = Npc.Get();
	UGrimQuestComponent* Quest = GetQuest();
	if (!TowerNpc || !Quest)
	{
		CloseConversation();
		return;
	}
	Phase = EPhase::Waiting;
	MenuBox->ClearChildren();
	LineText->SetText(FText::FromString(TEXT("...")));
	HintText->SetText(FText::GetEmpty());
	SetFocusToSelf();
	Quest->RequestNpc(TowerNpc, MenuServices[Index]);
}

void UGrimDialogueWidget::HandleNpcResponse(UGrimQuestComponent* Quest, AActor* ResponseNpc, FName Context)
{
	AGrimTowerNPC* TowerNpc = Npc.Get();
	if (Phase == EPhase::Closed || !TowerNpc || (ResponseNpc && ResponseNpc != TowerNpc))
	{
		return;
	}
	TArray<FString> Lines;
	TowerNpc->BuildLines(Context, Quest->GetLog(), Lines);
	// 시험이 시작되면 메뉴로 돌아가지 않고 닫는다(시험의 방으로 간다).
	ShowLines(Lines, Context == GrimDialogueContexts::TrialStarted ? EAfterLines::Close : EAfterLines::BackToMenu);
}

FReply UGrimDialogueWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (Phase == EPhase::Closed)
	{
		return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	}
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::Tab)
	{
		CloseConversation();
		return FReply::Handled();
	}
	if (Key == EKeys::F || Key == EKeys::SpaceBar || Key == EKeys::Enter)
	{
		Advance();
		return FReply::Handled();
	}
	if (Phase == EPhase::Menu)
	{
		static const FKey NumberKeys[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine };
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(NumberKeys); ++Index)
		{
			if (Key == NumberKeys[Index])
			{
				ChooseService(Index);
				return FReply::Handled();
			}
		}
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
