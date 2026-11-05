#include "Environment/GrimClimateTypes.h"

float FGrimWeatherWeights::Get(EGrimWeatherType Type) const
{
	switch (Type)
	{
	case EGrimWeatherType::Clear: return Clear;
	case EGrimWeatherType::Cloudy: return Cloudy;
	case EGrimWeatherType::Fog: return Fog;
	case EGrimWeatherType::Rain: return Rain;
	case EGrimWeatherType::Thunderstorm: return Thunderstorm;
	case EGrimWeatherType::Snow: return Snow;
	case EGrimWeatherType::Sandstorm: return Sandstorm;
	default: return 0.f;
	}
}

void FGrimWeatherWeights::Set(EGrimWeatherType Type, float Value)
{
	switch (Type)
	{
	case EGrimWeatherType::Clear: Clear = Value; break;
	case EGrimWeatherType::Cloudy: Cloudy = Value; break;
	case EGrimWeatherType::Fog: Fog = Value; break;
	case EGrimWeatherType::Rain: Rain = Value; break;
	case EGrimWeatherType::Thunderstorm: Thunderstorm = Value; break;
	case EGrimWeatherType::Snow: Snow = Value; break;
	case EGrimWeatherType::Sandstorm: Sandstorm = Value; break;
	default: break;
	}
}

float FGrimWeatherWeights::Sum() const
{
	return Clear + Cloudy + Fog + Rain + Thunderstorm + Snow + Sandstorm;
}

const FGrimWeatherWeights& FGrimClimateRow::GetTransitions(EGrimWeatherType From) const
{
	switch (From)
	{
	case EGrimWeatherType::Cloudy: return FromCloudy;
	case EGrimWeatherType::Fog: return FromFog;
	case EGrimWeatherType::Rain: return FromRain;
	case EGrimWeatherType::Thunderstorm: return FromThunderstorm;
	case EGrimWeatherType::Snow: return FromSnow;
	case EGrimWeatherType::Sandstorm: return FromSandstorm;
	default: return FromClear;
	}
}

bool FGrimClimateRow::IsWeatherAllowed(EGrimWeatherType Type) const
{
	switch (Type)
	{
	case EGrimWeatherType::Snow: return bAllowSnow;
	case EGrimWeatherType::Sandstorm: return bAllowSandstorm;
	case EGrimWeatherType::Count: return false;
	default: return true;
	}
}

FGrimWeatherMix FGrimWeatherMix::Pure(EGrimWeatherType Type)
{
	FGrimWeatherMix Mix;
	if (Type != EGrimWeatherType::Count)
	{
		Mix[Type] = 1.f;
	}
	return Mix;
}

float FGrimWeatherMix::Sum() const
{
	float Total = 0.f;
	for (float W : Weights)
	{
		Total += W;
	}
	return Total;
}

EGrimWeatherType FGrimWeatherMix::GetDominant() const
{
	int32 Best = 0;
	for (int32 i = 1; i < GrimWeatherTypeCount; ++i)
	{
		if (Weights[i] > Weights[Best])
		{
			Best = i;
		}
	}
	return static_cast<EGrimWeatherType>(Best);
}

void FGrimWeatherMix::AddScaled(const FGrimWeatherMix& Other, float Scale)
{
	for (int32 i = 0; i < GrimWeatherTypeCount; ++i)
	{
		Weights[i] += Other.Weights[i] * Scale;
	}
}
