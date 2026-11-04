#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogGrimEnvironment, Log, All);

/**
 * 시간 · 하늘 · 기온의 순수 계산. 월드 없이 테스트한다(Grimoire.Environment.*).
 * 시각은 하루 비율(0 = 자정, 0.5 = 정오), 누적 일수 TotalDays 의 소수부다.
 */
struct GRIMOIRE_API FGrimTimeOfDayMath
{
	/** 스냅숏(서버 시각 SnapshotServerSeconds 에 TotalDays)에서 지금 서버 시각까지 외삽. 시간은 거꾸로 가지 않는다. */
	static double ExtrapolateTotalDays(double SnapshotTotalDays, double SnapshotServerSeconds, double NowServerSeconds, double DayLengthSeconds);

	static float GetTimeOfDay(double TotalDays);
	static int32 GetDayIndex(double TotalDays);

	/** 해 뜨는 시각 · 지는 시각. 낮은 정오를 가운데로 DayRatio 만큼. */
	static float GetDawn(float DayRatio);
	static float GetDusk(float DayRatio);
	static bool IsNight(float TimeOfDay, float DayRatio);

	/** 해의 궤도 각(도): 해돋이 0 → 정오 90 → 해넘이 180 → 자정 270. 낮과 밤은 각자 반 바퀴. */
	static float GetSunAngleDegrees(float TimeOfDay, float DayRatio);

	/** 시간대 기온: Coldest 에서 Min, Warmest 에서 Max, 사이는 반 코사인. */
	static float GetAmbientTemperature(float TimeOfDay, float MinTemperature, float MaxTemperature, float ColdestTimeOfDay, float WarmestTimeOfDay);
};
