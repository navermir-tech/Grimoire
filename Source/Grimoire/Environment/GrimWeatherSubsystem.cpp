#include "Environment/GrimWeatherSubsystem.h"
#include "Environment/GrimEnvironmentSettings.h"
#include "Environment/GrimTimeOfDayMath.h"
#include "Environment/GrimTimeOfDaySubsystem.h"
#include "Engine/World.h"

bool UGrimWeatherSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

float UGrimWeatherSubsystem::GetAmbientTemperature(FVector Location) const
{
	const UGrimEnvironmentSettings* Settings = GetDefault<UGrimEnvironmentSettings>();

	float TimeOfDay = Settings->StartTimeOfDay;
	if (const UGrimTimeOfDaySubsystem* Time = GetWorld() ? GetWorld()->GetSubsystem<UGrimTimeOfDaySubsystem>() : nullptr)
	{
		TimeOfDay = Time->GetTimeOfDay();
	}

	return FGrimTimeOfDayMath::GetAmbientTemperature(TimeOfDay,
		Settings->MinAmbientTemperature, Settings->MaxAmbientTemperature,
		Settings->ColdestTimeOfDay, Settings->WarmestTimeOfDay);
}

FGrimClimateExposure UGrimWeatherSubsystem::GetClimateExposure(FVector Location) const
{
	// 임시 식(M2 날씨 갈래가 지역 표 · 날씨 가산으로 바꾼다). 쾌적 범위 밖으로 4℃ 마다 한 등급.
	const float Temperature = GetAmbientTemperature(Location);
	FGrimClimateExposure Exposure;
	Exposure.Cold = FMath::Max(0, FMath::CeilToInt((14.f - Temperature) / 4.f));
	Exposure.Heat = FMath::Max(0, FMath::CeilToInt((Temperature - 26.f) / 4.f));
	return Exposure;
}
