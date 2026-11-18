#pragma once

#include "CoreMinimal.h"
#include "Environment/GrimWeatherSubsystem.h"
#include "Survival/GrimSurvivalSettings.h"

DECLARE_LOG_CATEGORY_EXTERN(LogGrimSurvival, Log, All);

/** 개인 몫 등급의 재료. 컴포넌트가 자기 폰에서 모아 온다. */
struct FGrimPersonalClimate
{
	bool bWet = false;
	bool bBurning = false;
	bool bNearHeatSource = false;
	bool bSwimming = false;
	// 실내(지붕)는 개인 몫이 아니라 날씨 몫을 고른다 — 컴포넌트가 UGrimWeatherSubsystem::IsActorSheltered 로(통합 때).
};

/** 한 생존 틱의 입력. 컴포넌트가 월드에서 모아 온다(값은 현재 값). */
struct FGrimSurvivalInputs
{
	float Health = 100.f;
	float MaxHealth = 100.f;
	float Hunger = 100.f;
	float BodyTemperature = 37.f;
	float Fatigue = 0.f;
	float Mana = 0.f;
	float MaxMana = 0.f;

	/** 날씨 몫 + 개인 몫을 합친 등급(0 이상). */
	FGrimClimateExposure Exposure;
	float HeatResist = 0.f;
	float ColdResist = 0.f;

	bool bNight = false;
	bool bSleeping = false;
	bool bDowned = false;
	bool bStomachSick = false;
	bool bColdSick = false;
	/** 열병(Q115) — 목표 체온이 오르고 피곤함이 빨리 쌓인다 */
	bool bFeverSick = false;
};

/** 한 틱 동안의 변화량(GE SetByCaller 로 베이스에 더한다). */
struct FGrimSurvivalDeltas
{
	float Health = 0.f;
	float Hunger = 0.f;
	float BodyTemperature = 0.f;
	float Fatigue = 0.f;
	float Mana = 0.f;
};

/** 임계값 판정. 체온 단계는 게이지가 아니라 모자람 등급으로 정한다. */
struct FGrimSurvivalStatus
{
	bool bHungry = false;
	bool bStarving = false;

	/** 모자람 등급(0 이상). */
	int32 ColdGrade = 0;
	int32 HeatGrade = 0;

	bool bChilly = false;
	bool bHypothermia = false;
	bool bHypothermiaSevere = false;
	bool bWarm = false;
	bool bHyperthermia = false;
	bool bHyperthermiaSevere = false;

	/** 지침(피곤함 TiredThreshold 이상, 탈진 전) — 시전 시간이 있는 마법이 느려지고 맞혀도 가끔 빗나간다(Q132) */
	bool bTired = false;

	/** 탈진 — 달리기 · 마법 시전 불가, 지친 걸음(R29 · Q116). 강제 기절은 없다. */
	bool bExhausted = false;

	bool IsModerateExposure() const { return bHypothermia || bHyperthermia; }
	bool IsSevereExposure() const { return bHypothermiaSevere || bHyperthermiaSevere; }
};

/** 패널티 GE 의 배율(어트리뷰트 MoveSpeedScale · CastSpeedScale · ManaCostScale 에 곱한다). */
struct FGrimSurvivalModifiers
{
	float MoveSpeed = 1.f;
	float CastSpeed = 1.f;
	float ManaCost = 1.f;

	bool Equals(const FGrimSurvivalModifiers& Other, float Tolerance = 1e-4f) const
	{
		return FMath::IsNearlyEqual(MoveSpeed, Other.MoveSpeed, Tolerance)
			&& FMath::IsNearlyEqual(CastSpeed, Other.CastSpeed, Tolerance)
			&& FMath::IsNearlyEqual(ManaCost, Other.ManaCost, Tolerance);
	}

	bool IsIdentity() const { return Equals(FGrimSurvivalModifiers()); }
};

/**
 * 생존 순수 계산 (docs/systems/survival). 월드 없이 테스트한다(Grimoire.Survival.*).
 *
 * 등급     Exposure = 날씨 몫(GetClimateExposure) + 개인 몫(젖음 · 불붙음 · 열원 · 물속)
 * 모자람   Deficit  = max(0, Exposure − Resist)   (더위 · 추위 따로)
 * 목표 체온 = 정상 − 추위 모자람 × 1.5 + 더위 모자람 × 1.5, 체온은 목표 쪽으로 지수 수렴.
 */
