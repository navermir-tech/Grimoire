#include "Misc/AutomationTest.h"
#include "Environment/GrimClimateTypes.h"
#include "Environment/GrimWeatherMath.h"
#include "Environment/GrimWeatherSettings.h"
#include "Environment/GrimWeatherSubsystem.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace GrimWeatherTestsLocal
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	/** 초원 비슷한 시험 행: 맑음에서 맑음 2 · 흐림 1 · 눈 5(허용 안 함) */
	FGrimClimateRow MakeTestRow()
	{
		FGrimClimateRow Row;
		Row.FromClear.Clear = 2.f;
		Row.FromClear.Cloudy = 1.f;
		Row.FromClear.Snow = 5.f;
		Row.FromCloudy.Rain = 1.f;
		Row.NightCold = 1;
		Row.RainCold = 1;
		Row.SnowCold = 2;
		Row.SandstormHeat = 1;
		return Row;
	}

	FGrimClimateRow FindRow(const TArray<FName>& Ids, const TArray<FGrimClimateRow>& Rows, const TCHAR* Id)
	{
		const int32 Index = Ids.IndexOfByKey(FName(Id));
		return Rows.IsValidIndex(Index) ? Rows[Index] : FGrimClimateRow();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimWeatherTransitionTest, "Grimoire.Environment.Weather.Transition", GrimWeatherTestsLocal::TestFlags)
