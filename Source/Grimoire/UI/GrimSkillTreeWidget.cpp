#include "UI/GrimSkillTreeWidget.h"
#include "UI/GrimUIData.h"
#include "Progression/GrimLevelComponent.h"
#include "Progression/GrimProgressionSettings.h"
#include "Progression/GrimSkillTreeComponent.h"
#include "Core/GrimPlayerState.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "GrimSkillTree"

namespace GrimSkillTreeWidgetLocal // 유니티 빌드에서 이름이 부딪히지 않게
{
	const FLinearColor Ink(0.95f, 0.90f, 0.78f, 1.f);
	const FLinearColor FaintInk(0.75f, 0.70f, 0.60f, 1.f);
	const FLinearColor ActiveTab(0.85f, 0.7f, 0.4f, 1.f);
	const FLinearColor IdleTab(0.5f, 0.45f, 0.35f, 0.35f);
	const FLinearColor LockedColor(0.32f, 0.30f, 0.27f, 1.f);
	const FLinearColor LockedLine(0.4f, 0.37f, 0.32f, 0.5f);
	const FLinearColor SelectedFrame(1.f, 1.f, 1.f, 1.f);
	const FLinearColor DetailColor(0.17f, 0.13f, 0.09f, 1.f);
	const float HeaderHeight = 24.f;
	const float Margin = 8.f;
	const float LineThickness = 3.f;

	FLinearColor BranchColor(FName Branch)
	{
		const FGrimSkillBranchDisplay* Display = UGrimSkillTreeSettings::Get()->FindBranch(Branch);
		return Display ? Display->Color : FLinearColor(0.7f, 0.65f, 0.55f);
	}

	FText BranchName(FName Branch)
	{
		const FGrimSkillBranchDisplay* Display = UGrimSkillTreeSettings::Get()->FindBranch(Branch);
		return Display && !Display->DisplayName.IsEmpty() ? Display->DisplayName : FText::FromName(Branch);
	}

	FText TreeName(FName Tree)
	{
		const FGrimSkillTreeDisplay* Display = UGrimSkillTreeSettings::Get()->FindTree(Tree);
		return Display && !Display->DisplayName.IsEmpty() ? Display->DisplayName : FText::FromName(Tree);
	}

	FText KindName(EGrimSkillNodeKind Kind)
	{
		switch (Kind)
		{
		case EGrimSkillNodeKind::Power: return LOCTEXT("KindPower", "위력");
		case EGrimSkillNodeKind::Efficiency: return LOCTEXT("KindEfficiency", "효율");
		case EGrimSkillNodeKind::Status: return LOCTEXT("KindStatus", "상태");
		case EGrimSkillNodeKind::Interaction: return LOCTEXT("KindInteraction", "상호작용");
		case EGrimSkillNodeKind::Survival: return LOCTEXT("KindSurvival", "생존");
		case EGrimSkillNodeKind::Trait: return LOCTEXT("KindTrait", "특성");
		default: return FText::GetEmpty();
		}
	}

	FText StateText(EGrimSkillUnlockResult State, const FGrimSkillNode& Node)
	{
		switch (State)
		{
		case EGrimSkillUnlockResult::AlreadyUnlocked: return LOCTEXT("StateUnlocked", "찍었다");
		case EGrimSkillUnlockResult::Unlocked: return LOCTEXT("StateAvailable", "찍을 수 있다");
		case EGrimSkillUnlockResult::MissingPrerequisite: return LOCTEXT("StatePrereq", "잠김 — 이어진 앞 노드를 먼저 찍어야 한다");
		case EGrimSkillUnlockResult::CircleTooLow: return FText::Format(LOCTEXT("StateCircle", "잠김 — {0}서클 이상"), Node.RequiredCircle);
		case EGrimSkillUnlockResult::NotEnoughPoints: return LOCTEXT("StatePoints", "포인트가 모자라다");
		default: return LOCTEXT("StateUnknown", "찍을 수 없다");
		}
	}

