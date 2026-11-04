#include "Misc/AutomationTest.h"
#include "Survival/GrimSurvivalModel.h"
#include "Survival/GrimVitalAttributeSet.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr EAutomationTestFlags GrimSurvivalTestFlags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	/** 체온만 DurationSeconds 동안 1초 틱으로 진행한다. */
	float SimulateBodyTemperature(float Start, float Ambient, float Heat, float Wet, float DurationSeconds, const FGrimSurvivalTuning& Tuning)
	{
		FGrimSurvivalInputs Inputs;
		Inputs.BodyTemperature = Start;
		Inputs.AmbientTemperature = Ambient;
		Inputs.HeatWarmth = Heat;
		Inputs.WetIntensity = Wet;
		for (int32 i = 0; i < FMath::RoundToInt32(DurationSeconds); ++i)
		{
			Inputs.BodyTemperature += FGrimSurvivalModel::ComputeTick(Inputs, 1.f, Tuning).BodyTemperature;
		}
		return Inputs.BodyTemperature;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimBodyTemperatureConvergeTest, "Grimoire.Survival.BodyTemperature.Converge", GrimSurvivalTestFlags)
bool FGrimBodyTemperatureConvergeTest::RunTest(const FString& Parameters)
{
	const FGrimSurvivalTuning Tuning;

	// 쾌적 기온이면 정상 체온이 목표.
	TestEqual(TEXT("쾌적 → 정상 체온"), FGrimSurvivalModel::ComputeTargetBodyTemperature(Tuning.ComfortTemperature, 0.f, 0.f, Tuning), Tuning.NormalBodyTemperature, 1e-4f);

	// 추운 밤(6℃): 목표 쪽으로 단조롭게 내려가 충분한 시간 뒤 목표에 닿는다.
	const float NightTarget = FGrimSurvivalModel::ComputeTargetBodyTemperature(6.f, 0.f, 0.f, Tuning);
	TestTrue(TEXT("추운 밤 목표는 저체온"), NightTarget < Tuning.HypothermiaThreshold);
	const float After1 = SimulateBodyTemperature(37.f, 6.f, 0.f, 0.f, Tuning.BodyTemperatureTimeConstantSeconds, Tuning);
	const float After10 = SimulateBodyTemperature(37.f, 6.f, 0.f, 0.f, Tuning.BodyTemperatureTimeConstantSeconds * 10.f, Tuning);
	TestTrue(TEXT("시정수 뒤 내려감"), After1 < 37.f && After1 > NightTarget);
	TestEqual(TEXT("시정수 뒤 차이 약 37%"), (After1 - NightTarget) / (37.f - NightTarget), FMath::Exp(-1.f), 0.01f);
	TestEqual(TEXT("충분히 지나면 목표"), After10, NightTarget, 0.01f);

	// 틱 간격이 달라도 같은 곡선.
	const float OneStep = FGrimSurvivalModel::StepBodyTemperature(37.f, NightTarget, 10.f, Tuning);
	const float TenSteps = SimulateBodyTemperature(37.f, 6.f, 0.f, 0.f, 10.f, Tuning);
	TestEqual(TEXT("틱 간격 무관"), OneStep, TenSteps, 1e-3f);

	// 열원: 가까울수록 따뜻하고 반경 밖은 0. 모닥불 옆이면 추운 밤에도 안전 범위.
	TestEqual(TEXT("반경 밖 열 0"), FGrimSurvivalModel::ComputeHeatWarmth(1.f, Tuning.HeatSourceRadius + 1.f, Tuning), 0.f);
	TestTrue(TEXT("가까울수록 따뜻"), FGrimSurvivalModel::ComputeHeatWarmth(1.f, 100.f, Tuning) > FGrimSurvivalModel::ComputeHeatWarmth(1.f, 400.f, Tuning));
	const float Heat = FGrimSurvivalModel::ComputeHeatWarmth(1.f, 150.f, Tuning);
	const float FireTarget = FGrimSurvivalModel::ComputeTargetBodyTemperature(6.f, Heat, 0.f, Tuning);
	TestTrue(TEXT("불 옆 목표가 더 높다"), FireTarget > NightTarget);
	TestTrue(TEXT("불 옆은 저체온 아님"), FireTarget >= Tuning.HypothermiaThreshold);

	// 젖으면 더 춥다.
	const float WetTarget = FGrimSurvivalModel::ComputeTargetBodyTemperature(6.f, Heat, 1.f, Tuning);
	TestTrue(TEXT("젖으면 목표가 낮다"), WetTarget < FireTarget);
	TestEqual(TEXT("젖음 1 = 체감 −4℃"), FireTarget - WetTarget, Tuning.CoolingPerWetIntensity * Tuning.BodyTemperatureSensitivity, 1e-4f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimHungerTest, "Grimoire.Survival.Hunger", GrimSurvivalTestFlags)
bool FGrimHungerTest::RunTest(const FString& Parameters)
{
	const FGrimSurvivalTuning Tuning;

	FGrimSurvivalInputs Inputs;
	Inputs.Health = 50.f;
	Inputs.MaxHealth = 100.f;
	Inputs.Hunger = 80.f;
	Inputs.BodyTemperature = Tuning.NormalBodyTemperature;
	Inputs.AmbientTemperature = Tuning.ComfortTemperature;

	// 배고픔: 1분에 HungerDrainPerMinute 만큼 준다.
	const FGrimSurvivalDeltas Minute = FGrimSurvivalModel::ComputeTick(Inputs, 60.f, Tuning);
	TestEqual(TEXT("분당 감소"), Minute.Hunger, -Tuning.HungerDrainPerMinute, 1e-4f);

	// 배부르면 HP 자연 회복.
	const FGrimSurvivalDeltas Fed = FGrimSurvivalModel::ComputeTick(Inputs, 1.f, Tuning);
	TestEqual(TEXT("배부르면 회복"), Fed.Health, Tuning.HealthRegenPerSecond, 1e-4f);

	// 최대 HP 를 넘지 않는다.
	Inputs.Health = 99.9f;
	TestEqual(TEXT("최대치에서 멈춤"), FGrimSurvivalModel::ComputeTick(Inputs, 10.f, Tuning).Health, 0.1f, 1e-3f);

	// 배고프면(임계값 미만) 회복이 멈춘다.
	Inputs.Health = 50.f;
	Inputs.Hunger = Tuning.HungryThreshold - 1.f;
	TestEqual(TEXT("배고프면 회복 정지"), FGrimSurvivalModel::ComputeTick(Inputs, 1.f, Tuning).Health, 0.f);
	TestTrue(TEXT("Hungry 상태"), FGrimSurvivalModel::EvaluateStatus(Inputs.Hunger, Inputs.BodyTemperature, Tuning).bHungry);

	// 0 이면 굶주림: HP 감소, 배고픔은 0 아래로 가지 않는다.
	Inputs.Hunger = 0.f;
	const FGrimSurvivalDeltas Starving = FGrimSurvivalModel::ComputeTick(Inputs, 1.f, Tuning);
	TestEqual(TEXT("굶주리면 HP 감소"), Starving.Health, -Tuning.StarvingDamagePerSecond, 1e-4f);
	TestEqual(TEXT("배고픔 0 유지"), Starving.Hunger, 0.f);
	TestTrue(TEXT("Starving 상태"), FGrimSurvivalModel::EvaluateStatus(0.f, Inputs.BodyTemperature, Tuning).bStarving);

	// 가득 찬 배는 (MaxHunger / 분당 감소)분 뒤 빈다.
	Inputs.Hunger = Tuning.MaxHunger;
	Inputs.Health = 100.f;
	float Hunger = Inputs.Hunger;
	const int32 Seconds = FMath::RoundToInt32(Tuning.MaxHunger / Tuning.HungerDrainPerMinute * 60.f);
	for (int32 i = 0; i < Seconds; ++i)
	{
		Inputs.Hunger = Hunger;
		Hunger += FGrimSurvivalModel::ComputeTick(Inputs, 1.f, Tuning).Hunger;
	}
	TestEqual(TEXT("다 비는 시간"), Hunger, 0.f, 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimExposureTest, "Grimoire.Survival.BodyTemperature.Exposure", GrimSurvivalTestFlags)
bool FGrimExposureTest::RunTest(const FString& Parameters)
{
	const FGrimSurvivalTuning Tuning;

	FGrimSurvivalInputs Inputs;
	Inputs.Health = 50.f;
	Inputs.MaxHealth = 100.f;
	Inputs.Hunger = 100.f;
	Inputs.AmbientTemperature = Tuning.ComfortTemperature;

	Inputs.BodyTemperature = Tuning.HypothermiaThreshold - 0.5f;
	TestTrue(TEXT("저체온 상태"), FGrimSurvivalModel::EvaluateStatus(Inputs.Hunger, Inputs.BodyTemperature, Tuning).bHypothermia);
	TestEqual(TEXT("저체온 HP 감소"), FGrimSurvivalModel::ComputeTick(Inputs, 1.f, Tuning).Health, -Tuning.ExposureDamagePerSecond, 1e-4f);

	Inputs.BodyTemperature = Tuning.HyperthermiaThreshold + 0.5f;
	TestTrue(TEXT("과열 상태"), FGrimSurvivalModel::EvaluateStatus(Inputs.Hunger, Inputs.BodyTemperature, Tuning).bHyperthermia);

	Inputs.BodyTemperature = Tuning.NormalBodyTemperature;
	const FGrimSurvivalStatus Normal = FGrimSurvivalModel::EvaluateStatus(Inputs.Hunger, Inputs.BodyTemperature, Tuning);
	TestFalse(TEXT("정상 체온"), Normal.bHypothermia || Normal.bHyperthermia || Normal.bHungry || Normal.bStarving);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimVitalClampTest, "Grimoire.Survival.AttributeClamp", GrimSurvivalTestFlags)
bool FGrimVitalClampTest::RunTest(const FString& Parameters)
{
	const FGrimSurvivalTuning& Tuning = GetDefault<UGrimSurvivalSettings>()->Tuning;
	UGrimVitalAttributeSet* Set = NewObject<UGrimVitalAttributeSet>();
	Set->InitMaxHealth(100.f);

	float Value = 150.f;
	Set->PreAttributeBaseChange(UGrimVitalAttributeSet::GetHealthAttribute(), Value);
	TestEqual(TEXT("HP ≤ MaxHealth"), Value, 100.f);

	Value = -5.f;
	Set->PreAttributeBaseChange(UGrimVitalAttributeSet::GetHungerAttribute(), Value);
	TestEqual(TEXT("배고픔 ≥ 0"), Value, 0.f);

	Value = 1000.f;
	Set->PreAttributeBaseChange(UGrimVitalAttributeSet::GetBodyTemperatureAttribute(), Value);
	TestEqual(TEXT("체온 ≤ 최대"), Value, Tuning.MaxBodyTemperature);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
