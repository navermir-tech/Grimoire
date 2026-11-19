#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "InputCoreTypes.h"
#include "GrimSurvivalSettings.generated.h"

/** 생존 기본 수치 (R03) — HP · 배고픔 · 체온 게이지. */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimSurvivalTuning
{
	GENERATED_BODY()

	// 초기값
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Initial")
	float InitialMaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Initial")
	float InitialHunger = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Initial")
	float InitialBodyTemperature = 37.f;

	// 주기
	/** 생존 틱 간격(초). 이 간격마다 서버가 GE 로 배고픔 · HP · 체온 · 피곤함 변화를 적용한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tick", meta = (ClampMin = "0.1"))
	float TickIntervalSeconds = 1.f;

	// 배고픔
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hunger")
	float MaxHunger = 100.f;

	/** 분당 감소량. 1 이면 가득 찬 배가 100분(하루 48분 기준 약 이틀) 간다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hunger", meta = (ClampMin = "0"))
	float HungerDrainPerMinute = 1.f;

	/** 이 값 미만이면 Status.Hungry(HP 자연 회복 정지). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hunger")
	float HungryThreshold = 20.f;

	// HP
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "0"))
	float HealthRegenPerSecond = 0.5f;

	/** 배고픔 0(Status.Starving)일 때 초당 HP 감소. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "0"))
	float StarvingDamagePerSecond = 0.5f;

	/** 체온 모자람 3 이상(.Severe)일 때 초당 HP 감소. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "0"))
	float ExposureDamagePerSecond = 0.2f;

	// 체온 게이지 (판정은 등급 — FGrimClimateTuning)
	/** 모자람 0 일 때의 목표 체온. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BodyTemperature")
	float NormalBodyTemperature = 37.f;

	/** 수렴 시정수(초): 이만큼 지나면 목표와의 차이가 약 37% 로 준다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BodyTemperature", meta = (ClampMin = "1"))
	float BodyTemperatureTimeConstantSeconds = 120.f;

	/** 어트리뷰트 클램프. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BodyTemperature")
	float MinBodyTemperature = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BodyTemperature")
	float MaxBodyTemperature = 45.f;

	/** 게이지의 안전 범위 표시(HUD 색). 상태 판정은 모자람 등급으로 한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BodyTemperature")
	float HypothermiaThreshold = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BodyTemperature")
	float HyperthermiaThreshold = 39.f;

	/** 이 반경(cm) 안의 State.Burning 액터가 열원이다(「열원 근처」 추위 감산). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BodyTemperature", meta = (ClampMin = "0"))
	float HeatSourceRadius = 600.f;
};

/**
 * 더위 · 추위 등급 (스펙 「체온 모델 — 더위 · 추위 등급과 저항」). 날씨 몫은 UGrimWeatherSubsystem::GetClimateExposure,
 * 개인 몫 · 모자람 → 목표 체온 · 패널티는 여기. (나중에 DT_ClimateExposure 로 옮길 수 있다.)
 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimClimateTuning
{
	GENERATED_BODY()

	// 개인 몫
	/** State.Wet 이면 추위 +. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Personal")
	int32 WetCold = 1;

	/** State.Burning(자기 몸)이면 더위 +. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Personal")
	int32 BurningHeat = 2;

	/** HeatSourceRadius 안에 불붙은 것이 있으면 추위 −. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Personal")
	int32 NearHeatSourceCold = 2;

	/** State.Swimming 이면 더위 −(물속). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Personal")
	int32 SwimmingHeat = 1;

	// 모자람 → 체온 · 단계
	/** 모자람 1 당 목표 체온 변화(℃). 목표 = 정상 − 추위 모자람 × 이 값 + 더위 모자람 × 이 값. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deficit", meta = (ClampMin = "0"))
	float BodyTemperaturePerDeficit = 1.5f;

	/** 이 모자람부터 Status.Hypothermia / Hyperthermia(2단계). 그 아래(1 이상)는 Chilly / Warm. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deficit", meta = (ClampMin = "1"))
	int32 ModerateDeficit = 2;

	/** 이 모자람부터 .Severe(HP 감소). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Deficit", meta = (ClampMin = "1"))
	int32 SevereDeficit = 3;

	// 패널티 (배율 — 1 = 그대로)
	/** 추위 1단계: 이동 · 시전 속도 배율. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Penalty", meta = (ClampMin = "0"))
	float ChillyMoveSpeedScale = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Penalty", meta = (ClampMin = "0"))
	float ChillyCastSpeedScale = 0.9f;

	/** 추위 2단계 이상. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Penalty", meta = (ClampMin = "0"))
	float HypothermiaMoveSpeedScale = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Penalty", meta = (ClampMin = "0"))
	float HypothermiaCastSpeedScale = 0.75f;

	/** 더위 1단계: 마나 소모 배율. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Penalty", meta = (ClampMin = "0"))
	float WarmManaCostScale = 1.15f;

	/** 더위 2단계 이상. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Penalty", meta = (ClampMin = "0"))
	float HyperthermiaManaCostScale = 1.35f;

	/** 2단계 이상(어느 쪽이든)일 때 피곤함 증가 배율. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Penalty", meta = (ClampMin = "0"))
	float ModerateFatigueRateScale = 1.5f;
};

/** 피곤함 (R03). */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimFatigueTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fatigue")
	float MaxFatigue = 100.f;

	/** 깨어 있을 때 분당 증가. 1.5 면 낮 30분 + 밤 18분(×1.5)을 꼬박 새면 약 85. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fatigue", meta = (ClampMin = "0"))
	float GainPerMinute = 1.5f;

	/** 밤에는 이 배율로 쌓인다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fatigue", meta = (ClampMin = "0"))
	float NightGainScale = 1.5f;

	/** 이 값 이상(탈진 전까지)이면 Status.Tired 「지침」(Q132): 시전 시간이 있는 마법이 느려지고, 맞혀도 가끔 빗나간다(「MISS」). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fatigue")
	float TiredThreshold = 50.f;

	/** 지침: 시전 시간(정의의 CastWindup)에 곱한다. 쿨다운은 바꾸지 않는다(Q132). 수치 제안 ×1.3(Q165) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fatigue", meta = (ClampMin = "1"))
	float TiredCastTimeScale = 1.3f;

	/** 시전 시간이 이 값(초) 이상인 마법만 「시전 시간이 있는 마법」 — 그보다 짧은 시전 대기는 즉발로 보고 늘리지 않는다(Q132 · Q166) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fatigue", meta = (ClampMin = "0", Units = "s"))
	float TiredCastTimeMinSeconds = 0.25f;

	/** 지침 빗나감 확률: 피곤함 TiredThreshold 에서 Min, 탈진 문턱 바로 아래에서 Max 로 곧게(제안 10 % → 30 % — Q165) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fatigue", meta = (ClampMin = "0", ClampMax = "1"))
	float TiredMissChanceMin = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fatigue", meta = (ClampMin = "0", ClampMax = "1"))
	float TiredMissChanceMax = 0.3f;

	/** 이 값 이상이면 Status.Exhausted. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fatigue")
	float ExhaustedThreshold = 80.f;

	/** Exhausted(탈진) 이동 속도 배율 — 지친 걸음(R29 · Q116). 탈진은 달리기 불가 · 마법 시전 불가(시전 차단은 마법 쪽 MagicBlockingTags). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fatigue", meta = (ClampMin = "0"))
	float ExhaustedMoveSpeedScale = 0.7f;

	/** 자는 동안 초당 회복. 밤 건너뛰기는 한 번에 0 으로. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fatigue", meta = (ClampMin = "0"))
	float SleepRecoveryPerSecond = 5.f;
};

/** 질병 (R03 · R29) — 배탈 · 감기 · 열병. 빛(신성) 치료 · 약초는 나중, 지금은 시간으로 낫는다. */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimDiseaseTuning
{
	GENERATED_BODY()

	/** 어트리뷰트 DiseaseResist(0~100) 초기값. 확률 × (1 − 저항/100). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Disease")
	float InitialDiseaseResist = 0.f;

	// 배탈 Status.Sick.Stomach — 날것 섭취
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stomach", meta = (ClampMin = "0", ClampMax = "1"))
	float RawFoodStomachChance = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stomach", meta = (ClampMin = "0"))
	float StomachDurationSeconds = 180.f;

	/** 배탈 중 배고픔 감소 배율. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stomach", meta = (ClampMin = "0"))
	float StomachHungerDrainScale = 2.f;

	// 체온 질병(R29 · Q115): 모자람이 심한 단계(Climate.SevereDeficit, 3) 이상인 채로 이만큼 이어지면 — 추위는 감기, 더위는 열병.
	// 모자람이 그 아래로 내려가면 시계는 처음부터. 걸리면(또는 이미 앓고 있으면) 다시 처음부터 잰다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exposure", meta = (ClampMin = "1", Units = "s"))
	float ExposureSicknessSeconds = 300.f;

	/** 시간이 차면 걸릴 확률(× (1 − 질병 저항/100)). 1 = 반드시 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exposure", meta = (ClampMin = "0", ClampMax = "1"))
	float ExposureSicknessChance = 1.f;

	// 감기 Status.Sick.Cold — 오래 추움
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cold", meta = (ClampMin = "0"))
	float ColdDurationSeconds = 300.f;

	/** 감기 중 초당 마나 회복 감소(회복이 도는 동안만 — 배고프면 회복이 이미 멈춰 있다). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cold", meta = (ClampMin = "0"))
	float ColdManaRegenPenaltyPerSecond = 1.f;

	// 열병 Status.Sick.Fever — 오래 더움(감기와 대칭 제안: 같은 시간으로 낫고, 「열 = 체온 ↑」 + 몸이 빨리 지친다)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fever", meta = (ClampMin = "0"))
	float FeverDurationSeconds = 300.f;

	/** 열병 중 목표 체온에 더한다(℃) — 게이지가 올라 보인다(판정은 등급이라 패널티 단계는 그대로) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fever", meta = (ClampMin = "0"))
	float FeverBodyTemperatureRise = 1.5f;

	/** 열병 중 피곤함이 쌓이는 배율 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fever", meta = (ClampMin = "1"))
	float FeverFatigueRateScale = 1.5f;
};

/** 잠 · 밤 건너뛰기. */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimSleepTuning
{
	GENERATED_BODY()

	/** 서버 옵션(Q114): 접속자 중 잠든 비율이 이 값 이상이면 밤을 건너뛴다. 기본 1 = 전원(밤이 있다), 서버 주인만 낮춘다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sleep", meta = (ClampMin = "0.01", ClampMax = "1"))
	float SleepSkipRatio = 1.f;

	/** 건너뛰기 전 페이드(초). 이벤트만 낸다 — 화면 연출은 UI 몫. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sleep", meta = (ClampMin = "0"))
	float SkipFadeSeconds = 3.f;

	/** 건너뛴 시간(실시간 환산 분) × 분당 배고픔 감소 × 이 값만큼 배고픔이 준다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sleep", meta = (ClampMin = "0"))
	float SkippedHungerScale = 0.5f;

	/** 해 뜬 뒤 조금 지난 시각으로(하루 비율). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sleep", meta = (ClampMin = "0"))
	float WakeAfterDawn = 0.005f;

	/** 이 반경(cm) 안에 적대(IGrimTargetable 종류 Enemy)가 있으면 잘 수 없고, 자다가도 깬다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sleep", meta = (ClampMin = "0"))
	float HostileBlockRadius = 1500.f;

	/** 침대 · 침낭과 이 거리(cm) 안이면 눕기 키가 뜬다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sleep", meta = (ClampMin = "0"))
	float BedInteractRadius = 250.f;

	/** 침대 근처에서 누르는 키(눕기 · 일어나기). E · Q 는 마법 고르기라 F. 캐릭터 상호작용 키가 생기면 그쪽으로 옮긴다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sleep")
	FKey BedInteractKey = EKeys::F;

	/** 자는 동안 · 기절 중 이동 속도 배율. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sleep", meta = (ClampMin = "0"))
	float SleepingMoveSpeedScale = 0.f;
};

/** 쓰러짐 · 죽음 · 부활 (스펙 「죽음」 제안). */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimDownedTuning
{
	GENERATED_BODY()

	/** HP 0 → Status.Downed 지속(초). 그 안에 일으키지 않으면 사망 → 부활. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Downed", meta = (ClampMin = "0"))
	float DownedSeconds = 60.f;

	/** 쓰러진 동안 이동(기어가기) 배율. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Downed", meta = (ClampMin = "0"))
	float DownedMoveSpeedScale = 0.25f;

	/** 동료가 일으키면 MaxHealth × 이 값. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Downed", meta = (ClampMin = "0.01", ClampMax = "1"))
	float ReviveHealthFraction = 0.3f;

	/** 부활하면 MaxHealth × 이 값. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Downed", meta = (ClampMin = "0.01", ClampMax = "1"))
	float RespawnHealthFraction = 0.5f;

	/** 부활 때 배고픔이 이보다 낮으면 여기까지 채운다(굶어 죽고 바로 또 죽지 않게). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Downed", meta = (ClampMin = "0"))
	float RespawnMinHunger = 30.f;
};

/** 순수 계산(FGrimSurvivalModel)과 테스트가 받는 수치 묶음. 설정의 각 줄을 모은 것. */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimSurvivalRules
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Survival")
	FGrimSurvivalTuning Vital;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Survival")
	FGrimClimateTuning Climate;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Survival")
	FGrimFatigueTuning Fatigue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Survival")
	FGrimDiseaseTuning Disease;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Survival")
	FGrimSleepTuning Sleep;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Survival")
	FGrimDownedTuning Downed;
};

/** 생존 설정. 정본은 Config/DefaultGame.ini 의 [/Script/Grimoire.GrimSurvivalSettings] — 묶음마다 한 줄. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Grimoire Survival"))
class GRIMOIRE_API UGrimSurvivalSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("Grimoire"); }

	/** 설정 줄들을 한 묶음으로(값 복사). */
	FGrimSurvivalRules MakeRules() const;

	static FGrimSurvivalRules GetRules() { return GetDefault<UGrimSurvivalSettings>()->MakeRules(); }

	UPROPERTY(Config, EditAnywhere, Category = "Survival")
	FGrimSurvivalTuning Tuning;

	UPROPERTY(Config, EditAnywhere, Category = "Survival")
	FGrimClimateTuning Climate;

	UPROPERTY(Config, EditAnywhere, Category = "Survival")
	FGrimFatigueTuning Fatigue;

	UPROPERTY(Config, EditAnywhere, Category = "Survival")
	FGrimDiseaseTuning Disease;

	UPROPERTY(Config, EditAnywhere, Category = "Survival")
	FGrimSleepTuning Sleep;

	UPROPERTY(Config, EditAnywhere, Category = "Survival")
	FGrimDownedTuning Downed;
};
