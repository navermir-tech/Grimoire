#include "Online/GrimUserOptions.h"
#include "Online/GrimOnlineMath.h"
#include "Online/GrimOnlineSettings.h"
#include "Character/GrimPlayerController.h"
#include "Targeting/GrimTargetingComponent.h"
#include "Targeting/GrimTargetingSettings.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformProcess.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Parse.h"

namespace GrimUserOptionsLocal // 유니티 빌드 이름 충돌 방지
{
	const TCHAR* Section = TEXT("/Script/Grimoire.GrimUserOptions");

	bool CanUseConfig()
	{
		return GConfig && !GGameUserSettingsIni.IsEmpty();
	}
}

FGrimUserOptions FGrimUserOptions::Load()
{
	using namespace GrimUserOptionsLocal;
	FGrimUserOptions Options;
	const UGrimTargetingSettings* Targeting = GetDefault<UGrimTargetingSettings>();
	Options.bSoftLockEnabled = Targeting->bSoftLockEnabledByDefault;
	Options.bZoomAimToggle = Targeting->bZoomAimToggleByDefault;
	Options.ZoomLookSensitivity = Targeting->ZoomLookSensitivityScale;

	if (CanUseConfig())
	{
		GConfig->GetString(Section, TEXT("PlayerName"), Options.PlayerName, GGameUserSettingsIni);
		GConfig->GetBool(Section, TEXT("bSoftLockEnabled"), Options.bSoftLockEnabled, GGameUserSettingsIni);
		GConfig->GetBool(Section, TEXT("bZoomAimToggle"), Options.bZoomAimToggle, GGameUserSettingsIni);
		GConfig->GetFloat(Section, TEXT("ZoomLookSensitivity"), Options.ZoomLookSensitivity, GGameUserSettingsIni);
		GConfig->GetFloat(Section, TEXT("LookSensitivity"), Options.LookSensitivity, GGameUserSettingsIni);
		GConfig->GetString(Section, TEXT("LastSlot"), Options.LastSlot, GGameUserSettingsIni);
		GConfig->GetString(Section, TEXT("LastAddress"), Options.LastAddress, GGameUserSettingsIni);
		GConfig->GetInt(Section, TEXT("LastPort"), Options.LastPort, GGameUserSettingsIni);
		GConfig->GetInt(Section, TEXT("LastMaxPlayers"), Options.LastMaxPlayers, GGameUserSettingsIni);
	}

	FString Override;
	if (FParse::Value(FCommandLine::Get(), TEXT("GrimName="), Override))
	{
		Options.PlayerName = Override;
		Options.bNameFromCommandLine = true;
	}

	const UGrimOnlineSettings* Settings = UGrimOnlineSettings::Get();
	Options.PlayerName = FGrimOnlineMath::SanitizePlayerName(Options.PlayerName, Settings->PlayerNameMaxLength);
	Options.LookSensitivity = FMath::Clamp(Options.LookSensitivity, (float)Settings->LookSensitivityRange.X, (float)Settings->LookSensitivityRange.Y);
	Options.ZoomLookSensitivity = FMath::Clamp(Options.ZoomLookSensitivity, (float)Settings->ZoomSensitivityRange.X, (float)Settings->ZoomSensitivityRange.Y);
	return Options;
}

void FGrimUserOptions::Save() const
{
	using namespace GrimUserOptionsLocal;
	if (!CanUseConfig())
	{
		return;
	}
	if (!bNameFromCommandLine)
	{
		GConfig->SetString(Section, TEXT("PlayerName"), *PlayerName, GGameUserSettingsIni);
	}
	GConfig->SetBool(Section, TEXT("bSoftLockEnabled"), bSoftLockEnabled, GGameUserSettingsIni);
	GConfig->SetBool(Section, TEXT("bZoomAimToggle"), bZoomAimToggle, GGameUserSettingsIni);
	GConfig->SetFloat(Section, TEXT("ZoomLookSensitivity"), ZoomLookSensitivity, GGameUserSettingsIni);
	GConfig->SetFloat(Section, TEXT("LookSensitivity"), LookSensitivity, GGameUserSettingsIni);
	GConfig->SetString(Section, TEXT("LastSlot"), *LastSlot, GGameUserSettingsIni);
	GConfig->SetString(Section, TEXT("LastAddress"), *LastAddress, GGameUserSettingsIni);
	GConfig->SetInt(Section, TEXT("LastPort"), LastPort, GGameUserSettingsIni);
	GConfig->SetInt(Section, TEXT("LastMaxPlayers"), LastMaxPlayers, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}

FString FGrimUserOptions::GetEffectivePlayerName() const
{
	const UGrimOnlineSettings* Settings = UGrimOnlineSettings::Get();
	FString Name = FGrimOnlineMath::SanitizePlayerName(PlayerName, Settings->PlayerNameMaxLength);
	if (Name.IsEmpty())
	{
		Name = FGrimOnlineMath::SanitizePlayerName(FPlatformProcess::UserName(), Settings->PlayerNameMaxLength);
	}
	if (Name.IsEmpty())
	{
		Name = FGrimOnlineMath::SanitizePlayerName(Settings->FallbackPlayerName, Settings->PlayerNameMaxLength);
	}
	return Name;
}

bool FGrimUserOptions::ApplyTo(APlayerController* PC) const
{
	if (!PC || !PC->IsLocalController() || !PC->HasActorBegunPlay())
	{
		return false;
	}

	// 시점 감도 — 엔진의 (옛) 시점 입력 배율. 클래스 기본값(설정)에 곱한다.
	const APlayerController* Defaults = PC->GetClass()->GetDefaultObject<APlayerController>();
	PC->InputYawScale_DEPRECATED = Defaults->InputYawScale_DEPRECATED * LookSensitivity;
	PC->InputPitchScale_DEPRECATED = Defaults->InputPitchScale_DEPRECATED * LookSensitivity;

	const AGrimPlayerController* GrimPC = Cast<AGrimPlayerController>(PC);
	if (UGrimTargetingComponent* Targeting = GrimPC ? GrimPC->GetTargetingComponent() : nullptr)
	{
		if (!Targeting->HasBegunPlay())
		{
			return false;
		}
		Targeting->SetSoftLockEnabled(bSoftLockEnabled);
		Targeting->SetZoomToggleMode(bZoomAimToggle);
		Targeting->SetZoomLookSensitivityScale(ZoomLookSensitivity);
	}
	return true;
}

void FGrimUserOptions::ApplyToAllLocalPlayers()
{
	if (!GEngine)
	{
		return;
	}
	const FGrimUserOptions Options = Load();
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		const UGameInstance* GameInstance = Context.OwningGameInstance;
		if (!GameInstance || !Context.World() || !Context.World()->IsGameWorld())
		{
			continue;
		}
		for (ULocalPlayer* LocalPlayer : GameInstance->GetLocalPlayers())
		{
			Options.ApplyTo(LocalPlayer ? LocalPlayer->PlayerController.Get() : nullptr);
		}
	}
}