	FVector2D NodeCenter(const FGrimSkillNode& Node)
	{
		const UGrimSkillTreeSettings* Settings = UGrimSkillTreeSettings::Get();
		return FVector2D(Margin + (Node.Grid.X + 0.5f) * Settings->GridCellWidth, HeaderHeight + (Node.Grid.Y + 0.5f) * Settings->GridCellHeight);
	}
}

TSharedRef<SWidget> UGrimSkillTreeWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

void UGrimSkillTreeWidget::NativeDestruct()
{
	if (UGrimSkillTreeComponent* Tree = BoundTree.Get())
	{
		Tree->OnUnlockResult.RemoveDynamic(this, &ThisClass::HandleUnlockResult);
	}
	BoundTree = nullptr;
	Super::NativeDestruct();
}

UGrimSkillTreeComponent* UGrimSkillTreeWidget::GetSkillTree() const
{
	const APlayerController* PC = GetOwningPlayer();
	const AGrimPlayerState* PlayerState = PC ? PC->GetPlayerState<AGrimPlayerState>() : nullptr;
	return PlayerState ? PlayerState->GetSkillTree() : nullptr;
}

void UGrimSkillTreeWidget::BuildTree()
{
	using namespace GrimSkillTreeWidgetLocal;
	const UGrimSkillTreeSettings* Settings = UGrimSkillTreeSettings::Get();
	const FGrimSkillTreeDefinition& Definition = UGrimSkillTreeSettings::GetDefinition();
	TreeOrder = Settings->GetTreeOrder(Definition);

	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	// 트리 탭 · 남은 포인트
	UHorizontalBox* TabRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Root->AddChildToVerticalBox(TabRow)->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
	TreeButtons.Reset();
	for (int32 Index = 0; Index < TreeOrder.Num(); ++Index)
	{
		UGrimIdButton* Button = WidgetTree->ConstructWidget<UGrimIdButton>(UGrimIdButton::StaticClass());
		Button->Id = Index;
		Button->Bind();
		Button->OnIdClicked.BindUObject(this, &ThisClass::HandleTreeTabClicked);
		Button->SetContent(GrimUIData::MakeText(WidgetTree, TreeName(TreeOrder[Index]), 13, Ink, true));
		TabRow->AddChildToHorizontalBox(Button)->SetPadding(FMargin(0.f, 0.f, 6.f, 0.f));
		TreeButtons.Add(Button);
	}
	PointsText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 13, Ink, true);
	UHorizontalBoxSlot* PointsSlot = TabRow->AddChildToHorizontalBox(PointsText);
	PointsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	PointsSlot->SetHorizontalAlignment(HAlign_Right);
	PointsSlot->SetVerticalAlignment(VAlign_Center);

	// 노드 격자(넓으면 가로로 민다)
	float MaxX = 0.f;
	float MaxY = 0.f;
	for (const FGrimSkillNode& Node : Definition.Nodes)
	{
		MaxX = FMath::Max(MaxX, static_cast<float>(Node.Grid.X));
		MaxY = FMath::Max(MaxY, static_cast<float>(Node.Grid.Y));
	}
	const float CanvasWidth = Margin * 2.f + (MaxX + 1.f) * Settings->GridCellWidth;
	const float CanvasHeight = HeaderHeight + (MaxY + 1.f) * Settings->GridCellHeight;

	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	Scroll->SetOrientation(Orient_Horizontal);
	Root->AddChildToVerticalBox(GrimUIData::MakeSizeBox(WidgetTree, Scroll, 0.f, CanvasHeight + 12.f));
	Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	Scroll->AddChild(GrimUIData::MakeSizeBox(WidgetTree, Canvas, CanvasWidth, CanvasHeight));

	// 고른 노드 · 찍기
	UBorder* Detail = GrimUIData::MakeBorder(WidgetTree, DetailColor, FMargin(10.f, 6.f));
	Root->AddChildToVerticalBox(Detail)->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
	UHorizontalBox* DetailRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Detail->SetContent(DetailRow);

	UVerticalBox* DetailText = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	DetailName = GrimUIData::MakeText(WidgetTree, LOCTEXT("PickNode", "노드를 누르면 여기에 자세히 나온다"), 14, Ink, true);
	DetailText->AddChildToVerticalBox(DetailName);
	DetailInfo = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 11, FaintInk);
	DetailText->AddChildToVerticalBox(DetailInfo);
	DetailBody = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 12, Ink);
	DetailBody->SetAutoWrapText(true);
	DetailText->AddChildToVerticalBox(DetailBody);
	DetailState = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 11, FaintInk, true);
	DetailText->AddChildToVerticalBox(DetailState);
	DetailRow->AddChildToHorizontalBox(DetailText)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	UVerticalBox* ButtonBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UnlockButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	UnlockButton->OnClicked.AddDynamic(this, &ThisClass::HandleUnlockClicked);
	UnlockButton->SetBackgroundColor(ActiveTab);
	UnlockButton->SetContent(GrimUIData::MakeSizeBox(WidgetTree, GrimUIData::MakeText(WidgetTree, LOCTEXT("Unlock", "찍기"), 14, Ink, true), 70.f, 0.f));
	ButtonBox->AddChildToVerticalBox(UnlockButton)->SetHorizontalAlignment(HAlign_Center);
	ResultText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 10, FaintInk);
	ButtonBox->AddChildToVerticalBox(ResultText)->SetHorizontalAlignment(HAlign_Center);
	DetailRow->AddChildToHorizontalBox(ButtonBox)->SetVerticalAlignment(VAlign_Center);

	Root->AddChildToVerticalBox(GrimUIData::MakeText(WidgetTree,
		LOCTEXT("TreeHint", "포인트는 일부러 모자라다 — 레벨 상한까지 올라도 노드의 3분의 1 남짓만 찍힌다. 다시 찍기는 마법탑의 정화 의식(트리 하나씩)."), 10, FaintInk))
		->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));

	SetTree(TreeIndex);
}

