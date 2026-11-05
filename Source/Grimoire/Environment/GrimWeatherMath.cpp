#include "Environment/GrimWeatherMath.h"
#include "Environment/GrimTimeOfDayMath.h"

EGrimWeatherType FGrimWeatherMath::PickNextWeather(const FGrimClimateRow& Row, EGrimWeatherType Current, float Random01)
{
	const FGrimWeatherWeights& Transitions = Row.GetTransitions(Current);

	float Weights[GrimWeatherTypeCount];
	for (EGrimWeatherType Type : TEnumRange<EGrimWeatherType>())
	{
		const float W = Transitions.Get(Type);
		Weights[static_cast<int32>(Type)] = (Row.IsWeatherAllowed(Type) && W > 0.f) ? W : 0.f;
	}

	const int32 Picked = PickWeightedIndex(TArrayView<const float>(Weights, GrimWeatherTypeCount), Random01);
	if (Picked == INDEX_NONE)
	{
		return Current;
	}
	return ApplyPrecipitation(Row, static_cast<EGrimWeatherType>(Picked));
}

EGrimWeatherType FGrimWeatherMath::ApplyPrecipitation(const FGrimClimateRow& Row, EGrimWeatherType Type)
{
	if (Row.Precipitation == EGrimPrecipitation::Snow && Row.bAllowSnow
		&& (Type == EGrimWeatherType::Rain || Type == EGrimWeatherType::Thunderstorm))
	{
		return EGrimWeatherType::Snow;
	}
	return Type;
}

EGrimWeatherType FGrimWeatherMath::ClampToAllowed(const FGrimClimateRow& Row, EGrimWeatherType Type)
{
	if (Type == EGrimWeatherType::Count)
	{
		return EGrimWeatherType::Clear;
	}
	if (Row.IsWeatherAllowed(Type))
	{
		return Type;
	}
	return Type == EGrimWeatherType::Snow ? EGrimWeatherType::Rain : EGrimWeatherType::Cloudy;
}

float FGrimWeatherMath::GetTransitionAlpha(double NowSeconds, double StartSeconds, float DurationSeconds)
{
	if (DurationSeconds <= 0.f)
	{
		return 1.f;
	}
	const float Linear = static_cast<float>(FMath::Clamp((NowSeconds - StartSeconds) / DurationSeconds, 0.0, 1.0));
	return FMath::SmoothStep(0.f, 1.f, Linear);
}

FGrimWeatherMix FGrimWeatherMath::GetRegionMix(const FGrimRegionWeather& State, double NowServerSeconds)
{
	const float Alpha = GetTransitionAlpha(NowServerSeconds, State.TransitionStartServerSeconds, State.TransitionSeconds);
	FGrimWeatherMix Mix;
	Mix.AddScaled(FGrimWeatherMix::Pure(State.Previous), 1.f - Alpha);
	Mix.AddScaled(FGrimWeatherMix::Pure(State.Type), Alpha);
	return Mix;
}

float FGrimWeatherMath::GetBoxBlendWeight(const FVector& LocalPoint, const FVector& Extent, float BlendDistance)
{
	// 각 축에서 면까지 안쪽 거리 중 가장 작은 것 = 경계까지 거리. 음수면 바깥.
	const FVector Abs = LocalPoint.GetAbs();
	const double Inside = FMath::Min3(Extent.X - Abs.X, Extent.Y - Abs.Y, Extent.Z - Abs.Z);
	if (Inside < 0.0)
	{
		return 0.f;
	}
	if (BlendDistance <= 0.f)
	{
		return 1.f;
	}
	return static_cast<float>(FMath::Clamp(Inside / BlendDistance, 0.0, 1.0));
}

float FGrimWeatherMath::NormalizeRegionWeights(TArrayView<float> InOutVolumeWeights)
{
	float Total = 0.f;
	for (float& W : InOutVolumeWeights)
	{
		W = FMath::Clamp(W, 0.f, 1.f);
		Total += W;
	}
	if (Total > 1.f)
	{
		for (float& W : InOutVolumeWeights)
		{
			W /= Total;
		}
		return 0.f;
	}
	return 1.f - Total;
}

