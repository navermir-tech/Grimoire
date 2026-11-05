#pragma once

#include "CoreMinimal.h"
#include "GrimTerrainTypes.generated.h"

GRIMOIRE_API DECLARE_LOG_CATEGORY_EXTERN(LogGrimTerrain, Log, All);

// 지형 1차(M3 갈래 W) — 높이 지도 생성기가 읽는 레이아웃. 정본은 Scripts/Terrain/openbeta_layout.json(이 구조체의 JSON).
// 단위: 거리 · 높이는 미터. 좌표는 "지도 좌표" — 왼쪽 아래(남서) 모서리가 (0, 0), +X(U) = 동쪽, +Y(V) = 북쪽.
// 게임 월드 좌표(cm): 월드 +X = 북쪽(지도 V), +Y = 동쪽(지도 U). 지도 좌표에서 WorldOrigin 을 뺀 뒤 100 을 곱한다(마법탑이 월드 원점).
// 하늘(GrimSkyController — 해돋이 Yaw 90 = +Y)과 언리얼 위에서 보기(위 = +X, 오른쪽 = +Y)가 북 · 동과 맞는다.
// 랜드스케이프는 월드 축을 따르므로 가져올 때 높이 지도를 바꿔 놓는다(로컬 X = 북쪽 4 km, Y = 동쪽 2 km).

