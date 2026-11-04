#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GrimEnvironmentSettings.generated.h"

/**
 * 환경 설정 (R04). 정본은 Config/DefaultGame.ini 의 [/Script/Grimoire.GrimEnvironmentSettings].
 * 서버 값이 기준이다 — 하루 길이 · 낮 비율은 서버가 복제해 클라이언트가 따른다.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Grimoire Environment"))
class GRIMOIRE_API UGrimEnvironmentSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("Grimoire"); }

	/** 하루(낮 + 밤) 실시간 길이(분). Q6 — 기본 48. */
	UPROPERTY(Config, EditAnywhere, Category = "Time", meta = (ClampMin = "0.1"))
	float DayLengthMinutes = 48.f;

	/** 하루 중 낮의 비율. 0.625 = 낮 30분 · 밤 18분(48분 기준). 낮은 정오(0.5)를 가운데로 둔다. */
	UPROPERTY(Config, EditAnywhere, Category = "Time", meta = (ClampMin = "0.05", ClampMax = "0.95"))
	float DayRatio = 0.625f;

	/** 서버 시작 시각(하루 비율, 0 = 자정). 0.3 = 07:12. */
	UPROPERTY(Config, EditAnywhere, Category = "Time", meta = (ClampMin = "0", ClampMax = "1"))
	float StartTimeOfDay = 0.3f;

	/** 서버가 시간 스냅숏을 다시 복제하는 간격(초). 사이에는 클라이언트가 외삽한다. */
	UPROPERTY(Config, EditAnywhere, Category = "Time", meta = (ClampMin = "1"))
	float ResyncIntervalSeconds = 10.f;

	/** 새 스냅숏과 외삽 값의 차이를 부드럽게 메우는 시간(초). */
	UPROPERTY(Config, EditAnywhere, Category = "Time", meta = (ClampMin = "0"))
	float ClientCorrectionSeconds = 1.f;

	/** 전역 기온 곡선(M1): 가장 추운 시각에 최저, 가장 더운 시각에 최고, 사이는 코사인. 섭씨. */
	UPROPERTY(Config, EditAnywhere, Category = "Temperature")
	float MinAmbientTemperature = 6.f;

	UPROPERTY(Config, EditAnywhere, Category = "Temperature")
	float MaxAmbientTemperature = 22.f;

	/** 0.2 = 04:48 */
	UPROPERTY(Config, EditAnywhere, Category = "Temperature", meta = (ClampMin = "0", ClampMax = "1"))
	float ColdestTimeOfDay = 0.2f;

	/** 0.6 = 14:24 */
	UPROPERTY(Config, EditAnywhere, Category = "Temperature", meta = (ClampMin = "0", ClampMax = "1"))
	float WarmestTimeOfDay = 0.6f;
};
