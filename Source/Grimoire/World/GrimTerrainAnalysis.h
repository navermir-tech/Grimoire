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

	static FGrimTerrainAnalysisResult Run(const FGrimTerrainLayout& Layout, const FGrimTerrainMaps& Maps);
};
