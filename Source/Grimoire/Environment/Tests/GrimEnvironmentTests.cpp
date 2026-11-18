#include "Misc/AutomationTest.h"
#include "Environment/GrimEnvironmentSettings.h"
#include "Environment/GrimSkyController.h"
#include "Environment/GrimTimeOfDayMath.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr EAutomationTestFlags GrimEnvironmentTestFlags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimTimeExtrapolationTest, "Grimoire.Environment.TimeOfDay.Extrapolation", GrimEnvironmentTestFlags)
bool FGrimTimeExtrapolationTest::RunTest(const FString& Parameters)
{
	const double DayLength = 48.0 * 60.0;

	// 서버 100초에 0.25(06:00)였으면, 하루의 1/4(720초) 뒤에는 정오.
	const double Noon = FGrimTimeOfDayMath::ExtrapolateTotalDays(0.25, 100.0, 100.0 + DayLength / 4.0, DayLength);
	TestEqual(TEXT("하루 1/4 뒤 정오"), Noon, 0.5, 1e-9);

	// 날짜를 넘기면 DayIndex 가 오르고 시각은 소수부.
	const double NextDay = FGrimTimeOfDayMath::ExtrapolateTotalDays(0.9, 0.0, DayLength * 0.2, DayLength);
	TestEqual(TEXT("다음 날"), FGrimTimeOfDayMath::GetDayIndex(NextDay), 1);
	TestEqual(TEXT("다음 날 시각"), FGrimTimeOfDayMath::GetTimeOfDay(NextDay), 0.1f, 1e-4f);

	// 클라이언트 시계가 스냅숏보다 늦어도 시간은 거꾸로 가지 않는다.
	TestEqual(TEXT("과거 시각은 스냅숏 그대로"), FGrimTimeOfDayMath::ExtrapolateTotalDays(0.3, 50.0, 49.0, DayLength), 0.3, 1e-9);

	// 같은 스냅숏에서 외삽한 두 시점은 경과 시간에 비례한다(클라이언트 · 서버가 같은 식을 쓴다).
	const double A = FGrimTimeOfDayMath::ExtrapolateTotalDays(1.0, 10.0, 70.0, DayLength);
	const double B = FGrimTimeOfDayMath::ExtrapolateTotalDays(1.0, 10.0, 130.0, DayLength);
	TestEqual(TEXT("1분 차이"), (B - A) * DayLength, 60.0, 1e-6);

	// 기본 설정: 48분, 낮 비율 0.625.
	const UGrimEnvironmentSettings* Defaults = GetDefault<UGrimEnvironmentSettings>();
	TestTrue(TEXT("하루 길이 설정"), Defaults->DayLengthMinutes > 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimTimeNightTest, "Grimoire.Environment.TimeOfDay.Night", GrimEnvironmentTestFlags)
bool FGrimTimeNightTest::RunTest(const FString& Parameters)
{
	const float Ratio = 0.625f;
	TestEqual(TEXT("해돋이 04:30"), FGrimTimeOfDayMath::GetDawn(Ratio), 0.1875f, 1e-5f);
	TestEqual(TEXT("해넘이 19:30"), FGrimTimeOfDayMath::GetDusk(Ratio), 0.8125f, 1e-5f);

	TestTrue(TEXT("자정은 밤"), FGrimTimeOfDayMath::IsNight(0.f, Ratio));
	TestFalse(TEXT("정오는 낮"), FGrimTimeOfDayMath::IsNight(0.5f, Ratio));
	TestFalse(TEXT("해돋이 순간은 낮"), FGrimTimeOfDayMath::IsNight(0.1875f, Ratio));
	TestTrue(TEXT("해넘이 순간은 밤"), FGrimTimeOfDayMath::IsNight(0.8125f, Ratio));

	TestEqual(TEXT("해돋이 각 0"), FGrimTimeOfDayMath::GetSunAngleDegrees(0.1875f, Ratio), 0.f, 1e-3f);
	TestEqual(TEXT("정오 각 90"), FGrimTimeOfDayMath::GetSunAngleDegrees(0.5f, Ratio), 90.f, 1e-3f);
	TestEqual(TEXT("해넘이 각 180"), FGrimTimeOfDayMath::GetSunAngleDegrees(0.8125f, Ratio), 180.f, 1e-3f);
	TestEqual(TEXT("자정 각 270"), FGrimTimeOfDayMath::GetSunAngleDegrees(0.f, Ratio), 270.f, 1e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimAmbientTemperatureTest, "Grimoire.Environment.Temperature.TimeCurve", GrimEnvironmentTestFlags)
bool FGrimAmbientTemperatureTest::RunTest(const FString& Parameters)
{
	const float Min = 6.f, Max = 22.f, Coldest = 0.2f, Warmest = 0.6f;
	auto Temp = [&](float T) { return FGrimTimeOfDayMath::GetAmbientTemperature(T, Min, Max, Coldest, Warmest); };

	TestEqual(TEXT("가장 추운 시각 = 최저"), Temp(Coldest), Min, 1e-3f);
	TestEqual(TEXT("가장 더운 시각 = 최고"), Temp(Warmest), Max, 1e-3f);
	TestTrue(TEXT("자정이 정오보다 춥다"), Temp(0.f) < Temp(0.5f));
	TestTrue(TEXT("밤 22시가 낮 14시보다 춥다"), Temp(22.f / 24.f) < Temp(14.f / 24.f));

	// 하루 내내 범위 안, 자정을 넘을 때 끊기지 않는다.
	for (int32 i = 0; i <= 96; ++i)
	{
		const float T = Temp(i / 96.f);
		TestTrue(TEXT("범위 안"), T >= Min - 1e-3f && T <= Max + 1e-3f);
	}
	TestEqual(TEXT("자정 연속"), Temp(0.f), Temp(0.99999f), 1e-2f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimNightBlendTest, "Grimoire.Environment.Sky.NightBlend", GrimEnvironmentTestFlags)
bool FGrimNightBlendTest::RunTest(const FString& Parameters)
{
	// 밤 화면(Q101 · BUG-008)은 밤 섞임만큼 걸린다: 낮에는 정확히 0(낮 화면을 건드리지 않는다), 밤에는 1, 사이는 끊김 없이.
	const AGrimSkyController* Sky = GetDefault<AGrimSkyController>();
	const UGrimEnvironmentSettings* Settings = GetDefault<UGrimEnvironmentSettings>();
	const float Fade = Sky->HorizonFade;

	TestEqual(TEXT("한낮은 0"), FGrimTimeOfDayMath::GetNightBlend(0.9f, Fade), 0.f);
	TestEqual(TEXT("해넘이 순간(해가 지평선)도 0 — 해질녘 화면 그대로"), FGrimTimeOfDayMath::GetNightBlend(0.f, Fade), 0.f);
	TestEqual(TEXT("한밤은 1"), FGrimTimeOfDayMath::GetNightBlend(-0.9f, Fade), 1.f);
	TestEqual(TEXT("해가 2 × HorizonFade 아래면 1"), FGrimTimeOfDayMath::GetNightBlend(-2.f * Fade, Fade), 1.f);
	TestEqual(TEXT("그 가운데는 절반"), FGrimTimeOfDayMath::GetNightBlend(-Fade, Fade), 0.5f, 1e-4f);
	TestEqual(TEXT("낮 섞임: 지평선은 절반"), FGrimTimeOfDayMath::GetDayBlend(0.f, Fade), 0.5f, 1e-4f);
	TestTrue(TEXT("해와 밤이 같이 다 켜지지 않는다"), FGrimTimeOfDayMath::GetDayBlend(-0.05f, Fade) + FGrimTimeOfDayMath::GetNightBlend(-0.05f, Fade) <= 1.f + 1e-4f);

	// 하루를 0.6초(48분 하루) 간격으로 걸어 본다 — 해 높이는 하늘 컨트롤러와 같은 식(사인 × 궤도 기울기의 코사인).
	const float TiltCos = FMath::Cos(FMath::DegreesToRadians(Sky->OrbitTiltDegrees));
	auto BlendAt = [&](float TimeOfDay)
	{
		const float Angle = FMath::DegreesToRadians(FGrimTimeOfDayMath::GetSunAngleDegrees(TimeOfDay, Settings->DayRatio));
		return FGrimTimeOfDayMath::GetNightBlend(FMath::Sin(Angle) * TiltCos, Fade);
	};
	const int32 Steps = 4800;
	float MaxStep = 0.f;
	for (int32 i = 0; i < Steps; ++i)
	{
		const float A = BlendAt(static_cast<float>(i) / Steps);
		const float B = BlendAt(static_cast<float>(i + 1) / Steps);
		TestTrue(TEXT("범위 0~1"), A >= 0.f && A <= 1.f);
		MaxStep = FMath::Max(MaxStep, FMath::Abs(B - A));
	}
	TestTrue(TEXT("해질녘 · 새벽에 튀지 않는다(0.6초에 5% 미만)"), MaxStep < 0.05f);
	TestEqual(TEXT("정오"), BlendAt(0.5f), 0.f);
	TestEqual(TEXT("자정"), BlendAt(0.f), 1.f);

	// 달빛 최저 높이: 해가 막 졌을 때(맞은편 달이 지평선) 달은 같은 방위에서 최저 높이로, 높이 뜬 달은 그대로 맞은편.
	const FVector SunJustSet = FVector(1.0, 0.0, -0.05).GetSafeNormal();
	const FVector LowMoon = FGrimTimeOfDayMath::GetMoonDirection(SunJustSet, 30.f);
	TestEqual(TEXT("낮은 달은 30도로 올린다"), LowMoon.Z, 0.5, 1e-4);
	TestTrue(TEXT("방위는 해의 맞은편"), LowMoon.X < -0.8 && FMath::Abs(LowMoon.Y) < 1e-4);
	TestEqual(TEXT("단위 벡터"), LowMoon.Size(), 1.0, 1e-4);
	const FVector SunDeep = FVector(0.3, 0.0, -0.95).GetSafeNormal();
	TestTrue(TEXT("높이 뜬 달은 그대로 맞은편"), FGrimTimeOfDayMath::GetMoonDirection(SunDeep, 30.f).Equals(-SunDeep, 1e-4));
	TestTrue(TEXT("0도면 늘 맞은편"), FGrimTimeOfDayMath::GetMoonDirection(SunJustSet, 0.f).Equals(-SunJustSet, 1e-4));
	TestTrue(TEXT("기본 최저 높이"), Sky->MoonMinElevationDegrees > 0.f && Sky->MoonMinElevationDegrees < 80.f);

	// 값: 노출 하한은 엔진 기본(-10 = 끝없이 끌어올림)보다 높아야 밤이 낮처럼 보이지 않는다. 채도는 0~1.
	TestTrue(TEXT("밤 노출 하한이 묶여 있다"), Settings->NightExposureMinEV100 > -10.f && Settings->NightExposureMinEV100 < 10.f);
	TestTrue(TEXT("해넘이 노출 하한 → 밤 노출 하한으로 오른다(어스름이 밤보다 밝게 끌어올려지지 않음)"),
		Settings->TwilightExposureMinEV100 > -10.f && Settings->TwilightExposureMinEV100 <= Settings->NightExposureMinEV100);
	TestTrue(TEXT("밤 채도"), Settings->NightSaturation > 0.f && Settings->NightSaturation <= 1.f);
	TestTrue(TEXT("밤 어두운 영역 채도가 전체보다 낮다"), Settings->NightShadowSaturation >= 0.f && Settings->NightShadowSaturation <= Settings->NightSaturation);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
