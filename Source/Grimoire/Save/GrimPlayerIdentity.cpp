#include "Save/GrimPlayerIdentity.h"
#include "Save/GrimSaveRecords.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/NetConnection.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Guid.h"
#include "Misc/Parse.h"

const TCHAR* FGrimPlayerIdentity::LoginOptionName = TEXT("GrimPlayerId");

namespace GrimPlayerIdentityLocal // 유니티 빌드 이름 충돌 방지
{
	const TCHAR* ConfigSection = TEXT("/Script/Grimoire.GrimLocalPlayer");
	const TCHAR* ConfigKey = TEXT("PlayerId");

	/** 이 프로세스에서 한 번 정한 값(GameUserSettings.ini 를 매번 읽지 않게) */
	FString& CachedStoredId()
	{
		static FString Value;
		return Value;
	}

	FString GetStoredOrCreatedId()
	{
		FString& Cached = CachedStoredId();
		if (!Cached.IsEmpty())
		{
			return Cached;
		}

		FString Stored;
		if (GConfig && !GGameUserSettingsIni.IsEmpty())
		{
			GConfig->GetString(ConfigSection, ConfigKey, Stored, GGameUserSettingsIni);
		}
		Stored = FGrimSaveRecordMath::SanitizeId(Stored);
		if (Stored.IsEmpty())
		{
			Stored = FGuid::NewGuid().ToString(EGuidFormats::Digits);
			if (GConfig && !GGameUserSettingsIni.IsEmpty())
			{
				GConfig->SetString(ConfigSection, ConfigKey, *Stored, GGameUserSettingsIni);
				GConfig->Flush(false, GGameUserSettingsIni);
			}
			UE_LOG(LogGrimSave, Log, TEXT("이 PC 의 PlayerId 를 새로 만들었다: %s (GameUserSettings.ini)"), *Stored);
		}
		Cached = Stored;
		return Cached;
	}

	int32 GetPIEInstance(const ULocalPlayer* LocalPlayer)
	{
		const UGameInstance* GameInstance = LocalPlayer ? LocalPlayer->GetGameInstance() : nullptr;
		const FWorldContext* Context = GameInstance ? GameInstance->GetWorldContext() : nullptr;
		return (Context && Context->WorldType == EWorldType::PIE) ? Context->PIEInstance : INDEX_NONE;
	}
}

FString FGrimPlayerIdentity::GetLocalPlayerId(const ULocalPlayer* LocalPlayer)
{
	using namespace GrimPlayerIdentityLocal;

	FString Id;
	FString Override;
	if (FParse::Value(FCommandLine::Get(), TEXT("GrimPlayerId="), Override))
	{
		Id = FGrimSaveRecordMath::SanitizeId(Override);
	}
	if (Id.IsEmpty())
	{
		Id = GetStoredOrCreatedId();
	}

	const int32 PIEInstance = GetPIEInstance(LocalPlayer);
	if (PIEInstance > 0)
	{
		Id = FGrimSaveRecordMath::SanitizeId(FString::Printf(TEXT("%s-PIE%d"), *Id, PIEInstance));
	}
	return Id;
}

FString FGrimPlayerIdentity::ParseLoginOption(const FString& RequestURL)
{
	if (RequestURL.IsEmpty())
	{
		return FString();
	}
	const FURL URL(nullptr, *RequestURL, TRAVEL_Absolute);
	const FString Key = FString(LoginOptionName) + TEXT("=");
	const TCHAR* Value = URL.GetOption(*Key, nullptr);
	return Value ? FGrimSaveRecordMath::SanitizeId(Value) : FString();
}

FString FGrimPlayerIdentity::ResolveServerSide(const APlayerState* PlayerState, FString* OutSource)
{
	auto Done = [OutSource](const FString& Id, const TCHAR* Source)
	{
		if (OutSource)
		{
			*OutSource = Source;
		}
		return Id;
	};

	const APlayerController* PC = PlayerState ? PlayerState->GetPlayerController() : nullptr;
	if (!PlayerState || !PC || PlayerState->IsABot())
	{
		return Done(FString(), TEXT("없음"));
	}

	if (PC->IsLocalController())
	{
		return Done(GetLocalPlayerId(PC->GetLocalPlayer()), TEXT("로컬"));
	}

	if (const UNetConnection* Connection = PC->GetNetConnection())
	{
		const FString FromOption = ParseLoginOption(Connection->RequestURL);
		if (!FromOption.IsEmpty())
		{
			return Done(FromOption, TEXT("로그인 옵션"));
		}
	}

	const FUniqueNetIdRepl& NetId = PlayerState->GetUniqueId();
	if (NetId.IsValid())
	{
		const FString FromNet = FGrimSaveRecordMath::SanitizeId(TEXT("Net-") + NetId->ToString());
		if (FromNet.Len() > 4)
		{
			return Done(FromNet, TEXT("UniqueNetId"));
		}
	}

	const FString FromName = FGrimSaveRecordMath::SanitizeId(TEXT("Name-") + PlayerState->GetPlayerName());
	return Done(FromName.Len() > 5 ? FromName : FString(), TEXT("이름"));
}

FString UGrimLocalPlayer::GetGameLoginOptions() const
{
	const FString Id = FGrimPlayerIdentity::GetLocalPlayerId(this);
	return Id.IsEmpty() ? Super::GetGameLoginOptions() : FString::Printf(TEXT("%s=%s"), FGrimPlayerIdentity::LoginOptionName, *Id);
}
