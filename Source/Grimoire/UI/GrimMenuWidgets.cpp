#include "UI/GrimMenuWidgets.h"
#include "UI/GrimUIData.h"
#include "Online/GrimMenuSubsystem.h"
#include "Online/GrimOnlineSettings.h"
#include "Online/GrimUserOptions.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CheckBox.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

#define LOCTEXT_NAMESPACE "GrimMenu"

namespace GrimMenuWidgetsLocal // 유니티 빌드에서 이름이 부딪히지 않게
{
	const FLinearColor Backdrop(0.03f, 0.03f, 0.05f, 1.f);
	const FLinearColor Dim(0.f, 0.f, 0.f, 0.6f);
	const FLinearColor PanelColor(0.08f, 0.07f, 0.05f, 0.94f);
	const FLinearColor Ink(0.95f, 0.90f, 0.78f, 1.f);
	const FLinearColor Faint(0.72f, 0.68f, 0.60f, 1.f);
	const FLinearColor Gold(1.f, 0.85f, 0.45f, 1.f);
	const FLinearColor ErrorInk(1.f, 0.48f, 0.42f, 1.f);
	const FLinearColor InfoInk(0.60f, 0.88f, 0.60f, 1.f);
	const FLinearColor ButtonColor(0.30f, 0.26f, 0.20f, 1.f);

	// 버튼 번호
	enum : int32
	{
		B_New = 1, B_Continue, B_Host, B_Join, B_Settings, B_Quit, B_Back,
		B_NewSolo, B_NewHost, B_ContSolo, B_ContHost, B_HostStart, B_HostMinus, B_HostPlus,
		B_JoinStart, B_FindLAN, B_SettingsSave, B_Cancel,
		B_SlotBase = 1000, B_LANBase = 2000
	};

	enum : int32
	{
		P_Resume = 1, P_Settings, P_Invite, P_MainMenu, P_Quit, P_Save, P_Back
	};

	UGrimIdButton* MakeButton(UWidgetTree* Tree, const FText& Text, int32 Id, UTextBlock** OutLabel = nullptr, int32 FontSize = 15)
	{
		UGrimIdButton* Button = Tree->ConstructWidget<UGrimIdButton>(UGrimIdButton::StaticClass());
		Button->Id = Id;
		Button->Bind();
		Button->SetBackgroundColor(ButtonColor);
		UTextBlock* Label = GrimUIData::MakeText(Tree, Text, FontSize, Ink);
		Label->SetJustification(ETextJustify::Center);
		Button->SetContent(Label);
		if (OutLabel)
		{
			*OutLabel = Label;
		}
		return Button;
	}

	UVerticalBoxSlot* AddRow(UVerticalBox* Box, UWidget* Widget, float Top = 4.f)
	{
		UVerticalBoxSlot* Slot = Box->AddChildToVerticalBox(Widget);
		Slot->SetPadding(FMargin(0.f, Top, 0.f, 0.f));
		Slot->SetHorizontalAlignment(HAlign_Fill);
		return Slot;
	}

	UEditableTextBox* MakeTextBox(UWidgetTree* Tree, const FText& Hint)
	{
		UEditableTextBox* Box = Tree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
		Box->SetHintText(Hint);
		return Box;
	}

	/** 「이름표 + 칸」 한 줄 */
	UHorizontalBox* MakeLabeled(UWidgetTree* Tree, const FText& Label, UWidget* Widget, float LabelWidth = 120.f)
	{
		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UTextBlock* Text = GrimUIData::MakeText(Tree, Label, 13, Faint);
		Row->AddChildToHorizontalBox(GrimUIData::MakeSizeBox(Tree, Text, LabelWidth, 0.f))->SetVerticalAlignment(VAlign_Center);
		UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(Widget);
		Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		Slot->SetVerticalAlignment(VAlign_Center);
		return Row;
	}

	FText FormatLocalTime(const FDateTime& Utc)
	{
		if (Utc.GetTicks() <= 0)
		{
			return LOCTEXT("NoTime", "때 모름");
		}
		const FDateTime Local = Utc + (FDateTime::Now() - FDateTime::UtcNow());
		return FText::FromString(Local.ToString(TEXT("%Y-%m-%d %H:%M")));
	}

	FText DescribeSlot(const FGrimSaveSlotInfo& Info)
	{
		if (!Info.bReadable)
		{
			return FText::Format(LOCTEXT("SlotBroken", "{0}  ·  {1}  ·  읽지 못함(이어하면 백업 · 새로 시작)"), FText::FromString(Info.Slot), FormatLocalTime(Info.SavedAt));
		}
		return FText::Format(LOCTEXT("SlotLine", "{0}  ·  {1}  ·  저장 {2}번  ·  플레이어 {3}"),
			FText::FromString(Info.Slot), FormatLocalTime(Info.SavedAt), Info.SaveCount, Info.PlayerFiles);
	}
}

// ═════════════════════════ 설정 칸 ═════════════════════════

TSharedRef<SWidget> UGrimMenuSettingsPanel::RebuildWidget()
{
	EnsureBuilt();
	return Super::RebuildWidget();
}

void UGrimMenuSettingsPanel::EnsureBuilt()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
		LoadFromOptions();
	}
}

