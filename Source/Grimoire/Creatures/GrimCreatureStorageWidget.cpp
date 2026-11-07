#include "Creatures/GrimCreatureStorageWidget.h"
#include "Creatures/GrimCreature.h"
#include "Creatures/GrimCreatureBox.h"
#include "Creatures/GrimCreatureSettings.h"
#include "Creatures/GrimCreatureSpecies.h"
#include "Creatures/GrimCreatureStorageComponent.h"
#include "Creatures/GrimCreatureStorageMath.h"
#include "Creatures/GrimCreatureStorageSettings.h"
#include "Creatures/GrimCreatureTamingComponent.h"
#include "Creatures/GrimTamerComponent.h"
#include "Creatures/GrimTamingMath.h"
#include "Creatures/GrimTamingSettings.h"
#include "Items/GrimInventoryWidget.h"
#include "UI/GrimUIData.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

#define LOCTEXT_NAMESPACE "GrimCreatureStorage"

namespace GrimCreatureStorageWidgetLocal // 유니티 빌드 이름 충돌 방지
{
	const FLinearColor PanelColor(0.04f, 0.06f, 0.05f, 0.9f);
	const FLinearColor TitleColor(0.75f, 0.95f, 0.7f);
	const FLinearColor TextColor(0.93f, 0.93f, 0.93f);
	const FLinearColor FaintColor(0.65f, 0.68f, 0.65f);
	const FLinearColor RowColor(0.12f, 0.14f, 0.12f, 0.95f);
	const FLinearColor ButtonColor(0.2f, 0.3f, 0.24f);
	const FLinearColor NoticeColor(1.f, 0.85f, 0.45f);
	constexpr float RefreshSeconds = 0.5f;
	constexpr float NoticeSeconds = 3.5f;
	constexpr int32 MaxTargets = 8;

	float ColumnWidth(bool bStandalone)
	{
		return bStandalone ? 420.f : 360.f;
	}

	UVerticalBox* MakeColumn(UWidgetTree* Tree, UHorizontalBox* Parent, const FText& Title, float Width, UWidget*& OutColumnRoot, UTextBlock** OutHint)
	{
		UVerticalBox* Column = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Column->AddChildToVerticalBox(GrimUIData::MakeText(Tree, Title, 15, TitleColor, true))->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
		if (OutHint)
		{
			*OutHint = GrimUIData::MakeText(Tree, FText::GetEmpty(), 12, FaintColor);
			(*OutHint)->SetAutoWrapText(true);
			(*OutHint)->SetVisibility(ESlateVisibility::Collapsed);
			Column->AddChildToVerticalBox(*OutHint)->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
		}
		UScrollBox* Scroll = Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
		UVerticalBox* List = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Scroll->AddChild(List);
		Column->AddChildToVerticalBox(GrimUIData::MakeSizeBox(Tree, Scroll, Width, 360.f));
		USizeBox* Sized = GrimUIData::MakeSizeBox(Tree, Column, Width, 0.f);
		Parent->AddChildToHorizontalBox(Sized)->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
		OutColumnRoot = Sized;
		return List;
	}
}

// ───────────────────────── 줄 값(정적 — 테스트 · 덤프) ─────────────────────────

