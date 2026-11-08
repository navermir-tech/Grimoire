#include "Survival/GrimSurvivalModel.h"

DEFINE_LOG_CATEGORY(LogGrimSurvival);

FGrimClimateExposure FGrimSurvivalModel::ComputePersonalExposure(const FGrimPersonalClimate& Personal, const FGrimClimateTuning& Climate)
{
	FGrimClimateExposure Exposure;
	if (Personal.bWet)
	{
		Exposure.Cold += Climate.WetCold;
	}
	if (Personal.bNearHeatSource)
	{
		Exposure.Cold -= Climate.NearHeatSourceCold;
	}
	if (Personal.bBurning)
	{
		Exposure.Heat += Climate.BurningHeat;
	}
	if (Personal.bSwimming)
	{
		Exposure.Heat -= Climate.SwimmingHeat;
	}
	return Exposure;
}

FGrimClimateExposure FGrimSurvivalModel::CombineExposure(const FGrimClimateExposure& World, const FGrimClimateExposure& Personal)
{
	FGrimClimateExposure Exposure;
	Exposure.Heat = FMath::Max(0, World.Heat + Personal.Heat);
	Exposure.Cold = FMath::Max(0, World.Cold + Personal.Cold);
	return Exposure;
}

FGrimClimateExposure FGrimSurvivalModel::ComputeDeficit(const FGrimClimateExposure& Exposure, float HeatResist, float ColdResist)
{
	// 저항은 정수처럼 쓰지만 float 어트리뷰트다 — 2.5 는 3 을 다 막지 못한다(모자람 1).
	auto Deficit = [](int32 Grade, float Resist)
	{
		return FMath::Max(0, FMath::CeilToInt32(static_cast<float>(Grade) - Resist - KINDA_SMALL_NUMBER));
	};
	FGrimClimateExposure Result;
	Result.Heat = Deficit(Exposure.Heat, HeatResist);
	Result.Cold = Deficit(Exposure.Cold, ColdResist);
	return Result;
}

float FGrimSurvivalModel::ComputeTargetBodyTemperature(const FGrimClimateExposure& Deficit, const FGrimSurvivalRules& Rules)
{
	const float Target = Rules.Vital.NormalBodyTemperature
		+ (Deficit.Heat - Deficit.Cold) * Rules.Climate.BodyTemperaturePerDeficit;
	return FMath::Clamp(Target, Rules.Vital.MinBodyTemperature, Rules.Vital.MaxBodyTemperature);
}

float FGrimSurvivalModel::StepBodyTemperature(float Current, float Target, float DeltaSeconds, const FGrimSurvivalTuning& Tuning)
{
	const float TimeConstant = FMath::Max(1.f, Tuning.BodyTemperatureTimeConstantSeconds);
	const float Alpha = 1.f - FMath::Exp(-FMath::Max(0.f, DeltaSeconds) / TimeConstant);
	return Current + (Target - Current) * Alpha;
}

FGrimSurvivalStatus FGrimSurvivalModel::EvaluateStatus(float Hunger, const FGrimClimateExposure& Deficit, float Fatigue, const FGrimSurvivalRules& Rules)
{
	const FGrimClimateTuning& Climate = Rules.Climate;
	const int32 Moderate = FMath::Max(1, Climate.ModerateDeficit);
	const int32 Severe = FMath::Max(Moderate, Climate.SevereDeficit);

	FGrimSurvivalStatus Status;
	Status.bStarving = Hunger <= 0.f;
	Status.bHungry = Hunger < Rules.Vital.HungryThreshold;

	Status.ColdGrade = FMath::Max(0, Deficit.Cold);
	Status.HeatGrade = FMath::Max(0, Deficit.Heat);

	// 1단계는 따로, 2단계 이상은 Hypothermia, 3단계 이상은 그 위에 Severe.
	Status.bChilly = Status.ColdGrade >= 1 && Status.ColdGrade < Moderate;
	Status.bHypothermia = Status.ColdGrade >= Moderate;
	Status.bHypothermiaSevere = Status.ColdGrade >= Severe;
	Status.bWarm = Status.HeatGrade >= 1 && Status.HeatGrade < Moderate;
	Status.bHyperthermia = Status.HeatGrade >= Moderate;
	Status.bHyperthermiaSevere = Status.HeatGrade >= Severe;

	Status.bExhausted = Fatigue >= Rules.Fatigue.ExhaustedThreshold;
	return Status;
}

