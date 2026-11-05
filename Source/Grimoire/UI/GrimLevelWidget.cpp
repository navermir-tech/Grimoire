#include "UI/GrimLevelWidget.h"
#include "UI/GrimUIData.h"
#include "UI/GrimUISettings.h"
#include "Progression/GrimLevelComponent.h"
#include "Progression/GrimSkillTreeComponent.h"
#include "Core/GrimPlayerState.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Misc/App.h"
#include "UObject/WeakObjectPtrTemplates.h"

#define LOCTEXT_NAMESPACE "GrimLevelWidget"

namespace GrimLevelWidgetLocal // 유니티 빌드에서 이름이 부딪히지 않게
{
	const FLinearColor PanelColor(0.08f, 0.07f, 0.05f, 0.7f);
	const FLinearColor Ink(0.95f, 0.90f, 0.78f, 1.f);
	const FLinearColor FaintInk(0.75f, 0.70f, 0.60f, 1.f);
	const FLinearColor XPColor(0.55f, 0.85f, 0.45f, 1.f);
	const FLinearColor LevelUpColor(1.f, 0.85f, 0.35f, 1.f);

	bool GStandaloneVisible = false;
	TArray<TWeakObjectPtr<UGrimLevelWidget>> GStandaloneWidgets;

	FAutoConsoleCommand GLevelWidgetCommand(TEXT("Grim.UI.Level"),
		TEXT("레벨 · 경험 막대를 단독으로 띄우기 [0|1] (없으면 토글) — HUD 에 끼우기 전 확인용"),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			const bool bVisible = Args.Num() > 0 ? FCString::Atoi(*Args[0]) != 0 : !UGrimLevelWidget::IsStandaloneVisible();
			UGrimLevelWidget::SetStandaloneVisible(bVisible);
		}));

	FText FormatXP(float Value)
	{
		return FText::AsNumber(FMath::FloorToInt(Value));
	}
}

bool UGrimLevelWidget::IsStandaloneVisible()
{
	return GrimLevelWidgetLocal::GStandaloneVisible;
}

void UGrimLevelWidget::SetStandaloneVisible(bool bVisible)
{
	using namespace GrimLevelWidgetLocal;
	GStandaloneVisible = bVisible;
	for (const TWeakObjectPtr<UGrimLevelWidget>& Widget : GStandaloneWidgets)
	{
		if (Widget.IsValid())
		{
			Widget->RemoveFromParent();
		}
	}
	GStandaloneWidgets.Reset();
	if (!bVisible || !GEngine || !FApp::CanEverRender())
	{
		return;
	}
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		const UGameInstance* GameInstance = Context.OwningGameInstance;
		if (!GameInstance || !Context.World() || !Context.World()->IsGameWorld())
		{
			continue;
		}
		for (ULocalPlayer* LocalPlayer : GameInstance->GetLocalPlayers())
		{
			APlayerController* PC = LocalPlayer ? LocalPlayer->PlayerController.Get() : nullptr;
			if (!PC)
			{
				continue;
			}
			if (UGrimLevelWidget* Widget = CreateWidget<UGrimLevelWidget>(PC, UGrimLevelWidget::StaticClass(), TEXT("GrimLevel")))
			{
				Widget->AddToPlayerScreen(5);
				Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
				GStandaloneWidgets.Add(Widget);
			}
		}
	}
}

TSharedRef<SWidget> UGrimLevelWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

