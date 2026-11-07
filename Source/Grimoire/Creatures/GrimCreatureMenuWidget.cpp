#include "Creatures/GrimCreatureMenuWidget.h"
#include "Creatures/GrimCreature.h"
#include "Creatures/GrimCreatureSpecies.h"
#include "Creatures/GrimCreatureStorageComponent.h"
#include "Creatures/GrimCreatureTamingComponent.h"
#include "Creatures/GrimMountComponent.h"
#include "Creatures/GrimTamingMath.h"
#include "Creatures/GrimTamingSettings.h"
#include "UI/GrimUIData.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

#define LOCTEXT_NAMESPACE "GrimCreatureMenu"

namespace GrimCreatureMenuLocal // 유니티 빌드 이름 충돌 방지
{
	const FLinearColor PanelColor(0.05f, 0.07f, 0.06f, 0.88f);
	const FLinearColor NameColor(0.75f, 0.95f, 0.7f);
	const FLinearColor TextColor(0.95f, 0.95f, 0.95f);
	const FLinearColor FaintColor(0.7f, 0.72f, 0.7f);
	const FLinearColor ButtonColor(0.18f, 0.28f, 0.22f);
	const FLinearColor GainColor(0.55f, 0.9f, 0.55f);
	const FLinearColor LoyaltyColor(0.95f, 0.75f, 0.3f);
	constexpr float PanelWidth = 560.f;
	constexpr float NoticeSeconds = 3.5f;

	FString CommandName(EGrimTamedCommand Command)
	{
		switch (Command)
		{
		case EGrimTamedCommand::Wait: return TEXT("기다리는 중");
		case EGrimTamedCommand::Roam: return TEXT("거점에 풀어놓음");
		default: return TEXT("따라오는 중");
		}
	}
}

TSharedRef<SWidget> UGrimCreatureMenuWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

void UGrimCreatureMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetIsFocusable(true);
}

void UGrimCreatureMenuWidget::BuildTree()
{
	using namespace GrimCreatureMenuLocal;
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Canvas;

	PromptText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 16, TextColor, true);
	UCanvasPanelSlot* PromptSlot = Canvas->AddChildToCanvas(PromptText);
	PromptSlot->SetAnchors(FAnchors(0.5f, 1.f));
	PromptSlot->SetAlignment(FVector2D(0.5f, 1.f));
	PromptSlot->SetPosition(FVector2D(0.f, -200.f));
	PromptSlot->SetAutoSize(true);
	PromptText->SetVisibility(ESlateVisibility::Collapsed);

	NoticeText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 15, GainColor, true);
	UCanvasPanelSlot* NoticeSlot = Canvas->AddChildToCanvas(NoticeText);
	NoticeSlot->SetAnchors(FAnchors(0.5f, 0.f));
	NoticeSlot->SetAlignment(FVector2D(0.5f, 0.f));
	NoticeSlot->SetPosition(FVector2D(0.f, 140.f));
	NoticeSlot->SetAutoSize(true);
	NoticeText->SetVisibility(ESlateVisibility::Collapsed);

	UBorder* Border = GrimUIData::MakeBorder(WidgetTree, PanelColor, FMargin(18.f, 12.f));
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
	DetailText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 12, FaintColor);
	Column->AddChildToVerticalBox(DetailText)->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	LoyaltyText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 13, TextColor);
	Column->AddChildToVerticalBox(LoyaltyText);
	LoyaltyBar = GrimUIData::MakeBar(WidgetTree, LoyaltyColor);
	Column->AddChildToVerticalBox(GrimUIData::MakeSizeBox(WidgetTree, LoyaltyBar, PanelWidth - 36.f, 10.f))->SetPadding(FMargin(0.f, 2.f, 0.f, 6.f));
	ReasonsText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 12, FaintColor);
	Column->AddChildToVerticalBox(ReasonsText)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	MenuBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Column->AddChildToVerticalBox(MenuBox);
	RenameBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
	RenameBox->SetHintText(LOCTEXT("RenameHint", "새 이름을 쓰고 Enter (비우면 지움)"));
	RenameBox->OnTextCommitted.AddDynamic(this, &UGrimCreatureMenuWidget::HandleRenameCommitted);
	RenameBox->SetVisibility(ESlateVisibility::Collapsed);
	Column->AddChildToVerticalBox(RenameBox)->SetPadding(FMargin(0.f, 4.f));
	HintText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 11, FaintColor);
	Column->AddChildToVerticalBox(HintText)->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
}