TArray<FGrimCreatureRowView> UGrimCreatureStorageWidget::GatherRows(const UGrimCreatureStorageComponent* InStorage, const AGrimCreatureBox* InBox)
{
	TArray<FGrimCreatureRowView> Rows;
	if (!InStorage)
	{
		return Rows;
	}
	const float HealRate = UGrimCreatureStorageSettings::Get()->StoredHealPerMinute;
	const double Now = static_cast<double>(FDateTime::UtcNow().ToUnixTimestamp());
	for (const AGrimCreature* Creature : InStorage->GetPartyCreatures())
	{
		FGrimCreatureRowView& Row = Rows.AddDefaulted_GetRef();
		Row.Place = EGrimCreaturePlace::Party;
		Row.Instance = Creature->GetInstance();
		Row.HealthRatio = Creature->GetHealthRatio();
		Row.bKnockedOut = Creature->GetTaming() && Creature->GetTaming()->IsKnockedOut();
	}
	for (const FGrimOwnedCreatureRecord& Record : InStorage->GetStored())
	{
		FGrimCreatureRowView& Row = Rows.AddDefaulted_GetRef();
		Row.Place = EGrimCreaturePlace::Storage;
		Row.Instance = Record.Instance;
		Row.HealthRatio = GrimCreatureStorageMath::GetHealedRatio(Record.HealthRatio, Record.SavedUtcSeconds, Now, HealRate);
	}
	if (InBox)
	{
		const FString& MyId = InStorage->GetOwnerPlayerId();
		for (const FGrimCreatureBoxEntry& Entry : InBox->GetEntries())
		{
			if (Entry.Record.Instance.OwnerId != MyId)
			{
				continue;
			}
			FGrimCreatureRowView& Row = Rows.AddDefaulted_GetRef();
			Row.Place = Entry.bReleased ? EGrimCreaturePlace::Released : EGrimCreaturePlace::Box;
			Row.Instance = Entry.Record.Instance;
			Row.HealthRatio = GrimCreatureStorageMath::GetHealedRatio(Entry.Record.HealthRatio, Entry.Record.SavedUtcSeconds, Now, HealRate);
			if (Entry.bReleased)
			{
				if (const AGrimCreature* Creature = InBox->FindReleasedActor(Entry.Record.Instance.SpeciesId, Entry.Record.Instance.Seed))
				{
					Row.Instance = Creature->GetInstance();
					Row.HealthRatio = Creature->GetHealthRatio();
				}
			}
		}
	}
	return Rows;
}

FString UGrimCreatureStorageWidget::DescribeRow(const FGrimCreatureRowView& Row)
{
	const FGrimCreatureInstance& Instance = Row.Instance;
	const UGrimCreatureSpecies* Species = UGrimCreatureSettings::Get()->FindSpecies(Instance.SpeciesId);
	const FString SpeciesName = Species && !Species->DisplayName.IsEmpty() ? Species->DisplayName.ToString() : Instance.SpeciesId.ToString();
	const FString Name = Instance.Nickname.IsEmpty() ? SpeciesName : FString::Printf(TEXT("%s (%s)"), *Instance.Nickname, *SpeciesName);
	const UGrimTamingSettings* Taming = UGrimTamingSettings::Get();
	const FGrimLoyaltyStageTuning* Stage = Taming->FindStage(GrimTamingMath::GetStage(Instance.Loyalty, Taming->Loyalty));
	return FString::Printf(TEXT("%s · %s%s\n크기 ×%.2f%s · 충성도 %.0f %s · 체력 %.0f%%%s"),
		*Name, Instance.Sex == EGrimCreatureSex::Male ? TEXT("수") : TEXT("암"), Instance.bMutant ? TEXT(" · 돌연변이") : TEXT(""),
		Instance.SizeScale, Instance.bGiant ? TEXT(" 거대") : TEXT(""), Instance.Loyalty, Stage ? *Stage->DisplayName : TEXT(""),
		FMath::Clamp(Row.HealthRatio, 0.f, 1.f) * 100.f, Row.bKnockedOut ? TEXT(" · 쓰러짐") : TEXT(""));
}

