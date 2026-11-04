#include "Misc/AutomationTest.h"
#include "Environment/GrimEnvironmentSettings.h"
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

#endif // WITH_DEV_AUTOMATION_TESTS
