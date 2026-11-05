#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Online/GrimSessionSubsystem.h"
#include "GrimMenuWidgets.generated.h"

class UBorder;
class UCheckBox;
class UEditableTextBox;
class UGrimIdButton;
class UScrollBox;
class USlider;
class UTextBlock;
class UVerticalBox;
class UWidgetSwitcher;

/**
 * 설정 칸(메인 메뉴 · Esc 메뉴가 같이 쓴다): 표시 이름, 소프트 락온(Q18), 줌 조준 누르기/토글(R24), 조준 감도, 시점 감도.
 * 값은 FGrimUserOptions(GameUserSettings.ini). 저장 · 뒤로 버튼은 부모가 둔다.
 */
UCLASS()
class GRIMOIRE_API UGrimMenuSettingsPanel : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 보관 값 → 칸 */
	void LoadFromOptions();

	/** 칸 → 보관 + 지금 컨트롤러에 적용 */
	void SaveAndApply();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void BuildTree();
	void EnsureBuilt();
	void RefreshValueTexts();

	UFUNCTION()
	void HandleSliderChanged(float Value);

	UPROPERTY()
	TObjectPtr<UEditableTextBox> NameBox;

	UPROPERTY()
	TObjectPtr<UCheckBox> SoftLockBox;

	UPROPERTY()
	TObjectPtr<UCheckBox> ZoomToggleBox;

	UPROPERTY()
	TObjectPtr<USlider> ZoomSlider;

	UPROPERTY()
	TObjectPtr<UTextBlock> ZoomValueText;

	UPROPERTY()
	TObjectPtr<USlider> LookSlider;

	UPROPERTY()
	TObjectPtr<UTextBlock> LookValueText;
};

/**
 * 메인 메뉴(L_MainMenu): 새로 시작(슬롯 이름 → 혼자 / 호스트) · 이어하기(세이브 슬롯) · 서버 열기(슬롯 · 인원) · IP 참가(주소 · 포트 · LAN 찾기) · 설정 · 나가기.
 * 접속 중 · 맵 여는 중에는 덮개(취소 버튼), 실패 · 끊김 메시지는 아래 줄(세션 서브시스템이 남긴 것). 함수는 모두 UGrimSessionSubsystem.
 * 키: Esc · 패드 B = 뒤로.
 */
UCLASS()
class GRIMOIRE_API UGrimMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void FocusFirst();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	enum EPage : int32 { PageMain, PageNewGame, PageContinue, PageHost, PageJoin, PageSettings };

	void BuildTree();
	void ShowPage(int32 Page);
	void HandleButton(int32 Id);
	void HandleSessionChanged();
	void RefreshStatus();
	void RefreshSlots();
	void RefreshLAN();
	void RefreshHostTexts();
	void ShowLocalError(const FText& Text, bool bError = true);
	UGrimSessionSubsystem* GetSession() const;
	UGrimIdButton* AddButton(UVerticalBox* Box, const FText& Text, int32 Id, UTextBlock** OutLabel = nullptr);

	UFUNCTION()
	void HandleNewSlotCommitted(const FText& Text, ETextCommit::Type CommitType);

	UFUNCTION()
	void HandleJoinCommitted(const FText& Text, ETextCommit::Type CommitType);

	UPROPERTY()
	TObjectPtr<UWidgetSwitcher> Pages;

	UPROPERTY()
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY()
	TObjectPtr<UTextBlock> MessageText;

	UPROPERTY()
	TObjectPtr<UBorder> BusyOverlay;

	UPROPERTY()
	TObjectPtr<UTextBlock> BusyText;

	UPROPERTY()
	TObjectPtr<UGrimIdButton> BusyCancelButton;

	UPROPERTY()
	TObjectPtr<UEditableTextBox> NewSlotBox;

	UPROPERTY()
	TObjectPtr<UTextBlock> NewHostLabel;

	UPROPERTY()
	TObjectPtr<UScrollBox> SlotList;

	UPROPERTY()
	TObjectPtr<UTextBlock> SlotInfoText;

	UPROPERTY()
	TObjectPtr<UGrimIdButton> ContinueSoloButton;

	UPROPERTY()
	TObjectPtr<UGrimIdButton> ContinueHostButton;

	UPROPERTY()
	TObjectPtr<UTextBlock> ContinueHostLabel;

	UPROPERTY()
	TObjectPtr<UEditableTextBox> HostSlotBox;

	UPROPERTY()
	TObjectPtr<UTextBlock> HostMaxText;

	UPROPERTY()
	TObjectPtr<UTextBlock> HostInfoText;

	UPROPERTY()
	TObjectPtr<UEditableTextBox> JoinAddressBox;

	UPROPERTY()
	TObjectPtr<UEditableTextBox> JoinPortBox;

	UPROPERTY()
	TObjectPtr<UScrollBox> LANList;

	UPROPERTY()
	TObjectPtr<UTextBlock> LANStatusText;

	UPROPERTY()
	TObjectPtr<UGrimMenuSettingsPanel> SettingsPanel;

	/** 페이지마다 처음 초점을 줄 버튼 */
	UPROPERTY()
	TArray<TObjectPtr<UWidget>> FirstFocus;

	TArray<FGrimSaveSlotInfo> Slots;
	int32 SelectedSlot = INDEX_NONE;
	int32 HostMaxPlayers = 8;
	int32 CurrentPage = PageMain;
	FText LocalError;
	bool bLocalIsError = true;
	FDelegateHandle SessionChangedHandle;
	float RefreshTimer = 0.f;
};

/**
 * 게임 안 Esc 메뉴: 이어하기 · 설정 · 친구 초대(Steam 자리) · 메인 메뉴로 · 나가기.
 * 호스트가 메인 메뉴로 · 나가기를 고르면 세이브 시스템이 맵을 떠날 때 저장하고 서버가 닫힌다 — 접속자가 있으면 한 번 더 눌러야 한다.
 * 키: Esc = 닫기(설정에서는 뒤로), 패드 Options(Menu) = 닫기, 패드 B = 뒤로 · 닫기.
 */
UCLASS()
class GRIMOIRE_API UGrimPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ResetToMain();
	void FocusFirst();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void BuildTree();
	void HandleButton(int32 Id);
	void RequestClose();
	void Refresh();
	UGrimSessionSubsystem* GetSession() const;

	UPROPERTY()
	TObjectPtr<UWidgetSwitcher> Pages;

	UPROPERTY()
	TObjectPtr<UTextBlock> InfoText;

	UPROPERTY()
	TObjectPtr<UTextBlock> NoticeText;

	UPROPERTY()
	TObjectPtr<UTextBlock> MainMenuLabel;

	UPROPERTY()
	TObjectPtr<UTextBlock> QuitLabel;

	UPROPERTY()
	TObjectPtr<UGrimIdButton> ResumeButton;

	UPROPERTY()
	TObjectPtr<UGrimIdButton> SettingsSaveButton;

	UPROPERTY()
	TObjectPtr<UGrimMenuSettingsPanel> SettingsPanel;

	/** 확인을 기다리는 버튼(0 = 없음) */
	int32 PendingConfirm = 0;
	float RefreshTimer = 0.f;
};
