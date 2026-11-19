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

	// ── 밤 화면(Q101 「달빛 아래 실루엣이 보이는 정도」). AGrimSkyController 가 밤 섞임(FGrimTimeOfDayMath::GetNightBlend)으로 섞어 건다 —
	//    낮 · 해넘이 순간에는 0 이라 아무것도 바꾸지 않고, 해가 지평선 아래로 내려가는 동안 부드럽게 옮겨 간다. 클라이언트 화면 전용(복제 없음).

	/** 밤 자동 노출 하한(EV100). 장면이 이보다 어두우면 더 끌어올리지 않는다 — 달빛 장면은 어두운 채로 남고,
	 *  이보다 밝은 곳(등불 켠 실내 · 모닥불 곁)은 지금처럼 자동 노출이 맞춘다. 엔진 기본은 -10(끝없이 끌어올림 → 밤이 낮처럼 보임, BUG-008).
	 *  낮추면 밤이 밝아지고 올리면 어두워진다(1 = 두 배) */
	UPROPERTY(Config, EditAnywhere, Category = "Night", meta = (ClampMin = "-10", ClampMax = "10"))
	float NightExposureMinEV100 = -2.f;

	/** 해가 막 진 때의 노출 하한(EV100) — 여기서 밤 섞임을 따라 NightExposureMinEV100 으로 오른다. 해넘이 장면의 평균 노출
	 *  (L_OpenBeta 큰길에서 약 -5.3, 2026-11-18 VisualizeHDR)보다 조금 낮게 둬서 노을에서 어스름으로 넘어갈 때 화면이 튀지 않게 한다 */
	UPROPERTY(Config, EditAnywhere, Category = "Night", meta = (ClampMin = "-10", ClampMax = "10"))
	float TwilightExposureMinEV100 = -5.5f;

	/** 밤 화면 전체 채도(1 = 그대로) */
	UPROPERTY(Config, EditAnywhere, Category = "Night", meta = (ClampMin = "0", ClampMax = "1"))
	float NightSaturation = 0.85f;

	/** 밤 어두운 영역 채도 — 달빛만 받는 땅 · 나무는 색이 빠지고, 불빛을 받아 밝은 곳은 색이 남는다 */
	UPROPERTY(Config, EditAnywhere, Category = "Night", meta = (ClampMin = "0", ClampMax = "1"))
	float NightShadowSaturation = 0.4f;

	/** 실내(카메라 위에 지붕 + 둘레 벽)에서는 밤 화면을 뺀다 — 등불 켠 탑 안은 밤에도 예전 그대로. 수평 트레이스 길이(cm) */
	UPROPERTY(Config, EditAnywhere, Category = "Night|Indoor", meta = (ClampMin = "100"))
	float IndoorProbeDistance = 4000.f;

	/** 수평 트레이스 방향 수 · 그중 막혀야 하는 수(나무 아래 같은 반쯤 트인 곳은 실외로) */
	UPROPERTY(Config, EditAnywhere, Category = "Night|Indoor", meta = (ClampMin = "1", ClampMax = "32"))
	int32 IndoorProbeDirections = 8;

	UPROPERTY(Config, EditAnywhere, Category = "Night|Indoor", meta = (ClampMin = "1", ClampMax = "32"))
	int32 IndoorWallsRequired = 6;

	/** 실내 판정 간격(초) · 바뀌었을 때 밤 화면을 빼고 넣는 시간(초) */
	UPROPERTY(Config, EditAnywhere, Category = "Night|Indoor", meta = (ClampMin = "0"))
	float IndoorProbeIntervalSeconds = 0.25f;

	UPROPERTY(Config, EditAnywhere, Category = "Night|Indoor", meta = (ClampMin = "0"))
	float IndoorBlendSeconds = 1.f;

	/** 「어두운 영역」으로 치는 밝기 상한(노출을 건 뒤의 선형 밝기, 중간 회색 = 0.18). 엔진 기본 0.09 */
	UPROPERTY(Config, EditAnywhere, Category = "Night", meta = (ClampMin = "0.01", ClampMax = "1"))
	float NightShadowsMax = 0.3f;
};