FGrimSurvivalModifiers FGrimSurvivalModel::ComputeModifiers(const FGrimSurvivalStatus& Status, bool bSleeping, bool bDowned, const FGrimSurvivalRules& Rules)
{
	const FGrimClimateTuning& Climate = Rules.Climate;
	FGrimSurvivalModifiers Modifiers;

	if (Status.bHypothermia)
	{
		Modifiers.MoveSpeed *= Climate.HypothermiaMoveSpeedScale;
		Modifiers.CastSpeed *= Climate.HypothermiaCastSpeedScale;
	}
	else if (Status.bChilly)
	{
		Modifiers.MoveSpeed *= Climate.ChillyMoveSpeedScale;
		Modifiers.CastSpeed *= Climate.ChillyCastSpeedScale;
	}

	if (Status.bHyperthermia)
	{
		Modifiers.ManaCost *= Climate.HyperthermiaManaCostScale;
	}
	else if (Status.bWarm)
	{
		Modifiers.ManaCost *= Climate.WarmManaCostScale;
	}

	if (Status.bExhausted)
	{
		// 지친 걸음. 시전은 속도가 아니라 차단(마법 쪽 MagicBlockingTags 의 Status.Exhausted).
		Modifiers.MoveSpeed *= Rules.Fatigue.ExhaustedMoveSpeedScale;
	}
	if (bSleeping)
	{
		Modifiers.MoveSpeed *= Rules.Sleep.SleepingMoveSpeedScale;
	}
	if (bDowned)
	{
		Modifiers.MoveSpeed *= Rules.Downed.DownedMoveSpeedScale;
	}
	return Modifiers;
}

float FGrimSurvivalModel::ComputeFatigueDelta(float Fatigue, float DeltaSeconds, bool bSleeping, bool bDowned, bool bNight, bool bModerateExposure, const FGrimSurvivalRules& Rules)
{
	const FGrimFatigueTuning& Tuning = Rules.Fatigue;
	const float Dt = FMath::Max(0.f, DeltaSeconds);
	if (bDowned)
	{
		return 0.f;
	}
	if (bSleeping)
	{
		return -FMath::Min(FMath::Max(0.f, Fatigue), Tuning.SleepRecoveryPerSecond * Dt);
	}

	float Rate = Tuning.GainPerMinute / 60.f;
	if (bNight)
	{
		Rate *= Tuning.NightGainScale;
	}
	if (bModerateExposure)
	{
		Rate *= Rules.Climate.ModerateFatigueRateScale;
	}
	return FMath::Min(FMath::Max(0.f, Tuning.MaxFatigue - Fatigue), Rate * Dt);
}