TArray<EGrimCreaturePlace> UGrimCreatureStorageWidget::GetTargets(EGrimCreaturePlace From, bool bHasBox)
{
	switch (From)
	{
	case EGrimCreaturePlace::Party:
		return bHasBox ? TArray<EGrimCreaturePlace>{ EGrimCreaturePlace::Storage, EGrimCreaturePlace::Box, EGrimCreaturePlace::Released } : TArray<EGrimCreaturePlace>{ EGrimCreaturePlace::Storage };
	case EGrimCreaturePlace::Storage:
		return bHasBox ? TArray<EGrimCreaturePlace>{ EGrimCreaturePlace::Party, EGrimCreaturePlace::Box } : TArray<EGrimCreaturePlace>{ EGrimCreaturePlace::Party };
	case EGrimCreaturePlace::Box:
		return { EGrimCreaturePlace::Party, EGrimCreaturePlace::Storage, EGrimCreaturePlace::Released };
	case EGrimCreaturePlace::Released:
		return { EGrimCreaturePlace::Box, EGrimCreaturePlace::Party };
	default:
		return {};
	}
}

FString UGrimCreatureStorageWidget::TargetLabel(EGrimCreaturePlace From, EGrimCreaturePlace To)
{
	switch (To)
	{
	case EGrimCreaturePlace::Party: return TEXT("데리고 다니기");
	case EGrimCreaturePlace::Storage: return From == EGrimCreaturePlace::Party ? TEXT("보관함에 넣기") : TEXT("보관함으로");
	case EGrimCreaturePlace::Box: return From == EGrimCreaturePlace::Released ? TEXT("거두기") : TEXT("상자로");
	case EGrimCreaturePlace::Released: return TEXT("풀어놓기");
	default: return TEXT("?");
	}
}

// ───────────────────────── 위젯 ─────────────────────────

void UGrimCreatureStorageWidget::SetStorage(UGrimCreatureStorageComponent* InStorage)
{
	Storage = InStorage;
	LastSignature.Reset();
	BindTamer();
}

UGrimCreatureStorageComponent* UGrimCreatureStorageWidget::ResolveStorage()
{
	if (!Storage.IsValid())
	{
		if (const APlayerController* PC = GetOwningPlayer())
		{
			Storage = UGrimCreatureStorageComponent::FindForActor(PC);
			if (Storage.IsValid())
			{
				BindTamer();
			}
		}
	}
	return Storage.Get();
}

void UGrimCreatureStorageWidget::BindTamer()
{
	UGrimTamerComponent* Tamer = Storage.IsValid() ? Storage->GetTamer() : nullptr;
	if (Tamer == BoundTamer.Get())
	{
		return;
	}
	if (UGrimTamerComponent* Old = BoundTamer.Get())
	{
		Old->OnNotice.Remove(NoticeHandle);
	}
	BoundTamer = Tamer;
	if (Tamer)
	{
		NoticeHandle = Tamer->OnNotice.AddUObject(this, &UGrimCreatureStorageWidget::HandleNotice);
	}
}

void UGrimCreatureStorageWidget::NativeDestruct()
{
	if (UGrimTamerComponent* Old = BoundTamer.Get())
	{
		Old->OnNotice.Remove(NoticeHandle);
	}
	Super::NativeDestruct();
}

TSharedRef<SWidget> UGrimCreatureStorageWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

