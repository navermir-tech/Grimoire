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

	/** 낮 섞임(0 = 밤, 1 = 낮): 해 높이(방향의 Z = 사인 값)가 -HorizonFade 에서 +HorizonFade 로 오르는 동안 스무스스텝.
	 *  해 세기 · 하늘빛이 따른다. */
	static float GetDayBlend(float SunHeightSin, float HorizonFade);
	/** 밤 섞임(0 = 아직 해질녘, 1 = 밤): 해가 지평선(0)에서 2 × HorizonFade 아래로 내려가는 동안 스무스스텝.
	 *  달빛 세기와 밤 화면(노출 하한 · 채도)이 따른다 — 해가 지평선 위에 걸친 해넘이 · 해돋이 순간에는 0 이라 해질녘 화면이 그대로다(Q101). */
	static float GetNightBlend(float SunHeightSin, float HorizonFade);

	/** 달 방향(땅에서 달 쪽, 단위 벡터): 해의 맞은편이되 높이가 MinElevationDegrees 아래면 같은 방위에서 그 높이로 올린다.
	 *  해가 막 진 뒤 · 뜨기 전에는 맞은편 달이 지평선에 붙어 빛이 땅을 스치기만 해서, 밤보다 해질녘이 더 캄캄해졌다(Q101). */
	static FVector GetMoonDirection(const FVector& SunDirection, float MinElevationDegrees);

	/** 시간대 기온: Coldest 에서 Min, Warmest 에서 Max, 사이는 반 코사인. */
	static float GetAmbientTemperature(float TimeOfDay, float MinTemperature, float MaxTemperature, float ColdestTimeOfDay, float WarmestTimeOfDay);
};
