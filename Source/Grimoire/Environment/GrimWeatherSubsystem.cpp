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
