#pragma once

#include "CoreMinimal.h"
#include "Environment/GrimClimateTypes.h"

/**
 * 날씨 · 기후의 순수 계산. 월드 없이 테스트한다(Grimoire.Environment.Weather.* · .Climate.*).
 * 전이(마르코프) · 시간 블렌드 · 지역 블렌드 · 등급 · 기온 곡선 · 낙뢰 대상 고르기.
 */
struct GRIMOIRE_API FGrimWeatherMath
{
	/**
	 * 지금 날씨 Current 의 전이표 행에서 다음 날씨를 고른다(Random01 ∈ [0, 1)).
	 * 지역이 허용하지 않는 날씨(Snow · Sandstorm)는 가중치를 0 으로 본다. 가중치 합이 0 이면 그대로.
	 * 고른 뒤 ApplyPrecipitation 을 거친다(눈 지역의 비 → 눈).
	 */
	static EGrimWeatherType PickNextWeather(const FGrimClimateRow& Row, EGrimWeatherType Current, float Random01);

	/** 강수가 Snow 이고 Snow 를 허용한 지역이면 Rain · Thunderstorm → Snow. 그 밖에는 그대로. */
	static EGrimWeatherType ApplyPrecipitation(const FGrimClimateRow& Row, EGrimWeatherType Type);

	/** 지역에서 허용되지 않는 날씨를 가까운 허용 날씨로(Snow → Rain, Sandstorm → Cloudy). 콘솔 · 초기값 검사용. */
	static EGrimWeatherType ClampToAllowed(const FGrimClimateRow& Row, EGrimWeatherType Type);

	/** 블렌드 진행 0~1(스무스스텝). Duration 0 이하면 바로 1. */
	static float GetTransitionAlpha(double NowSeconds, double StartSeconds, float DurationSeconds);

	/** 지역 상태의 지금 섞임: Previous × (1 − α) + Type × α. */
	static FGrimWeatherMix GetRegionMix(const FGrimRegionWeather& State, double NowServerSeconds);

	/**
	 * 상자 볼륨의 지역 가중치: 바깥 0, 경계에서 안쪽으로 BlendDistance 들어가면 1(사이는 선형).
	 * LocalPoint 는 상자 중심 기준(회전을 푼) 좌표, Extent 는 반 크기.
	 */
	static float GetBoxBlendWeight(const FVector& LocalPoint, const FVector& Extent, float BlendDistance);

	/**
	 * 볼륨 가중치들(각 0~1)을 정리하고 기본 지역 가중치를 돌려준다.
	 * 합이 1 을 넘으면 합 1 이 되게 나누고 기본 지역은 0, 아니면 기본 지역 = 1 − 합.
	 */
	static float NormalizeRegionWeights(TArrayView<float> InOutVolumeWeights);

	/** 지역 · 시간대 등급(밤 / 낮 칸). */
	static FGrimExposureValue GetRegionExposure(const FGrimClimateRow& Row, bool bNight);

	/** 날씨 가산: 비 · 뇌우 → RainCold, 눈 → SnowCold, 모래폭풍 → SandstormHeat, 맑은 한낮 → ClearNoonHeat(섞임 비율만큼). */
	static FGrimExposureValue GetWeatherExposure(const FGrimClimateRow& Row, const FGrimWeatherMix& Mix, bool bNoon);

	/** 실수 등급을 반올림(0.5 는 올림). 음수는 0. */
	static int32 RoundGrade(float Value);

	/** 더위와 추위를 서로 깎는다: 더위 3 · 추위 1 → 더위 2 · 추위 0. */
	static void NetGrades(int32 Heat, int32 Cold, int32& OutHeat, int32& OutCold);

	/** 지역 기온 곡선(섭씨, 날씨 몫 없음). */
	static float GetRegionTemperature(const FGrimClimateRow& Row, float TimeOfDay);

	/** 한낮(StartHour ≤ 시 < EndHour)인가. 시는 0~24. */
	static bool IsNoon(float TimeOfDay, float StartHour, float EndHour);

	/** 가중치 비례로 하나 고른다(Random01 ∈ [0, 1)). 모두 0 이하면 INDEX_NONE. */
	static int32 PickWeightedIndex(TArrayView<const float> Weights, float Random01);

	/** 낙뢰 대상 점수: 기본 + 높이(기준보다 높은 m) × 가중 + 젖음 세기 × 가중 + 전도체 가중. */
	static float GetLightningScore(float HeightAboveReferenceCm, float WetIntensity, bool bConductive,
		float BaseScore, float HeightWeightPerMeter, float WetWeight, float ConductiveWeight);
};