bool FGrimWeatherTransitionTest::RunTest(const FString& Parameters)
{
	using namespace GrimWeatherTestsLocal;
	FGrimClimateRow Row = MakeTestRow();

	// 허용 안 된 눈은 빼고 맑음 2 : 흐림 1 → [0, 2/3) 맑음, [2/3, 1) 흐림.
	TestEqual(TEXT("작은 난수 → 맑음"), FGrimWeatherMath::PickNextWeather(Row, EGrimWeatherType::Clear, 0.f), EGrimWeatherType::Clear);
	TestEqual(TEXT("0.6 → 맑음"), FGrimWeatherMath::PickNextWeather(Row, EGrimWeatherType::Clear, 0.6f), EGrimWeatherType::Clear);
	TestEqual(TEXT("0.7 → 흐림"), FGrimWeatherMath::PickNextWeather(Row, EGrimWeatherType::Clear, 0.7f), EGrimWeatherType::Cloudy);
	TestEqual(TEXT("1 → 마지막 양수 칸(흐림)"), FGrimWeatherMath::PickNextWeather(Row, EGrimWeatherType::Clear, 1.f), EGrimWeatherType::Cloudy);

	// 전이표 행이 모두 0 이면 그대로 머문다.
	TestEqual(TEXT("빈 행은 머문다"), FGrimWeatherMath::PickNextWeather(Row, EGrimWeatherType::Fog, 0.5f), EGrimWeatherType::Fog);

	// 눈을 허용하면 눈이 고를 수 있게 된다(맑음 2 · 흐림 1 · 눈 5 → 0.5 는 눈).
	Row.bAllowSnow = true;
	TestEqual(TEXT("허용하면 눈"), FGrimWeatherMath::PickNextWeather(Row, EGrimWeatherType::Clear, 0.5f), EGrimWeatherType::Snow);

	// 강수가 눈인 지역: 비 → 눈. 눈을 허용하지 않으면 비 그대로.
	Row.Precipitation = EGrimPrecipitation::Snow;
	TestEqual(TEXT("눈 지역의 비 → 눈"), FGrimWeatherMath::PickNextWeather(Row, EGrimWeatherType::Cloudy, 0.5f), EGrimWeatherType::Snow);
	TestEqual(TEXT("눈 지역의 뇌우 → 눈"), FGrimWeatherMath::ApplyPrecipitation(Row, EGrimWeatherType::Thunderstorm), EGrimWeatherType::Snow);
	Row.bAllowSnow = false;
	TestEqual(TEXT("눈 불허면 비 그대로"), FGrimWeatherMath::ApplyPrecipitation(Row, EGrimWeatherType::Rain), EGrimWeatherType::Rain);

	// 허용되지 않는 날씨를 정하면 가까운 것으로.
	TestEqual(TEXT("눈 불허 → 비"), FGrimWeatherMath::ClampToAllowed(Row, EGrimWeatherType::Snow), EGrimWeatherType::Rain);
	TestEqual(TEXT("모래폭풍 불허 → 흐림"), FGrimWeatherMath::ClampToAllowed(Row, EGrimWeatherType::Sandstorm), EGrimWeatherType::Cloudy);

	// 마르코프 빈도: 맑음 2 : 흐림 1 을 많이 굴리면 비율이 2/3 근처.
	Row = MakeTestRow();
	FRandomStream Stream(1234);
	int32 ClearCount = 0;
	const int32 Trials = 6000;
	for (int32 i = 0; i < Trials; ++i)
	{
		ClearCount += FGrimWeatherMath::PickNextWeather(Row, EGrimWeatherType::Clear, Stream.FRand()) == EGrimWeatherType::Clear ? 1 : 0;
	}
	TestEqual(TEXT("빈도 2/3"), static_cast<float>(ClearCount) / Trials, 2.f / 3.f, 0.03f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimWeatherBlendTest, "Grimoire.Environment.Weather.Blend", GrimWeatherTestsLocal::TestFlags)
bool FGrimWeatherBlendTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("시작 전 0"), FGrimWeatherMath::GetTransitionAlpha(90.0, 100.0, 150.f), 0.f, 1e-5f);
	TestEqual(TEXT("시작 0"), FGrimWeatherMath::GetTransitionAlpha(100.0, 100.0, 150.f), 0.f, 1e-5f);
	TestEqual(TEXT("가운데 0.5"), FGrimWeatherMath::GetTransitionAlpha(175.0, 100.0, 150.f), 0.5f, 1e-4f);
	TestEqual(TEXT("끝 1"), FGrimWeatherMath::GetTransitionAlpha(250.0, 100.0, 150.f), 1.f, 1e-5f);
	TestEqual(TEXT("길이 0 은 바로 1"), FGrimWeatherMath::GetTransitionAlpha(100.0, 100.0, 0.f), 1.f, 1e-5f);
	TestTrue(TEXT("스무스스텝: 1/4 지점은 선형보다 작다"), FGrimWeatherMath::GetTransitionAlpha(137.5, 100.0, 150.f) < 0.25f);

	FGrimRegionWeather State;
	State.Previous = EGrimWeatherType::Clear;
	State.Type = EGrimWeatherType::Rain;
	State.TransitionStartServerSeconds = 100.0;
	State.TransitionSeconds = 150.f;

	const FGrimWeatherMix Mid = FGrimWeatherMath::GetRegionMix(State, 175.0);
	TestEqual(TEXT("가운데 맑음 0.5"), Mid.Get(EGrimWeatherType::Clear), 0.5f, 1e-4f);
	TestEqual(TEXT("가운데 비 0.5"), Mid.Get(EGrimWeatherType::Rain), 0.5f, 1e-4f);
	TestEqual(TEXT("합 1"), Mid.Sum(), 1.f, 1e-5f);

	const FGrimWeatherMix Done = FGrimWeatherMath::GetRegionMix(State, 1000.0);
	TestEqual(TEXT("끝나면 비만"), Done.Get(EGrimWeatherType::Rain), 1.f, 1e-5f);
	TestEqual(TEXT("가장 큰 날씨"), Done.GetDominant(), EGrimWeatherType::Rain);

	// 같은 날씨로의 전이는 섞여도 그 날씨 하나.
	State.Previous = EGrimWeatherType::Fog;
	State.Type = EGrimWeatherType::Fog;
	TestEqual(TEXT("같은 날씨"), FGrimWeatherMath::GetRegionMix(State, 120.0).Get(EGrimWeatherType::Fog), 1.f, 1e-5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimClimateRegionBlendTest, "Grimoire.Environment.Climate.RegionBlend", GrimWeatherTestsLocal::TestFlags)