void UGrimCreatureMenuWidget::ShowPrompt(const FString& Text)
{
	if (PromptText && !bOpen)
	{
		PromptText->SetText(FText::FromString(Text));
		PromptText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UGrimCreatureMenuWidget::HidePrompt()
{
	if (PromptText)
	{
		PromptText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UGrimCreatureMenuWidget::ShowNotice(const FString& Text)
{
	if (NoticeText)
	{
		NoticeText->SetText(FText::FromString(Text));
		NoticeText->SetVisibility(ESlateVisibility::HitTestInvisible);
		NoticeRemaining = GrimCreatureMenuLocal::NoticeSeconds;
	}
	if (bOpen)
	{
		RefreshInfo();
	}
}

void UGrimCreatureMenuWidget::OpenMenu(AGrimCreature* InCreature)
{
	if (!InCreature || !Panel)
	{
		return;
	}
	Creature = InCreature;
	bOpen = true;
	bRenaming = false;
	bReleaseArmed = false;
	HidePrompt();
	Panel->SetVisibility(ESlateVisibility::Visible);
	RenameBox->SetVisibility(ESlateVisibility::Collapsed);
	RebuildMenu();
	RefreshInfo();
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetShowMouseCursor(true);
	}
	FocusSelf();
}

void UGrimCreatureMenuWidget::CloseMenu()
{
	if (!bOpen)
	{
		return;
	}
	bOpen = false;
	bRenaming = false;
	Creature = nullptr;
	if (Panel)
	{
		Panel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (MenuBox)
	{
		MenuBox->ClearChildren();
	}
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
	}
}

void UGrimCreatureMenuWidget::FocusSelf()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		FInputModeUIOnly Mode;
		Mode.SetWidgetToFocus(bRenaming && RenameBox ? RenameBox->TakeWidget() : TakeWidget());
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(Mode);
	}
}

void UGrimCreatureMenuWidget::RebuildMenu()
{
	using namespace GrimCreatureMenuLocal;
	MenuBox->ClearChildren();
	Entries.Reset();
	const AGrimCreature* Target = Creature.Get();
	const UGrimMountComponent* Mount = Target ? Target->GetMount() : nullptr;
	const bool bAtBase = Target && Target->GetTaming() && Target->GetTaming()->IsAtBase();
	const UGrimCreatureStorageComponent* Storage = Tamer.IsValid() ? UGrimCreatureStorageComponent::FindForActor(Tamer->GetOwner()) : nullptr;
	if (!bAtBase)
	{
		// 거점에 풀어놓은 생물은 상자에서 데리고 다닌다(R38).
		Entries.Add(EEntry::Follow);
		Entries.Add(EEntry::Wait);
		if (Mount && Mount->IsMountable())
		{
			Entries.Add(EEntry::Mount);
		}
	}
	Entries.Add(EEntry::Feed);
	Entries.Add(EEntry::Rename);
	if (!bAtBase && Storage)
	{
		Entries.Add(EEntry::Store);
	}
	Entries.Add(EEntry::Release);
	Entries.Add(EEntry::Close);

	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		FString Label;
		switch (Entries[Index])
		{
		case EEntry::Follow: Label = TEXT("따라오기"); break;
		case EEntry::Wait: Label = TEXT("기다리기"); break;
		case EEntry::Mount: Label = TEXT("타기"); break;
		case EEntry::Feed: Label = TEXT("먹이 주기"); break;
		case EEntry::Rename: Label = TEXT("이름 짓기"); break;
		case EEntry::Store: Label = TEXT("보관함에 넣기"); break;
		case EEntry::Release: Label = bReleaseArmed ? TEXT("정말 놓아주기 — 한 번 더") : TEXT("놓아주기"); break;
		default: Label = TEXT("닫기"); break;
		}
		UGrimIdButton* Button = WidgetTree->ConstructWidget<UGrimIdButton>(UGrimIdButton::StaticClass());
		Button->Id = Index;
		Button->Bind();
		Button->OnIdClicked.BindUObject(this, &UGrimCreatureMenuWidget::Choose);
		Button->SetBackgroundColor(ButtonColor);
		Button->SetContent(GrimUIData::MakeText(WidgetTree, FText::FromString(FString::Printf(TEXT("%d. %s"), Index + 1, *Label)), 14, TextColor));
		MenuBox->AddChildToVerticalBox(Button)->SetPadding(FMargin(0.f, 2.f));
	}
	HintText->SetText(LOCTEXT("Hint", "[1~7] 고르기    [F] · [Esc] 닫기"));
}