void UGrimLevelWidget::BuildTree()
{
	using namespace GrimLevelWidgetLocal;

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Canvas;

	UBorder* Panel = GrimUIData::MakeBorder(WidgetTree, PanelColor, FMargin(10.f, 6.f));
	UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Panel);
	// 화면 아래 가운데(HUD 의 마법 슬롯 위). HUD 에 끼우면 HUD 가 자리를 정한다.
	PanelSlot->SetAnchors(FAnchors(0.5f, 1.f));
	PanelSlot->SetAlignment(FVector2D(0.5f, 1.f));
	PanelSlot->SetPosition(FVector2D(0.f, -150.f));
	PanelSlot->SetAutoSize(true);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->SetContent(Column);

	LevelUpText = GrimUIData::MakeText(WidgetTree, LOCTEXT("LevelUp", "레벨 업!"), 18, LevelUpColor, true);
	LevelUpText->SetJustification(ETextJustify::Center);
	LevelUpText->SetVisibility(ESlateVisibility::Collapsed);
	Column->AddChildToVerticalBox(LevelUpText)->SetHorizontalAlignment(HAlign_Center);

	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Column->AddChildToVerticalBox(Line);

	LevelText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 16, Ink, true);
	Line->AddChildToHorizontalBox(GrimUIData::MakeSizeBox(WidgetTree, LevelText, 64.f, 0.f))->SetVerticalAlignment(VAlign_Center);

	UVerticalBox* BarBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	XPBar = GrimUIData::MakeBar(WidgetTree, XPColor);
	BarBox->AddChildToVerticalBox(GrimUIData::MakeSizeBox(WidgetTree, XPBar, 260.f, 10.f))->SetPadding(FMargin(0.f, 2.f));
	XPText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 10, FaintInk);
	BarBox->AddChildToVerticalBox(XPText);
	Line->AddChildToHorizontalBox(BarBox)->SetVerticalAlignment(VAlign_Center);

	PointsText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 11, Ink);
	Line->AddChildToHorizontalBox(PointsText)->SetPadding(FMargin(10.f, 0.f, 0.f, 0.f));
}

void UGrimLevelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Grace = FMath::Max(Grace - InDeltaTime, 0.f);

	if (LevelUpRemaining > 0.f)
	{
		LevelUpRemaining -= InDeltaTime;
		if (LevelUpRemaining <= 0.f && LevelUpText)
		{
			LevelUpText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	RefreshTimer -= InDeltaTime;
	if (RefreshTimer <= 0.f)
	{
		RefreshTimer = 0.1f;
		Refresh();
	}
}

void UGrimLevelWidget::Refresh()
{
	if (!LevelText)
	{
		return;
	}
	const APlayerController* PC = GetOwningPlayer();
	const AGrimPlayerState* PlayerState = PC ? PC->GetPlayerState<AGrimPlayerState>() : nullptr;
	const UGrimLevelComponent* Level = PlayerState ? PlayerState->GetCharacterLevel() : nullptr;
	const UGrimSkillTreeComponent* Tree = PlayerState ? PlayerState->GetSkillTree() : nullptr;
	if (!Level)
	{
		LevelText->SetText(FText::GetEmpty());
		XPText->SetText(FText::GetEmpty());
		PointsText->SetText(FText::GetEmpty());
		XPBar->SetPercent(0.f);
		return;
	}

	const FGrimLevelProgress Progress = Level->GetProgress();
	LevelText->SetText(FText::Format(LOCTEXT("Level", "Lv {0}"), Progress.Level));
	XPBar->SetPercent(Progress.Fraction);
	XPText->SetText(Progress.bAtCap
		? FText::Format(LOCTEXT("XPMax", "경험 {0} — 최고 레벨"), GrimLevelWidgetLocal::FormatXP(Progress.TotalXP))
		: FText::Format(LOCTEXT("XP", "경험 {0} / {1}"), GrimLevelWidgetLocal::FormatXP(Progress.TotalXP), GrimLevelWidgetLocal::FormatXP(Progress.NextLevelXP)));
	const int32 Points = Tree ? Tree->GetAvailablePoints() : Level->GetTotalSkillPoints();
	PointsText->SetText(Points > 0 ? FText::Format(LOCTEXT("Points", "성장 포인트 {0}"), Points) : FText::GetEmpty());

	if (LastLevel != INDEX_NONE && Progress.Level > LastLevel && Grace <= 0.f)
	{
		LevelUpText->SetText(FText::Format(LOCTEXT("LevelUpTo", "레벨 업! — Lv {0}"), Progress.Level));
		LevelUpText->SetVisibility(ESlateVisibility::HitTestInvisible);
		LevelUpRemaining = UGrimUISettings::Get()->ToastSeconds;
	}
	LastLevel = Progress.Level;
}

#undef LOCTEXT_NAMESPACE
