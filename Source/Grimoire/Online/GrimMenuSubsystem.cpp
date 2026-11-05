#include "Online/GrimMenuSubsystem.h"
#include "Online/GrimMenuGameMode.h"
#include "Online/GrimOnlineSettings.h"
#include "Online/GrimSessionSubsystem.h"
#include "Online/GrimUserOptions.h"
#include "UI/GrimMenuWidgets.h"
#include "UI/GrimUISubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Components/InputComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"

namespace GrimMenuSubsystemLocal // 유니티 빌드 이름 충돌 방지
{
	FAutoConsoleCommand GPauseCommand(TEXT("Grim.Online.Pause"),
		TEXT("게임 안 Esc 메뉴 열기/닫기. [0|1] (없으면 토글)"),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			UGrimMenuSubsystem::ForEach([&Args](UGrimMenuSubsystem& Menu)
			{
				if (Args.Num() == 0)
				{
					Menu.TogglePauseMenu();
				}
				else
				{
					Menu.SetPauseMenuOpen(FCString::Atoi(*Args[0]) != 0);
				}
			});
		}));
}

void UGrimMenuSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &ThisClass::Tick), 0.25f);
}

void UGrimMenuSubsystem::Deinitialize()
{
	FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
	TearDownController();
	Super::Deinitialize();
}

void UGrimMenuSubsystem::ForEach(TFunctionRef<void(UGrimMenuSubsystem&)> Fn)
{
	if (!GEngine)
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
			if (UGrimMenuSubsystem* Menu = LocalPlayer ? LocalPlayer->GetSubsystem<UGrimMenuSubsystem>() : nullptr)
			{
				Fn(*Menu);
			}
		}
	}
}

bool UGrimMenuSubsystem::Tick(float DeltaTime)
{
	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	APlayerController* PC = LocalPlayer ? LocalPlayer->PlayerController.Get() : nullptr;
	if (PC && (!PC->IsLocalController() || PC->IsA<AGrimMenuPlayerController>() || !PC->GetWorld() || !PC->GetWorld()->IsGameWorld()))
	{
		PC = nullptr;
	}
	if (PC != BoundController.Get())
	{
		TearDownController();
		if (PC)
		{
			SetupForController(PC);
		}
	}
	if (PC && !bOptionsApplied)
	{
		// 조준 컴포넌트가 BeginPlay 로 기본값을 넣은 뒤에 덮는다.
		bOptionsApplied = FGrimUserOptions::Load().ApplyTo(PC);
		if (bOptionsApplied)
		{
			UE_LOG(LogGrimOnline, Log, TEXT("[Online] 플레이어 옵션 적용: %s"), *GetNameSafe(PC));
		}
	}
	return true;
}

void UGrimMenuSubsystem::SetupForController(APlayerController* PC)
{
	BoundController = PC;
	bOptionsApplied = false;
	MenuInput = NewObject<UInputComponent>(PC, TEXT("GrimMenuInput"));
	MenuInput->RegisterComponent();
	for (const FKey& Key : { EKeys::Escape, EKeys::Gamepad_Special_Right })
	{
		FInputKeyBinding& Binding = MenuInput->BindKey(Key, IE_Pressed, this, &ThisClass::HandleMenuKey);
		Binding.bConsumeInput = true;
		Binding.bExecuteWhenPaused = true;
	}
	PC->PushInputComponent(MenuInput);
}

void UGrimMenuSubsystem::TearDownController()
{
	if (bPauseOpen)
	{
		SetPauseMenuOpen(false);
	}
	if (APlayerController* PC = BoundController.Get())
	{
		if (MenuInput)
		{
			PC->PopInputComponent(MenuInput);
		}
	}
	if (MenuInput)
	{
		MenuInput->DestroyComponent();
		MenuInput = nullptr;
	}
	if (PauseMenu)
	{
		PauseMenu->RemoveFromParent();
		PauseMenu = nullptr;
	}
	bPauseOpen = false;
	bPausedGame = false;
	bOptionsApplied = false;
	BoundController = nullptr;
}

void UGrimMenuSubsystem::HandleMenuKey()
{
	// 그리모어 화면이 열려 있으면 Esc 는 그것부터 닫는다.
	if (UGrimUISubsystem* UI = GetLocalPlayer() ? GetLocalPlayer()->GetSubsystem<UGrimUISubsystem>() : nullptr; UI && UI->IsGrimoireOpen())
	{
		UI->SetGrimoireOpen(false);
		return;
	}
	TogglePauseMenu();
}

void UGrimMenuSubsystem::TogglePauseMenu()
{
	SetPauseMenuOpen(!bPauseOpen);
}

void UGrimMenuSubsystem::SetPauseMenuOpen(bool bOpen)
{
	APlayerController* PC = BoundController.Get();
	if (!PC || bOpen == bPauseOpen)
	{
		return;
	}
	bPauseOpen = bOpen;

	if (bOpen)
	{
		if (FApp::CanEverRender())
		{
			if (!PauseMenu)
			{
				PauseMenu = CreateWidget<UGrimPauseMenuWidget>(PC, UGrimPauseMenuWidget::StaticClass(), TEXT("GrimPauseMenu"));
			}
			if (PauseMenu)
			{
				PauseMenu->ResetToMain();
				PauseMenu->AddToPlayerScreen(30);
				FInputModeUIOnly Mode;
				Mode.SetWidgetToFocus(PauseMenu->TakeWidget());
				Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
				PC->SetInputMode(Mode);
				PauseMenu->FocusFirst();
			}
		}
		PC->SetShowMouseCursor(true);
		const UGrimSessionSubsystem* Session = UGrimSessionSubsystem::Get(PC);
		if (UGrimOnlineSettings::Get()->bPauseWhenStandalone && Session && Session->GetState() == EGrimConnectionState::Standalone)
		{
			bPausedGame = UGameplayStatics::SetGamePaused(PC, true);
		}
		UE_LOG(LogGrimOnline, Display, TEXT("[Online] Esc 메뉴 열림%s"), bPausedGame ? TEXT(" — 혼자라 게임을 멈춘다") : TEXT(""));
	}
	else
	{
		if (PauseMenu)
		{
			PauseMenu->RemoveFromParent();
		}
		if (bPausedGame)
		{
			UGameplayStatics::SetGamePaused(PC, false);
			bPausedGame = false;
		}
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
		UE_LOG(LogGrimOnline, Display, TEXT("[Online] Esc 메뉴 닫힘"));
	}
}
