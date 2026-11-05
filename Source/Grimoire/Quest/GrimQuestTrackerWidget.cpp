#include "Quest/GrimQuestTrackerWidget.h"
#include "Quest/GrimQuestSettings.h"
#include "UI/GrimUIData.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#define LOCTEXT_NAMESPACE "GrimQuestTracker"

namespace GrimQuestTrackerLocal
{
	const FLinearColor TitleColor(1.f, 0.85f, 0.45f);
	const FLinearColor BodyColor(0.92f, 0.92f, 0.92f);
	const FLinearColor PanelColor(0.f, 0.f, 0.f, 0.35f);
	constexpr float PanelWidth = 380.f;
}

void UGrimQuestTrackerWidget::SetQuestComponent(UGrimQuestComponent* InQuest)
{
	if (UGrimQuestComponent* Old = Quest.Get())
	{
		Old->OnNotice.Remove(NoticeHandle);
	}
	Quest = InQuest;
	if (InQuest)
	{
		NoticeHandle = InQuest->OnNotice.AddUObject(this, &UGrimQuestTrackerWidget::HandleNotice);
	}
}

void UGrimQuestTrackerWidget::NativeDestruct()
{
	if (UGrimQuestComponent* Old = Quest.Get())
	{
		Old->OnNotice.Remove(NoticeHandle);
	}
	Super::NativeDestruct();
}

TSharedRef<SWidget> UGrimQuestTrackerWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

void UGrimQuestTrackerWidget::BuildTree()
{
	using namespace GrimQuestTrackerLocal;

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Canvas;
	Canvas->SetVisibility(ESlateVisibility::HitTestInvisible);

	UBorder* Panel = GrimUIData::MakeBorder(WidgetTree, PanelColor, FMargin(12.f, 10.f));
	PanelRoot = GrimUIData::MakeSizeBox(WidgetTree, Panel, PanelWidth, 0.f);
	UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(PanelRoot);
	PanelSlot->SetAnchors(FAnchors(1.f, 0.f));
	PanelSlot->SetAlignment(FVector2D(1.f, 0.f));
	PanelSlot->SetPosition(FVector2D(-24.f, 120.f));
	PanelSlot->SetAutoSize(true);

	List = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->SetContent(List);

	NoticeText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 15, TitleColor, true);
	NoticeText->SetAutoWrapText(true);
	NoticeText->SetVisibility(ESlateVisibility::Collapsed);
	List->AddChildToVerticalBox(NoticeText)->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
}

void UGrimQuestTrackerWidget::DescribeProgress(const FGrimQuestDatabase& Database, const FGrimQuestProgress& Progress, float ServerTime, FString& OutTitle, FString& OutBody)
{
	const FGrimQuestDef* Def = Database.Find(Progress.QuestId);
	if (!Def || !Def->Steps.IsValidIndex(Progress.Step))
	{
		OutTitle = Progress.QuestId.ToString();
		OutBody.Reset();
		return;
	}
	OutTitle = Def->Title;
	const FGrimQuestStepDef& Step = Def->Steps[Progress.Step];
	OutBody = Step.Text;
	for (int32 Index = 0; Index < Step.Objectives.Num(); ++Index)
	{
		const FGrimQuestObjectiveDef& Objective = Step.Objectives[Index];
		if (Objective.Text.IsEmpty())
		{
			continue;
		}
		const int32 Count = Progress.Counts.IsValidIndex(Index) ? Progress.Counts[Index] : 0;
		const int32 Need = FMath::Max(1, Objective.Count);
		const TCHAR* Mark = Count >= Need ? TEXT("■") : TEXT("□");
		OutBody += Need > 1
			? FString::Printf(TEXT("\n %s %s %d/%d"), Mark, *Objective.Text, Count, Need)
			: FString::Printf(TEXT("\n %s %s"), Mark, *Objective.Text);
	}
	if (Progress.Deadline > 0.f)
	{
		OutBody += FString::Printf(TEXT("\n 남은 시간 %d초"), FMath::Max(0, FMath::CeilToInt(Progress.Deadline - ServerTime)));
	}
}