void UGrimMenuSettingsPanel::BuildTree()
{
	using namespace GrimMenuWidgetsLocal;
	const UGrimOnlineSettings* Settings = UGrimOnlineSettings::Get();

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Column;

	NameBox = MakeTextBox(WidgetTree, LOCTEXT("NameHint", "친구에게 보이는 이름"));
	AddRow(Column, MakeLabeled(WidgetTree, LOCTEXT("NameLabel", "표시 이름"), NameBox));

	auto AddCheck = [this, Column](const FText& Label) -> UCheckBox*
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UCheckBox* Box = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass());
		Row->AddChildToHorizontalBox(Box)->SetVerticalAlignment(VAlign_Center);
		UTextBlock* Text = GrimUIData::MakeText(WidgetTree, Label, 13, Ink);
		Row->AddChildToHorizontalBox(Text)->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
		AddRow(Column, Row, 8.f);
		return Box;
	};
	SoftLockBox = AddCheck(LOCTEXT("SoftLock", "소프트 락온 — 가까운 적을 자동으로 겨눈다(끄면 화면 가운데로)"));
	ZoomToggleBox = AddCheck(LOCTEXT("ZoomToggle", "줌 조준 토글 — 끄면 누르는 동안만 조준"));

	auto AddSlider = [this, Column](const FText& Label, const FVector2D& Range, TObjectPtr<UTextBlock>& OutValue) -> USlider*
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		USlider* Slider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass());
		Slider->SetMinValue(Range.X);
		Slider->SetMaxValue(Range.Y);
		Slider->SetStepSize(0.05f);
		Slider->OnValueChanged.AddDynamic(this, &UGrimMenuSettingsPanel::HandleSliderChanged);
		UHorizontalBoxSlot* SliderSlot = Row->AddChildToHorizontalBox(Slider);
		SliderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		SliderSlot->SetVerticalAlignment(VAlign_Center);
		OutValue = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 13, Ink);
		Row->AddChildToHorizontalBox(GrimUIData::MakeSizeBox(WidgetTree, OutValue, 48.f, 0.f))->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
		AddRow(Column, MakeLabeled(WidgetTree, Label, Row), 8.f);
		return Slider;
	};
	ZoomSlider = AddSlider(LOCTEXT("ZoomSens", "조준 감도(줌 중)"), Settings->ZoomSensitivityRange, ZoomValueText);
	LookSlider = AddSlider(LOCTEXT("LookSens", "시점 감도"), Settings->LookSensitivityRange, LookValueText);

	AddRow(Column, GrimUIData::MakeText(WidgetTree, LOCTEXT("SettingsNote", "저장하면 바로 적용된다. 이름은 다음 접속부터 친구에게 보인다."), 11, Faint), 10.f);
}

void UGrimMenuSettingsPanel::LoadFromOptions()
{
	if (!NameBox)
	{
		EnsureBuilt();
		if (!NameBox)
		{
			return;
		}
	}
	const FGrimUserOptions Options = FGrimUserOptions::Load();
	NameBox->SetText(FText::FromString(Options.GetEffectivePlayerName()));
	SoftLockBox->SetIsChecked(Options.bSoftLockEnabled);
	ZoomToggleBox->SetIsChecked(Options.bZoomAimToggle);
	ZoomSlider->SetValue(Options.ZoomLookSensitivity);
	LookSlider->SetValue(Options.LookSensitivity);
	RefreshValueTexts();
}

void UGrimMenuSettingsPanel::SaveAndApply()
{
	if (!NameBox)
	{
		return;
	}
	FGrimUserOptions Options = FGrimUserOptions::Load();
	Options.PlayerName = FGrimOnlineMath::SanitizePlayerName(NameBox->GetText().ToString(), UGrimOnlineSettings::Get()->PlayerNameMaxLength);
	Options.bNameFromCommandLine = false;
	Options.bSoftLockEnabled = SoftLockBox->IsChecked();
	Options.bZoomAimToggle = ZoomToggleBox->IsChecked();
	Options.ZoomLookSensitivity = ZoomSlider->GetValue();
	Options.LookSensitivity = LookSlider->GetValue();
	Options.Save();
	FGrimUserOptions::ApplyToAllLocalPlayers();
	UE_LOG(LogGrimOnline, Display, TEXT("[Online] 설정 저장: 이름 %s · 소프트 락온 %d · 줌 토글 %d · 조준 감도 %.2f · 시점 감도 %.2f"),
		*Options.PlayerName, Options.bSoftLockEnabled ? 1 : 0, Options.bZoomAimToggle ? 1 : 0, Options.ZoomLookSensitivity, Options.LookSensitivity);
	LoadFromOptions();
}

void UGrimMenuSettingsPanel::HandleSliderChanged(float Value)
{
	RefreshValueTexts();
}

void UGrimMenuSettingsPanel::RefreshValueTexts()
{
	FNumberFormattingOptions Format;
	Format.MinimumFractionalDigits = 2;
	Format.MaximumFractionalDigits = 2;
	if (ZoomValueText && ZoomSlider)
	{
		ZoomValueText->SetText(FText::AsNumber(ZoomSlider->GetValue(), &Format));
	}
	if (LookValueText && LookSlider)
	{
		LookValueText->SetText(FText::AsNumber(LookSlider->GetValue(), &Format));
	}
}

// ═════════════════════════ 메인 메뉴 ═════════════════════════

UGrimSessionSubsystem* UGrimMainMenuWidget::GetSession() const
{
	return UGrimSessionSubsystem::Get(GetOwningPlayer());
}

TSharedRef<SWidget> UGrimMainMenuWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

UGrimIdButton* UGrimMainMenuWidget::AddButton(UVerticalBox* Box, const FText& Text, int32 Id, UTextBlock** OutLabel)
{
	UGrimIdButton* Button = GrimMenuWidgetsLocal::MakeButton(WidgetTree, Text, Id, OutLabel);
	Button->OnIdClicked.BindUObject(this, &UGrimMainMenuWidget::HandleButton);
	GrimMenuWidgetsLocal::AddRow(Box, Button, 6.f);
	return Button;
}