/** 남쪽 언덕(마법탑 자리) — 꼭대기는 반지름 PlateauRadius 까지 완전한 평지 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimTerrainHill
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FVector2D Center = FVector2D(1000.0, 380.0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float PlateauRadius = 45.f;
	/** 언덕 비탈이 둘레 땅과 만나는 반지름 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float Radius = 230.f;
	/** 꼭대기 평지 높이(해수면 기준) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float Height = 42.f;
	/** 비탈 반지름의 들쭉날쭉한 정도(0~0.5) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float Irregularity = 0.15f;
};

/** 연못 — 낮은 곳에 대접 모양으로 판다. 물 높이는 둘레 땅 높이에서 정한다 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimTerrainPond
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FName Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FVector2D Center = FVector2D::ZeroVector;
	/** 물가선 반지름 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float Radius = 40.f;
	/** 가운데 물 깊이 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float Depth = 3.f;
	/** 물가선 밖으로 땅을 다듬는 폭(모래 · 둔덕) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float ShoreWidth = 14.f;
	/** 둘레 땅보다 물이 얼마나 낮은가 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float WaterBelowGround = 0.8f;
};

/** 강 · 개울 · 계곡 · 길처럼 꺾은선을 따라가는 지형 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimTerrainChannel
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FName Name;
	/** 위(상류) → 아래(하류) 순서 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") TArray<FVector2D> Points;
	/** 바닥(물) 폭 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float Width = 6.f;
	/** 물 높이 아래로 판 깊이(계곡이면 둘레보다 낮춘 깊이) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float Depth = 1.f;
	/** 둑 · 비탈 폭 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float BankWidth = 8.f;
	/** 개울: 둘레 땅보다 물이 얼마나 낮은가. 강은 해수면을 쓴다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float WaterBelowGround = 1.f;
};

/** 거점 세울 만한 평지 — 둘레를 평평하게 고른다 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimTerrainSite
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FName Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FVector2D Center = FVector2D::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float Radius = 30.f;
	/** 평지 밖으로 둘레 땅과 이어 주는 폭 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float BlendWidth = 25.f;
};

/** 표식(서식 구역 등) — 지형은 바꾸지 않고 자리만 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimTerrainMarkerSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FName Name;
	/** GameplayTag 이름(예: Habitat.Jackalope) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FName Tag;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FVector2D Center = FVector2D::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float Radius = 100.f;
};

/** 식생 규칙 하나(나무 · 덤불 · 바위). 밀도는 1 헥타르(100 × 100 m)당 개수 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimTerrainFoliageRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FName Name;
	/** 초원(숲 무늬 · 계곡 밖) 밀도 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float DensityPerHa = 2.f;
	/** 숲 무늬(노이즈가 문턱을 넘는 곳) 밀도 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float ForestDensityPerHa = 0.f;
	/** 계곡 안 밀도 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float ValleyDensityPerHa = 0.f;
	/** 가파른 비탈(SteepSlopeDeg 이상) 밀도 — 바위용 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float SteepDensityPerHa = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float SteepSlopeDeg = 30.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float MaxSlopeDeg = 30.f;
	/** 해수면 기준 높이 범위 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float MinHeight = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float MaxHeight = 200.f;
	/** 물가(연못 · 강 · 개울 · 바다)에서 이만큼은 떨어진다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float MinWaterDistance = 5.f;
	/** 길에서 이만큼은 떨어진다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float MinPathDistance = 4.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float MinScale = 0.8f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float MaxScale = 1.3f;
	/** 땅속으로 묻는 깊이(cm, 크기 1 기준) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float SinkCm = 0.f;
};

USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimTerrainLayout
{
	GENERATED_BODY()

	/** 같은 시드 · 같은 레이아웃이면 언제나 같은 지형 · 같은 식생 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") int32 Seed = 1;

	// ── 랜드스케이프 크기: 구성 요소 = SectionsPerComponent² 개 섹션, 섹션 = SectionQuads 칸 ──
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") int32 SectionQuads = 127;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") int32 SectionsPerComponent = 2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") int32 ComponentsX = 8;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") int32 ComponentsY = 16;
	/** 한 칸(정점 간격) 크기(m) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float QuadSize = 1.f;
	/** 이 지도 좌표가 월드 원점(0, 0)이 된다 — 마법탑 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FVector2D WorldOrigin = FVector2D(1000.0, 380.0);

	// ── 초원 바탕 ──
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float BaseHeight = 22.f;
	/** 북쪽으로 갈수록 오르는 높이(NorthRiseStart~End 사이에서) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float NorthRise = 16.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float NorthRiseStart = 600.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float NorthRiseEnd = 3600.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float NoiseAmplitude = 7.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float NoiseScale = 380.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") int32 NoiseOctaves = 5;
	/** 중간 크기 언덕(구릉) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float RollingAmplitude = 3.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float RollingScale = 150.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float DetailAmplitude = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float DetailScale = 24.f;

	// ── 가장자리 산등성이(지도 끝을 막는다). 동쪽은 큰 강 건너편 ──
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float WestRidgeWidth = 230.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float WestRidgeHeight = 110.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float NorthRidgeWidth = 280.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float NorthRidgeHeight = 140.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float EastRidgeWidth = 150.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float EastRidgeHeight = 75.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float RidgeNoiseScale = 260.f;
	/** 산등성이 폭이 들쭉날쭉한 정도(0~0.8) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float RidgeWobble = 0.5f;

	// ── 해안(남쪽 바다) ──
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float SeaLevel = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float SeaDepth = 25.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float SeaSlopeWidth = 180.f;
	/** 물가선의 평균 V */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float CoastLine = 230.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float CoastWobble = 45.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float CoastWobbleScale = 420.f;
	/** 모래밭 폭(물가선 → 땅 높이) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float CoastWidth = 120.f;
	/** 탑 언덕 앞의 곶 — 물가선을 남쪽으로 밀고 절벽으로 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float HeadlandBulge = 70.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float HeadlandWidth = 160.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float CliffWidth = 55.f;

	// ── 지형 요소 ──
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FGrimTerrainHill TowerHill;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FGrimTerrainChannel Valley;
	/** 큰 강 — 물은 해수면, 바다로 이어진다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FGrimTerrainChannel River;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") TArray<FGrimTerrainChannel> Creeks;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") TArray<FGrimTerrainPond> Ponds;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") TArray<FGrimTerrainChannel> Paths;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") TArray<FGrimTerrainSite> BaseSites;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") TArray<FGrimTerrainMarkerSpec> Markers;

	// ── 식생 ──
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") TArray<FGrimTerrainFoliageRule> Foliage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float ForestNoiseScale = 340.f;
	/** 숲 노이즈(-1~1)가 이 값을 넘으면 숲 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float ForestThreshold = 0.18f;
	/** 탑 평지 둘레로 식생을 비우는 여유 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float TowerClearance = 40.f;
};

/** 생성기가 계산한 물 하나(연못 · 개울 · 강 · 바다) — 물 평면 · 물 볼륨을 놓는 데 쓴다 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimTerrainWaterBody
{
	GENERATED_BODY()

	/** Pond · Creek · River · Sea */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain") FName Kind;
	UPROPERTY(BlueprintReadOnly, Category = "Terrain") FName Name;
	/** 월드 좌표(cm). 연못은 가운데 하나, 개울 · 강은 상류 → 하류 점들. Z = 물 높이 */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain") TArray<FVector> Points;
	/** 연못 = 물 반지름, 개울 · 강 = 물 폭(cm) */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain") float Size = 0.f;
	/** 물 높이 아래 가장 깊은 곳까지(cm) */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain") float Depth = 0.f;
};

/** 생성 결과 요약(파이썬 · 로그용) */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimTerrainSummary
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Terrain") int32 SizeX = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Terrain") int32 SizeY = 0;
	/** 랜드스케이프 액터 위치(cm) — 지도 (0, 0) 정점 */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain") FVector LandscapeLocation = FVector::ZeroVector;
	/** 랜드스케이프 크기(배율, cm) */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain") FVector LandscapeScale = FVector(100.0);
	UPROPERTY(BlueprintReadOnly, Category = "Terrain") float MinHeight = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "Terrain") float MaxHeight = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "Terrain") float TowerPlateauHeight = 0.f;
	/** 지도 크기(m) */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain") FVector2D MapSize = FVector2D::ZeroVector;
	UPROPERTY(BlueprintReadOnly, Category = "Terrain") TArray<FGrimTerrainWaterBody> Water;
};
