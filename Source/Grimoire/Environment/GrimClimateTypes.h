#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GrimClimateTypes.generated.h"

/** 날씨 종류 (R04). Snow · Sandstorm 은 기후 표가 허용한 지역에서만(Q14). 순서는 FGrimWeatherMix 의 칸 순서다. */
UENUM(BlueprintType)
enum class EGrimWeatherType : uint8
{
	Clear,
	Cloudy,
	Fog,
	Rain,
	Thunderstorm,
	Snow,
	Sandstorm,

	Count UMETA(Hidden)
};

ENUM_RANGE_BY_COUNT(EGrimWeatherType, EGrimWeatherType::Count);

constexpr int32 GrimWeatherTypeCount = static_cast<int32>(EGrimWeatherType::Count);

/** 지역의 강수 종류. Snow 면 상태 머신이 고른 Rain · Thunderstorm 이 Snow 가 된다(그 지역이 Snow 를 허용할 때만). */
UENUM(BlueprintType)
enum class EGrimPrecipitation : uint8
{
	Rain,
	Snow
};

/** 다음 날씨 가중치 한 줄(마르코프 전이표의 한 행). 자기 자신 가중치 = 그대로 머물 확률. */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimWeatherWeights
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather", meta = (ClampMin = "0"))
	float Clear = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather", meta = (ClampMin = "0"))
	float Cloudy = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather", meta = (ClampMin = "0"))
	float Fog = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather", meta = (ClampMin = "0"))
	float Rain = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather", meta = (ClampMin = "0"))
	float Thunderstorm = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather", meta = (ClampMin = "0"))
	float Snow = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather", meta = (ClampMin = "0"))
	float Sandstorm = 0.f;

	float Get(EGrimWeatherType Type) const;
	void Set(EGrimWeatherType Type, float Value);
	float Sum() const;
};

/**
 * 지역 기후 한 행 (DT_ClimateRegions). 원본은 Content/Grimoire/Data/ClimateRegions.csv, 행 이름 = 지역 id.
 * 기온 곡선 · 시간대별 더위/추위 등급 · 날씨 가산 · 전이표 · 강수 · 전용 날씨 허용을 한 줄에 둔다.
 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimClimateRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 사람용 설명(규칙에 영향 없음) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
	FString Note;

	/** 기온 곡선(섭씨): ColdestTimeOfDay 에 최저, WarmestTimeOfDay 에 최고, 사이는 반 코사인 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Temperature")
	float MinTemperature = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Temperature")
	float MaxTemperature = 22.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Temperature", meta = (ClampMin = "0", ClampMax = "1"))
	float ColdestTimeOfDay = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Temperature", meta = (ClampMin = "0", ClampMax = "1"))
	float WarmestTimeOfDay = 0.6f;

	/** 시간대별 등급 (docs/spec/survival 「체온 모델」 — 지역 · 시간대) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exposure", meta = (ClampMin = "0"))
	int32 DayHeat = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exposure", meta = (ClampMin = "0"))
	int32 DayCold = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exposure", meta = (ClampMin = "0"))
	int32 NightHeat = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exposure", meta = (ClampMin = "0"))
	int32 NightCold = 0;

	/** 날씨 가산 (「체온 모델」 — 날씨). 비 · 뇌우 추위, 눈 추위, 모래폭풍 더위, 맑은 한낮 더위(사막류만 0 보다 큼) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exposure", meta = (ClampMin = "0"))
	int32 RainCold = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exposure", meta = (ClampMin = "0"))
	int32 SnowCold = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exposure", meta = (ClampMin = "0"))
	int32 SandstormHeat = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exposure", meta = (ClampMin = "0"))
	int32 ClearNoonHeat = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather")
	EGrimPrecipitation Precipitation = EGrimPrecipitation::Rain;

	/** 지역 전용 날씨 허용(Q14). 허용하지 않으면 전이표에 가중치가 있어도 고르지 않는다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather")
	bool bAllowSnow = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather")
	bool bAllowSandstorm = false;

	/** 서버 시작 때 날씨 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather")
	EGrimWeatherType InitialWeather = EGrimWeatherType::Clear;

	/** 한 날씨가 최소로 머무는 시간(초, 실시간). 지난 뒤부터 설정의 TransitionCheckIntervalSeconds 마다 전이를 굴린다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather", meta = (ClampMin = "0"))
	float MinWeatherSeconds = 300.f;

	/** 전이표: 지금 날씨 → 다음 날씨 가중치. 가중치가 모두 0 인 행은 그대로 머문다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transitions")
	FGrimWeatherWeights FromClear;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transitions")
	FGrimWeatherWeights FromCloudy;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transitions")
	FGrimWeatherWeights FromFog;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transitions")
	FGrimWeatherWeights FromRain;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transitions")
	FGrimWeatherWeights FromThunderstorm;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transitions")
	FGrimWeatherWeights FromSnow;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transitions")
	FGrimWeatherWeights FromSandstorm;

	const FGrimWeatherWeights& GetTransitions(EGrimWeatherType From) const;
	bool IsWeatherAllowed(EGrimWeatherType Type) const;
};

/**
 * 지역 하나의 날씨 상태(복제). 바뀔 때만 보낸다 — 블렌드 진행은 모두가 서버 월드 시각으로 같은 식으로 계산한다.
 * Previous → Type 으로 TransitionStartServerSeconds 부터 TransitionSeconds 동안 섞인다.
 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimRegionWeather
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Weather")
	FName RegionId;

	UPROPERTY(BlueprintReadOnly, Category = "Weather")
	EGrimWeatherType Type = EGrimWeatherType::Clear;

	UPROPERTY(BlueprintReadOnly, Category = "Weather")
	EGrimWeatherType Previous = EGrimWeatherType::Clear;

	UPROPERTY(BlueprintReadOnly, Category = "Weather")
	double TransitionStartServerSeconds = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Weather")
	float TransitionSeconds = 0.f;
};

/** 날씨 종류별 섞임 비율(합 1). 지역 블렌드 · 시간 블렌드의 결과. */
struct GRIMOIRE_API FGrimWeatherMix
{
	float Weights[GrimWeatherTypeCount] = {};

