#pragma once

#include "CoreMinimal.h"
#include "World/GrimTerrainTypes.h"

/** 생성기가 만든 길 하나 — 2 m 마다 (U, V, 길 높이) */
struct GRIMOIRE_API FGrimTerrainPathLine
{
	FName Name;
	TArray<FVector> Points;
	double Length = 0.0;
};

/** 생성기가 만든 격자 데이터. 한 정점 = 지도 좌표 (X * QuadSize, Y * QuadSize) m */
struct GRIMOIRE_API FGrimTerrainMaps
{
	int32 SizeX = 0;
	int32 SizeY = 0;
	float QuadSize = 1.f;

	/** 높이(m, 해수면 기준) */
	TArray<float> Heights;
	/** 물가 · 물속(모래) 무게 0~255 — 랜드스케이프 레이어 Shore */
	TArray<uint8> Shore;
	/** 길 무게 — 레이어 Path */
	TArray<uint8> Path;
	/** 계곡 숲 바닥 무게 — 레이어 Forest */
	TArray<uint8> Forest;
	/** 가장 가까운 물가선까지(m). 물속은 음수. 멀면 잘림 */
	TArray<float> WaterDistance;
	/** 가장 가까운 길 가운데선까지(m). 멀면 잘림 */
	TArray<float> PathDistance;

	float TowerPlateauHeight = 0.f;
	TArray<FGrimTerrainWaterBody> Water;
	/** 놓인 언덕(손 + 시드 + 바위 노두) */
	TArray<FGrimTerrainPlacedHill> PlacedHills;
	TArray<FGrimTerrainPathLine> PathLines;

	int32 Index(int32 X, int32 Y) const { return Y * SizeX + X; }
	bool IsValid() const { return SizeX > 1 && SizeY > 1 && Heights.Num() == SizeX * SizeY; }
	/** 지도 좌표(m)의 가장 가까운 정점 번호(밖이면 가장자리) */
	int32 NearestIndex(double U, double V) const
	{
		const int32 X = FMath::Clamp(FMath::RoundToInt32(U / QuadSize), 0, SizeX - 1);
		const int32 Y = FMath::Clamp(FMath::RoundToInt32(V / QuadSize), 0, SizeY - 1);
		return Index(X, Y);
	}
};

/**
 * 지형 높이 지도 생성기 — 순수 계산(엔진 월드 · 에셋을 건드리지 않는다).
 * 큰 모양(구역 바닥) + 노이즈 + 중간 모양(언덕 · 바위 능선) + 작은 모양(둔턱 · 침식 골 · 바위 노두) → 가장자리 산등성이 · 고개 →
 * 계곡(한쪽 절벽) · 탑 언덕 · 해안 · 큰 강 → 길(경사를 고른 뒤 깎고 메움) → 개울(아래로만, 폭포) → 연못 · 거점 평지 순서로 높이를 만들고,
 * 물가 · 길 · 계곡 무게와 물 · 길까지 거리를 함께 낸다. 같은 레이아웃이면 언제나 같은 결과다(ParallelFor 도 정점마다 독립).
 */
class GRIMOIRE_API FGrimTerrainGenerator
{
public:
	/** 랜드스케이프가 받아 주는 크기인지(섹션 정점 수가 2 의 거듭제곱 등) */
	static bool Validate(const FGrimTerrainLayout& Layout, FString& OutError);

	/** 정점 수(구성 요소 × 섹션 × 칸 + 1) */
	static FIntPoint GetVertexCount(const FGrimTerrainLayout& Layout);

	static void Generate(const FGrimTerrainLayout& Layout, FGrimTerrainMaps& Out);

	/** 높이(m) ↔ 랜드스케이프 16비트 값. Z 배율 100 이면 1 m = 128, 32768 = 해수면 0 */
	static uint16 HeightToLandscape(float HeightM, float ZScaleCm = 100.f);
	static float LandscapeToHeight(uint16 Value, float ZScaleCm = 100.f);

	/** 지도 좌표(m)의 높이 — 두 정점 사이는 선형 보간. 밖은 가장자리 값 */
	static float SampleHeight(const FGrimTerrainMaps& Maps, double U, double V);
	/** 지도 좌표(m)의 경사(도) */
	static float SampleSlopeDeg(const FGrimTerrainMaps& Maps, double U, double V);

	/** 다각형까지 부호 있는 거리(m) — 안이면 +, 밖이면 − */
	static double PolygonSignedDistance(const TArray<FVector2D>& Polygon, const FVector2D& P);

	/** 언덕 실루엣 높이(0~1): t = 가운데 거리 / 반지름. (1 − t)^k (1 + k t) — 꼭대기 둥글고 가운데 곧고 밑동이 퍼진다 */
	static float HillProfile(float T, float Sharpness);
	/** 그 실루엣의 가장 가파른 기울기 = 높이 / 반지름 × 이 값 */
	static float HillMaxSlopeFactor(float Sharpness);

	/** 지도 좌표(U 동 · V 북, m) ↔ 월드(cm). 월드 +X = 북쪽, +Y = 동쪽 — 하늘(GrimSkyController 해돋이 Yaw 90 = +Y)과 언리얼 위에서 보기(X 위 · Y 오른쪽)에 맞춘다 */
	static FVector2D WorldToMap(const FGrimTerrainLayout& Layout, const FVector2D& WorldCm);
	static FVector MapToWorld(const FGrimTerrainLayout& Layout, double U, double V, double HeightM);

	/** 식생 규칙 하나를 뿌린 결과(월드 cm 변환). 지터 격자 + 칸마다 고정 시드 — 다시 돌려도 같다 */
	static void ScatterFoliage(const FGrimTerrainLayout& Layout, const FGrimTerrainMaps& Maps, const FGrimTerrainFoliageRule& Rule, TArray<FTransform>& OutWorld);

	static FGrimTerrainSummary MakeSummary(const FGrimTerrainLayout& Layout, const FGrimTerrainMaps& Maps);
};
