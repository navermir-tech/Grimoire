#include "Survival/GrimSurvivalModel.h"

DEFINE_LOG_CATEGORY(LogGrimSurvival);

float FGrimSurvivalModel::ComputeHeatWarmth(float BurningIntensity, float Distance, const FGrimSurvivalTuning& Tuning)
{
	if (BurningIntensity <= 0.f || Tuning.HeatSourceRadius <= 0.f || Distance >= Tuning.HeatSourceRadius)
	{
		return 0.f;
	}
	const float Falloff = 1.f - FMath::Max(0.f, Distance) / Tuning.HeatSourceRadius;
	return BurningIntensity * Tuning.WarmthPerBurningIntensity * Falloff;
}

float FGrimSurvivalModel::ComputeFeltTemperature(float AmbientTemperature, float HeatWarmth, float WetIntensity, const FGrimSurvivalTuning& Tuning)
{
	return AmbientTemperature + FMath::Max(0.f, HeatWarmth) - FMath::Max(0.f, WetIntensity) * Tuning.CoolingPerWetIntensity;
}

float FGrimSurvivalModel::ComputeTargetBodyTemperature(float AmbientTemperature, float HeatWarmth, float WetIntensity, const FGrimSurvivalTuning& Tuning)
{
	const float Felt = ComputeFeltTemperature(AmbientTemperature, HeatWarmth, WetIntensity, Tuning);
	const float Target = Tuning.NormalBodyTemperature + (Felt - Tuning.ComfortTemperature) * Tuning.BodyTemperatureSensitivity;
	return FMath::Clamp(Target, Tuning.MinBodyTemperature, Tuning.MaxBodyTemperature);
}

float FGrimSurvivalModel::StepBodyTemperature(float Current, float Target, float DeltaSeconds, const FGrimSurvivalTuning& Tuning)
{
	const float TimeConstant = FMath::Max(1.f, Tuning.BodyTemperatureTimeConstantSeconds);
	const float Alpha = 1.f - FMath::Exp(-FMath::Max(0.f, DeltaSeconds) / TimeConstant);
	return Current + (Target - Current) * Alpha;
}

FGrimSurvivalStatus FGrimSurvivalModel::EvaluateStatus(float Hunger, float BodyTemperature, const FGrimSurvivalTuning& Tuning)
{
	FGrimSurvivalStatus Status;
	Status.bStarving = Hunger <= 0.f;
	Status.bHungry = Hunger < Tuning.HungryThreshold;
	Status.bHypothermia = BodyTemperature < Tuning.HypothermiaThreshold;
	Status.bHyperthermia = BodyTemperature > Tuning.HyperthermiaThreshold;
	return Status;
}

FGrimSurvivalDeltas FGrimSurvivalModel::ComputeTick(const FGrimSurvivalInputs& Inputs, float DeltaSeconds, const FGrimSurvivalTuning& Tuning)
{
	FGrimSurvivalDeltas Deltas;
	const float Dt = FMath::Max(0.f, DeltaSeconds);

	// 배고픔: 0 아래로 내려가지 않는다.
	Deltas.Hunger = -FMath::Min(Inputs.Hunger, Tuning.HungerDrainPerMinute / 60.f * Dt);

	// 체온: 목표 쪽으로 수렴.
	const float Target = ComputeTargetBodyTemperature(Inputs.AmbientTemperature, Inputs.HeatWarmth, Inputs.WetIntensity, Tuning);
	Deltas.BodyTemperature = StepBodyTemperature(Inputs.BodyTemperature, Target, Dt, Tuning) - Inputs.BodyTemperature;

	// HP: 굶주림 · 체온 이상이면 감소, 아니고 배고프지 않으면 자연 회복. 상태는 이번 틱 시작 값으로 판단한다.
	const FGrimSurvivalStatus Status = EvaluateStatus(Inputs.Hunger, Inputs.BodyTemperature, Tuning);
	float Damage = 0.f;
	if (Status.bStarving)
	{
		Damage += Tuning.StarvingDamagePerSecond * Dt;
	}
	if (Status.bHypothermia || Status.bHyperthermia)
	{
		Damage += Tuning.ExposureDamagePerSecond * Dt;
	}

	if (Damage > 0.f)
	{
		Deltas.Health = -FMath::Min(Inputs.Health, Damage);
	}
	else if (!Status.bHungry && Inputs.Health > 0.f)
	{
		Deltas.Health = FMath::Min(FMath::Max(0.f, Inputs.MaxHealth - Inputs.Health), Tuning.HealthRegenPerSecond * Dt);
	}
	return Deltas;
}
