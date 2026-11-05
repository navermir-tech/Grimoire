#include "Environment/GrimWeatherSettings.h"
#include "NiagaraSystem.h"

namespace GrimWeatherSettingsLocal
{
	FGrimWeatherProfile MakeProfile(float TemperatureOffset, float Rain, float Snow, float Dust, float Visibility, float SoftLockScale,
		float SunScale, float SkyLightScale, float FogAdd)
	{
		FGrimWeatherProfile P;
		P.TemperatureOffset = TemperatureOffset;
		P.RainAmount = Rain;
		P.SnowAmount = Snow;
		P.DustAmount = Dust;
		P.VisibilityDistance = Visibility;
		P.SoftLockRangeScale = SoftLockScale;
		P.SunIntensityScale = SunScale;
		P.SkyLightScale = SkyLightScale;
		P.FogDensityAdd = FogAdd;
		return P;
	}
}

UGrimWeatherSettings::UGrimWeatherSettings()
{
	using namespace GrimWeatherSettingsLocal;

	// ini 가 없을 때의 기본값. 정본은 DefaultGame.ini.
	ClearProfile = MakeProfile(0.f, 0.f, 0.f, 0.f, 100000.f, 1.f, 1.f, 1.f, 0.f);
	CloudyProfile = MakeProfile(-1.5f, 0.f, 0.f, 0.f, 60000.f, 1.f, 0.45f, 0.7f, 0.01f);
	FogProfile = MakeProfile(-1.f, 0.f, 0.f, 0.f, 3000.f, 0.6f, 0.5f, 0.8f, 0.15f);
	RainProfile = MakeProfile(-3.f, 1.f, 0.f, 0.f, 20000.f, 0.9f, 0.3f, 0.55f, 0.04f);
	ThunderstormProfile = MakeProfile(-4.f, 1.f, 0.f, 0.f, 12000.f, 0.85f, 0.15f, 0.4f, 0.06f);
	SnowProfile = MakeProfile(-4.f, 0.f, 1.f, 0.f, 8000.f, 0.8f, 0.4f, 0.7f, 0.08f);
	SandstormProfile = MakeProfile(3.f, 0.f, 0.f, 1.f, 4000.f, 0.65f, 0.35f, 0.6f, 0.12f);

	const TCHAR* Templates = TEXT("/Niagara/DefaultAssets/Templates/Systems/");
	RainSystem = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(FString(Templates) + TEXT("FountainLightweight.FountainLightweight")));
	SnowSystem = RainSystem;
	DustSystem = RainSystem;
	LightningSystem = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(FString(Templates) + TEXT("RadialBurst.RadialBurst")));
}

const FGrimWeatherProfile& UGrimWeatherSettings::GetProfile(EGrimWeatherType Type) const
{
	switch (Type)
	{
	case EGrimWeatherType::Cloudy: return CloudyProfile;
	case EGrimWeatherType::Fog: return FogProfile;
	case EGrimWeatherType::Rain: return RainProfile;
	case EGrimWeatherType::Thunderstorm: return ThunderstormProfile;
	case EGrimWeatherType::Snow: return SnowProfile;
	case EGrimWeatherType::Sandstorm: return SandstormProfile;
	default: return ClearProfile;
	}
}