	static FGrimWeatherMix Pure(EGrimWeatherType Type);

	float Get(EGrimWeatherType Type) const { return Weights[static_cast<int32>(Type)]; }
	float& operator[](EGrimWeatherType Type) { return Weights[static_cast<int32>(Type)]; }
	float Sum() const;

	/** 가장 큰 칸(같으면 앞의 것) */
	EGrimWeatherType GetDominant() const;

	/** this += Other × Scale */
	void AddScaled(const FGrimWeatherMix& Other, float Scale);
};

/** 실수 더위 · 추위(블렌드 중간값). 반올림해서 정수 등급이 된다. */
struct GRIMOIRE_API FGrimExposureValue
{
	float Heat = 0.f;
	float Cold = 0.f;
};

/** 날씨 종류 하나의 공통 수치 · 연출 (DefaultGame.ini [/Script/Grimoire.GrimWeatherSettings]). 지역마다 다른 값은 기후 표에 있다. */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimWeatherProfile
{
	GENERATED_BODY()

	/** 지역 기온 곡선에 더하는 값(섭씨) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather")
	float TemperatureOffset = 0.f;

	/** 비 양(0~1). 설정의 RainStimulusMinAmount 이상이면 야외 사물 · 폰에 물 자극 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather", meta = (ClampMin = "0", ClampMax = "1"))
	float RainAmount = 0.f;

	/** 눈 양(0~1). 연출만 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather", meta = (ClampMin = "0", ClampMax = "1"))
	float SnowAmount = 0.f;

	/** 모래 양(0~1). 연출만 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather", meta = (ClampMin = "0", ClampMax = "1"))
	float DustAmount = 0.f;

	/** 시야 거리(cm). AI 감지 · UI 표식 거리 쪽이 읽는다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather", meta = (ClampMin = "0"))
	float VisibilityDistance = 100000.f;

	/** 소프트 락온 사거리 배율(조준 쪽이 읽는다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weather", meta = (ClampMin = "0", ClampMax = "1"))
	float SoftLockRangeScale = 1.f;

	/** 연출: 해 · 달 세기 배율, 하늘빛 배율, 높이 안개 밀도(더함) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (ClampMin = "0"))
	float SunIntensityScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (ClampMin = "0"))
	float SkyLightScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (ClampMin = "0"))
	float FogDensityAdd = 0.f;
};
