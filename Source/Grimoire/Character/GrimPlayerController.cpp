#include "Character/GrimPlayerController.h"
#include "Grimoire.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"

AGrimPlayerController::AGrimPlayerController()
{
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
