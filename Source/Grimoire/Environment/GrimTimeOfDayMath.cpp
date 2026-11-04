#include "Environment/GrimTimeOfDayMath.h"

DEFINE_LOG_CATEGORY(LogGrimEnvironment);

namespace
{
	// [0, 1) 로 감싼다.
	float Wrap01(float Value)
	{
		const float Result = FMath::Fmod(Value, 1.f);
		return Result < 0.f ? Result + 1.f : Result;
	}
}

double FGrimTimeOfDayMath::ExtrapolateTotalDays(double SnapshotTotalDays, double SnapshotServerSeconds, double NowServerSeconds, double DayLengthSeconds)
{
	if (DayLengthSeconds <= 0.0)
	{
		return SnapshotTotalDays;
	}
	const double Elapsed = FMath::Max(0.0, NowServerSeconds - SnapshotServerSeconds);
	return SnapshotTotalDays + Elapsed / DayLengthSeconds;
}

float FGrimTimeOfDayMath::GetTimeOfDay(double TotalDays)
{
	return static_cast<float>(TotalDays - FMath::FloorToDouble(TotalDays));
}

int32 FGrimTimeOfDayMath::GetDayIndex(double TotalDays)
{
	return FMath::FloorToInt32(TotalDays);
}

float FGrimTimeOfDayMath::GetDawn(float DayRatio)
{
	return 0.5f - FMath::Clamp(DayRatio, 0.f, 1.f) * 0.5f;
}

float FGrimTimeOfDayMath::GetDusk(float DayRatio)
{
	return 0.5f + FMath::Clamp(DayRatio, 0.f, 1.f) * 0.5f;
}

bool FGrimTimeOfDayMath::IsNight(float TimeOfDay, float DayRatio)
{
	const float T = Wrap01(TimeOfDay);
	return T < GetDawn(DayRatio) || T >= GetDusk(DayRatio);
}

float FGrimTimeOfDayMath::GetSunAngleDegrees(float TimeOfDay, float DayRatio)
{
	const float T = Wrap01(TimeOfDay);
	const float Dawn = GetDawn(DayRatio);
	const float Dusk = GetDusk(DayRatio);
	const float DayLength = Dusk - Dawn;
	const float NightLength = 1.f - DayLength;

	if (T >= Dawn && T < Dusk)
	{
		return DayLength > 0.f ? (T - Dawn) / DayLength * 180.f : 0.f;
	}
	const float SinceDusk = Wrap01(T - Dusk);
	return 180.f + (NightLength > 0.f ? SinceDusk / NightLength * 180.f : 0.f);
}

float FGrimTimeOfDayMath::GetAmbientTemperature(float TimeOfDay, float MinTemperature, float MaxTemperature, float ColdestTimeOfDay, float WarmestTimeOfDay)
{
	const float T = Wrap01(TimeOfDay);
	const float Coldest = Wrap01(ColdestTimeOfDay);
	const float RiseLength = Wrap01(WarmestTimeOfDay - Coldest);
	const float FallLength = 1.f - RiseLength;
	const float SinceColdest = Wrap01(T - Coldest);
	const float Range = MaxTemperature - MinTemperature;

	if (RiseLength <= KINDA_SMALL_NUMBER || FallLength <= KINDA_SMALL_NUMBER)
	{
		return (MinTemperature + MaxTemperature) * 0.5f;
	}

	if (SinceColdest < RiseLength)
	{
		const float Alpha = SinceColdest / RiseLength;
		return MinTemperature + Range * (1.f - FMath::Cos(PI * Alpha)) * 0.5f;
	}
	const float Alpha = (SinceColdest - RiseLength) / FallLength;
	return MaxTemperature - Range * (1.f - FMath::Cos(PI * Alpha)) * 0.5f;
}
