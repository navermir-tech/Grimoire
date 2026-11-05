#pragma once

#include "CoreMinimal.h"
#include "World/GrimTerrainGenerator.h"

/** 이름 붙은 시선 하나의 결과(예: 탑 문 앞 → 고목) */
struct GRIMOIRE_API FGrimTerrainViewResult
{
	FName From;
	FName Target;
	bool bVisible = false;
	double Distance = 0.0;
};

/** 먼저 완성하는 구간(마탑 구간, 레이아웃 Analysis.Sector — docs/dev/level-design.md 「7」)의 숫자 */
struct GRIMOIRE_API FGrimTerrainSectorResult
{
	bool bValid = false;
	FName Name;
	/** 구간 안 물 밖 땅 넓이(m², 경사 샘플로 어림) */
	double LandArea = 0.0;

	// 경사 분포(구간 안 물 밖 땅)
	int32 SlopeSamples = 0;
	double RollingFraction = 0.0;
	double FlatFraction = 0.0;
	double GentleFraction = 0.0;
	double SteepFraction = 0.0;

	// 언덕(작은 모양 빼고, 가운데가 구간 안) — 가장 가까운 언덕까지 거리(가운데끼리)
	int32 HillCount = 0;
	int32 HandHillCount = 0;
	int32 SmallShapeCount = 0;
	int32 HillOutOfRange = 0;
	double HillSpacingMedian = 0.0;
	double HillSpacingMin = 0.0;
	double HillSpacingMax = 0.0;
	double HillHeightMin = 0.0;
	double HillHeightMax = 0.0;
	double HillDiameterMin = 0.0;
	double HillDiameterMax = 0.0;

	// POI — 구간 안 수 · 가장 가까운 다른 POI 까지 거리
	int32 PoiCount = 0;
	double PoiSpacingMin = 0.0;
	double PoiSpacingMax = 0.0;
	int32 PoiSpacingFailures = 0;
	TArray<FName> PoiSpacingFailNames;

	// 길(구간 안) — 밀도 · 가림 · 랜드마크
	double PathLength = 0.0;
	int32 PathSamples = 0;
	double MaxPoiDistance = 0.0;
	FVector2D MaxPoiDistanceAt = FVector2D::ZeroVector;
	int32 DensityFailures = 0;

	/** 가려진 비율 — 전체 · 거리마다(OcclusionDistances 순서) · 길 위 점 하나에서 가장 낮은 값 */
	double Occlusion = 0.0;
	TArray<double> OcclusionDistances;
	TArray<double> OcclusionByDistance;
	double OcclusionPointMin = 0.0;
	FVector2D OcclusionPointMinAt = FVector2D::ZeroVector;
	int32 OcclusionPointFailures = 0;

	int32 LandmarksMin = 0;
	double LandmarksMean = 0.0;
	int32 LandmarkFailures = 0;
	/** MinLandmarksVisible 개 이상 보이는 길 위 점의 비율 */
	double LandmarkFraction = 0.0;
	TArray<FVector2D> LandmarkFailPoints;
	/** 길 위 어딘가에서 한 번이라도 보인 랜드마크 */
	TArray<FName> LandmarksSeen;

	FString ToString() const;
};

/** 「다 만든 뒤 확인할 것」(docs/dev/level-design.md 「6」) 1~4 의 숫자 */
struct GRIMOIRE_API FGrimTerrainAnalysisResult
{
	// 1. 경사 분포(분석 사각형 안의 땅)
	int32 SlopeSamples = 0;
	double RollingFraction = 0.0;
	double FlatFraction = 0.0;
	double GentleFraction = 0.0;
	double SteepFraction = 0.0;

	// 2. 구역 평균 높이(ZoneSteps 순서)와 이웃 차
	TArray<FName> ZoneNames;
	TArray<double> ZoneMeans;
	TArray<double> ZoneSteps;

	// 3. 시선: 길 위 샘플 점마다 수평선에 보이는 POI · 랜드마크 수
	int32 PathSamples = 0;
	int32 SightFailures = 0;
	int32 MinVisible = 0;
	double MeanVisible = 0.0;
	TArray<FVector2D> SightFailPoints;

	// 4. 밀도: 길 위 점에서 가장 가까운 POI 까지 거리의 최댓값
	double MaxPoiDistance = 0.0;
	FVector2D MaxPoiDistanceAt = FVector2D::ZeroVector;
	int32 DensityFailures = 0;

	// 길 경사(개울 여울 자리 빼고)
	double MaxPathGradeDeg = 0.0;
	FVector2D MaxPathGradeAt = FVector2D::ZeroVector;
	double PathLength = 0.0;

	// 먼저 완성하는 구간(마탑 구간)
	FGrimTerrainSectorResult Sector;

	// 이름 붙은 시선(레이아웃 Analysis.ViewChecks 는 테스트가 따로 본다)
	FString ToString() const;
};

/**
 * 지형 설계 검사 — 순수 계산. 생성기 결과(높이 지도 · 길 선)와 레이아웃(구역 · 랜드마크 · POI)으로
 * 경사 분포, 구역 높이 차, 길 위 시선(높이 지도 시야), POI 밀도, 길 경사를 잰다. 기준값은 레이아웃 Analysis.
 * 나무 · 소품이 가리는 것은 보지 않는다(땅만).
 */
class GRIMOIRE_API FGrimTerrainAnalysis
{
public:
	/** 눈(지도 U, V, 높이 m)에서 목표점이 보이는가 — 사이의 땅이 시선 아래인가 */
	static bool IsVisible(const FGrimTerrainMaps& Maps, const FVector& Eye, const FVector& Target, double SampleStep = 4.0);

	/** 랜드마크 꼭대기 점(지도 U, V, 높이) */
	static FVector LandmarkTop(const FGrimTerrainMaps& Maps, const FGrimTerrainLandmark& Landmark);
	static FVector PoiTop(const FGrimTerrainLayout& Layout, const FGrimTerrainMaps& Maps, const FGrimTerrainPoi& Poi);

	/** 지도 자리에 가장 가까운 폭포(생성기 Falls) — 없으면 nullptr */
	static const FGrimTerrainWaterBody* NearestFalls(const FGrimTerrainLayout& Layout, const FGrimTerrainMaps& Maps, const FVector2D& MapCenter);

	/** 그 자리(땅 + 눈높이)에서 보이는 — FloorHeight 가 땅보다 높으면 그 위에 선다(개울 위 다리 노면 = 길 높이) — POI · 랜드마크 이름 */
	static TArray<FName> VisibleFrom(const FGrimTerrainLayout& Layout, const FGrimTerrainMaps& Maps, double U, double V, double EyeHeight, double FloorHeight = -1.0e9);

	/** 눈(지도 U, V, 높이 m)에서 보이는 랜드마크 이름(ViewDistance 안) */
	static TArray<FName> VisibleLandmarks(const FGrimTerrainLayout& Layout, const FGrimTerrainMaps& Maps, const FVector& Eye);

	/** 지도 자리가 구간 다각형 안인가 */
	static bool IsInSector(const FGrimTerrainSectorSpec& Sector, const FVector2D& P);

	/** 구간 검사(레이아웃 Analysis.Sector) — 다각형이 없으면 bValid = false */
	static FGrimTerrainSectorResult RunSector(const FGrimTerrainLayout& Layout, const FGrimTerrainMaps& Maps);

	static FGrimTerrainAnalysisResult Run(const FGrimTerrainLayout& Layout, const FGrimTerrainMaps& Maps);
};