void UGrimSkillTreeWidget::SetTree(int32 InTreeIndex)
{
	TreeIndex = FMath::Clamp(InTreeIndex, 0, FMath::Max(TreeOrder.Num() - 1, 0));
	for (int32 Index = 0; Index < TreeButtons.Num(); ++Index)
	{
		TreeButtons[Index]->SetBackgroundColor(Index == TreeIndex ? GrimSkillTreeWidgetLocal::ActiveTab : GrimSkillTreeWidgetLocal::IdleTab);
	}
	SelectedNode = NAME_None;
	RebuildCanvas();
	RefreshTimer = 0.f;
}

void UGrimSkillTreeWidget::HandleTreeTabClicked(int32 InTreeIndex)
{
	SetTree(InTreeIndex);
}

void UGrimSkillTreeWidget::RebuildCanvas()
{
	using namespace GrimSkillTreeWidgetLocal;
	if (!Canvas)
	{
		return;
	}
	Canvas->ClearChildren();
	Nodes.Reset();
	Lines.Reset();
	if (!TreeOrder.IsValidIndex(TreeIndex))
	{
		Canvas->AddChildToCanvas(GrimUIData::MakeText(WidgetTree, LOCTEXT("NoTable", "성장 트리 표를 읽지 못했다"), 13, FaintInk));
		return;
	}
	const FName TreeId = TreeOrder[TreeIndex];
	const UGrimSkillTreeSettings* Settings = UGrimSkillTreeSettings::Get();
	const FGrimSkillTreeDefinition& Definition = UGrimSkillTreeSettings::GetDefinition();

	// 갈래 이름(갈래 노드들의 가운데 위)
	TMap<FName, FVector2D> BranchSpan; // X = 왼쪽, Y = 오른쪽 (격자)
	TArray<FName> BranchOrder;
	for (const FGrimSkillNode& Node : Definition.Nodes)
	{
		if (Node.Tree != TreeId)
		{
			continue;
		}
		if (FVector2D* Span = BranchSpan.Find(Node.Branch))
		{
			Span->X = FMath::Min(Span->X, Node.Grid.X);
			Span->Y = FMath::Max(Span->Y, Node.Grid.X);
		}
		else
		{
			BranchSpan.Add(Node.Branch, FVector2D(Node.Grid.X, Node.Grid.X));
			BranchOrder.Add(Node.Branch);
		}
	}
	for (const FName Branch : BranchOrder)
	{
		const FVector2D Span = BranchSpan[Branch];
		UTextBlock* Label = GrimUIData::MakeText(WidgetTree, BranchName(Branch), 12, BranchColor(Branch), true);
		Label->SetJustification(ETextJustify::Center);
		UCanvasPanelSlot* LabelSlot = Canvas->AddChildToCanvas(Label);
		const float Center = Margin + ((Span.X + Span.Y) * 0.5f + 0.5f) * Settings->GridCellWidth;
		const float Width = (Span.Y - Span.X + 1.f) * Settings->GridCellWidth;
		LabelSlot->SetPosition(FVector2D(Center - Width * 0.5f, 0.f));
		LabelSlot->SetSize(FVector2D(Width, HeaderHeight));
	}

	// 선(노드보다 먼저 = 아래에 그려진다)
	for (const FGrimSkillNode& Node : Definition.Nodes)
	{
		if (Node.Tree != TreeId)
		{
			continue;
		}
		for (const FName PrerequisiteId : Node.Prerequisites)
		{
			const FGrimSkillNode* Prerequisite = Definition.Find(PrerequisiteId);
			if (!Prerequisite)
			{
				continue;
			}
			const FVector2D A = NodeCenter(*Prerequisite);
			const FVector2D B = NodeCenter(Node);
			const FVector2D Delta = B - A;
			const float Length = Delta.Size();
			UBorder* Line = GrimUIData::MakeBorder(WidgetTree, LockedLine, FMargin(0.f));
			Line->SetVisibility(ESlateVisibility::HitTestInvisible);
			UCanvasPanelSlot* LineSlot = Canvas->AddChildToCanvas(Line);
			const FVector2D Mid = (A + B) * 0.5f;
			LineSlot->SetPosition(FVector2D(Mid.X - Length * 0.5f, Mid.Y - LineThickness * 0.5f));
			LineSlot->SetSize(FVector2D(Length, LineThickness));
			Line->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
			Line->SetRenderTransformAngle(FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X)));
			Lines.Add({ PrerequisiteId, Node.Id, Line });
		}
	}

	// 노드
	for (int32 Index = 0; Index < Definition.Nodes.Num(); ++Index)
	{
		const FGrimSkillNode& Node = Definition.Nodes[Index];
		if (Node.Tree != TreeId)
		{
			continue;
		}
		FNodeWidgets& Widgets = Nodes.AddDefaulted_GetRef();
		Widgets.Id = Node.Id;
		Widgets.Branch = Node.Branch;
		Widgets.Button = WidgetTree->ConstructWidget<UGrimIdButton>(UGrimIdButton::StaticClass());
		Widgets.Button->Id = Index;
		Widgets.Button->Bind();
		Widgets.Button->OnIdClicked.BindUObject(this, &ThisClass::HandleNodeClicked);
		Widgets.Button->SetBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.f));

		// 테(상태) 안에 속(갈래색) 안에 이름
		Widgets.Frame = GrimUIData::MakeBorder(WidgetTree, LockedColor, FMargin(Node.Kind == EGrimSkillNodeKind::Trait ? 3.f : 2.f));
		Widgets.Fill = GrimUIData::MakeBorder(WidgetTree, LockedColor, FMargin(2.f));
		Widgets.Fill->SetHorizontalAlignment(HAlign_Center);
		Widgets.Fill->SetVerticalAlignment(VAlign_Center);
		Widgets.Name = GrimUIData::MakeText(WidgetTree, Node.DisplayName, 9, Ink, Node.Kind == EGrimSkillNodeKind::Trait);
		Widgets.Name->SetJustification(ETextJustify::Center);
		Widgets.Name->SetAutoWrapText(true);
		Widgets.Fill->SetContent(Widgets.Name);
		Widgets.Frame->SetContent(Widgets.Fill);
		Widgets.Button->SetContent(Widgets.Frame);

		FVector2D Size = Settings->NodeSize;
		if (Node.Kind == EGrimSkillNodeKind::Trait)
		{
			Size *= 1.15f; // 큰 노드
		}
		UCanvasPanelSlot* NodeSlot = Canvas->AddChildToCanvas(Widgets.Button);
		NodeSlot->SetPosition(NodeCenter(Node) - Size * 0.5f);
		NodeSlot->SetSize(Size);
	}
}