struct GRIMOIRE_API FGrimSurvivalModel
{
	// 체온 등급
	/** 개인 몫. 추위는 음수(열원 근처)일 수 있다. */
	static FGrimClimateExposure ComputePersonalExposure(const FGrimPersonalClimate& Personal, const FGrimClimateTuning& Climate);

	/** 날씨 몫 + 개인 몫, 0 아래는 0. */
	static FGrimClimateExposure CombineExposure(const FGrimClimateExposure& World, const FGrimClimateExposure& Personal);

	/** 모자람 = 등급 − 저항(소수 저항은 모자란 쪽으로 올림), 0 아래는 0. */
	static FGrimClimateExposure ComputeDeficit(const FGrimClimateExposure& Exposure, float HeatResist, float ColdResist);

	/** 목표 체온. 열병이면 FeverBodyTemperatureRise 만큼 더(R29 「열 = 체온 ↑」). */
	static float ComputeTargetBodyTemperature(const FGrimClimateExposure& Deficit, const FGrimSurvivalRules& Rules, bool bFever = false);

	/** DeltaSeconds 뒤의 체온. 틱 간격이 바뀌어도 같은 곡선을 따른다. */
	static float StepBodyTemperature(float Current, float Target, float DeltaSeconds, const FGrimSurvivalTuning& Tuning);

	static FGrimSurvivalStatus EvaluateStatus(float Hunger, const FGrimClimateExposure& Deficit, float Fatigue, const FGrimSurvivalRules& Rules);

	/** 상태 → 패널티 배율. 자는 중 · 쓰러짐은 이동을 막거나 줄인다. */
	static FGrimSurvivalModifiers ComputeModifiers(const FGrimSurvivalStatus& Status, bool bSleeping, bool bDowned, const FGrimSurvivalRules& Rules);

	/** 피곤함 변화: 깨어 있으면 증가(밤 · 2단계 체온 · 열병이면 더 빠르게), 자면 회복, 쓰러짐이면 그대로. */
	static float ComputeFatigueDelta(float Fatigue, float DeltaSeconds, bool bSleeping, bool bDowned, bool bNight, bool bModerateExposure, const FGrimSurvivalRules& Rules, bool bFever = false);

	// 지침(Q132)
	/** 빗나감 확률: 지침 문턱 아래 · 탈진이면 0, 문턱에서 TiredMissChanceMin → 탈진 문턱에서 Max 로 곧게. */
	static float ComputeTiredMissChance(float Fatigue, const FGrimSurvivalRules& Rules);

	/** 지침이면 시전 시간이 있는 마법(CastTime ≥ TiredCastTimeMinSeconds)만 × TiredCastTimeScale. 즉발 · 쿨다운은 그대로. */
	static float ComputeTiredCastTime(float CastTime, float Fatigue, const FGrimSurvivalRules& Rules);

	/** 빗나감 굴림(Roll 0~1) */
	static bool RollMiss(float Chance, float Roll) { return Chance > 0.f && Roll < Chance; }

	static FGrimSurvivalDeltas ComputeTick(const FGrimSurvivalInputs& Inputs, float DeltaSeconds, const FGrimSurvivalRules& Rules);

	// 질병
	/** 원인 확률 × (1 − DiseaseResist/100), 0~1. */
	static float ComputeDiseaseChance(float BaseChance, float DiseaseResist);

	/**
	 * 체온 질병 시계(R29 · Q115): 모자람이 심한 단계(SevereDeficit) 이상이면 시간을 쌓고, ExposureSicknessSeconds 를 채우면 true 를 돌려주며
	 * 처음부터 다시 잰다. 조건이 풀리면(모자람이 3 아래로) 0 으로. 추위(감기) · 더위(열병) 따로 쓴다.
	 */
	static bool AccumulateExposureSickness(float& AccumulatedSeconds, float DeltaSeconds, int32 DeficitGrade, const FGrimSurvivalRules& Rules);

	// 잠
	/** 잠든 수 / 접속자 수 ≥ Ratio 이고 밤이면 건너뛴다. 접속자 0 이면 false. */
	static bool ShouldSkipNight(int32 SleepingCount, int32 PlayerCount, float Ratio, bool bNight);

	/** 지금 시각에서 목표 시각까지 앞으로 가는 하루 비율(0~1, 같으면 0). */
	static float ComputeDaysUntil(float NowTimeOfDay, float TargetTimeOfDay);

	/** 건너뛴 하루 비율 → 배고픔 감소량(양수). */
	static float ComputeSkippedHungerLoss(float SkippedDays, float DayLengthSeconds, const FGrimSurvivalRules& Rules);
};