FGrimExposureValue FGrimWeatherMath::GetRegionExposure(const FGrimClimateRow& Row, bool bNight)
{
	FGrimExposureValue Out;
	Out.Heat = static_cast<float>(bNight ? Row.NightHeat : Row.DayHeat);
	Out.Cold = static_cast<float>(bNight ? Row.NightCold : Row.DayCold);
	return Out;
}

FGrimExposureValue FGrimWeatherMath::GetWeatherExposure(const FGrimClimateRow& Row, const FGrimWeatherMix& Mix, bool bNoon)
{
	FGrimExposureValue Out;
	Out.Cold += (Mix.Get(EGrimWeatherType::Rain) + Mix.Get(EGrimWeatherType::Thunderstorm)) * Row.RainCold;
	Out.Cold += Mix.Get(EGrimWeatherType::Snow) * Row.SnowCold;
	Out.Heat += Mix.Get(EGrimWeatherType::Sandstorm) * Row.SandstormHeat;
	if (bNoon)
	{
		Out.Heat += Mix.Get(EGrimWeatherType::Clear) * Row.ClearNoonHeat;
	}
	return Out;
}

int32 FGrimWeatherMath::RoundGrade(float Value)
{
	// 블렌드 중의 부동소수 오차로 0.4999 가 되어 등급이 내려가지 않게 조금 올린다.
	return FMath::Max(0, FMath::FloorToInt(Value + 0.5f + KINDA_SMALL_NUMBER));
}

void FGrimWeatherMath::NetGrades(int32 Heat, int32 Cold, int32& OutHeat, int32& OutCold)
{
	OutHeat = FMath::Max(0, Heat - Cold);
	OutCold = FMath::Max(0, Cold - Heat);
}

float FGrimWeatherMath::GetRegionTemperature(const FGrimClimateRow& Row, float TimeOfDay)
{
	return FGrimTimeOfDayMath::GetAmbientTemperature(TimeOfDay, Row.MinTemperature, Row.MaxTemperature, Row.ColdestTimeOfDay, Row.WarmestTimeOfDay);
}

bool FGrimWeatherMath::IsNoon(float TimeOfDay, float StartHour, float EndHour)
{
	const float Hours = FGrimTimeOfDayMath::GetTimeOfDay(TimeOfDay) * 24.f;
	return Hours >= StartHour && Hours < EndHour;
}

int32 FGrimWeatherMath::PickWeightedIndex(TArrayView<const float> Weights, float Random01)
{
	float Total = 0.f;
	for (float W : Weights)
	{
		Total += FMath::Max(0.f, W);
	}
	if (Total <= 0.f)
	{
		return INDEX_NONE;
	}

	const float Target = FMath::Clamp(Random01, 0.f, 1.f) * Total;
	float Running = 0.f;
	int32 LastPositive = INDEX_NONE;
	for (int32 i = 0; i < Weights.Num(); ++i)
	{
		const float W = FMath::Max(0.f, Weights[i]);
		if (W <= 0.f)
		{
			continue;
		}
		LastPositive = i;
		Running += W;
		if (Target < Running)
		{
			return i;
		}
	}
	// Random01 = 1 · 부동소수 오차: 마지막 양수 칸.
	return LastPositive;
}

float FGrimWeatherMath::GetLightningScore(float HeightAboveReferenceCm, float WetIntensity, bool bConductive,
	float BaseScore, float HeightWeightPerMeter, float WetWeight, float ConductiveWeight)
{
	return FMath::Max(0.f, BaseScore)
		+ FMath::Max(0.f, HeightAboveReferenceCm) / 100.f * HeightWeightPerMeter
		+ FMath::Max(0.f, WetIntensity) * WetWeight
		+ (bConductive ? ConductiveWeight : 0.f);
}
