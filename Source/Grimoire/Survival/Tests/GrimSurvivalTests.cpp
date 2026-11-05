#include "Misc/AutomationTest.h"
#include "Survival/GrimSurvivalModel.h"
#include "Survival/GrimSurvivalTags.h"
#include "Survival/GrimSurvivalTickEffect.h"
#include "Survival/GrimVitalAttributeSet.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS

// 유니티 빌드에서 다른 테스트 파일의 도우미와 부딪히지 않게 이름을 붙인다.
namespace GrimSurvivalTestsPrivate
{
	constexpr EAutomationTestFlags PureFlags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;
	constexpr EAutomationTestFlags WorldFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	FGrimClimateExposure MakeExposure(int32 Heat, int32 Cold)
	{
		FGrimClimateExposure Exposure;
		Exposure.Heat = Heat;
		Exposure.Cold = Cold;
		return Exposure;
	}

	/** 쾌적 · 배부름 · 쌩쌩한 입력. */
	FGrimSurvivalInputs MakeComfortInputs(const FGrimSurvivalRules& Rules)
	{
		FGrimSurvivalInputs Inputs;
		Inputs.Health = 50.f;
		Inputs.MaxHealth = 100.f;
		Inputs.Hunger = 80.f;
		Inputs.BodyTemperature = Rules.Vital.NormalBodyTemperature;
		Inputs.Fatigue = 0.f;
		Inputs.Mana = 50.f;
		Inputs.MaxMana = 100.f;
		return Inputs;
	}

	/** 체온만 DurationSeconds 동안 1초 틱으로 진행한다. */
	float SimulateBodyTemperature(float Start, const FGrimClimateExposure& Exposure, float DurationSeconds, const FGrimSurvivalRules& Rules)
	{
		FGrimSurvivalInputs Inputs = MakeComfortInputs(Rules);
		Inputs.BodyTemperature = Start;
		Inputs.Exposure = Exposure;
		for (int32 i = 0; i < FMath::RoundToInt32(DurationSeconds); ++i)
		{
			Inputs.BodyTemperature += FGrimSurvivalModel::ComputeTick(Inputs, 1.f, Rules).BodyTemperature;
		}
		return Inputs.BodyTemperature;
	}

	/** 테스트용 빈 월드 하나. 소멸자에서 정리한다. */
	struct FTestWorld
	{
		UWorld* World = nullptr;

		FTestWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false);
			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
		}

		~FTestWorld()
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}

		UAbilitySystemComponent* SpawnPlayer()
		{
			AActor* Actor = World->SpawnActor<AActor>();
			UAbilitySystemComponent* ASC = NewObject<UAbilitySystemComponent>(Actor, TEXT("ASC"));
			ASC->RegisterComponent();
			ASC->AddAttributeSetSubobject(NewObject<UGrimMagicAttributeSet>(Actor));
			ASC->AddAttributeSetSubobject(NewObject<UGrimVitalAttributeSet>(Actor));
			ASC->InitAbilityActorInfo(Actor, Actor);
			return ASC;
		}
	};
}

