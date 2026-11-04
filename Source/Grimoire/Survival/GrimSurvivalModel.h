#pragma once

#include "CoreMinimal.h"
#include "Survival/GrimSurvivalSettings.h"

DECLARE_LOG_CATEGORY_EXTERN(LogGrimSurvival, Log, All);

/** 한 생존 틱의 입력. 컴포넌트가 월드에서 모아 온다. */
struct FGrimSurvivalInputs
{
	float Health = 100.f;
	float MaxHealth = 100.f;
	float Hunger = 100.f;
	float BodyTemperature = 37.f;
	float AmbientTemperature = 20.f;
	/** 반경 내 열원이 더하는 체감 기온 합(℃). ComputeHeatWarmth 로 하나씩 더한다. */
	float HeatWarmth = 0.f;
	/** 자기 폰의 State.Wet 세기. */
	float WetIntensity = 0.f;
};

/** 한 틱 동안의 변화량(GE SetByCaller 로 적용). */
struct FGrimSurvivalDeltas
{
	float Health = 0.f;
	float Hunger = 0.f;
	float BodyTemperature = 0.f;
};

struct FGrimSurvivalStatus
{
	bool bHungry = false;
	bool bStarving = false;
	bool bHypothermia = false;
	bool bHyperthermia = false;
};

/**
 * 생존 순수 계산 (docs/systems/survival 「체온 모델」). 월드 없이 테스트한다(Grimoire.Survival.*).
 *
 * 체감 기온 = 주변 기온 + Σ 열원 − 젖음 × CoolingPerWetIntensity
 * 목표 체온 = 정상 체온 + (체감 기온 − 쾌적 기온) × 민감도
 * 체온은 목표 쪽으로 지수 수렴(시정수 BodyTemperatureTimeConstantSeconds).
 */
struct GRIMOIRE_API FGrimSurvivalModel
{
	/** 세기 Intensity 의 열원이 Distance(cm) 떨어져 있을 때 더하는 체감 기온. 반경 밖은 0. */
	static float ComputeHeatWarmth(float BurningIntensity, float Distance, const FGrimSurvivalTuning& Tuning);

	static float ComputeFeltTemperature(float AmbientTemperature, float HeatWarmth, float WetIntensity, const FGrimSurvivalTuning& Tuning);
	static float ComputeTargetBodyTemperature(float AmbientTemperature, float HeatWarmth, float WetIntensity, const FGrimSurvivalTuning& Tuning);

	/** DeltaSeconds 뒤의 체온. 틱 간격이 바뀌어도 같은 곡선을 따른다. */
	static float StepBodyTemperature(float Current, float Target, float DeltaSeconds, const FGrimSurvivalTuning& Tuning);

	static FGrimSurvivalStatus EvaluateStatus(float Hunger, float BodyTemperature, const FGrimSurvivalTuning& Tuning);

	static FGrimSurvivalDeltas ComputeTick(const FGrimSurvivalInputs& Inputs, float DeltaSeconds, const FGrimSurvivalTuning& Tuning);
};
