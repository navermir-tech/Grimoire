#include "World/GrimZoneSettings.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace GrimZoneSettingsLocal
{
	FGrimZoneDatabase GDatabase;
	bool GLoaded = false;
}

const FGrimZoneDatabase& UGrimZoneSettings::GetDatabase()
{
	using namespace GrimZoneSettingsLocal;
	if (!GLoaded)
	{
		GLoaded = true;
		const FString Path = FPaths::Combine(FPaths::ProjectContentDir(), Get()->ZonesFile);
		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *Path))
		{
			UE_LOG(LogGrimZone, Error, TEXT("구역 표 %s 를 열지 못함 — 구역 이름이 뜨지 않는다"), *Path);
			GDatabase = FGrimZoneDatabase();
			return GDatabase;
		}
		FString Error;
		if (!FGrimZoneMath::ParseJson(Text, GDatabase, Error))
		{
			UE_LOG(LogGrimZone, Error, TEXT("구역 표 %s: %s"), *Get()->ZonesFile, *Error);
		}
		UE_LOG(LogGrimZone, Log, TEXT("구역 표 %s — 구역 %d"), *Get()->ZonesFile, GDatabase.Zones.Num());
	}
	return GDatabase;
}

void UGrimZoneSettings::ReloadData()
{
	GrimZoneSettingsLocal::GLoaded = false;
	GetDatabase();
}

const FGrimZoneBannerStyle& UGrimZoneSettings::GetStyle(EGrimZoneTier Tier, bool bFirstVisit) const
{
	if (Tier == EGrimZoneTier::Region)
	{
		return bFirstVisit ? RegionFirst : RegionRevisit;
	}
	return bFirstVisit ? SectorFirst : SectorRevisit;
}
