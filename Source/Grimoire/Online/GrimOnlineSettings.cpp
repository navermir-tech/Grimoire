#include "Online/GrimOnlineSettings.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

float UGrimOnlineSettings::GetConnectTimeoutSeconds() const
{
	float Override = 0.f;
	if (FParse::Value(FCommandLine::Get(), TEXT("GrimConnectTimeout="), Override) && Override > 0.f)
	{
		return Override;
	}
	return FMath::Max(1.f, ConnectTimeoutSeconds);
}

FString UGrimOnlineSettings::GetMainMenuPackage() const
{
	return MainMenuMap.GetLongPackageName();
}

FString UGrimOnlineSettings::GetGameMapPackage() const
{
	return GameMap.GetLongPackageName();
}