void UGrimQuestTrackerWidget::Refresh()
{
	using namespace GrimQuestTrackerLocal;
	const UGrimQuestComponent* QuestComponent = Quest.Get();
	if (!QuestComponent || !List)
	{
		return;
	}
	const FGrimQuestLog& Log = QuestComponent->GetLog();
	const FGrimQuestDatabase& Database = UGrimQuestSettings::GetQuestDatabase();

	// 시험(시간 제한)을 위에 둔다.
	TArray<const FGrimQuestProgress*> Ordered;
	for (const FGrimQuestProgress& Progress : Log.Active)
	{
		const FGrimQuestDef* Def = Database.Find(Progress.QuestId);
		Ordered.Insert(&Progress, Def && Def->IsTrial() ? 0 : Ordered.Num());
	}

	while (TitleTexts.Num() < Ordered.Num())
	{
		UTextBlock* Title = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 16, TitleColor, true);
		UTextBlock* Body = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 13, BodyColor);
		Title->SetAutoWrapText(true);
		Body->SetAutoWrapText(true);
		List->AddChildToVerticalBox(Title)->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
		List->AddChildToVerticalBox(Body)->SetPadding(FMargin(0.f, 2.f, 0.f, 6.f));
		TitleTexts.Add(Title);
		BodyTexts.Add(Body);
	}

	const float ServerTime = QuestComponent->GetServerTime();
	for (int32 Index = 0; Index < TitleTexts.Num(); ++Index)
	{
		const bool bUsed = Ordered.IsValidIndex(Index);
		TitleTexts[Index]->SetVisibility(bUsed ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		BodyTexts[Index]->SetVisibility(bUsed ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (bUsed)
		{
			FString Title, Body;
			DescribeProgress(Database, *Ordered[Index], ServerTime, Title, Body);
			TitleTexts[Index]->SetText(FText::FromString(Title));
			BodyTexts[Index]->SetText(FText::FromString(Body));
		}
	}

	// 뿌리는 숨기지 않는다(숨기면 틱이 멈춘다) — 패널만.
	const bool bAnything = Ordered.Num() > 0 || NoticeRemaining > 0.f;
	if (PanelRoot)
	{
		PanelRoot->SetVisibility(bAnything ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UGrimQuestTrackerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
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
	if (RefreshAccumulator >= 0.2f)
	{
		RefreshAccumulator = 0.f;
		Refresh();
	}
}

void UGrimQuestTrackerWidget::HandleNotice(UGrimQuestComponent* InQuest, EGrimQuestNotice Notice, FName QuestId, int32 Step)
{
	const FGrimQuestDef* Def = UGrimQuestSettings::GetQuestDatabase().Find(QuestId);
	if (!Def || !NoticeText)
	{
		return;
	}
	FText Text;
	switch (Notice)
	{
	case EGrimQuestNotice::QuestStarted:
		Text = FText::Format(LOCTEXT("Started", "새 퀘스트: {0}"), FText::FromString(Def->Title));
		break;
	case EGrimQuestNotice::StepStarted:
		Text = Def->Steps.IsValidIndex(Step) ? FText::FromString(Def->Steps[Step].Text) : FText::GetEmpty();
		break;
	case EGrimQuestNotice::QuestCompleted:
		Text = Def->IsTrial()
			? FText::Format(LOCTEXT("TrialPassed", "{0} 통과 — {1}서클"), FText::FromString(Def->Title), FText::AsNumber(Def->TrialCircle))
			: FText::Format(LOCTEXT("Completed", "퀘스트 완료: {0}"), FText::FromString(Def->Title));
		break;
	case EGrimQuestNotice::QuestFailed:
		Text = FText::Format(LOCTEXT("Failed", "{0} 실패 — 스승에게 다시 청할 수 있다"), FText::FromString(Def->Title));
		break;
	}
	if (Text.IsEmpty())
	{
		return;
	}
	NoticeText->SetText(Text);
	NoticeText->SetVisibility(ESlateVisibility::HitTestInvisible);
	NoticeRemaining = UGrimQuestSettings::Get()->NoticeSeconds;
	Refresh();
}

#undef LOCTEXT_NAMESPACE