// 등급 = 날씨 몫 + 개인 몫, 모자람 = 등급 − 저항 (스펙 「체온 모델」 예시 포함).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimSurvivalClimateExposureTest, "Grimoire.Survival.Climate.Exposure", GrimSurvivalTestsPrivate::PureFlags)
bool FGrimSurvivalClimateExposureTest::RunTest(const FString& Parameters)
{
	using namespace GrimSurvivalTestsPrivate;
	const FGrimClimateTuning Climate;

	FGrimPersonalClimate Personal;
	FGrimClimateExposure Own = FGrimSurvivalModel::ComputePersonalExposure(Personal, Climate);
	TestTrue(TEXT("개인 몫 없음"), Own.Heat == 0 && Own.Cold == 0);

	Personal.bWet = true;
	TestEqual(TEXT("젖음 추위 +1"), FGrimSurvivalModel::ComputePersonalExposure(Personal, Climate).Cold, Climate.WetCold);
	Personal.bNearHeatSource = true;
	TestEqual(TEXT("젖음 + 열원 = 추위 −1"), FGrimSurvivalModel::ComputePersonalExposure(Personal, Climate).Cold, Climate.WetCold - Climate.NearHeatSourceCold);
	Personal = FGrimPersonalClimate();
	Personal.bBurning = true;
	TestEqual(TEXT("불붙음 더위 +2"), FGrimSurvivalModel::ComputePersonalExposure(Personal, Climate).Heat, Climate.BurningHeat);
	Personal.bSwimming = true;
	TestEqual(TEXT("물속 더위 −1"), FGrimSurvivalModel::ComputePersonalExposure(Personal, Climate).Heat, Climate.BurningHeat - Climate.SwimmingHeat);

	// 합치면 0 아래는 0: 밤 추위 2 + 모닥불 −2 = 0, 추위 1 + 모닥불 −2 = 0.
	TestEqual(TEXT("밤 + 모닥불 = 0"), FGrimSurvivalModel::CombineExposure(MakeExposure(0, 2), MakeExposure(0, -2)).Cold, 0);
	TestEqual(TEXT("음수는 0"), FGrimSurvivalModel::CombineExposure(MakeExposure(0, 1), MakeExposure(0, -2)).Cold, 0);
	TestEqual(TEXT("밤 + 젖음 = 3"), FGrimSurvivalModel::CombineExposure(MakeExposure(0, 2), MakeExposure(0, 1)).Cold, 3);

	// 스펙 예: 화산 황야 한낮 더위 +3, 저항 +3 → 0, 저항 +2 → 1.
	TestEqual(TEXT("저항 합 = 등급이면 모자람 0"), FGrimSurvivalModel::ComputeDeficit(MakeExposure(3, 0), 3.f, 0.f).Heat, 0);
	TestEqual(TEXT("맨손이면 모자람 1"), FGrimSurvivalModel::ComputeDeficit(MakeExposure(3, 0), 2.f, 0.f).Heat, 1);
	TestEqual(TEXT("저항이 넘쳐도 0"), FGrimSurvivalModel::ComputeDeficit(MakeExposure(3, 0), 5.f, 0.f).Heat, 0);
	TestEqual(TEXT("소수 저항은 다 못 막는다"), FGrimSurvivalModel::ComputeDeficit(MakeExposure(3, 0), 2.5f, 0.f).Heat, 1);
	// 털가죽 로브: 추위 +3 · 더위 −1 → 더위 1 에서 모자람 2.
	const FGrimClimateExposure Robe = FGrimSurvivalModel::ComputeDeficit(MakeExposure(1, 4), -1.f, 3.f);
	TestEqual(TEXT("음수 저항은 더 모자라게"), Robe.Heat, 2);
	TestEqual(TEXT("추위 4 − 3 = 1"), Robe.Cold, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimSurvivalClimateStatusTest, "Grimoire.Survival.Climate.Status", GrimSurvivalTestsPrivate::PureFlags)
bool FGrimSurvivalClimateStatusTest::RunTest(const FString& Parameters)
{
	using namespace GrimSurvivalTestsPrivate;
	const FGrimSurvivalRules Rules;

	auto Eval = [&Rules](int32 Heat, int32 Cold) { return FGrimSurvivalModel::EvaluateStatus(100.f, MakeExposure(Heat, Cold), 0.f, Rules); };

	const FGrimSurvivalStatus None = Eval(0, 0);
	TestFalse(TEXT("0 → 없음"), None.bChilly || None.bHypothermia || None.bWarm || None.bHyperthermia);

	const FGrimSurvivalStatus Chilly = Eval(0, 1);
	TestTrue(TEXT("1 → Chilly"), Chilly.bChilly && !Chilly.bHypothermia);

	const FGrimSurvivalStatus Hypo = Eval(0, 2);
	TestTrue(TEXT("2 → Hypothermia(Chilly 아님)"), Hypo.bHypothermia && !Hypo.bChilly && !Hypo.bHypothermiaSevere);
	TestTrue(TEXT("2 → 중간"), Hypo.IsModerateExposure() && !Hypo.IsSevereExposure());

	const FGrimSurvivalStatus Severe = Eval(0, 4);
	TestTrue(TEXT("3+ → Hypothermia + Severe"), Severe.bHypothermia && Severe.bHypothermiaSevere);

	TestTrue(TEXT("더위 1 → Warm"), Eval(1, 0).bWarm);
	TestTrue(TEXT("더위 2 → Hyperthermia"), Eval(2, 0).bHyperthermia && !Eval(2, 0).bWarm);
	TestTrue(TEXT("더위 3 → 열사병"), Eval(3, 0).bHyperthermiaSevere);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimSurvivalBodyTemperatureConvergeTest, "Grimoire.Survival.BodyTemperature.Converge", GrimSurvivalTestsPrivate::PureFlags)
bool FGrimSurvivalBodyTemperatureConvergeTest::RunTest(const FString& Parameters)
{
	using namespace GrimSurvivalTestsPrivate;
	const FGrimSurvivalRules Rules;
	const float Normal = Rules.Vital.NormalBodyTemperature;
	const float Per = Rules.Climate.BodyTemperaturePerDeficit;

	// 목표 = 37 ∓ 1.5 × 모자람.
	TestEqual(TEXT("모자람 0 → 정상"), FGrimSurvivalModel::ComputeTargetBodyTemperature(MakeExposure(0, 0), Rules), Normal, 1e-4f);
	TestEqual(TEXT("추위 2 → 34"), FGrimSurvivalModel::ComputeTargetBodyTemperature(MakeExposure(0, 2), Rules), Normal - 2.f * Per, 1e-4f);
	TestEqual(TEXT("더위 3 → 41.5"), FGrimSurvivalModel::ComputeTargetBodyTemperature(MakeExposure(3, 0), Rules), Normal + 3.f * Per, 1e-4f);
	TestEqual(TEXT("범위로 자름"), FGrimSurvivalModel::ComputeTargetBodyTemperature(MakeExposure(0, 100), Rules), Rules.Vital.MinBodyTemperature, 1e-4f);

	// 등급은 저항으로 막는다 — 같은 추위라도 저항이 있으면 목표가 정상.
	FGrimSurvivalInputs Inputs = MakeComfortInputs(Rules);
	Inputs.Exposure = MakeExposure(0, 2);
	Inputs.ColdResist = 2.f;
	TestEqual(TEXT("저항이 막으면 체온 그대로"), FGrimSurvivalModel::ComputeTick(Inputs, 10.f, Rules).BodyTemperature, 0.f, 1e-4f);

	// 추운 밤(추위 2): 목표 쪽으로 천천히(몇 분) 내려가 충분한 시간 뒤 닿는다.
	const FGrimClimateExposure Night = MakeExposure(0, 2);
	const float NightTarget = FGrimSurvivalModel::ComputeTargetBodyTemperature(Night, Rules);
	const float TimeConstant = Rules.Vital.BodyTemperatureTimeConstantSeconds;
	const float After1 = SimulateBodyTemperature(Normal, Night, TimeConstant, Rules);
	const float After10 = SimulateBodyTemperature(Normal, Night, TimeConstant * 10.f, Rules);
	TestTrue(TEXT("시정수 뒤 내려감"), After1 < Normal && After1 > NightTarget);
	TestEqual(TEXT("시정수 뒤 차이 약 37%"), (After1 - NightTarget) / (Normal - NightTarget), FMath::Exp(-1.f), 0.01f);
	TestEqual(TEXT("충분히 지나면 목표"), After10, NightTarget, 0.01f);

	// 잠깐 지나가는 것은 괜찮다: 10초면 0.3℃ 도 안 내려간다.
	TestTrue(TEXT("10초는 거의 그대로"), Normal - SimulateBodyTemperature(Normal, Night, 10.f, Rules) < 0.3f);

	// 틱 간격이 달라도 같은 곡선.
	const float OneStep = FGrimSurvivalModel::StepBodyTemperature(Normal, NightTarget, 10.f, Rules.Vital);
	TestEqual(TEXT("틱 간격 무관"), OneStep, SimulateBodyTemperature(Normal, Night, 10.f, Rules), 1e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimSurvivalExposureDamageTest, "Grimoire.Survival.BodyTemperature.Exposure", GrimSurvivalTestsPrivate::PureFlags)
bool FGrimSurvivalExposureDamageTest::RunTest(const FString& Parameters)
{
	using namespace GrimSurvivalTestsPrivate;
	const FGrimSurvivalRules Rules;
	FGrimSurvivalInputs Inputs = MakeComfortInputs(Rules);

	// 1 · 2단계는 HP 를 깎지 않는다(회복도 계속).
	Inputs.Exposure = MakeExposure(0, 2);
	TestEqual(TEXT("2단계는 회복 계속"), FGrimSurvivalModel::ComputeTick(Inputs, 1.f, Rules).Health, Rules.Vital.HealthRegenPerSecond, 1e-4f);

	// 3단계 이상이면 HP 감소, 즉사는 없다(HP 는 0 아래로 가지 않는다).
	Inputs.Exposure = MakeExposure(0, 3);
	TestEqual(TEXT("심한 저체온 HP 감소"), FGrimSurvivalModel::ComputeTick(Inputs, 1.f, Rules).Health, -Rules.Vital.ExposureDamagePerSecond, 1e-4f);
	Inputs.Exposure = MakeExposure(4, 0);
	TestEqual(TEXT("열사병 HP 감소"), FGrimSurvivalModel::ComputeTick(Inputs, 1.f, Rules).Health, -Rules.Vital.ExposureDamagePerSecond, 1e-4f);
	Inputs.Health = 0.1f;
	TestEqual(TEXT("0 아래로 안 감"), FGrimSurvivalModel::ComputeTick(Inputs, 10.f, Rules).Health, -0.1f, 1e-4f);

	// 쓰러져 있으면 HP 그대로(일으키거나 시간이 끝날 때까지).
	Inputs.Health = 0.f;
	Inputs.bDowned = true;
	TestEqual(TEXT("쓰러짐 HP 변화 없음"), FGrimSurvivalModel::ComputeTick(Inputs, 1.f, Rules).Health, 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimSurvivalHungerTest, "Grimoire.Survival.Hunger", GrimSurvivalTestsPrivate::PureFlags)
bool FGrimSurvivalHungerTest::RunTest(const FString& Parameters)
{
	using namespace GrimSurvivalTestsPrivate;
	const FGrimSurvivalRules Rules;
	const FGrimSurvivalTuning& Tuning = Rules.Vital;
	FGrimSurvivalInputs Inputs = MakeComfortInputs(Rules);

	// 배고픔: 1분에 HungerDrainPerMinute 만큼 준다. 배탈이면 배율만큼 빨리.
	TestEqual(TEXT("분당 감소"), FGrimSurvivalModel::ComputeTick(Inputs, 60.f, Rules).Hunger, -Tuning.HungerDrainPerMinute, 1e-4f);
	Inputs.bStomachSick = true;
	TestEqual(TEXT("배탈 분당 감소"), FGrimSurvivalModel::ComputeTick(Inputs, 60.f, Rules).Hunger,
		-Tuning.HungerDrainPerMinute * Rules.Disease.StomachHungerDrainScale, 1e-4f);
	Inputs.bStomachSick = false;

	// 배부르면 HP 자연 회복, 최대 HP 를 넘지 않는다.
	TestEqual(TEXT("배부르면 회복"), FGrimSurvivalModel::ComputeTick(Inputs, 1.f, Rules).Health, Tuning.HealthRegenPerSecond, 1e-4f);
	Inputs.Health = 99.9f;
	TestEqual(TEXT("최대치에서 멈춤"), FGrimSurvivalModel::ComputeTick(Inputs, 10.f, Rules).Health, 0.1f, 1e-3f);

	// 배고프면(임계값 미만) 회복이 멈춘다.
	Inputs.Health = 50.f;
	Inputs.Hunger = Tuning.HungryThreshold - 1.f;
	TestEqual(TEXT("배고프면 회복 정지"), FGrimSurvivalModel::ComputeTick(Inputs, 1.f, Rules).Health, 0.f);
	TestTrue(TEXT("Hungry 상태"), FGrimSurvivalModel::EvaluateStatus(Inputs.Hunger, MakeExposure(0, 0), 0.f, Rules).bHungry);

	// 0 이면 굶주림: HP 감소, 배고픔은 0 아래로 가지 않는다.
	Inputs.Hunger = 0.f;
	const FGrimSurvivalDeltas Starving = FGrimSurvivalModel::ComputeTick(Inputs, 1.f, Rules);
	TestEqual(TEXT("굶주리면 HP 감소"), Starving.Health, -Tuning.StarvingDamagePerSecond, 1e-4f);
	TestEqual(TEXT("배고픔 0 유지"), Starving.Hunger, 0.f);
	TestTrue(TEXT("Starving 상태"), FGrimSurvivalModel::EvaluateStatus(0.f, MakeExposure(0, 0), 0.f, Rules).bStarving);

	// 가득 찬 배는 (MaxHunger / 분당 감소)분 뒤 빈다.
	Inputs.Hunger = Tuning.MaxHunger;
	Inputs.Health = 100.f;
	float Hunger = Inputs.Hunger;
	const int32 Seconds = FMath::RoundToInt32(Tuning.MaxHunger / Tuning.HungerDrainPerMinute * 60.f);
	for (int32 i = 0; i < Seconds; ++i)
	{
		Inputs.Hunger = Hunger;
		Hunger += FGrimSurvivalModel::ComputeTick(Inputs, 1.f, Rules).Hunger;
	}
	TestEqual(TEXT("다 비는 시간"), Hunger, 0.f, 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimSurvivalFatigueTest, "Grimoire.Survival.Fatigue", GrimSurvivalTestsPrivate::PureFlags)
bool FGrimSurvivalFatigueTest::RunTest(const FString& Parameters)
{
	using namespace GrimSurvivalTestsPrivate;
	const FGrimSurvivalRules Rules;
	const FGrimFatigueTuning& Fatigue = Rules.Fatigue;

	// 깨어 있으면 분당 GainPerMinute, 밤 ×1.5, 2단계 체온이면 또 ×1.5.
	TestEqual(TEXT("낮 분당 증가"), FGrimSurvivalModel::ComputeFatigueDelta(0.f, 60.f, false, false, false, false, Rules), Fatigue.GainPerMinute, 1e-4f);
	TestEqual(TEXT("밤 ×1.5"), FGrimSurvivalModel::ComputeFatigueDelta(0.f, 60.f, false, false, true, false, Rules), Fatigue.GainPerMinute * Fatigue.NightGainScale, 1e-4f);
	TestEqual(TEXT("저체온 가속"), FGrimSurvivalModel::ComputeFatigueDelta(0.f, 60.f, false, false, false, true, Rules),
		Fatigue.GainPerMinute * Rules.Climate.ModerateFatigueRateScale, 1e-4f);
	TestEqual(TEXT("최대에서 멈춤"), FGrimSurvivalModel::ComputeFatigueDelta(Fatigue.MaxFatigue - 0.1f, 60.f, false, false, false, false, Rules), 0.1f, 1e-4f);

	// 자면 회복, 쓰러져 있으면 그대로.
	TestEqual(TEXT("잠 회복"), FGrimSurvivalModel::ComputeFatigueDelta(50.f, 1.f, true, false, true, false, Rules), -Fatigue.SleepRecoveryPerSecond, 1e-4f);
	TestEqual(TEXT("0 아래로 안 감"), FGrimSurvivalModel::ComputeFatigueDelta(1.f, 10.f, true, false, false, false, Rules), -1.f, 1e-4f);
	TestEqual(TEXT("쓰러짐은 그대로"), FGrimSurvivalModel::ComputeFatigueDelta(50.f, 10.f, false, true, true, false, Rules), 0.f);

	// 틱도 같은 값을 낸다.
	FGrimSurvivalInputs Inputs = MakeComfortInputs(Rules);
	Inputs.bNight = true;
	TestEqual(TEXT("틱의 피곤함"), FGrimSurvivalModel::ComputeTick(Inputs, 60.f, Rules).Fatigue, Fatigue.GainPerMinute * Fatigue.NightGainScale, 1e-4f);

	// 임계값: 80 이상 Exhausted, 100 기절.
	TestFalse(TEXT("79 는 멀쩡"), FGrimSurvivalModel::EvaluateStatus(100.f, MakeExposure(0, 0), Fatigue.ExhaustedThreshold - 1.f, Rules).bExhausted);
	TestTrue(TEXT("80 Exhausted"), FGrimSurvivalModel::EvaluateStatus(100.f, MakeExposure(0, 0), Fatigue.ExhaustedThreshold, Rules).bExhausted);
	const FGrimSurvivalStatus Collapse = FGrimSurvivalModel::EvaluateStatus(100.f, MakeExposure(0, 0), Fatigue.CollapseThreshold, Rules);
	TestTrue(TEXT("100 기절"), Collapse.bCollapse && Collapse.bExhausted);

	// 기본값으로 하루(낮 30분 · 밤 18분)를 꼬박 새면 Exhausted 근처, 기절은 아니다.
	const float Day = 48.f * 0.625f;
	const float Night = 48.f - Day;
	const float OneDay = FGrimSurvivalModel::ComputeFatigueDelta(0.f, Day * 60.f, false, false, false, false, Rules)
		+ FGrimSurvivalModel::ComputeFatigueDelta(0.f, Night * 60.f, false, false, true, false, Rules);
	TestTrue(TEXT("하루 새면 지침, 기절은 아님"), OneDay >= Fatigue.ExhaustedThreshold && OneDay < Fatigue.CollapseThreshold);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimSurvivalPenaltyTest, "Grimoire.Survival.Penalty", GrimSurvivalTestsPrivate::PureFlags)
bool FGrimSurvivalPenaltyTest::RunTest(const FString& Parameters)
{
	using namespace GrimSurvivalTestsPrivate;
	const FGrimSurvivalRules Rules;
	const FGrimClimateTuning& Climate = Rules.Climate;

	auto Mods = [&Rules](int32 Heat, int32 Cold, float Fatigue, bool bSleeping = false, bool bDowned = false)
	{
		const FGrimSurvivalStatus Status = FGrimSurvivalModel::EvaluateStatus(100.f, MakeExposure(Heat, Cold), Fatigue, Rules);
		return FGrimSurvivalModel::ComputeModifiers(Status, bSleeping, bDowned, Rules);
	};

	TestTrue(TEXT("정상은 패널티 없음"), Mods(0, 0, 0.f).IsIdentity());

	const FGrimSurvivalModifiers Chilly = Mods(0, 1, 0.f);
	TestEqual(TEXT("쌀쌀함 이동"), Chilly.MoveSpeed, Climate.ChillyMoveSpeedScale, 1e-4f);
	TestEqual(TEXT("쌀쌀함 시전"), Chilly.CastSpeed, Climate.ChillyCastSpeedScale, 1e-4f);
	TestEqual(TEXT("추위는 마나 소모 그대로"), Chilly.ManaCost, 1.f, 1e-4f);

	const FGrimSurvivalModifiers Hypo = Mods(0, 3, 0.f);
	TestEqual(TEXT("저체온 이동(3단계도 2단계 배율)"), Hypo.MoveSpeed, Climate.HypothermiaMoveSpeedScale, 1e-4f);
	TestTrue(TEXT("저체온이 더 느리다"), Hypo.MoveSpeed < Chilly.MoveSpeed && Hypo.CastSpeed < Chilly.CastSpeed);

	TestEqual(TEXT("더움 마나 소모"), Mods(1, 0, 0.f).ManaCost, Climate.WarmManaCostScale, 1e-4f);
	TestEqual(TEXT("과열 마나 소모"), Mods(2, 0, 0.f).ManaCost, Climate.HyperthermiaManaCostScale, 1e-4f);
	TestEqual(TEXT("더위는 이동 그대로"), Mods(2, 0, 0.f).MoveSpeed, 1.f, 1e-4f);

	// 피곤함 −30% 는 추위와 곱해진다.
	TestEqual(TEXT("지침 시전 −30%"), Mods(0, 0, Rules.Fatigue.ExhaustedThreshold).CastSpeed, Rules.Fatigue.ExhaustedCastSpeedScale, 1e-4f);
	TestEqual(TEXT("지침 × 쌀쌀함"), Mods(0, 1, Rules.Fatigue.ExhaustedThreshold).CastSpeed,
		Rules.Fatigue.ExhaustedCastSpeedScale * Climate.ChillyCastSpeedScale, 1e-4f);

	TestEqual(TEXT("자는 중 이동"), Mods(0, 0, 0.f, true).MoveSpeed, Rules.Sleep.SleepingMoveSpeedScale, 1e-4f);
	TestEqual(TEXT("쓰러짐 이동"), Mods(0, 0, 0.f, false, true).MoveSpeed, Rules.Downed.DownedMoveSpeedScale, 1e-4f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimSurvivalDiseaseTest, "Grimoire.Survival.Disease", GrimSurvivalTestsPrivate::PureFlags)
bool FGrimSurvivalDiseaseTest::RunTest(const FString& Parameters)
{
	using namespace GrimSurvivalTestsPrivate;
	const FGrimSurvivalRules Rules;
	const FGrimDiseaseTuning& Disease = Rules.Disease;

	// 확률 × (1 − 저항/100).
	TestEqual(TEXT("저항 0"), FGrimSurvivalModel::ComputeDiseaseChance(0.3f, 0.f), 0.3f, 1e-5f);
	TestEqual(TEXT("저항 50"), FGrimSurvivalModel::ComputeDiseaseChance(0.3f, 50.f), 0.15f, 1e-5f);
	TestEqual(TEXT("저항 100 = 면역"), FGrimSurvivalModel::ComputeDiseaseChance(0.3f, 100.f), 0.f, 1e-5f);

	// 젖은 채 추운 시간이 주기를 채울 때마다 한 번 굴린다. 조건이 풀리면 처음부터.
	float Seconds = 0.f;
	int32 Rolls = 0;
	for (int32 i = 0; i < FMath::RoundToInt32(Disease.WetColdSecondsPerRoll * 2.5f); ++i)
	{
		Rolls += FGrimSurvivalModel::AccumulateWetCold(Seconds, 1.f, true, Disease);
	}
	TestEqual(TEXT("2.5 주기 → 2번"), Rolls, 2);
	TestEqual(TEXT("말린 뒤엔 0"), FGrimSurvivalModel::AccumulateWetCold(Seconds, 1.f, false, Disease), 0);
	TestEqual(TEXT("쌓인 시간 초기화"), Seconds, 0.f);

	// 감기: 마나 회복이 도는 동안만 깎는다.
	FGrimSurvivalInputs Inputs = MakeComfortInputs(Rules);
	Inputs.bColdSick = true;
	TestEqual(TEXT("감기 마나 회복 감소"), FGrimSurvivalModel::ComputeTick(Inputs, 1.f, Rules).Mana, -Disease.ColdManaRegenPenaltyPerSecond, 1e-4f);
	Inputs.Mana = Inputs.MaxMana;
	TestEqual(TEXT("가득 차 있으면 그대로"), FGrimSurvivalModel::ComputeTick(Inputs, 1.f, Rules).Mana, 0.f);
	Inputs.Mana = 50.f;
	Inputs.Hunger = 0.f;
	TestEqual(TEXT("배고파 회복이 멈췄으면 그대로"), FGrimSurvivalModel::ComputeTick(Inputs, 1.f, Rules).Mana, 0.f);
	Inputs.Hunger = 80.f;
	Inputs.bColdSick = false;
	TestEqual(TEXT("감기 아니면 그대로"), FGrimSurvivalModel::ComputeTick(Inputs, 1.f, Rules).Mana, 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimSurvivalSleepTest, "Grimoire.Survival.Sleep", GrimSurvivalTestsPrivate::PureFlags)
bool FGrimSurvivalSleepTest::RunTest(const FString& Parameters)
{
	const FGrimSurvivalRules Rules;

	// 잠든 비율.
	TestTrue(TEXT("혼자 자면 건너뜀"), FGrimSurvivalModel::ShouldSkipNight(1, 1, 1.f, true));
	TestFalse(TEXT("낮엔 건너뛰지 않음"), FGrimSurvivalModel::ShouldSkipNight(1, 1, 1.f, false));
	TestFalse(TEXT("전원 기준, 2명 중 1명"), FGrimSurvivalModel::ShouldSkipNight(1, 2, 1.f, true));
	TestTrue(TEXT("50%, 2명 중 1명"), FGrimSurvivalModel::ShouldSkipNight(1, 2, 0.5f, true));
	TestFalse(TEXT("50%, 3명 중 1명"), FGrimSurvivalModel::ShouldSkipNight(1, 3, 0.5f, true));
	TestTrue(TEXT("50%, 3명 중 2명"), FGrimSurvivalModel::ShouldSkipNight(2, 3, 0.5f, true));
	TestFalse(TEXT("접속자 0"), FGrimSurvivalModel::ShouldSkipNight(0, 0, 1.f, true));
	TestFalse(TEXT("아무도 안 잠"), FGrimSurvivalModel::ShouldSkipNight(0, 3, 0.01f, true));

	// 건너뛴 시간: 앞으로만.
	TestEqual(TEXT("자정 뒤 → 해돋이"), FGrimSurvivalModel::ComputeDaysUntil(0.1f, 0.2f), 0.1f, 1e-5f);
	TestEqual(TEXT("저녁 → 다음 날 해돋이"), FGrimSurvivalModel::ComputeDaysUntil(0.9f, 0.2f), 0.3f, 1e-5f);
	TestEqual(TEXT("같은 시각"), FGrimSurvivalModel::ComputeDaysUntil(0.2f, 0.2f), 0.f, 1e-5f);

	// 배고픔: 건너뛴 실시간 분 × 분당 감소 × SkippedHungerScale.
	const float DayLengthSeconds = 48.f * 60.f;
	const float Expected = 0.3f * 48.f * Rules.Vital.HungerDrainPerMinute * Rules.Sleep.SkippedHungerScale;
	TestEqual(TEXT("건너뛴 시간만큼 배고픔"), FGrimSurvivalModel::ComputeSkippedHungerLoss(0.3f, DayLengthSeconds, Rules), Expected, 1e-3f);
	TestTrue(TEXT("밤 하나는 조금만"), Expected < Rules.Vital.MaxHunger * 0.25f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimSurvivalAttributeClampTest, "Grimoire.Survival.AttributeClamp", GrimSurvivalTestsPrivate::PureFlags)
bool FGrimSurvivalAttributeClampTest::RunTest(const FString& Parameters)
{
	const UGrimSurvivalSettings* Settings = GetDefault<UGrimSurvivalSettings>();
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
	TestEqual(TEXT("체온 ≤ 최대"), Value, Settings->Tuning.MaxBodyTemperature);

	Value = 1000.f;
	Set->PreAttributeBaseChange(UGrimVitalAttributeSet::GetFatigueAttribute(), Value);
	TestEqual(TEXT("피곤함 ≤ 최대"), Value, Settings->Fatigue.MaxFatigue);

	Value = -3.f;
	Set->PreAttributeBaseChange(UGrimVitalAttributeSet::GetColdResistAttribute(), Value);
	TestEqual(TEXT("저항은 음수 가능"), Value, -3.f);

	TestEqual(TEXT("이동 배율 기본 1"), Set->GetMoveSpeedScale(), 1.f);
	TestEqual(TEXT("시전 배율 기본 1"), Set->GetCastSpeedScale(), 1.f);
	TestEqual(TEXT("마나 소모 배율 기본 1"), Set->GetManaCostScale(), 1.f);
	return true;
}

// GE 들이 어트리뷰트 · 태그에 맞게 붙는가(빈 월드의 ASC).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimSurvivalEffectsTest, "Grimoire.Survival.Effects", GrimSurvivalTestsPrivate::WorldFlags)
bool FGrimSurvivalEffectsTest::RunTest(const FString& Parameters)
{
	using namespace GrimSurvivalTestsPrivate;
	FTestWorld TestWorld;
	UAbilitySystemComponent* ASC = TestWorld.SpawnPlayer();
	auto Get = [ASC](const FGameplayAttribute& Attribute) { return ASC->GetNumericAttribute(Attribute); };

	// 패널티: 배율을 곱하고, 떼면 1 로.
	FGameplayEffectSpecHandle Penalty = ASC->MakeOutgoingSpec(UGrimSurvivalPenaltyEffect::StaticClass(), 1.f, ASC->MakeEffectContext());
	Penalty.Data->SetSetByCallerMagnitude(UGrimSurvivalPenaltyEffect::MoveSpeedMagnitudeName, 0.75f);
	Penalty.Data->SetSetByCallerMagnitude(UGrimSurvivalPenaltyEffect::CastSpeedMagnitudeName, 0.7f);
	Penalty.Data->SetSetByCallerMagnitude(UGrimSurvivalPenaltyEffect::ManaCostMagnitudeName, 1.35f);
	const FActiveGameplayEffectHandle PenaltyHandle = ASC->ApplyGameplayEffectSpecToSelf(*Penalty.Data.Get());
	TestEqual(TEXT("이동 배율"), Get(UGrimVitalAttributeSet::GetMoveSpeedScaleAttribute()), 0.75f, 1e-4f);
	TestEqual(TEXT("시전 배율"), Get(UGrimVitalAttributeSet::GetCastSpeedScaleAttribute()), 0.7f, 1e-4f);
	TestEqual(TEXT("마나 소모 배율"), Get(UGrimVitalAttributeSet::GetManaCostScaleAttribute()), 1.35f, 1e-4f);
	ASC->RemoveActiveGameplayEffect(PenaltyHandle);
	TestEqual(TEXT("떼면 이동 1"), Get(UGrimVitalAttributeSet::GetMoveSpeedScaleAttribute()), 1.f, 1e-4f);

	// 질병: 태그를 주고 지속 시간이 SetByCaller.
	FGameplayEffectSpecHandle Cold = ASC->MakeOutgoingSpec(UGrimDiseaseColdEffect::StaticClass(), 1.f, ASC->MakeEffectContext());
	Cold.Data->SetSetByCallerMagnitude(UGrimDiseaseEffect::DurationMagnitudeName, 300.f);
	const FActiveGameplayEffectHandle ColdHandle = ASC->ApplyGameplayEffectSpecToSelf(*Cold.Data.Get());
	TestTrue(TEXT("감기 태그"), ASC->HasMatchingGameplayTag(GrimSurvivalTags::Status_Sick_Cold));
	TestTrue(TEXT("상위 Status.Sick 로도 보인다"), ASC->HasMatchingGameplayTag(GrimSurvivalTags::Status_Sick));
	if (const FActiveGameplayEffect* Active = ASC->GetActiveGameplayEffect(ColdHandle))
	{
		TestEqual(TEXT("지속 시간"), Active->GetDuration(), 300.f, 1e-3f);
	}
	ASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(GrimSurvivalTags::Status_Sick));
	TestFalse(TEXT("Status.Sick 로 모두 낫는다"), ASC->HasMatchingGameplayTag(GrimSurvivalTags::Status_Sick_Cold));

	// 저항 버프.
	FGameplayEffectSpecHandle Resist = ASC->MakeOutgoingSpec(UGrimClimateResistEffect::StaticClass(), 1.f, ASC->MakeEffectContext());
	Resist.Data->SetSetByCallerMagnitude(UGrimClimateResistEffect::HeatMagnitudeName, -1.f);
	Resist.Data->SetSetByCallerMagnitude(UGrimClimateResistEffect::ColdMagnitudeName, 3.f);
	ASC->ApplyGameplayEffectSpecToSelf(*Resist.Data.Get());
	TestEqual(TEXT("더위저항"), Get(UGrimVitalAttributeSet::GetHeatResistAttribute()), -1.f, 1e-4f);
	TestEqual(TEXT("추위저항"), Get(UGrimVitalAttributeSet::GetColdResistAttribute()), 3.f, 1e-4f);

	// 틱 보조: 피곤함 · 마나 베이스 가산.
	ASC->SetNumericAttributeBase(UGrimMagicAttributeSet::GetMaxManaAttribute(), 100.f);
	ASC->SetNumericAttributeBase(UGrimMagicAttributeSet::GetManaAttribute(), 50.f);
	FGameplayEffectSpecHandle Extra = ASC->MakeOutgoingSpec(UGrimSurvivalExtraTickEffect::StaticClass(), 1.f, ASC->MakeEffectContext());
	Extra.Data->SetSetByCallerMagnitude(UGrimSurvivalExtraTickEffect::FatigueMagnitudeName, 12.f);
	Extra.Data->SetSetByCallerMagnitude(UGrimSurvivalExtraTickEffect::ManaMagnitudeName, -1.f);
	ASC->ApplyGameplayEffectSpecToSelf(*Extra.Data.Get());
	TestEqual(TEXT("피곤함 가산"), Get(UGrimVitalAttributeSet::GetFatigueAttribute()), 12.f, 1e-4f);
	TestEqual(TEXT("마나 감산"), Get(UGrimMagicAttributeSet::GetManaAttribute()), 49.f, 1e-4f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