void UGrimMainMenuWidget::BuildTree()
{
	using namespace GrimMenuWidgetsLocal;
	SetIsFocusable(true);
	const FGrimUserOptions Options = FGrimUserOptions::Load();
	const UGrimOnlineSettings* Settings = UGrimOnlineSettings::Get();
	HostMaxPlayers = FGrimOnlineMath::ClampMaxPlayers(Options.LastMaxPlayers > 0 ? Options.LastMaxPlayers : Settings->DefaultMaxPlayers, Settings->MaxPlayersLimit);

	UBorder* Root = GrimUIData::MakeBorder(WidgetTree, Backdrop, FMargin(0.f));
	WidgetTree->RootWidget = Root;
	UOverlay* Layers = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	Root->SetContent(Layers);

	// 가운데 기둥: 제목 · 이름 · 페이지
	UVerticalBox* Center = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UOverlaySlot* CenterSlot = Layers->AddChildToOverlay(Center);
	CenterSlot->SetHorizontalAlignment(HAlign_Center);
	CenterSlot->SetVerticalAlignment(VAlign_Center);

	UTextBlock* Title = GrimUIData::MakeText(WidgetTree, LOCTEXT("Title", "그리모리"), 40, Gold, true);
	Title->SetJustification(ETextJustify::Center);
	Center->AddChildToVerticalBox(Title)->SetHorizontalAlignment(HAlign_Center);
	NameText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 13, Faint);
	Center->AddChildToVerticalBox(NameText)->SetHorizontalAlignment(HAlign_Center);

	UBorder* Panel = GrimUIData::MakeBorder(WidgetTree, PanelColor, FMargin(24.f, 18.f));
	Center->AddChildToVerticalBox(GrimUIData::MakeSizeBox(WidgetTree, Panel, 560.f, 0.f))->SetPadding(FMargin(0.f, 18.f, 0.f, 0.f));
	Pages = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass());
	Panel->SetContent(Pages);
	FirstFocus.SetNum(PageSettings + 1);

	auto NewPage = [this](const FText& Heading) -> UVerticalBox*
	{
		UVerticalBox* Page = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		if (!Heading.IsEmpty())
		{
			Page->AddChildToVerticalBox(GrimUIData::MakeText(WidgetTree, Heading, 18, Gold, true));
		}
		Pages->AddChild(Page);
		return Page;
	};

	// 첫 화면
	{
		UVerticalBox* Page = NewPage(FText::GetEmpty());
		FirstFocus[PageMain] = AddButton(Page, LOCTEXT("New", "새로 시작"), B_New);
		AddButton(Page, LOCTEXT("Continue", "이어하기"), B_Continue);
		AddButton(Page, LOCTEXT("Host", "서버 열기"), B_Host);
		AddButton(Page, LOCTEXT("Join", "IP 참가"), B_Join);
		AddButton(Page, LOCTEXT("Settings", "설정"), B_Settings);
		AddButton(Page, LOCTEXT("Quit", "나가기"), B_Quit);
	}

	// 새로 시작
	{
		UVerticalBox* Page = NewPage(LOCTEXT("NewHeading", "새로 시작"));
		AddRow(Page, GrimUIData::MakeText(WidgetTree, LOCTEXT("NewHelp", "새 세계의 슬롯 이름 — 영문 · 숫자 · - · _ (세이브 폴더 이름이 된다)"), 12, Faint), 8.f);
		NewSlotBox = MakeTextBox(WidgetTree, LOCTEXT("NewSlotHint", "예: MyWorld"));
		NewSlotBox->OnTextCommitted.AddDynamic(this, &UGrimMainMenuWidget::HandleNewSlotCommitted);
		AddRow(Page, MakeLabeled(WidgetTree, LOCTEXT("SlotLabel", "슬롯"), NewSlotBox), 8.f);
		FirstFocus[PageNewGame] = NewSlotBox;
		AddButton(Page, LOCTEXT("NewSolo", "혼자 시작"), B_NewSolo);
		UTextBlock* HostLabel = nullptr;
		AddButton(Page, FText::GetEmpty(), B_NewHost, &HostLabel);
		NewHostLabel = HostLabel;
		AddButton(Page, LOCTEXT("Back", "뒤로"), B_Back);
	}

	// 이어하기
	{
		UVerticalBox* Page = NewPage(LOCTEXT("ContinueHeading", "이어하기"));
		SlotList = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
		AddRow(Page, GrimUIData::MakeSizeBox(WidgetTree, SlotList, 0.f, 240.f), 8.f);
		SlotInfoText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 12, Faint);
		AddRow(Page, SlotInfoText, 6.f);
		ContinueSoloButton = AddButton(Page, LOCTEXT("ContSolo", "혼자 이어하기"), B_ContSolo);
		UTextBlock* HostLabel = nullptr;
		ContinueHostButton = AddButton(Page, FText::GetEmpty(), B_ContHost, &HostLabel);
		ContinueHostLabel = HostLabel;
		FirstFocus[PageContinue] = AddButton(Page, LOCTEXT("Back", "뒤로"), B_Back);
	}

	// 서버 열기
	{
		UVerticalBox* Page = NewPage(LOCTEXT("HostHeading", "서버 열기 — 호스트도 함께 플레이"));
		HostSlotBox = MakeTextBox(WidgetTree, LOCTEXT("HostSlotHint", "슬롯 이름(있으면 이어서, 없으면 새로)"));
		AddRow(Page, MakeLabeled(WidgetTree, LOCTEXT("SlotLabel", "슬롯"), HostSlotBox), 10.f);
		FirstFocus[PageHost] = HostSlotBox;

		UHorizontalBox* MaxRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UGrimIdButton* Minus = MakeButton(WidgetTree, FText::FromString(TEXT("−")), B_HostMinus);
		Minus->OnIdClicked.BindUObject(this, &UGrimMainMenuWidget::HandleButton);
		MaxRow->AddChildToHorizontalBox(GrimUIData::MakeSizeBox(WidgetTree, Minus, 40.f, 0.f));
		HostMaxText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 15, Ink, true);
		HostMaxText->SetJustification(ETextJustify::Center);
		MaxRow->AddChildToHorizontalBox(GrimUIData::MakeSizeBox(WidgetTree, HostMaxText, 90.f, 0.f))->SetVerticalAlignment(VAlign_Center);
		UGrimIdButton* Plus = MakeButton(WidgetTree, FText::FromString(TEXT("+")), B_HostPlus);
		Plus->OnIdClicked.BindUObject(this, &UGrimMainMenuWidget::HandleButton);
		MaxRow->AddChildToHorizontalBox(GrimUIData::MakeSizeBox(WidgetTree, Plus, 40.f, 0.f));
		AddRow(Page, MakeLabeled(WidgetTree, LOCTEXT("MaxLabel", "최대 인원"), MaxRow), 8.f);

		HostInfoText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 12, Faint);
		HostInfoText->SetAutoWrapText(true);
		AddRow(Page, HostInfoText, 8.f);
		AddButton(Page, LOCTEXT("HostStart", "서버 열기"), B_HostStart);
		AddButton(Page, LOCTEXT("Back", "뒤로"), B_Back);
	}

	// IP 참가
	{
		UVerticalBox* Page = NewPage(LOCTEXT("JoinHeading", "IP 참가"));
		JoinAddressBox = MakeTextBox(WidgetTree, LOCTEXT("JoinAddressHint", "192.168.0.10 또는 192.168.0.10:7777"));
		JoinAddressBox->OnTextCommitted.AddDynamic(this, &UGrimMainMenuWidget::HandleJoinCommitted);
		AddRow(Page, MakeLabeled(WidgetTree, LOCTEXT("AddressLabel", "주소"), JoinAddressBox), 10.f);
		FirstFocus[PageJoin] = JoinAddressBox;
		JoinPortBox = MakeTextBox(WidgetTree, FText::AsNumber(Settings->DefaultPort, &FNumberFormattingOptions::DefaultNoGrouping()));
		JoinPortBox->OnTextCommitted.AddDynamic(this, &UGrimMainMenuWidget::HandleJoinCommitted);
		AddRow(Page, MakeLabeled(WidgetTree, LOCTEXT("PortLabel", "포트"), JoinPortBox), 6.f);
		AddButton(Page, LOCTEXT("JoinStart", "참가"), B_JoinStart);
		AddButton(Page, LOCTEXT("FindLAN", "LAN 찾기(같은 공유기)"), B_FindLAN);
		LANStatusText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 12, Faint);
		AddRow(Page, LANStatusText, 6.f);
		LANList = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
		AddRow(Page, GrimUIData::MakeSizeBox(WidgetTree, LANList, 0.f, 140.f), 4.f);
		AddButton(Page, LOCTEXT("Back", "뒤로"), B_Back);
	}

	// 설정
	{
		UVerticalBox* Page = NewPage(LOCTEXT("SettingsHeading", "설정"));
		SettingsPanel = WidgetTree->ConstructWidget<UGrimMenuSettingsPanel>(UGrimMenuSettingsPanel::StaticClass());
		AddRow(Page, SettingsPanel, 10.f);
		FirstFocus[PageSettings] = AddButton(Page, LOCTEXT("SettingsSave", "저장"), B_SettingsSave);
		AddButton(Page, LOCTEXT("Back", "뒤로"), B_Back);
	}

	// 아래 메시지 줄
	MessageText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 14, ErrorInk);
	MessageText->SetJustification(ETextJustify::Center);
	MessageText->SetAutoWrapText(true);
	UOverlaySlot* MessageSlot = Layers->AddChildToOverlay(MessageText);
	MessageSlot->SetHorizontalAlignment(HAlign_Center);
	MessageSlot->SetVerticalAlignment(VAlign_Bottom);
	MessageSlot->SetPadding(FMargin(40.f, 0.f, 40.f, 40.f));

	// 접속 중 · 여는 중 덮개
	BusyOverlay = GrimUIData::MakeBorder(WidgetTree, Dim, FMargin(0.f));
	UOverlaySlot* BusySlot = Layers->AddChildToOverlay(BusyOverlay);
	BusySlot->SetHorizontalAlignment(HAlign_Fill);
	BusySlot->SetVerticalAlignment(VAlign_Fill);
	UVerticalBox* BusyColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	BusyOverlay->SetContent(BusyColumn);
	BusyOverlay->SetHorizontalAlignment(HAlign_Center);
	BusyOverlay->SetVerticalAlignment(VAlign_Center);
	BusyText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 20, Ink, true);
	BusyText->SetJustification(ETextJustify::Center);
	BusyColumn->AddChildToVerticalBox(BusyText)->SetHorizontalAlignment(HAlign_Center);
	BusyCancelButton = MakeButton(WidgetTree, LOCTEXT("Cancel", "취소"), B_Cancel);
	BusyCancelButton->OnIdClicked.BindUObject(this, &UGrimMainMenuWidget::HandleButton);
	UVerticalBoxSlot* CancelSlot = BusyColumn->AddChildToVerticalBox(GrimUIData::MakeSizeBox(WidgetTree, BusyCancelButton, 160.f, 0.f));
	CancelSlot->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
	CancelSlot->SetHorizontalAlignment(HAlign_Center);
	BusyOverlay->SetVisibility(ESlateVisibility::Collapsed);

	RefreshHostTexts();
	RefreshStatus();
	ShowPage(PageMain);
}

void UGrimMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UGrimSessionSubsystem* Session = GetSession())
	{
		SessionChangedHandle = Session->OnChanged.AddUObject(this, &UGrimMainMenuWidget::HandleSessionChanged);
	}
	RefreshStatus();
}

void UGrimMainMenuWidget::NativeDestruct()
{
	if (UGrimSessionSubsystem* Session = GetSession())
	{
		Session->OnChanged.Remove(SessionChangedHandle);
	}
	Super::NativeDestruct();
}

void UGrimMainMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshTimer -= InDeltaTime;
	if (RefreshTimer <= 0.f)
	{
		RefreshTimer = 0.25f;
		RefreshStatus();
	}
}

FReply UGrimMainMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
	{
		const UGrimSessionSubsystem* Session = GetSession();
		if (Session && Session->GetState() == EGrimConnectionState::Connecting)
		{
			HandleButton(GrimMenuWidgetsLocal::B_Cancel);
		}
		else if (CurrentPage != PageMain)
		{
			ShowPage(PageMain);
		}
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UGrimMainMenuWidget::FocusFirst()
{
	if (FirstFocus.IsValidIndex(CurrentPage) && FirstFocus[CurrentPage])
	{
		FirstFocus[CurrentPage]->SetFocus();
	}
}

void UGrimMainMenuWidget::ShowPage(int32 Page)
{
	CurrentPage = Page;
	if (Pages)
	{
		Pages->SetActiveWidgetIndex(Page);
	}
	LocalError = FText::GetEmpty();
	RefreshStatus();
	FocusFirst();
}

void UGrimMainMenuWidget::ShowLocalError(const FText& Text, bool bError)
{
	LocalError = Text;
	bLocalIsError = bError;
	if (bError)
	{
		UE_LOG(LogGrimOnline, Display, TEXT("[Online] 메뉴: %s"), *Text.ToString());
	}
	RefreshStatus();
}

void UGrimMainMenuWidget::HandleSessionChanged()
{
	RefreshStatus();
	if (CurrentPage == PageJoin)
	{
		RefreshLAN();
	}
}

void UGrimMainMenuWidget::RefreshStatus()
{
	using namespace GrimMenuWidgetsLocal;
	const UGrimSessionSubsystem* Session = GetSession();
	if (!MessageText || !Session)
	{
		return;
	}
	NameText->SetText(FText::Format(LOCTEXT("NameLine", "{0} 님"), FText::FromString(Session->GetPlayerName())));

	if (!LocalError.IsEmpty())
	{
		MessageText->SetText(LocalError);
		MessageText->SetColorAndOpacity(FSlateColor(bLocalIsError ? ErrorInk : InfoInk));
	}
	else
	{
		MessageText->SetText(Session->GetMessage());
		MessageText->SetColorAndOpacity(FSlateColor(Session->IsMessageError() ? ErrorInk : InfoInk));
	}

	const EGrimConnectionState State = Session->GetState();
	if (State == EGrimConnectionState::Connecting)
	{
		BusyOverlay->SetVisibility(ESlateVisibility::Visible);
		BusyCancelButton->SetVisibility(ESlateVisibility::Visible);
		BusyText->SetText(FText::Format(LOCTEXT("Connecting", "{0} 에 접속 중… {1}초"),
			FText::FromString(Session->GetTargetAddress()), FText::AsNumber(FMath::FloorToInt(Session->GetConnectingSeconds()))));
	}
	else if (State == EGrimConnectionState::Opening)
	{
		BusyOverlay->SetVisibility(ESlateVisibility::Visible);
		BusyCancelButton->SetVisibility(ESlateVisibility::Collapsed);
		BusyText->SetText(LOCTEXT("Opening", "세계를 여는 중…"));
	}
	else
	{
		BusyOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UGrimMainMenuWidget::RefreshHostTexts()
{
	const UGrimOnlineSettings* Settings = UGrimOnlineSettings::Get();
	HostMaxPlayers = FGrimOnlineMath::ClampMaxPlayers(HostMaxPlayers, Settings->MaxPlayersLimit);
	const FText MaxText = FText::Format(LOCTEXT("MaxPlayers", "{0}명"), HostMaxPlayers);
	if (HostMaxText)
	{
		HostMaxText->SetText(MaxText);
	}
	if (NewHostLabel)
	{
		NewHostLabel->SetText(FText::Format(LOCTEXT("NewHost", "호스트로 시작 — 친구를 받는다(최대 {0}명)"), HostMaxPlayers));
	}
	if (ContinueHostLabel)
	{
		ContinueHostLabel->SetText(FText::Format(LOCTEXT("ContHost", "서버로 이어하기(최대 {0}명)"), HostMaxPlayers));
	}
	if (HostInfoText)
	{
		const FString Address = UGrimSessionSubsystem::GetLocalAddressText();
		HostInfoText->SetText(FText::Format(
			LOCTEXT("HostInfo", "친구는 「IP 참가」에 이 PC 주소 {0}:{1} 를 적는다. 같은 공유기 밖이면 공유기에서 UDP {1} 포트를 열어야 한다(Steam 출시 때 친구 초대로 바뀐다). 호스트가 나가면 세계를 저장하고 서버가 닫힌다."),
			FText::FromString(Address.IsEmpty() ? TEXT("?") : Address), FText::AsNumber(UGrimSessionSubsystem::GetListenPort(), &FNumberFormattingOptions::DefaultNoGrouping())));
	}
}

void UGrimMainMenuWidget::RefreshSlots()
{
	using namespace GrimMenuWidgetsLocal;
	if (!SlotList)
	{
		return;
	}
	Slots = UGrimSessionSubsystem::ListSaveSlots();
	SlotList->ClearChildren();
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		UGrimIdButton* Button = MakeButton(WidgetTree, DescribeSlot(Slots[Index]), B_SlotBase + Index, nullptr, 13);
		Button->OnIdClicked.BindUObject(this, &UGrimMainMenuWidget::HandleButton);
		SlotList->AddChild(Button);
	}
	if (Slots.Num() == 0)
	{
		SlotList->AddChild(GrimUIData::MakeText(WidgetTree, LOCTEXT("NoSlots", "세이브가 없다 — 「새로 시작」으로 세계를 만들어 주세요"), 13, Faint));
	}
	SelectedSlot = Slots.Num() > 0 ? 0 : INDEX_NONE;
	HandleButton(SelectedSlot != INDEX_NONE ? B_SlotBase + SelectedSlot : 0);
}

void UGrimMainMenuWidget::RefreshLAN()
{
	using namespace GrimMenuWidgetsLocal;
	const UGrimSessionSubsystem* Session = GetSession();
	if (!LANList || !Session)
	{
		return;
	}
	const TArray<FGrimFoundSession>& Found = Session->GetLANResults();
	LANStatusText->SetText(Session->IsSearchingLAN()
		? LOCTEXT("LANSearching", "같은 공유기의 서버를 찾는 중…")
		: FText::Format(LOCTEXT("LANFound", "찾은 서버 {0}개 — 눌러서 참가"), Found.Num()));
	LANList->ClearChildren();
	for (int32 Index = 0; Index < Found.Num(); ++Index)
	{
		const FGrimFoundSession& Entry = Found[Index];
		const FText Label = FText::Format(LOCTEXT("LANLine", "{0}  ·  슬롯 {1}  ·  빈자리 {2}/{3}  ·  {4}ms"),
			FText::FromString(Entry.HostName), FText::FromString(Entry.SlotName), Entry.OpenSlots, Entry.MaxSlots, Entry.PingMs);
		UGrimIdButton* Button = MakeButton(WidgetTree, Label, B_LANBase + Index, nullptr, 13);
		Button->OnIdClicked.BindUObject(this, &UGrimMainMenuWidget::HandleButton);
		LANList->AddChild(Button);
	}
}

void UGrimMainMenuWidget::HandleNewSlotCommitted(const FText& Text, ETextCommit::Type CommitType)
{
	if (CommitType == ETextCommit::OnEnter)
	{
		HandleButton(GrimMenuWidgetsLocal::B_NewSolo);
	}
}

void UGrimMainMenuWidget::HandleJoinCommitted(const FText& Text, ETextCommit::Type CommitType)
{
	if (CommitType == ETextCommit::OnEnter)
	{
		HandleButton(GrimMenuWidgetsLocal::B_JoinStart);
	}
}

void UGrimMainMenuWidget::HandleButton(int32 Id)
{
	using namespace GrimMenuWidgetsLocal;
	UGrimSessionSubsystem* Session = GetSession();
	if (!Session)
	{
		return;
	}
	const UGrimOnlineSettings* Settings = UGrimOnlineSettings::Get();
	FText Error;
	auto Try = [this, &Error](bool bOk)
	{
		if (!bOk)
		{
			ShowLocalError(Error);
		}
	};

	if (Id >= B_LANBase)
	{
		Try(Session->JoinLANSession(Id - B_LANBase, Error));
		return;
	}
	if (Id >= B_SlotBase)
	{
		SelectedSlot = Id - B_SlotBase;
		if (Slots.IsValidIndex(SelectedSlot))
		{
			const FGrimSaveSlotInfo& Info = Slots[SelectedSlot];
			SlotInfoText->SetText(FText::Format(LOCTEXT("SlotSelected", "고른 슬롯: {0}{1}"), FText::FromString(Info.Slot),
				Info.MapName.IsEmpty() ? FText::GetEmpty() : FText::Format(LOCTEXT("SlotMap", " — 맵 {0}"), FText::FromString(Info.MapName))));
		}
		return;
	}
	if (Id == 0)
	{
		// 이어하기 목록이 비었다.
		SlotInfoText->SetText(FText::GetEmpty());
		SelectedSlot = INDEX_NONE;
		return;
	}

	switch (Id)
	{
	case B_New:
		ShowPage(PageNewGame);
		break;
	case B_Continue:
		ShowPage(PageContinue);
		RefreshSlots();
		break;
	case B_Host:
	{
		const FGrimUserOptions Options = FGrimUserOptions::Load();
		HostSlotBox->SetText(FText::FromString(Slots.IsValidIndex(SelectedSlot) ? Slots[SelectedSlot].Slot : Options.LastSlot));
		RefreshHostTexts();
		ShowPage(PageHost);
		break;
	}
	case B_Join:
	{
		const FGrimUserOptions Options = FGrimUserOptions::Load();
		JoinAddressBox->SetText(FText::FromString(Options.LastAddress));
		JoinPortBox->SetText(Options.LastPort > 0 && Options.LastPort != Settings->DefaultPort
			? FText::AsNumber(Options.LastPort, &FNumberFormattingOptions::DefaultNoGrouping()) : FText::GetEmpty());
		ShowPage(PageJoin);
		RefreshLAN();
		break;
	}
	case B_Settings:
		SettingsPanel->LoadFromOptions();
		ShowPage(PageSettings);
		break;
	case B_Quit:
		Session->QuitGame();
		break;
	case B_Back:
		ShowPage(PageMain);
		break;
	case B_NewSolo:
	case B_NewHost:
		Try(Session->StartNewGame(NewSlotBox->GetText().ToString().TrimStartAndEnd(), Id == B_NewHost, HostMaxPlayers, Error));
		break;
	case B_ContSolo:
	case B_ContHost:
		if (!Slots.IsValidIndex(SelectedSlot))
		{
			ShowLocalError(LOCTEXT("PickSlot", "이어할 슬롯을 골라 주세요"));
			break;
		}
		Try(Session->ContinueGame(Slots[SelectedSlot].Slot, Id == B_ContHost, HostMaxPlayers, Error));
		break;
	case B_HostStart:
		Try(Session->HostServer(HostSlotBox->GetText().ToString().TrimStartAndEnd(), HostMaxPlayers, Error));
		break;
	case B_HostMinus:
	case B_HostPlus:
		HostMaxPlayers += Id == B_HostPlus ? 1 : -1;
		RefreshHostTexts();
		break;
	case B_JoinStart:
		Try(Session->JoinServer(JoinAddressBox->GetText().ToString(), JoinPortBox->GetText().ToString(), Error));
		break;
	case B_FindLAN:
		Session->FindLANSessions();
		RefreshLAN();
		break;
	case B_SettingsSave:
		SettingsPanel->SaveAndApply();
		ShowLocalError(LOCTEXT("Saved", "설정을 저장했다"), false);
		RefreshStatus();
		break;
	case B_Cancel:
		Session->CancelConnect();
		break;
	default:
		break;
	}
}

// ═════════════════════════ Esc 메뉴 ═════════════════════════

UGrimSessionSubsystem* UGrimPauseMenuWidget::GetSession() const
{
	return UGrimSessionSubsystem::Get(GetOwningPlayer());
}

TSharedRef<SWidget> UGrimPauseMenuWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

void UGrimPauseMenuWidget::BuildTree()
{
	using namespace GrimMenuWidgetsLocal;
	SetIsFocusable(true);

	UBorder* Root = GrimUIData::MakeBorder(WidgetTree, Dim, FMargin(0.f));
	WidgetTree->RootWidget = Root;
	Root->SetHorizontalAlignment(HAlign_Center);
	Root->SetVerticalAlignment(VAlign_Center);

	UBorder* Panel = GrimUIData::MakeBorder(WidgetTree, PanelColor, FMargin(24.f, 18.f));
	Root->SetContent(GrimUIData::MakeSizeBox(WidgetTree, Panel, 480.f, 0.f));
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Panel->SetContent(Column);

	Column->AddChildToVerticalBox(GrimUIData::MakeText(WidgetTree, LOCTEXT("PauseTitle", "메뉴"), 22, Gold, true));
	InfoText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 12, Faint);
	InfoText->SetAutoWrapText(true);
	AddRow(Column, InfoText, 4.f);

	Pages = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass());
	AddRow(Column, Pages, 8.f);

	auto Add = [this](UVerticalBox* Box, const FText& Text, int32 Id, UTextBlock** OutLabel = nullptr) -> UGrimIdButton*
	{
		UGrimIdButton* Button = MakeButton(WidgetTree, Text, Id, OutLabel);
		Button->OnIdClicked.BindUObject(this, &UGrimPauseMenuWidget::HandleButton);
		AddRow(Box, Button, 6.f);
		return Button;
	};

	UVerticalBox* MainPage = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Pages->AddChild(MainPage);
	UTextBlock* MainLabel = nullptr;
	UTextBlock* QuitTextLabel = nullptr;
	ResumeButton = Add(MainPage, LOCTEXT("Resume", "이어하기"), P_Resume);
	Add(MainPage, LOCTEXT("PauseSettings", "설정"), P_Settings);
	Add(MainPage, LOCTEXT("Invite", "친구 초대"), P_Invite);
	Add(MainPage, FText::GetEmpty(), P_MainMenu, &MainLabel);
	Add(MainPage, FText::GetEmpty(), P_Quit, &QuitTextLabel);
	MainMenuLabel = MainLabel;
	QuitLabel = QuitTextLabel;

	UVerticalBox* SettingsPage = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Pages->AddChild(SettingsPage);
	SettingsPanel = WidgetTree->ConstructWidget<UGrimMenuSettingsPanel>(UGrimMenuSettingsPanel::StaticClass());
	AddRow(SettingsPage, SettingsPanel, 4.f);
	SettingsSaveButton = Add(SettingsPage, LOCTEXT("SettingsSave", "저장"), P_Save);
	Add(SettingsPage, LOCTEXT("Back", "뒤로"), P_Back);

	NoticeText = GrimUIData::MakeText(WidgetTree, FText::GetEmpty(), 12, InfoInk);
	NoticeText->SetAutoWrapText(true);
	AddRow(Column, NoticeText, 10.f);

	Refresh();
}

