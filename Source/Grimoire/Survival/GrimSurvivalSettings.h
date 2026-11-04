#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GrimSurvivalSettings.generated.h"

/** 생존 수치 (R03). 순수 계산(FGrimSurvivalModel)과 테스트가 이 구조체만 받는다. */
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
	/** 생존 틱 간격(초). 이 간격마다 서버가 GE 하나로 배고픔 · HP · 체온 변화를 적용한다. */
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

	/** 저체온 · 과열일 때 초당 HP 감소. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "0"))
	float ExposureDamagePerSecond = 0.2f;

	// 체온
	/** 쾌적 체감 기온에서의 목표 체온. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BodyTemperature")
	float NormalBodyTemperature = 37.f;

	/** 이 체감 기온이면 목표 체온 = 정상 체온. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BodyTemperature")
	float ComfortTemperature = 20.f;

	/** 체감 기온 1℃ 차이당 목표 체온 변화. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BodyTemperature", meta = (ClampMin = "0"))
	float BodyTemperatureSensitivity = 0.25f;

	/** 수렴 시정수(초): 이만큼 지나면 목표와의 차이가 약 37% 로 준다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BodyTemperature", meta = (ClampMin = "1"))
	float BodyTemperatureTimeConstantSeconds = 120.f;

	/** 어트리뷰트 클램프. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BodyTemperature")
	float MinBodyTemperature = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BodyTemperature")
	float MaxBodyTemperature = 45.f;

	/** 안전 범위. 미만 Status.Hypothermia, 초과 Status.Hyperthermia. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BodyTemperature")
	float HypothermiaThreshold = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BodyTemperature")
	float HyperthermiaThreshold = 39.f;

	// 열원 · 젖음
	/** 이 반경(cm) 안의 State.Burning 액터가 열원이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heat", meta = (ClampMin = "0"))
	float HeatSourceRadius = 600.f;

	/** Burning 세기 1 이 바로 옆에서 올리는 체감 기온(℃). 거리에 따라 반경 끝에서 0 까지 선형으로 준다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heat", meta = (ClampMin = "0"))
	float WarmthPerBurningIntensity = 15.f;

	/** State.Wet 세기 1 이 내리는 체감 기온(℃). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Heat", meta = (ClampMin = "0"))
	float CoolingPerWetIntensity = 4.f;
};

/** 생존 설정. 정본은 Config/DefaultGame.ini 의 [/Script/Grimoire.GrimSurvivalSettings]. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Grimoire Survival"))
class GRIMOIRE_API UGrimSurvivalSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("Grimoire"); }

	UPROPERTY(Config, EditAnywhere, Category = "Survival", meta = (ShowOnlyInnerProperties))
	FGrimSurvivalTuning Tuning;
};