FGrimSurvivalDeltas FGrimSurvivalModel::ComputeTick(const FGrimSurvivalInputs& Inputs, float DeltaSeconds, const FGrimSurvivalRules& Rules)
{
	const FGrimSurvivalTuning& Vital = Rules.Vital;
	FGrimSurvivalDeltas Deltas;
	const float Dt = FMath::Max(0.f, DeltaSeconds);

	// 상태는 이번 틱 시작 값으로 판단한다.
	const FGrimClimateExposure Deficit = ComputeDeficit(Inputs.Exposure, Inputs.HeatResist, Inputs.ColdResist);
	const FGrimSurvivalStatus Status = EvaluateStatus(Inputs.Hunger, Deficit, Inputs.Fatigue, Rules);

	// 배고픔: 배탈이면 빨리 준다. 0 아래로 내려가지 않는다.
	const float DrainScale = Inputs.bStomachSick ? Rules.Disease.StomachHungerDrainScale : 1.f;
	Deltas.Hunger = -FMath::Min(FMath::Max(0.f, Inputs.Hunger), Vital.HungerDrainPerMinute / 60.f * DrainScale * Dt);

	// 체온 게이지: 모자람이 정한 목표 쪽으로 수렴.
	const float Target = ComputeTargetBodyTemperature(Deficit, Rules);
	Deltas.BodyTemperature = StepBodyTemperature(Inputs.BodyTemperature, Target, Dt, Vital) - Inputs.BodyTemperature;

	Deltas.Fatigue = ComputeFatigueDelta(Inputs.Fatigue, Dt, Inputs.bSleeping, Inputs.bDowned, Inputs.bNight, Status.IsModerateExposure(), Rules);

	// HP: 쓰러져 있으면 그대로(일으키거나 시간이 끝날 때까지). 굶주림 · 심한 체온 이상이면 감소, 아니고 배고프지 않으면 회복.
	if (!Inputs.bDowned)
	{
		float Damage = 0.f;
		if (Status.bStarving)
		{
			Damage += Vital.StarvingDamagePerSecond * Dt;
		}
		if (Status.IsSevereExposure())
		{
			Damage += Vital.ExposureDamagePerSecond * Dt;
		}

		if (Damage > 0.f)
		{
			Deltas.Health = -FMath::Min(FMath::Max(0.f, Inputs.Health), Damage);
		}
		else if (!Status.bHungry && Inputs.Health > 0.f)
		{
			Deltas.Health = FMath::Min(FMath::Max(0.f, Inputs.MaxHealth - Inputs.Health), Vital.HealthRegenPerSecond * Dt);
		}
	}

	// 감기: 마나 회복이 도는 동안(배고프지 않고 덜 찼을 때) 회복을 깎는다. 회복 자체는 Magic 의 GE 몫.
	if (Inputs.bColdSick && !Status.bHungry && Inputs.Mana < Inputs.MaxMana)
	{
		Deltas.Mana = -FMath::Min(FMath::Max(0.f, Inputs.Mana), Rules.Disease.ColdManaRegenPenaltyPerSecond * Dt);
	}
	return Deltas;
}

float FGrimSurvivalModel::ComputeDiseaseChance(float BaseChance, float DiseaseResist)
{
	return FMath::Clamp(BaseChance * (1.f - FMath::Clamp(DiseaseResist, 0.f, 100.f) / 100.f), 0.f, 1.f);
}

int32 FGrimSurvivalModel::AccumulateWetCold(float& AccumulatedSeconds, float DeltaSeconds, bool bWetAndCold, const FGrimDiseaseTuning& Disease)
{
	if (!bWetAndCold)
	{
		AccumulatedSeconds = 0.f;
		return 0;
	}
	const float Period = FMath::Max(1.f, Disease.WetColdSecondsPerRoll);
	AccumulatedSeconds += FMath::Max(0.f, DeltaSeconds);
	int32 Rolls = 0;
	while (AccumulatedSeconds >= Period)
	{
		AccumulatedSeconds -= Period;
		++Rolls;
	}
	return Rolls;
}

bool FGrimSurvivalModel::ShouldSkipNight(int32 SleepingCount, int32 PlayerCount, float Ratio, bool bNight)
{
	if (!bNight || PlayerCount <= 0 || SleepingCount <= 0)
	{
		return false;
	}
	// 비율을 사람 수로 올림: 3명 중 50% = 2명.
	const int32 Required = FMath::Clamp(FMath::CeilToInt32(PlayerCount * FMath::Clamp(Ratio, 0.f, 1.f) - KINDA_SMALL_NUMBER), 1, PlayerCount);
	return SleepingCount >= Required;
}

float FGrimSurvivalModel::ComputeDaysUntil(float NowTimeOfDay, float TargetTimeOfDay)
{
	const float Days = FMath::Fmod(TargetTimeOfDay - NowTimeOfDay + 1.f, 1.f);
	return Days < 0.f ? Days + 1.f : Days;
}

float FGrimSurvivalModel::ComputeSkippedHungerLoss(float SkippedDays, float DayLengthSeconds, const FGrimSurvivalRules& Rules)
{
	const float SkippedMinutes = FMath::Max(0.f, SkippedDays) * FMath::Max(0.f, DayLengthSeconds) / 60.f;
	return SkippedMinutes * Rules.Vital.HungerDrainPerMinute * Rules.Sleep.SkippedHungerScale;
}