void UGrimSkillTreeWidget::HandleNodeClicked(int32 NodeIndex)
{
	const FGrimSkillTreeDefinition& Definition = UGrimSkillTreeSettings::GetDefinition();
	if (Definition.Nodes.IsValidIndex(NodeIndex))
	{
		SelectedNode = Definition.Nodes[NodeIndex].Id;
		RefreshTimer = 0.f;
	}
}

void UGrimSkillTreeWidget::HandleUnlockClicked()
{
	if (UGrimSkillTreeComponent* Tree = GetSkillTree(); Tree && !SelectedNode.IsNone())
	{
		Tree->RequestUnlock(SelectedNode);
		RefreshTimer = 0.f;
	}
}

void UGrimSkillTreeWidget::HandleUnlockResult(UGrimSkillTreeComponent* SkillTree, FName NodeId, EGrimSkillUnlockResult Result)
{
	if (!ResultText)
	{
		return;
	}
	const FGrimSkillNode* Node = UGrimSkillTreeSettings::GetDefinition().Find(NodeId);
	ResultText->SetText(Result == EGrimSkillUnlockResult::Unlocked
		? FText::Format(LOCTEXT("ResultOk", "{0} 을(를) 찍었다"), Node ? Node->DisplayName : FText::FromName(NodeId))
		: (Node ? GrimSkillTreeWidgetLocal::StateText(Result, *Node) : LOCTEXT("ResultUnknown", "없는 노드")));
	ResultRemaining = 3.f;
	RefreshTimer = 0.f;
}

void UGrimSkillTreeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (ResultRemaining > 0.f)
	{
		ResultRemaining -= InDeltaTime;
		if (ResultRemaining <= 0.f && ResultText)
		{
			ResultText->SetText(FText::GetEmpty());
		}
	}
	RefreshTimer -= InDeltaTime;
	if (RefreshTimer <= 0.f)
	{
		RefreshTimer = 0.15f;
		Refresh();
	}
}

void UGrimSkillTreeWidget::Refresh()
{
	using namespace GrimSkillTreeWidgetLocal;
	UGrimSkillTreeComponent* Tree = GetSkillTree();
	if (Tree != BoundTree.Get())
	{
		if (UGrimSkillTreeComponent* Old = BoundTree.Get())
		{
			Old->OnUnlockResult.RemoveDynamic(this, &ThisClass::HandleUnlockResult);
		}
		BoundTree = Tree;
		if (Tree)
		{
			Tree->OnUnlockResult.AddUniqueDynamic(this, &ThisClass::HandleUnlockResult);
		}
	}

	if (PointsText)
	{
		if (Tree && TreeOrder.IsValidIndex(TreeIndex))
		{
			const UGrimLevelComponent* Level = UGrimLevelComponent::FindFor(Tree->GetOwner());
			PointsText->SetText(FText::Format(LOCTEXT("Points", "Lv {0} · 남은 포인트 {1} · 이 트리에 쓴 포인트 {2}"),
				Level ? Level->GetLevel() : 1, Tree->GetAvailablePoints(), Tree->GetSpentPoints(TreeOrder[TreeIndex])));
		}
		else
		{
			PointsText->SetText(FText::GetEmpty());
		}
	}

	const FGrimSkillTreeDefinition& Definition = UGrimSkillTreeSettings::GetDefinition();
	for (FNodeWidgets& Widgets : Nodes)
	{
		const FGrimSkillNode* Node = Definition.Find(Widgets.Id);
		if (!Node)
		{
			continue;
		}
		const EGrimSkillUnlockResult State = Tree ? Tree->CanUnlock(Widgets.Id) : EGrimSkillUnlockResult::Invalid;
		const FLinearColor Branch = BranchColor(Widgets.Branch);
		FLinearColor FillColor = LockedColor;
		FLinearColor FrameColor = LockedColor;
		FLinearColor TextColor = FaintInk;
		switch (State)
		{
		case EGrimSkillUnlockResult::AlreadyUnlocked:
			FillColor = Branch;
			FrameColor = FLinearColor(1.f, 0.85f, 0.4f);
			TextColor = FLinearColor::White;
			break;
		case EGrimSkillUnlockResult::Unlocked:
			FillColor = Branch * FLinearColor(0.45f, 0.45f, 0.45f, 1.f);
			FrameColor = Branch;
			TextColor = Ink;
			break;
		case EGrimSkillUnlockResult::NotEnoughPoints:
			FillColor = Branch * FLinearColor(0.3f, 0.3f, 0.3f, 1.f);
			FrameColor = Branch * FLinearColor(0.6f, 0.6f, 0.6f, 1.f);
			break;
		default:
			break;
		}
		if (Widgets.Id == SelectedNode)
		{
			FrameColor = SelectedFrame;
		}
		Widgets.Fill->SetBrushColor(FillColor);
		Widgets.Frame->SetBrushColor(FrameColor);
		Widgets.Name->SetColorAndOpacity(FSlateColor(TextColor));
	}
	for (FLineWidgets& Line : Lines)
	{
		const bool bLit = Tree && Tree->IsUnlocked(Line.From);
		const FGrimSkillNode* To = Definition.Find(Line.To);
		Line.Line->SetBrushColor(bLit && To ? BranchColor(To->Branch) : LockedLine);
	}

	RefreshDetail(Tree);
}

