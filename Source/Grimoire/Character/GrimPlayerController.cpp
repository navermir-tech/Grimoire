#include "Character/GrimPlayerController.h"
#include "Grimoire.h"
#include "Core/Cheats/GrimCheatManager.h"
#include "Core/Cheats/GrimSuperCheat.h"
#include "Core/GrimPlayerState.h"
#include "Targeting/GrimTargetingComponent.h"
#include "AbilitySystemComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"

AGrimPlayerController::AGrimPlayerController()
{
	CheatClass = UGrimCheatManager::StaticClass();

	// 템플릿 이동 · 시점(Content/Input) + 그리모리 마법 · 조준(Content/Grimoire/Input).
	DefaultMappingContexts = {
		TSoftObjectPtr<UInputMappingContext>(FSoftObjectPath(TEXT("/Game/Input/IMC_Default.IMC_Default"))),
		TSoftObjectPtr<UInputMappingContext>(FSoftObjectPath(TEXT("/Game/Input/IMC_MouseLook.IMC_MouseLook"))),
		TSoftObjectPtr<UInputMappingContext>(FSoftObjectPath(TEXT("/Game/Grimoire/Input/IMC_Spell.IMC_Spell"))),
		TSoftObjectPtr<UInputMappingContext>(FSoftObjectPath(TEXT("/Game/Grimoire/Input/IMC_Targeting.IMC_Targeting"))),
	};
	LockOnAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Grimoire/Input/IA_LockOn.IA_LockOn")));
	AimZoomAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Grimoire/Input/IA_AimZoom.IA_AimZoom")));

	Targeting = CreateDefaultSubobject<UGrimTargetingComponent>(TEXT("Targeting"));
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

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EnhancedInput)
	{
		return;
	}
	if (const UInputAction* Action = LockOnAction.LoadSynchronous())
	{
		EnhancedInput->BindAction(Action, ETriggerEvent::Started, this, &ThisClass::HandleLockOnPressed);
	}
	else
	{
		UE_LOG(LogGrimoire, Warning, TEXT("%s: 조준 입력 %s 를 찾지 못했다."), *GetNameSafe(this), *LockOnAction.ToString());
	}
	if (const UInputAction* Action = AimZoomAction.LoadSynchronous())
	{
		EnhancedInput->BindAction(Action, ETriggerEvent::Started, this, &ThisClass::HandleAimZoomPressed);
		EnhancedInput->BindAction(Action, ETriggerEvent::Completed, this, &ThisClass::HandleAimZoomReleased);
	}
}

void AGrimPlayerController::HandleLockOnPressed()
{
	if (Targeting)
	{
		Targeting->ToggleLock();
	}
}

void AGrimPlayerController::HandleAimZoomPressed()
{
	// TODO(R24): 줌인 조준은 스펙 조사 중. 지금은 상태만 넘기고 조준 · 카메라는 그대로.
	if (Targeting)
	{
		Targeting->SetZoomAimHeld(true);
	}
}

void AGrimPlayerController::HandleAimZoomReleased()
{
	if (Targeting)
	{
		Targeting->SetZoomAimHeld(false);
	}
}

void AGrimPlayerController::AddYawInput(float Val)
{
	if (Targeting && Val != 0.f && Targeting->HandleLookInput(FVector2D(Val, 0.f)))
	{
		return;
	}
	Super::AddYawInput(Val);
}

void AGrimPlayerController::AddPitchInput(float Val)
{
	if (Targeting && Val != 0.f && Targeting->HandleLookInput(FVector2D(0.f, Val)))
	{
		return;
	}
	Super::AddPitchInput(Val);
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
