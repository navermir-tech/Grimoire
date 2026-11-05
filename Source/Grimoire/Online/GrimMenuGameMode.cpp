#include "Online/GrimMenuGameMode.h"
#include "Online/GrimSessionSubsystem.h"
#include "Save/GrimSaveSettings.h"
#include "UI/GrimMenuWidgets.h"
#include "UI/GrimUISettings.h"
#include "UI/GrimUISubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"
#include "Misc/App.h"

AGrimMenuGameMode::AGrimMenuGameMode()
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = AGrimMenuPlayerController::StaticClass();
	bStartPlayersAsSpectators = false;
}

void AGrimMenuGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	// 세이브 시스템은 월드 BeginPlay(액터 BeginPlay 보다 먼저)에 슬롯을 정한다 — 그동안만 꺼 둔다.
	UGrimSaveSettings* SaveSettings = GetMutableDefault<UGrimSaveSettings>();
	bSavedEnabledValue = SaveSettings->bEnabled;
	SaveSettings->bEnabled = false;
	bSaveSettingOverridden = true;
	UE_LOG(LogGrimOnline, Log, TEXT("[Online] 메인 메뉴 맵 — 이 월드에서는 세이브를 쓰지 않는다"));
}

void AGrimMenuGameMode::BeginPlay()
{
	RestoreSaveSetting();
	Super::BeginPlay();
}

void AGrimMenuGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RestoreSaveSetting();
	Super::EndPlay(EndPlayReason);
}

void AGrimMenuGameMode::RestoreSaveSetting()
{
	if (bSaveSettingOverridden)
	{
		GetMutableDefault<UGrimSaveSettings>()->bEnabled = bSavedEnabledValue;
		bSaveSettingOverridden = false;
	}
}

void AGrimMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController())
	{
		return;
	}
	if (UGrimUISubsystem* UI = ULocalPlayer::GetSubsystem<UGrimUISubsystem>(GetLocalPlayer()))
	{
		UI->SetHUDVisible(false);
	}
	if (!FApp::CanEverRender())
	{
		return;
	}
	Menu = CreateWidget<UGrimMainMenuWidget>(this, UGrimMainMenuWidget::StaticClass(), TEXT("GrimMainMenu"));
	if (Menu)
	{
		Menu->AddToPlayerScreen(20);
		FInputModeUIOnly Mode;
		Mode.SetWidgetToFocus(Menu->TakeWidget());
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(Mode);
		SetShowMouseCursor(true);
		Menu->FocusFirst();
	}
}

void AGrimMenuPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsLocalController())
	{
		if (UGrimUISubsystem* UI = ULocalPlayer::GetSubsystem<UGrimUISubsystem>(GetLocalPlayer()))
		{
			UI->SetHUDVisible(UGrimUISettings::Get()->bShowHUD);
		}
	}
	if (Menu)
	{
		Menu->RemoveFromParent();
		Menu = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}