void UGrimSkillTreeWidget::RefreshDetail(const UGrimSkillTreeComponent* Tree)
{
	using namespace GrimSkillTreeWidgetLocal;
	if (!DetailName)
	{
		return;
	}
	const FGrimSkillTreeDefinition& Definition = UGrimSkillTreeSettings::GetDefinition();
	const FGrimSkillNode* Node = Definition.Find(SelectedNode);
	if (!Node)
	{
		DetailName->SetText(LOCTEXT("PickNode2", "노드를 누르면 여기에 자세히 나온다"));
		DetailInfo->SetText(FText::GetEmpty());
		DetailBody->SetText(FText::GetEmpty());
		DetailState->SetText(FText::GetEmpty());
		UnlockButton->SetIsEnabled(false);
		return;
	}

	DetailName->SetText(Node->DisplayName);
	DetailName->SetColorAndOpacity(FSlateColor(BranchColor(Node->Branch)));

	TArray<FString> PrerequisiteNames;
	for (const FName PrerequisiteId : Node->Prerequisites)
	{
		const FGrimSkillNode* Prerequisite = Definition.Find(PrerequisiteId);
		PrerequisiteNames.Add(Prerequisite ? Prerequisite->DisplayName.ToString() : PrerequisiteId.ToString());
	}
	FText Info = FText::Format(LOCTEXT("Info", "{0} · {1} · 비용 {2}포인트"), BranchName(Node->Branch), KindName(Node->Kind), Node->Cost);
	if (Node->RequiredCircle > 1)
	{
		Info = FText::Format(LOCTEXT("InfoCircle", "{0} · {1}서클 이상"), Info, Node->RequiredCircle);
	}
	if (!PrerequisiteNames.IsEmpty())
	{
		Info = FText::Format(LOCTEXT("InfoPrereq", "{0} · 먼저: {1}"), Info, FText::FromString(FString::Join(PrerequisiteNames, TEXT(", "))));
	}
	DetailInfo->SetText(Info);
	DetailBody->SetText(Node->Description);

	const EGrimSkillUnlockResult State = Tree ? Tree->CanUnlock(Node->Id) : EGrimSkillUnlockResult::Invalid;
	DetailState->SetText(StateText(State, *Node));
	DetailState->SetColorAndOpacity(FSlateColor(State == EGrimSkillUnlockResult::Unlocked ? FLinearColor(0.6f, 1.f, 0.6f) : FaintInk));
	UnlockButton->SetIsEnabled(State == EGrimSkillUnlockResult::Unlocked);
}

#undef LOCTEXT_NAMESPACE
