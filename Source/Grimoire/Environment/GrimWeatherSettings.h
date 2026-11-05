#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Environment/GrimClimateTypes.h"
#include "GrimWeatherSettings.generated.h"

class UDataTable;
class UNiagaraSystem;

/**
 * 날씨 · 기후 설정 (R04). 정본은 Config/DefaultGame.ini 의 [/Script/Grimoire.GrimWeatherSettings].
 * 지역마다 다른 값(기온 곡선 · 등급 · 전이표)은 기후 표(CSV)에, 날씨 종류 공통 값 · 예산 · 연출은 여기에 둔다.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Grimoire Weather"))
class GRIMOIRE_API UGrimWeatherSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UGrimWeatherSettings();

	virtual FName GetCategoryName() const override { return TEXT("Grimoire"); }

	const FGrimWeatherProfile& GetProfile(EGrimWeatherType Type) const;

	// ---- 기후 표

	/** 기후 표 원본 CSV. 프로젝트 Content 폴더 기준 */
	UPROPERTY(Config, EditAnywhere, Category = "Climate")
	FString ClimateTableCSV = TEXT("Grimoire/Data/ClimateRegions.csv");

	/** 지정하면 CSV 대신 이 DataTable 에셋(행 구조체 FGrimClimateRow)을 쓴다 */
	UPROPERTY(Config, EditAnywhere, Category = "Climate")
	TSoftObjectPtr<UDataTable> ClimateTableAsset;

	/** 지역 볼륨 밖의 기본 지역(탑 주변 초원). 기후 표의 행 이름 */
	UPROPERTY(Config, EditAnywhere, Category = "Climate")
	FName DefaultRegionId = TEXT("Meadow");

	/** 「맑은 한낮」 더위 가산이 붙는 시간(시, 0~24) */
	UPROPERTY(Config, EditAnywhere, Category = "Climate", meta = (ClampMin = "0", ClampMax = "24"))
	float NoonStartHour = 11.f;

	UPROPERTY(Config, EditAnywhere, Category = "Climate", meta = (ClampMin = "0", ClampMax = "24"))
	float NoonEndHour = 16.f;

	// ---- 상태 머신(서버)

	/** 최소 지속이 지난 뒤 전이를 굴리는 간격(초) */
	UPROPERTY(Config, EditAnywhere, Category = "StateMachine", meta = (ClampMin = "1"))
	float TransitionCheckIntervalSeconds = 60.f;

	/** 날씨가 바뀔 때 섞이는 시간(초). 스펙 2~3분 */
	UPROPERTY(Config, EditAnywhere, Category = "StateMachine", meta = (ClampMin = "0"))
	float BlendSeconds = 150.f;

	/** 0 이면 시작 때 시계로 씨앗을 정한다. 0 아니면 늘 같은 날씨 흐름(시험용) */
	UPROPERTY(Config, EditAnywhere, Category = "StateMachine")
	int32 RandomSeed = 0;

	// ---- 비 자극(서버)

	/** 야외 사물 · 폰에 물 자극을 주는 간격(초) */
	UPROPERTY(Config, EditAnywhere, Category = "Rain", meta = (ClampMin = "0.5"))
	float RainStimulusIntervalSeconds = 10.f;

	/** 물 자극 세기(반응 표 Water_Douse 하한 0.3 이상이어야 불이 꺼진다) */
	UPROPERTY(Config, EditAnywhere, Category = "Rain", meta = (ClampMin = "0"))
	float RainStimulusIntensity = 0.5f;

	/** 그 위치의 비 양이 이 값 이상일 때만 자극 */
	UPROPERTY(Config, EditAnywhere, Category = "Rain", meta = (ClampMin = "0", ClampMax = "1"))
	float RainStimulusMinAmount = 0.5f;

	/** 플레이어마다 이 반경(cm) 안을 오버랩해 대상을 찾는다 */
	UPROPERTY(Config, EditAnywhere, Category = "Rain", meta = (ClampMin = "100"))
	float RainStimulusRadius = 5000.f;

	/** 한 번의 비 자극에서 처리하는 대상 수 상한(지붕 트레이스 수 상한) */
	UPROPERTY(Config, EditAnywhere, Category = "Rain", meta = (ClampMin = "1"))
	int32 MaxRainTargetsPerPass = 64;

	// ---- 지붕 판정

	/** 위로 이만큼(cm) 트레이스해 막히면 지붕이 있다 */
	UPROPERTY(Config, EditAnywhere, Category = "Shelter", meta = (ClampMin = "10"))
	float ShelterTraceDistance = 3000.f;

	// ---- 낙뢰(서버)

	UPROPERTY(Config, EditAnywhere, Category = "Lightning", meta = (ClampMin = "0.5"))
	float LightningMinIntervalSeconds = 8.f;

	UPROPERTY(Config, EditAnywhere, Category = "Lightning", meta = (ClampMin = "0.5"))
	float LightningMaxIntervalSeconds = 25.f;

	/** 그 지역의 뇌우 섞임이 이 값 이상일 때만 친다 */
	UPROPERTY(Config, EditAnywhere, Category = "Lightning", meta = (ClampMin = "0", ClampMax = "1"))
	float LightningMinAmount = 0.5f;

	/** 고른 플레이어 둘레 이 반경(cm)에서 대상을 찾는다 */
	UPROPERTY(Config, EditAnywhere, Category = "Lightning", meta = (ClampMin = "100"))
	float LightningSearchRadius = 3000.f;

	/** 대상 후보 상한 */
	UPROPERTY(Config, EditAnywhere, Category = "Lightning", meta = (ClampMin = "1"))
	int32 MaxLightningCandidates = 32;

	/** 대상이 있어도 빈 땅에 칠 확률(연출만) */
	UPROPERTY(Config, EditAnywhere, Category = "Lightning", meta = (ClampMin = "0", ClampMax = "1"))
	float LightningGroundChance = 0.4f;

	/** 대상 점수: 기본 + 높이(플레이어보다 높은 m) × 가중 + 젖음 세기 × 가중 + 전도체(Material.Conductive) */
	UPROPERTY(Config, EditAnywhere, Category = "Lightning", meta = (ClampMin = "0"))
	float LightningBaseScore = 1.f;

	UPROPERTY(Config, EditAnywhere, Category = "Lightning", meta = (ClampMin = "0"))
	float LightningHeightWeightPerMeter = 0.5f;

	UPROPERTY(Config, EditAnywhere, Category = "Lightning", meta = (ClampMin = "0"))
	float LightningWetWeight = 2.f;

	UPROPERTY(Config, EditAnywhere, Category = "Lightning", meta = (ClampMin = "0"))
	float LightningConductiveWeight = 3.f;

	/** 낙뢰 자극 세기(빛). 피해 GE 는 나중 */
	UPROPERTY(Config, EditAnywhere, Category = "Lightning", meta = (ClampMin = "0"))
	float LightningStimulusIntensity = 3.f;

	// ---- 연출(클라이언트)

	/** 카메라 둘레 비 · 눈 · 모래 이펙트(임시: 엔진 기본 Niagara 템플릿을 뒤집어 씀) */
	UPROPERTY(Config, EditAnywhere, Category = "Visual")
	TSoftObjectPtr<UNiagaraSystem> RainSystem;

	UPROPERTY(Config, EditAnywhere, Category = "Visual")
	TSoftObjectPtr<UNiagaraSystem> SnowSystem;

	UPROPERTY(Config, EditAnywhere, Category = "Visual")
	TSoftObjectPtr<UNiagaraSystem> DustSystem;

	/** 낙뢰 지점에 한 번 터뜨리는 이펙트 */
	UPROPERTY(Config, EditAnywhere, Category = "Visual")
	TSoftObjectPtr<UNiagaraSystem> LightningSystem;

	/** 강수 이펙트를 카메라 위 이 높이(cm)에 둔다 */
	UPROPERTY(Config, EditAnywhere, Category = "Visual")
	float PrecipitationHeight = 600.f;

	/** 이 양 아래면 강수 이펙트를 끈다 */
	UPROPERTY(Config, EditAnywhere, Category = "Visual", meta = (ClampMin = "0", ClampMax = "1"))
	float PrecipitationVisibleThreshold = 0.15f;

	/** 번개 섬광: 하늘빛에 더하는 세기 · 길이(초) */
	UPROPERTY(Config, EditAnywhere, Category = "Visual", meta = (ClampMin = "0"))
	float LightningFlashSkyLight = 6.f;

	UPROPERTY(Config, EditAnywhere, Category = "Visual", meta = (ClampMin = "0.01"))
	float LightningFlashSeconds = 0.25f;

	// ---- 날씨 종류별 공통 값

	UPROPERTY(Config, EditAnywhere, Category = "Profiles")
	FGrimWeatherProfile ClearProfile;

	UPROPERTY(Config, EditAnywhere, Category = "Profiles")
	FGrimWeatherProfile CloudyProfile;

	UPROPERTY(Config, EditAnywhere, Category = "Profiles")
	FGrimWeatherProfile FogProfile;

	UPROPERTY(Config, EditAnywhere, Category = "Profiles")
	FGrimWeatherProfile RainProfile;

	UPROPERTY(Config, EditAnywhere, Category = "Profiles")
	FGrimWeatherProfile ThunderstormProfile;

	UPROPERTY(Config, EditAnywhere, Category = "Profiles")
	FGrimWeatherProfile SnowProfile;

	UPROPERTY(Config, EditAnywhere, Category = "Profiles")
	FGrimWeatherProfile SandstormProfile;
};
