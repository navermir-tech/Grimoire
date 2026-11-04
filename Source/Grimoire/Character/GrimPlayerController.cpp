#include "Character/GrimPlayerController.h"
#include "Grimoire.h"
#include "Core/Cheats/GrimCheatManager.h"
#include "Core/Cheats/GrimSuperCheat.h"
#include "Core/GrimPlayerState.h"
#include "AbilitySystemComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"

AGrimPlayerController::AGrimPlayerController()
{
	CheatClass = UGrimCheatManager::StaticClass();

	// 템플릿 이동 · 시점(Content/Input) + 그리모리 마법(Content/Grimoire/Input).
	DefaultMappingContexts = {
		TSoftObjectPtr<UInputMappingContext>(FSoftObjectPath(TEXT("/Game/Input/IMC_Default.IMC_Default"))),
		TSoftObjectPtr<UInputMappingContext>(FSoftObjectPath(TEXT("/Game/Input/IMC_MouseLook.IMC_MouseLook"))),
		TSoftObjectPtr<UInputMappingContext>(FSoftObjectPath(TEXT("/Game/Grimoire/Input/IMC_Spell.IMC_Spell"))),
	};
}

void AGrimPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalPlayerController())
	{
		SetInputMode(FInputModeGameOnly());
		SetShowMouseCursor(false);
	}
}

void AGrimPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!IsLocalPlayerController())
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (!Subsystem)
	{
		return;
	}

	for (const TSoftObjectPtr<UInputMappingContext>& ContextRef : DefaultMappingContexts)
	{
		if (const UInputMappingContext* Context = ContextRef.LoadSynchronous())
		{
			Subsystem->AddMappingContext(Context, 0);
		}
		else
		{
			UE_LOG(LogGrimoire, Warning, TEXT("%s: 매핑 컨텍스트 %s 를 찾지 못했다."), *GetNameSafe(this), *ContextRef.ToString());
		}
	}
}

void AGrimPlayerController::AddCheats(bool bForce)
{
#if !UE_BUILD_SHIPPING
	Super::AddCheats(true);
#else
	Super::AddCheats(bForce);
#endif
}

void AGrimPlayerController::Server_SetSuperCheat_Implementation(bool bEnable)
{
#if !UE_BUILD_SHIPPING
	const AGrimPlayerState* GrimPlayerState = GetPlayerState<AGrimPlayerState>();
	UAbilitySystemComponent* ASC = GrimPlayerState ? GrimPlayerState->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return;
	}
	GrimSuperCheat::SetEnabled(ASC, bEnable);
	UE_LOG(LogGrimoire, Log, TEXT("%s: 슈퍼치트 %s"), *GetNameSafe(GrimPlayerState), bEnable ? TEXT("켬") : TEXT("끔"));
	ClientMessage(FString::Printf(TEXT("GrimSuperCheat %s"), bEnable ? TEXT("ON") : TEXT("OFF")));
#endif
}
