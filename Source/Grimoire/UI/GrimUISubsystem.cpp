#include "UI/GrimUISubsystem.h"
#include "UI/GrimGrimoireWidget.h"
#include "UI/GrimHUDWidget.h"
#include "UI/GrimUISettings.h"
#include "Magic/GrimSpellbookComponent.h"
#include "Core/GrimPlayerState.h"
#include "Blueprint/UserWidget.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Misc/App.h"

DEFINE_LOG_CATEGORY_STATIC(LogGrimUI, Log, All);

namespace GrimUISubsystemLocal
{
	FAutoConsoleCommand GHUDCommand(TEXT("Grim.UI.HUD"),
		TEXT("HUD 보이기 0/1"),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			const bool bVisible = Args.Num() == 0 || FCString::Atoi(*Args[0]) != 0;
			UGrimUISubsystem::ForEach([bVisible](UGrimUISubsystem& UI) { UI.SetHUDVisible(bVisible); });
		}));

	FAutoConsoleCommand GGrimoireCommand(TEXT("Grim.UI.Grimoire"),
		TEXT("그리모어 화면 열기/닫기. [0|1] (없으면 토글)"),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			UGrimUISubsystem::ForEach([&Args](UGrimUISubsystem& UI)
			{
				if (Args.Num() == 0)
				{
					UI.ToggleGrimoire();
				}
				else
				{
					UI.SetGrimoireOpen(FCString::Atoi(*Args[0]) != 0);
				}
				UE_LOG(LogGrimUI, Display, TEXT("그리모어 화면 %s"), UI.IsGrimoireOpen() ? TEXT("열림") : TEXT("닫힘"));
			});
		}));

	FAutoConsoleCommand GSlotSetCommand(TEXT("Grim.UI.SlotSet"),
		TEXT("단축 슬롯 세트 바꾸기. <1부터 세트 번호>"),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			const int32 SetIndex = Args.Num() > 0 ? FCString::Atoi(*Args[0]) - 1 : 0;
			UGrimUISubsystem::ForEach([SetIndex](UGrimUISubsystem& UI) { UI.SelectSlotSet(SetIndex); });
		}));
}

void UGrimUISubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	bHUDVisible = UGrimUISettings::Get()->bShowHUD;
	TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &ThisClass::Tick), 0.25f);
}

void UGrimUISubsystem::Deinitialize()
{
	FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
	TearDownController();
	Super::Deinitialize();
}

void UGrimUISubsystem::ForEach(TFunctionRef<void(UGrimUISubsystem&)> Fn)
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
			if (UGrimUISubsystem* UI = LocalPlayer ? LocalPlayer->GetSubsystem<UGrimUISubsystem>() : nullptr)
			{
				Fn(*UI);
			}
		}
	}
}

bool UGrimUISubsystem::Tick(float DeltaTime)
{
	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	APlayerController* PC = LocalPlayer ? LocalPlayer->PlayerController.Get() : nullptr;
	if (PC && !PC->IsLocalController())
	{
		PC = nullptr;
	}
	if (PC != BoundController.Get())
	{
		TearDownController();
		if (PC && PC->GetWorld() && PC->GetWorld()->IsGameWorld())
		{
			SetupForController(PC);
		}
	}
	return true;
}