void UGrimCreatureStorageWidget::BuildTree()
{
	using namespace GrimCreatureStorageWidgetLocal;
	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	if (bStandalone)
	{
		UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Canvas;
		Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		UBorder* Panel = GrimUIData::MakeBorder(WidgetTree, PanelColor, FMargin(18.f, 14.f));
		Panel->SetContent(Content);
		UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Panel);
		PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		PanelSlot->SetAutoSize(true);
		PanelRoot = Panel;
		PanelRoot->SetVisibility(ESlateVisibility::Collapsed);
		const FKey& Key = UGrimTamingSettings::Get()->Behavior.MenuKey;
		Content->AddChildToVerticalBox(GrimUIData::MakeText(WidgetTree,
			FText::Format(LOCTEXT("BoxTitle", "거점 생물 상자  ([{0}] · [Esc] 닫기)"), Key.GetDisplayName()), 18, TitleColor, true))->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	}
	else
	{
		WidgetTree->RootWidget = Content;
		PanelRoot = Content;
	}

	HeaderText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 13, TextColor);
	Content->AddChildToVerticalBox(HeaderText)->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
	NoticeText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 13, NoticeColor, true);
	NoticeText->SetVisibility(ESlateVisibility::Collapsed);
	Content->AddChildToVerticalBox(NoticeText)->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));

	UHorizontalBox* Columns = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Content->AddChildToVerticalBox(Columns);
	const float Width = ColumnWidth(bStandalone);
	UWidget* Ignored = nullptr;
	PartyList = MakeColumn(WidgetTree, Columns, LOCTEXT("PartyTitle", "데리고 다님"), Width, Ignored, nullptr);
	UTextBlock* Hint = nullptr;
	StorageList = MakeColumn(WidgetTree, Columns, LOCTEXT("StorageTitle", "생물 보관함"), Width, Ignored, &Hint);
	StorageHint = Hint;
	UWidget* BoxRoot = nullptr;
	BoxList = MakeColumn(WidgetTree, Columns, LOCTEXT("BoxTitle2", "거점 생물 상자"), Width, BoxRoot, nullptr);
	BoxColumn = BoxRoot;
	BoxColumn->SetVisibility(bStandalone ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UGrimCreatureStorageWidget::OpenScreen(AGrimCreatureBox* InBox)
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC || !InBox)
	{
		return;
	}
	Box = InBox;
	bOpen = true;
	OpenedAt = FPlatformTime::Seconds();
	LastSignature.Reset();
	if (PanelRoot)
	{
		PanelRoot->SetVisibility(ESlateVisibility::Visible);
	}
	FInputModeGameAndUI Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Mode.SetHideCursorDuringCapture(false);
	PC->SetInputMode(Mode);
	PC->SetShowMouseCursor(true);
	PC->SetIgnoreLookInput(true);
	Refresh();
	UE_LOG(LogGrimTaming, Display, TEXT("[%s] 거점 생물 상자 화면 열기 %s"), PC->GetNetMode() == NM_Client ? TEXT("Client") : TEXT("Server"), *GetNameSafe(InBox));
}

void UGrimCreatureStorageWidget::CloseScreen()
{
	if (!bOpen)
	{
		return;
	}
	bOpen = false;
	Box = nullptr;
	if (PanelRoot && bStandalone)
	{
		PanelRoot->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
		PC->SetIgnoreLookInput(false);
	}
}

void UGrimCreatureStorageWidget::SetTabActive(bool bActive)
{
	bTabActive = bActive;
	if (bActive)
	{
		LastSignature.Reset();
		Refresh();
	}
}

void UGrimCreatureStorageWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
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
	if (bStandalone && bOpen)
	{
		APlayerController* PC = GetOwningPlayer();
		const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
		const AGrimCreatureBox* OpenBox = Box.Get();
		// 연 키가 같은 프레임에 닫지 않게 잠깐 기다린다.
		const bool bKeysLive = FPlatformTime::Seconds() - OpenedAt > 0.25;
		if (!OpenBox || !Pawn || !OpenBox->IsWithinManageRange(Pawn, 200.f)
			|| (bKeysLive && PC && (PC->WasInputKeyJustPressed(EKeys::Escape) || PC->WasInputKeyJustPressed(UGrimTamingSettings::Get()->Behavior.MenuKey))))
		{
			CloseScreen();
			return;
		}
	}
	const bool bVisibleNow = bStandalone ? bOpen : bTabActive;
	RefreshAccum += InDeltaTime;
	if (bVisibleNow && RefreshAccum >= GrimCreatureStorageWidgetLocal::RefreshSeconds)
	{
		RefreshAccum = 0.f;
		Refresh();
	}
}