bool FGrimClimateRegionBlendTest::RunTest(const FString& Parameters)
{
	const FVector Extent(1000.f, 1000.f, 500.f);
	TestEqual(TEXT("가운데 1"), FGrimWeatherMath::GetBoxBlendWeight(FVector::ZeroVector, Extent, 400.f), 1.f, 1e-5f);
	TestEqual(TEXT("바깥 0"), FGrimWeatherMath::GetBoxBlendWeight(FVector(1200.f, 0.f, 0.f), Extent, 400.f), 0.f, 1e-5f);
	TestEqual(TEXT("경계 0"), FGrimWeatherMath::GetBoxBlendWeight(FVector(1000.f, 0.f, 0.f), Extent, 400.f), 0.f, 1e-5f);
	TestEqual(TEXT("경계에서 200 안쪽 0.5"), FGrimWeatherMath::GetBoxBlendWeight(FVector(800.f, 0.f, 0.f), Extent, 400.f), 0.5f, 1e-5f);
	TestEqual(TEXT("가장 가까운 면 기준(Z)"), FGrimWeatherMath::GetBoxBlendWeight(FVector(0.f, 0.f, -400.f), Extent, 400.f), 0.25f, 1e-5f);
	TestEqual(TEXT("블렌드 0 이면 안은 1"), FGrimWeatherMath::GetBoxBlendWeight(FVector(990.f, 0.f, 0.f), Extent, 0.f), 1.f, 1e-5f);

	// 볼륨 하나 0.25 → 기본 지역 0.75.
	TArray<float> One = { 0.25f };
	TestEqual(TEXT("기본 지역 나머지"), FGrimWeatherMath::NormalizeRegionWeights(One), 0.75f, 1e-5f);

	// 겹친 두 볼륨 1 + 0.5 → 합 1 로 나눔(2/3 · 1/3), 기본 0.
	TArray<float> Two = { 1.f, 0.5f };
	TestEqual(TEXT("겹치면 기본 0"), FGrimWeatherMath::NormalizeRegionWeights(Two), 0.f, 1e-5f);
	TestEqual(TEXT("첫째 2/3"), Two[0], 2.f / 3.f, 1e-5f);
	TestEqual(TEXT("둘째 1/3"), Two[1], 1.f / 3.f, 1e-5f);

	TArray<float> None;
	TestEqual(TEXT("볼륨 없음 → 기본 1"), FGrimWeatherMath::NormalizeRegionWeights(None), 1.f, 1e-5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimClimateExposureTest, "Grimoire.Environment.Climate.Exposure", GrimWeatherTestsLocal::TestFlags)
bool FGrimClimateExposureTest::RunTest(const FString& Parameters)
{
	using namespace GrimWeatherTestsLocal;
	const FGrimClimateRow Row = MakeTestRow();

	const FGrimExposureValue Night = FGrimWeatherMath::GetRegionExposure(Row, true);
	const FGrimExposureValue Day = FGrimWeatherMath::GetRegionExposure(Row, false);
	TestEqual(TEXT("밤 추위 1"), Night.Cold, 1.f);
	TestEqual(TEXT("낮 0"), Day.Cold + Day.Heat, 0.f);

	// 날씨 가산: 비 · 뇌우 추위, 눈 추위, 모래폭풍 더위 — 섞임 비율만큼.
	TestEqual(TEXT("비 추위 +1"), FGrimWeatherMath::GetWeatherExposure(Row, FGrimWeatherMix::Pure(EGrimWeatherType::Rain), false).Cold, 1.f, 1e-5f);
	TestEqual(TEXT("뇌우 추위 +1"), FGrimWeatherMath::GetWeatherExposure(Row, FGrimWeatherMix::Pure(EGrimWeatherType::Thunderstorm), false).Cold, 1.f, 1e-5f);
	TestEqual(TEXT("눈 추위 +2"), FGrimWeatherMath::GetWeatherExposure(Row, FGrimWeatherMix::Pure(EGrimWeatherType::Snow), false).Cold, 2.f, 1e-5f);
	TestEqual(TEXT("모래폭풍 더위 +1"), FGrimWeatherMath::GetWeatherExposure(Row, FGrimWeatherMix::Pure(EGrimWeatherType::Sandstorm), false).Heat, 1.f, 1e-5f);
	TestEqual(TEXT("맑은 한낮 — 이 지역은 0"), FGrimWeatherMath::GetWeatherExposure(Row, FGrimWeatherMix::Pure(EGrimWeatherType::Clear), true).Heat, 0.f, 1e-5f);

	FGrimClimateRow Desert = Row;
	Desert.ClearNoonHeat = 1;
	TestEqual(TEXT("사막류 맑은 한낮 +1"), FGrimWeatherMath::GetWeatherExposure(Desert, FGrimWeatherMix::Pure(EGrimWeatherType::Clear), true).Heat, 1.f, 1e-5f);
	TestEqual(TEXT("한낮이 아니면 0"), FGrimWeatherMath::GetWeatherExposure(Desert, FGrimWeatherMix::Pure(EGrimWeatherType::Clear), false).Heat, 0.f, 1e-5f);

	FGrimWeatherMix Half;
	Half[EGrimWeatherType::Clear] = 0.5f;
	Half[EGrimWeatherType::Snow] = 0.5f;
	TestEqual(TEXT("반쯤 눈 → 추위 1"), FGrimWeatherMath::GetWeatherExposure(Row, Half, false).Cold, 1.f, 1e-5f);

	// 반올림 · 서로 깎기.
	TestEqual(TEXT("0.49 → 0"), FGrimWeatherMath::RoundGrade(0.49f), 0);
	TestEqual(TEXT("0.5 → 1"), FGrimWeatherMath::RoundGrade(0.5f), 1);
	TestEqual(TEXT("음수 → 0"), FGrimWeatherMath::RoundGrade(-2.f), 0);
	int32 Heat = 0, Cold = 0;
	FGrimWeatherMath::NetGrades(3, 1, Heat, Cold);
	TestTrue(TEXT("더위 3 · 추위 1 → 더위 2"), Heat == 2 && Cold == 0);
	FGrimWeatherMath::NetGrades(0, 4, Heat, Cold);
	TestTrue(TEXT("추위만"), Heat == 0 && Cold == 4);

	// 한낮 판정.
	TestTrue(TEXT("13시는 한낮"), FGrimWeatherMath::IsNoon(13.f / 24.f, 11.f, 16.f));
	TestFalse(TEXT("9시는 아님"), FGrimWeatherMath::IsNoon(9.f / 24.f, 11.f, 16.f));
	TestFalse(TEXT("16시 정각은 아님"), FGrimWeatherMath::IsNoon(16.f / 24.f, 11.f, 16.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimClimateTableTest, "Grimoire.Environment.Climate.Table", GrimWeatherTestsLocal::TestFlags)
bool FGrimClimateTableTest::RunTest(const FString& Parameters)
{
	using namespace GrimWeatherTestsLocal;
	const UGrimWeatherSettings* Settings = GetDefault<UGrimWeatherSettings>();
	const FString FilePath = FPaths::Combine(FPaths::ProjectContentDir(), Settings->ClimateTableCSV);

	TArray<FName> Ids;
	TArray<FGrimClimateRow> Rows;
	TArray<FString> Problems;
	TestTrue(TEXT("기후 표를 읽는다"), UGrimWeatherSubsystem::LoadClimateRowsFromCSV(FilePath, Ids, Rows, Problems));
	for (const FString& Problem : Problems)
	{
		AddError(FString::Printf(TEXT("기후 표 문제: %s"), *Problem));
	}
	TestTrue(TEXT("기본 지역이 표에 있다"), Ids.Contains(Settings->DefaultRegionId));

	// 스펙 「체온 모델」 예: 탑 주변 낮 0 · 밤 추위 +1, 설산 낮 추위 +2 · 밤 +4, 화산 황야 낮 더위 +3 · 밤 +1.
	const FGrimClimateRow Meadow = FindRow(Ids, Rows, TEXT("Meadow"));
	TestTrue(TEXT("초원 낮 0 · 밤 추위 1"), Meadow.DayHeat == 0 && Meadow.DayCold == 0 && Meadow.NightCold == 1);
	const FGrimClimateRow Snow = FindRow(Ids, Rows, TEXT("SnowPeak"));
	TestTrue(TEXT("설산 낮 추위 2 · 밤 4"), Snow.DayCold == 2 && Snow.NightCold == 4);
	TestTrue(TEXT("설산은 눈 허용 · 강수 눈"), Snow.bAllowSnow && Snow.Precipitation == EGrimPrecipitation::Snow);
	const FGrimClimateRow Volcano = FindRow(Ids, Rows, TEXT("VolcanicWaste"));
	TestTrue(TEXT("화산 낮 더위 3 · 밤 1"), Volcano.DayHeat == 3 && Volcano.NightHeat == 1);
	TestTrue(TEXT("화산은 모래폭풍 허용 · 맑은 한낮 +1"), Volcano.bAllowSandstorm && Volcano.ClearNoonHeat == 1);

	// 전용 날씨(Q14): 허용 안 한 지역에서는 고를 수 없어야 한다 · 모든 행에 머물 수 있는 전이가 있다.
	for (int32 i = 0; i < Rows.Num(); ++i)
	{
		const FGrimClimateRow& Row = Rows[i];
		TestTrue(*FString::Printf(TEXT("%s 맑음 전이 가중치"), *Ids[i].ToString()), Row.FromClear.Sum() > 0.f);
		TestTrue(*FString::Printf(TEXT("%s 초기 날씨 허용"), *Ids[i].ToString()), Row.IsWeatherAllowed(Row.InitialWeather));
		for (float R = 0.f; R < 1.f; R += 0.05f)
		{
			for (EGrimWeatherType From : TEnumRange<EGrimWeatherType>())
			{
				const EGrimWeatherType Next = FGrimWeatherMath::PickNextWeather(Row, From, R);
				if (!Row.IsWeatherAllowed(Next))
				{
					AddError(FString::Printf(TEXT("%s: 허용 안 된 날씨로 전이"), *Ids[i].ToString()));
				}
			}
		}
	}

	// 기온 곡선: 설산은 늘 영하, 화산은 늘 덥다, 초원 밤이 낮보다 춥다.
	TestTrue(TEXT("설산 한낮도 영하"), FGrimWeatherMath::GetRegionTemperature(Snow, 0.6f) < 0.f);
	TestTrue(TEXT("화산 새벽도 20 이상"), FGrimWeatherMath::GetRegionTemperature(Volcano, 0.2f) >= 20.f);
	TestTrue(TEXT("초원 자정 < 정오"), FGrimWeatherMath::GetRegionTemperature(Meadow, 0.f) < FGrimWeatherMath::GetRegionTemperature(Meadow, 0.5f));
	TestEqual(TEXT("초원 최저"), FGrimWeatherMath::GetRegionTemperature(Meadow, Meadow.ColdestTimeOfDay), Meadow.MinTemperature, 1e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimWeatherLightningTest, "Grimoire.Environment.Weather.Lightning", GrimWeatherTestsLocal::TestFlags)
bool FGrimWeatherLightningTest::RunTest(const FString& Parameters)
{
	const float Weights[] = { 0.f, 1.f, 0.f, 3.f };
	TestEqual(TEXT("0 → 첫 양수"), FGrimWeatherMath::PickWeightedIndex(Weights, 0.f), 1);
	TestEqual(TEXT("0.24 → 1"), FGrimWeatherMath::PickWeightedIndex(Weights, 0.24f), 1);
	TestEqual(TEXT("0.26 → 3"), FGrimWeatherMath::PickWeightedIndex(Weights, 0.26f), 3);
	TestEqual(TEXT("1 → 마지막 양수"), FGrimWeatherMath::PickWeightedIndex(Weights, 1.f), 3);
	const float Zeros[] = { 0.f, 0.f };
	TestEqual(TEXT("모두 0 → 없음"), FGrimWeatherMath::PickWeightedIndex(Zeros, 0.5f), static_cast<int32>(INDEX_NONE));

	// 점수: 높고 젖고 전도체일수록 크다.
	const float Plain = FGrimWeatherMath::GetLightningScore(0.f, 0.f, false, 1.f, 0.5f, 2.f, 3.f);
	const float High = FGrimWeatherMath::GetLightningScore(400.f, 0.f, false, 1.f, 0.5f, 2.f, 3.f);
	const float Wet = FGrimWeatherMath::GetLightningScore(0.f, 2.f, false, 1.f, 0.5f, 2.f, 3.f);
	const float Metal = FGrimWeatherMath::GetLightningScore(0.f, 0.f, true, 1.f, 0.5f, 2.f, 3.f);
	TestEqual(TEXT("기본"), Plain, 1.f, 1e-5f);
	TestEqual(TEXT("4m 높으면 +2"), High, 3.f, 1e-5f);
	TestEqual(TEXT("젖음 2 → +4"), Wet, 5.f, 1e-5f);
	TestEqual(TEXT("전도체 +3"), Metal, 4.f, 1e-5f);
	TestEqual(TEXT("낮은 곳은 빼지 않는다"), FGrimWeatherMath::GetLightningScore(-500.f, 0.f, false, 1.f, 0.5f, 2.f, 3.f), 1.f, 1e-5f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