void UGrimPauseMenuWidget::ResetToMain()
{
	PendingConfirm = 0;
	if (Pages)
	{
		Pages->SetActiveWidgetIndex(0);
	}
	if (NoticeText)
	{
		NoticeText->SetText(FText::GetEmpty());
	}
	Refresh();
}

void UGrimPauseMenuWidget::FocusFirst()
{
	UWidget* Target = (Pages && Pages->GetActiveWidgetIndex() == 1) ? static_cast<UWidget*>(SettingsSaveButton) : static_cast<UWidget*>(ResumeButton);
	if (Target)
	{
		Target->SetFocus();
	}
}

void UGrimPauseMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshTimer -= InDeltaTime;
	if (RefreshTimer <= 0.f)
	{
		RefreshTimer = 0.5f;
		Refresh();
	}
}

FReply UGrimPauseMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Gamepad_Special_Right)
	{
		RequestClose();
		return FReply::Handled();
	}
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
	{
		if (Pages && Pages->GetActiveWidgetIndex() == 1)
		{
			HandleButton(GrimMenuWidgetsLocal::P_Back);
		}
		else
		{
			RequestClose();
		}
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UGrimPauseMenuWidget::RequestClose()
{
	if (UGrimMenuSubsystem* Menu = GetOwningLocalPlayer() ? GetOwningLocalPlayer()->GetSubsystem<UGrimMenuSubsystem>() : nullptr)
	{
		Menu->SetPauseMenuOpen(false);
	}
}

void UGrimPauseMenuWidget::Refresh()
{
	using namespace GrimMenuWidgetsLocal;
	const UGrimSessionSubsystem* Session = GetSession();
	if (!InfoText || !Session)
	{
		return;
	}
	int32 Players = 0, Max = 0;
	Session->GetPlayerCounts(Players, Max);
	const int32 Others = FMath::Max(0, Players - 1);
	const EGrimConnectionState State = Session->GetState();
	const FText SlotText = FText::FromString(Session->GetCurrentSlot());

	switch (State)
	{
	case EGrimConnectionState::Hosting:
		InfoText->SetText(FText::Format(LOCTEXT("InfoHost", "호스트 · 슬롯 {0} · 접속 {1}/{2}명 · 주소 {3}"), SlotText, Players, Max, FText::FromString(Session->GetTargetAddress())));
		break;
	case EGrimConnectionState::Connected:
		InfoText->SetText(FText::Format(LOCTEXT("InfoClient", "{0} 에 접속함"), FText::FromString(Session->GetTargetAddress())));
		break;
	case EGrimConnectionState::Standalone:
		InfoText->SetText(FText::Format(LOCTEXT("InfoSolo", "혼자 · 슬롯 {0}"), SlotText));
		break;
	default:
		InfoText->SetText(FText::FromString(UGrimSessionSubsystem::StateName(State)));
		break;
	}

	const bool bServer = Session->IsServer();
	if (MainMenuLabel)
	{
		MainMenuLabel->SetText(PendingConfirm == P_MainMenu
			? FText::Format(LOCTEXT("ConfirmMenu", "정말 메인 메뉴로? 친구 {0}명의 접속이 끊긴다 — 한 번 더"), Others)
			: State == EGrimConnectionState::Hosting ? LOCTEXT("MenuHost", "메인 메뉴로 (저장 · 서버 닫힘)")
			: bServer ? LOCTEXT("MenuSolo", "메인 메뉴로 (저장)")
			: LOCTEXT("MenuClient", "메인 메뉴로 (접속 끊기)"));
	}
	if (QuitLabel)
	{
		QuitLabel->SetText(PendingConfirm == P_Quit
			? FText::Format(LOCTEXT("ConfirmQuit", "정말 나가기? 친구 {0}명의 접속이 끊긴다 — 한 번 더"), Others)
			: bServer ? LOCTEXT("QuitHost", "저장하고 나가기") : LOCTEXT("QuitClient", "게임 끄기"));
	}
}

void UGrimPauseMenuWidget::HandleButton(int32 Id)
{
	using namespace GrimMenuWidgetsLocal;
	UGrimSessionSubsystem* Session = GetSession();
	if (!Session)
	{
		return;
	}
	int32 Players = 0, Max = 0;
	Session->GetPlayerCounts(Players, Max);
	const bool bNeedsConfirm = Session->GetState() == EGrimConnectionState::Hosting && Players > 1;
	if (Id != P_MainMenu && Id != P_Quit)
	{
		PendingConfirm = 0;
	}

	switch (Id)
	{
	case P_Resume:
		RequestClose();
		break;
	case P_Settings:
		SettingsPanel->LoadFromOptions();
		Pages->SetActiveWidgetIndex(1);
		NoticeText->SetText(FText::GetEmpty());
		FocusFirst();
		break;
	case P_Invite:
	{
		FText Error;
		if (!Session->InviteFriends(Error))
		{
			NoticeText->SetText(Error);
		}
		break;
	}
	case P_MainMenu:
	case P_Quit:
		if (bNeedsConfirm && PendingConfirm != Id)
		{
			PendingConfirm = Id;
			Refresh();
			break;
		}
		PendingConfirm = 0;
		RequestClose();
		if (Id == P_MainMenu)
		{
			Session->ReturnToMainMenu();
		}
		else
		{
			Session->QuitGame();
		}
		break;
	case P_Save:
		SettingsPanel->SaveAndApply();
		NoticeText->SetText(LOCTEXT("Saved", "설정을 저장했다"));
		Pages->SetActiveWidgetIndex(0);
		FocusFirst();
		break;
	case P_Back:
		Pages->SetActiveWidgetIndex(0);
		FocusFirst();
		break;
	default:
		break;
	}
	Refresh();
}

#undef LOCTEXT_NAMESPACE