void UGrimUISubsystem::SetupForController(APlayerController* PC)
{
	const UGrimUISettings* Settings = UGrimUISettings::Get();
	BoundController = PC;

	// 입력: IMC_UI + 우리 입력 컴포넌트(컨트롤러 입력 스택에 올린다).
	if (UEnhancedInputLocalPlayerSubsystem* Input = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (const UInputMappingContext* Context = Settings->InputContext.LoadSynchronous())
		{
			Input->AddMappingContext(Context, Settings->InputPriority);
		}
		else
		{
			UE_LOG(LogGrimUI, Warning, TEXT("UI 입력 컨텍스트 %s 를 찾지 못했다"), *Settings->InputContext.ToString());
		}
	}
	UIInput = NewObject<UEnhancedInputComponent>(PC, TEXT("GrimUIInput"));
	UIInput->RegisterComponent();
	if (const UInputAction* Toggle = Settings->ToggleGrimoireAction.LoadSynchronous())
	{
		UIInput->BindAction(Toggle, ETriggerEvent::Started, this, &ThisClass::HandleToggleInput);
	}
	for (int32 Index = 0; Index < Settings->SlotSetActions.Num(); ++Index)
	{
		if (const UInputAction* SetAction = Settings->SlotSetActions[Index].LoadSynchronous())
		{
			UIInput->BindAction(SetAction, ETriggerEvent::Started, this, &ThisClass::HandleSlotSetInput, Index);
		}
	}
	PC->PushInputComponent(UIInput);

	// 위젯: 그릴 수 있을 때만.
	if (FApp::CanEverRender())
	{
		HUD = CreateWidget<UGrimHUDWidget>(PC, UGrimHUDWidget::StaticClass(), TEXT("GrimHUD"));
		if (HUD)
		{
			HUD->AddToPlayerScreen(0);
			HUD->SetVisibility(bHUDVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
			if (Settings->bHideSurvivalDebugWithHUD && bHUDVisible)
			{
				if (IConsoleVariable* SurvivalDebug = IConsoleManager::Get().FindConsoleVariable(TEXT("grim.Survival.ShowDebug")))
				{
					SurvivalDebug->Set(0, ECVF_SetByCode);
				}
			}
		}
	}
	UE_LOG(LogGrimUI, Log, TEXT("UI 준비: %s (위젯 %s)"), *GetNameSafe(PC), HUD ? TEXT("있음") : TEXT("없음 — 그리지 않는 프로세스"));
}

void UGrimUISubsystem::TearDownController()
{
	if (APlayerController* PC = BoundController.Get())
	{
		if (UIInput)
		{
			PC->PopInputComponent(UIInput);
		}
		if (bGrimoireOpen)
		{
			PC->SetInputMode(FInputModeGameOnly());
			PC->SetShowMouseCursor(false);
			PC->SetIgnoreLookInput(false);
		}
		if (UEnhancedInputLocalPlayerSubsystem* Input = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			if (const UInputMappingContext* Context = UGrimUISettings::Get()->InputContext.Get())
			{
				Input->RemoveMappingContext(Context);
			}
		}
	}
	if (UIInput)
	{
		UIInput->DestroyComponent();
		UIInput = nullptr;
	}
	if (HUD)
	{
		HUD->RemoveFromParent();
		HUD = nullptr;
	}
	if (Grimoire)
	{
		Grimoire->RemoveFromParent();
		Grimoire = nullptr;
	}
	bGrimoireOpen = false;
	BoundController = nullptr;
}

void UGrimUISubsystem::HandleToggleInput()
{
	ToggleGrimoire();
}

void UGrimUISubsystem::HandleSlotSetInput(int32 SetIndex)
{
	SelectSlotSet(SetIndex);
}

void UGrimUISubsystem::ToggleGrimoire()
{
	SetGrimoireOpen(!bGrimoireOpen);
}

void UGrimUISubsystem::SetGrimoireOpen(bool bOpen)
{
	APlayerController* PC = BoundController.Get();
	if (!PC || bOpen == bGrimoireOpen)
	{
		return;
	}
	bGrimoireOpen = bOpen;

	if (FApp::CanEverRender())
	{
		if (bOpen)
		{
			if (!Grimoire)
			{
				Grimoire = CreateWidget<UGrimGrimoireWidget>(PC, UGrimGrimoireWidget::StaticClass(), TEXT("GrimGrimoire"));
			}
			if (Grimoire)
			{
				Grimoire->MarkDirty();
				Grimoire->AddToPlayerScreen(10);
			}
		}
		else if (Grimoire)
		{
			Grimoire->RemoveFromParent();
		}
	}

	// 열려 있는 동안: 마우스 커서 · 시점 고정. 키 입력은 게임으로 계속 간다(같은 키로 닫는다).
	if (bOpen)
	{
		FInputModeGameAndUI Mode;
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Mode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(Mode);
		PC->SetShowMouseCursor(true);
		PC->SetIgnoreLookInput(true);
	}
	else
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
		PC->SetIgnoreLookInput(false);
	}
}

void UGrimUISubsystem::SelectSlotSet(int32 SetIndex)
{
	const APlayerController* PC = BoundController.Get();
	AGrimPlayerState* PlayerState = PC ? PC->GetPlayerState<AGrimPlayerState>() : nullptr;
	UGrimSpellbookComponent* Spellbook = PlayerState ? PlayerState->GetSpellbook() : nullptr;
	if (!Spellbook)
	{
		return;
	}
	if (PlayerState->HasAuthority())
	{
		Spellbook->SetActiveSlotSet(SetIndex);
	}
	else
	{
		Spellbook->Server_SetActiveSlotSet(SetIndex);
	}
}

void UGrimUISubsystem::SetHUDVisible(bool bVisible)
{
	bHUDVisible = bVisible;
	if (HUD)
	{
		HUD->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}