void UGrimCreatureStorageWidget::Refresh()
{
	using namespace GrimCreatureStorageWidgetLocal;
	UGrimCreatureStorageComponent* Owned = ResolveStorage();
	if (!Owned || !HeaderText)
	{
		if (HeaderText)
		{
			HeaderText->SetText(LOCTEXT("NoStorage", "생물 정보를 기다리는 중…"));
		}
		return;
	}
	const AGrimCreatureBox* OpenBox = bStandalone ? Box.Get() : nullptr;
	const TArray<FGrimCreatureRowView> Views = GatherRows(Owned, OpenBox);

	int32 PartyCount = 0;
	for (const FGrimCreatureRowView& View : Views)
	{
		PartyCount += View.Place == EGrimCreaturePlace::Party ? 1 : 0;
	}
	FString Header = FString::Printf(TEXT("데리고 다님 %d / %d"), PartyCount, Owned->GetPartyLimit());
	Header += FString::Printf(TEXT("    보관함 %d / %d"), Owned->GetStored().Num(), Owned->GetStorageCapacity());
	if (OpenBox)
	{
		Header += FString::Printf(TEXT("    상자 %d / %d (풀어놓음 %d / %d)"), OpenBox->GetEntries().Num(), OpenBox->GetCapacity(), OpenBox->GetReleasedCount(),
			UGrimCreatureStorageSettings::Get()->BoxMaxReleased);
	}
	HeaderText->SetText(FText::FromString(Header));
	if (StorageHint)
	{
		StorageHint->SetText(LOCTEXT("StorageHint", "「스킬북: 쿠스토디아」를 읽으면 보관함 칸이 늘어난다 — 마법탑에서 산다."));
		StorageHint->SetVisibility(Owned->IsStorageLearned() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	// 줄이 바뀌었을 때만 다시 만든다(누르는 도중에 단추가 사라지지 않게) — 그 밖에는 글만 고친다.
	FString Signature = FString::Printf(TEXT("%d|%d|"), Owned->IsStorageLearned() ? 1 : 0, OpenBox ? 1 : 0);
	for (const FGrimCreatureRowView& View : Views)
	{
		Signature += FString::Printf(TEXT("%d:%s:%d,"), static_cast<int32>(View.Place), *View.Instance.SpeciesId.ToString(), View.Instance.Seed);
	}
	if (Signature != LastSignature)
	{
		LastSignature = Signature;
		RebuildRows(Views);
	}
	for (int32 Index = 0; Index < Views.Num() && Index < RowTexts.Num(); ++Index)
	{
		if (RowTexts[Index])
		{
			RowTexts[Index]->SetText(FText::FromString(DescribeRow(Views[Index])));
		}
	}
}

void UGrimCreatureStorageWidget::RebuildRows(const TArray<FGrimCreatureRowView>& Views)
{
	using namespace GrimCreatureStorageWidgetLocal;
	PartyList->ClearChildren();
	StorageList->ClearChildren();
	BoxList->ClearChildren();
	RowKeys.Reset();
	RowTexts.Reset();
	const bool bHasBox = bStandalone && Box.IsValid();
	const float Width = ColumnWidth(bStandalone);

	for (int32 RowIndex = 0; RowIndex < Views.Num(); ++RowIndex)
	{
		const FGrimCreatureRowView& View = Views[RowIndex];
		FRowKey& Key = RowKeys.AddDefaulted_GetRef();
		Key.Place = View.Place;
		Key.SpeciesId = View.Instance.SpeciesId;
		Key.Seed = View.Instance.Seed;
		Key.Targets = GetTargets(View.Place, bHasBox);

		UBorder* RowPanel = GrimUIData::MakeBorder(WidgetTree, RowColor, FMargin(6.f, 4.f));
		UVerticalBox* RowBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		RowPanel->SetContent(RowBox);

		// 색 칸(영역마다) + 설명
		UHorizontalBox* Top = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		RowBox->AddChildToVerticalBox(Top);
		UVerticalBox* Swatches = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		UHorizontalBox* SwatchRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Swatches->AddChildToVerticalBox(SwatchRow);
		for (const FGrimCreatureRegionColor& Color : View.Instance.Colors)
		{
			UBorder* Swatch = GrimUIData::MakeBorder(WidgetTree, Color.Color.CopyWithNewOpacity(1.f), FMargin(0.f));
			SwatchRow->AddChildToHorizontalBox(GrimUIData::MakeSizeBox(WidgetTree, Swatch, 10.f, 14.f))->SetPadding(FMargin(0.f, 0.f, 2.f, 0.f));
		}
		Top->AddChildToHorizontalBox(GrimUIData::MakeSizeBox(WidgetTree, Swatches, 72.f, 0.f))->SetPadding(FMargin(0.f, 3.f, 6.f, 0.f));
		UTextBlock* Info = GrimUIData::MakeText(WidgetTree, FText::FromString(DescribeRow(View)), 12, TextColor);
		Info->SetAutoWrapText(true);
		Top->AddChildToHorizontalBox(GrimUIData::MakeSizeBox(WidgetTree, Info, Width - 100.f, 0.f));
		RowTexts.Add(Info);

		// 단추
		UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		RowBox->AddChildToVerticalBox(Buttons)->SetPadding(FMargin(78.f, 3.f, 0.f, 0.f));
		for (int32 TargetIndex = 0; TargetIndex < Key.Targets.Num() && TargetIndex < MaxTargets; ++TargetIndex)
		{
			// 포커스를 받지 않는 단추(가방과 같은 것) — 눌러도 키가 게임으로 가서 가방 키 · F 로 닫힌다.
			UGrimIdButton* Button = WidgetTree->ConstructWidget<UGrimItemButton>(UGrimItemButton::StaticClass());
			Button->Id = RowIndex * MaxTargets + TargetIndex;
			Button->Bind();
			Button->OnIdClicked.BindUObject(this, &UGrimCreatureStorageWidget::HandleRowButton);
			Button->SetBackgroundColor(ButtonColor);
			UTextBlock* Label = GrimUIData::MakeText(WidgetTree, FText::FromString(TargetLabel(View.Place, Key.Targets[TargetIndex])), 11, TextColor);
			Label->SetJustification(ETextJustify::Center);
			Button->SetContent(Label);
			Buttons->AddChildToHorizontalBox(GrimUIData::MakeSizeBox(WidgetTree, Button, 96.f, 26.f))->SetPadding(FMargin(0.f, 0.f, 4.f, 0.f));
		}

		UVerticalBox* Target = View.Place == EGrimCreaturePlace::Party ? PartyList.Get() : View.Place == EGrimCreaturePlace::Storage ? StorageList.Get() : BoxList.Get();
		Target->AddChildToVerticalBox(RowPanel)->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
	}
	if (BoxColumn)
	{
		BoxColumn->SetVisibility(bHasBox ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UGrimCreatureStorageWidget::HandleRowButton(int32 Id)
{
	using namespace GrimCreatureStorageWidgetLocal;
	const int32 RowIndex = Id / MaxTargets;
	const int32 TargetIndex = Id % MaxTargets;
	UGrimCreatureStorageComponent* Owned = ResolveStorage();
	if (!Owned || !RowKeys.IsValidIndex(RowIndex) || !RowKeys[RowIndex].Targets.IsValidIndex(TargetIndex))
	{
		return;
	}
	const FRowKey& Key = RowKeys[RowIndex];
	Owned->RequestTransfer(bStandalone ? Box.Get() : nullptr, Key.SpeciesId, Key.Seed, Key.Place, Key.Targets[TargetIndex]);
}

void UGrimCreatureStorageWidget::HandleNotice(UGrimTamerComponent* Tamer, const FString& Text)
{
	if (!NoticeText || !(bStandalone ? bOpen : bTabActive))
	{
		return;
	}
	NoticeText->SetText(FText::FromString(Text));
	NoticeText->SetVisibility(ESlateVisibility::HitTestInvisible);
	NoticeRemaining = GrimCreatureStorageWidgetLocal::NoticeSeconds;
	LastSignature.Reset();
}

#undef LOCTEXT_NAMESPACE
