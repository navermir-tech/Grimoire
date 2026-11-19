#include "UI/GrimZoneBannerWidget.h"

#include "UI/GrimUIData.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace GrimZoneBannerLocal
{
	const FLinearColor SubtitleColor(0.93f, 0.91f, 0.86f, 1.f);
	const FLinearColor RuleColor(1.f, 0.88f, 0.62f, 0.75f);
	/** 장식 줄 폭 = 이름 글씨 크기 × 이 값(큰 이름일수록 길게) */
	constexpr float RuleWidthPerPoint = 9.f;
}

TSharedRef<SWidget> UGrimZoneBannerWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

void UGrimZoneBannerWidget::BuildTree()
{
	using namespace GrimZoneBannerLocal;

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Canvas;
	Canvas->SetVisibility(ESlateVisibility::HitTestInvisible);

	Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UCanvasPanelSlot* ColumnSlot = Canvas->AddChildToCanvas(Column);
	const float Top = UGrimZoneSettings::Get()->BannerTopFraction;
	ColumnSlot->SetAnchors(FAnchors(0.5f, Top));
	ColumnSlot->SetAlignment(FVector2D(0.5f, 0.f));
	ColumnSlot->SetPosition(FVector2D::ZeroVector);
	ColumnSlot->SetAutoSize(true);

	TitleText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 44, FLinearColor::White, true);
	TitleText->SetJustification(ETextJustify::Center);
	Column->AddChildToVerticalBox(TitleText)->SetHorizontalAlignment(HAlign_Center);

	UBorder* Rule = GrimUIData::MakeBorder(WidgetTree, RuleColor, FMargin(0.f));
	RuleBox = GrimUIData::MakeSizeBox(WidgetTree, Rule, 400.f, 2.f);
	UVerticalBoxSlot* RuleSlot = Column->AddChildToVerticalBox(RuleBox);
	RuleSlot->SetHorizontalAlignment(HAlign_Center);
	RuleSlot->SetPadding(FMargin(0.f, 6.f, 0.f, 8.f));

	SubtitleText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 18, SubtitleColor);
	SubtitleText->SetJustification(ETextJustify::Center);
	Column->AddChildToVerticalBox(SubtitleText)->SetHorizontalAlignment(HAlign_Center);

	Column->SetVisibility(ESlateVisibility::Collapsed);
}

void UGrimZoneBannerWidget::Enqueue(const FGrimZoneDef& Zone, bool bFirstVisit)
{
	FEntry Entry;
	Entry.Zone = Zone.Name;
	Entry.Title = Zone.Label;
	Entry.Subtitle = Zone.GetDisplaySubtitle();
	Entry.Tier = Zone.Tier;
	Entry.bFirstVisit = bFirstVisit;

	// 「처음」이 오면 떠 있는 「다시」를 지금 불투명도에서 바로 사라지게
	if (bFirstVisit && bActive && !Showing.bFirstVisit)
	{
		const float Alpha = ComputeAlpha(Style, Elapsed);
		Elapsed = FMath::Max(Elapsed, Style.FadeInSeconds + Style.HoldSeconds + (1.f - Alpha) * Style.FadeOutSeconds);
	}

	Queue.Add(MoveTemp(Entry));
	const int32 MaxQueued = FMath::Max(1, UGrimZoneSettings::Get()->MaxQueuedBanners);
	while (Queue.Num() > MaxQueued)
	{
		const int32 Revisit = Queue.IndexOfByPredicate([](const FEntry& Item) { return !Item.bFirstVisit; });
		Queue.RemoveAt(Revisit != INDEX_NONE ? Revisit : 0);
	}
}

float UGrimZoneBannerWidget::ComputeAlpha(const FGrimZoneBannerStyle& InStyle, float InElapsed)
{
	if (InElapsed < 0.f)
	{
		return 0.f;
	}
	if (InElapsed < InStyle.FadeInSeconds)
	{
		return InElapsed / InStyle.FadeInSeconds;
	}
	const float HoldEnd = InStyle.FadeInSeconds + InStyle.HoldSeconds;
	if (InElapsed <= HoldEnd)
	{
		return 1.f;
	}
	if (InStyle.FadeOutSeconds <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(1.f - (InElapsed - HoldEnd) / InStyle.FadeOutSeconds, 0.f, 1.f);
}

void UGrimZoneBannerWidget::Begin(const FEntry& Entry)
{
	using namespace GrimZoneBannerLocal;
	Showing = Entry;
	Style = UGrimZoneSettings::Get()->GetStyle(Entry.Tier, Entry.bFirstVisit);
	Elapsed = 0.f;
	bActive = true;

	FSlateFontInfo TitleFont = GrimUIData::Font(Style.TitleSize, true);
	TitleFont.OutlineSettings.OutlineSize = FMath::Max(1, Style.TitleSize / 22);
	TitleFont.OutlineSettings.OutlineColor = FLinearColor(0.f, 0.f, 0.f, 0.7f);
	TitleText->SetFont(TitleFont);
	TitleText->SetColorAndOpacity(FSlateColor(Style.TitleColor));
	TitleText->SetText(Entry.Title);

	RuleBox->SetWidthOverride(Style.TitleSize * RuleWidthPerPoint);
	RuleBox->SetVisibility(Style.bShowRule ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	const bool bSubtitle = Style.bShowSubtitle && !Entry.Subtitle.IsEmpty();
	FSlateFontInfo SubtitleFont = GrimUIData::Font(Style.SubtitleSize);
	SubtitleFont.OutlineSettings.OutlineSize = 1;
	SubtitleFont.OutlineSettings.OutlineColor = FLinearColor(0.f, 0.f, 0.f, 0.6f);
	SubtitleText->SetFont(SubtitleFont);
	SubtitleText->SetText(Entry.Subtitle);
	SubtitleText->SetVisibility(bSubtitle ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	Column->SetVisibility(ESlateVisibility::HitTestInvisible);
	ApplyAlpha(0.f);
}

void UGrimZoneBannerWidget::ApplyAlpha(float Alpha)
{
	if (Column)
	{
		// 살짝 내려오며 나타난다(위 → 제자리 8 px)
		Column->SetRenderOpacity(Alpha);
		Column->SetRenderTranslation(FVector2D(0.f, -8.f * (1.f - FMath::Min(1.f, Elapsed / FMath::Max(Style.FadeInSeconds, 0.01f)))));
	}
}

void UGrimZoneBannerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!Column)
	{
		return;
	}
	if (!bActive)
	{
		if (Queue.Num() == 0)
		{
			return;
		}
		const FEntry Next = Queue[0];
		Queue.RemoveAt(0);
		Begin(Next);
	}
	Elapsed += InDeltaTime;
	ApplyAlpha(ComputeAlpha(Style, Elapsed));
	if (Elapsed >= Style.GetTotalSeconds())
	{
		bActive = false;
		Column->SetVisibility(ESlateVisibility::Collapsed);
	}
}