void UGrimCreatureMenuWidget::RefreshInfo()
{
	const AGrimCreature* Target = Creature.Get();
	const UGrimCreatureTamingComponent* Taming = Target ? Target->GetTaming() : nullptr;
	if (!Taming || !NameText)
	{
		return;
	}
	const UGrimTamingSettings* Settings = UGrimTamingSettings::Get();
	const FGrimCreatureInstance& Instance = Target->GetInstance();
	const UGrimCreatureSpecies* Species = Target->GetSpecies();
	const FString SpeciesName = Species && !Species->DisplayName.IsEmpty() ? Species->DisplayName.ToString() : Instance.SpeciesId.ToString();
	const FGrimPersonalityTuning* Personality = Settings->FindPersonality(Instance.Personality);
	const FString PersonalityName = Personality && !Personality->DisplayName.IsEmpty() ? Personality->DisplayName : TEXT("평범");

	NameText->SetText(FText::FromString(Instance.Nickname.IsEmpty() ? SpeciesName : FString::Printf(TEXT("%s (%s)"), *Instance.Nickname, *SpeciesName)));
	DetailText->SetText(FText::FromString(FString::Printf(TEXT("%s · %s%s · 성격 %s · 체력 %.0f/%.0f · %s%s"),
		Instance.Sex == EGrimCreatureSex::Male ? TEXT("수") : TEXT("암"), Instance.bGiant ? TEXT("거대 · ") : TEXT(""),
		Instance.bMutant ? TEXT("돌연변이") : TEXT("보통 색"), *PersonalityName, Target->GetHealth(), Target->GetMaxHealth(),
		*GrimCreatureMenuLocal::CommandName(Taming->GetCommand()), Taming->IsKnockedOut() ? TEXT(" · 쓰러짐") : TEXT(""))));

	const FGrimLoyaltyStageTuning* Stage = Settings->FindStage(Taming->GetStage());
	LoyaltyText->SetText(FText::FromString(FString::Printf(TEXT("충성도 %.0f / 100 — %s"), Taming->GetLoyalty(), Stage ? *Stage->DisplayName : TEXT(""))));
	LoyaltyBar->SetPercent(Taming->GetLoyalty() / 100.f);

	FString Reasons;
	for (const FGrimLoyaltyReason& Reason : Taming->GetRecentReasons())
	{
		Reasons += FString::Printf(TEXT("%s%s %+.0f"), Reasons.IsEmpty() ? TEXT("") : TEXT("\n"), *GrimTamingMath::EventDisplayText(Reason.Event), Reason.Delta);
	}
	ReasonsText->SetText(FText::FromString(Reasons.IsEmpty() ? FString(TEXT("최근 일 없음")) : Reasons));
}

void UGrimCreatureMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
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
	if (!bOpen)
	{
		return;
	}
	// 생물이 사라졌거나(놓아줌 · 떠남) 멀어지면 닫는다.
	const AGrimCreature* Target = Creature.Get();
	const APlayerController* PC = GetOwningPlayer();
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	const UGrimTamerComponent* Owner = Tamer.Get();
	const float Radius = UGrimTamingSettings::Get()->Behavior.InteractRadius + 200.f;
	if (!Target || Target->IsDead() || !Pawn || !Owner || !Owner->OwnsCreature(Target) || FVector::Dist(Pawn->GetActorLocation(), Target->GetActorLocation()) > Radius)
	{
		CloseMenu();
		return;
	}
	RefreshInfo();
}

void UGrimCreatureMenuWidget::Choose(int32 Index)
{
	UGrimTamerComponent* Owner = Tamer.Get();
	AGrimCreature* Target = Creature.Get();
	if (!bOpen || !Entries.IsValidIndex(Index) || !Owner || !Target)
	{
		return;
	}
	const EEntry Entry = Entries[Index];
	if (Entry != EEntry::Release && bReleaseArmed)
	{
		bReleaseArmed = false;
		RebuildMenu();
	}
	switch (Entry)
	{
	case EEntry::Follow:
		Owner->RequestAction(Target, EGrimTamerAction::Follow);
		break;
	case EEntry::Wait:
		Owner->RequestAction(Target, EGrimTamerAction::Wait);
		break;
	case EEntry::Mount:
		Owner->RequestAction(Target, EGrimTamerAction::Mount);
		CloseMenu();
		break;
	case EEntry::Feed:
		Owner->RequestAction(Target, EGrimTamerAction::Feed);
		break;
	case EEntry::Rename:
		BeginRename();
		break;
	case EEntry::Store:
		if (UGrimCreatureStorageComponent* Storage = UGrimCreatureStorageComponent::FindForActor(Owner->GetOwner()))
		{
			Storage->RequestTransfer(nullptr, Target->GetInstance().SpeciesId, Target->GetInstance().Seed, EGrimCreaturePlace::Party, EGrimCreaturePlace::Storage);
		}
		CloseMenu();
		break;
	case EEntry::Release:
		if (!bReleaseArmed)
		{
			bReleaseArmed = true;
			RebuildMenu();
		}
		else
		{
			Owner->RequestAction(Target, EGrimTamerAction::Release);
			CloseMenu();
		}
		break;
	default:
		CloseMenu();
		break;
	}
}

void UGrimCreatureMenuWidget::BeginRename()
{
	if (!RenameBox)
	{
		return;
	}
	bRenaming = true;
	const AGrimCreature* Target = Creature.Get();
	RenameBox->SetText(FText::FromString(Target ? Target->GetInstance().Nickname : FString()));
	RenameBox->SetVisibility(ESlateVisibility::Visible);
	FocusSelf();
}

void UGrimCreatureMenuWidget::ConfirmRename()
{
	UGrimTamerComponent* Owner = Tamer.Get();
	AGrimCreature* Target = Creature.Get();
	if (Owner && Target && RenameBox)
	{
		Owner->RequestRename(Target, RenameBox->GetText().ToString());
	}
	bRenaming = false;
	if (RenameBox)
	{
		RenameBox->SetVisibility(ESlateVisibility::Collapsed);
	}
	FocusSelf();
}

void UGrimCreatureMenuWidget::HandleRenameCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (!bRenaming)
	{
		return;
	}
	if (CommitMethod == ETextCommit::OnEnter)
	{
		ConfirmRename();
	}
	else if (CommitMethod == ETextCommit::OnCleared)
	{
		bRenaming = false;
		RenameBox->SetVisibility(ESlateVisibility::Collapsed);
		FocusSelf();
	}
}

FReply UGrimCreatureMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (!bOpen)
	{
		return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	}
	const FKey Key = InKeyEvent.GetKey();
	if (bRenaming)
	{
		// 글 상자가 글자를 받는다 — Esc 만 여기서.
		if (Key == EKeys::Escape)
		{
			bRenaming = false;
			RenameBox->SetVisibility(ESlateVisibility::Collapsed);
			FocusSelf();
			return FReply::Handled();
		}
		return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	}
	if (Key == EKeys::Escape || Key == EKeys::Tab || Key == UGrimTamingSettings::Get()->Behavior.MenuKey)
	{
		CloseMenu();
		return FReply::Handled();
	}
	static const FKey NumberKeys[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(NumberKeys); ++Index)
	{
		if (Key == NumberKeys[Index])
		{
			Choose(Index);
			return FReply::Handled();
		}
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
