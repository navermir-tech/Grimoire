#pragma once

#include "CoreMinimal.h"
#include "World/GrimTerrainTypes.h"

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

	int32 Index(int32 X, int32 Y) const { return Y * SizeX + X; }
	bool IsValid() const { return SizeX > 1 && SizeY > 1 && Heights.Num() == SizeX * SizeY; }
};

/**
 * 지형 1차 높이 지도 생성기 — 순수 계산(엔진 월드 · 에셋을 건드리지 않는다).
 * 노이즈 바탕 + 지역 마스크(가장자리 산등성이 · 계곡 · 탑 언덕 · 해안 · 큰 강 · 개울 · 연못 · 거점 평지) 순서로 높이를 만들고,
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

	/** 지도 좌표(U 동 · V 북, m) ↔ 월드(cm). 월드 +X = 북쪽, +Y = 동쪽 — 하늘(GrimSkyController 해돋이 Yaw 90 = +Y)과 언리얼 위에서 보기(X 위 · Y 오른쪽)에 맞춘다 */
	static FVector2D WorldToMap(const FGrimTerrainLayout& Layout, const FVector2D& WorldCm);
	static FVector MapToWorld(const FGrimTerrainLayout& Layout, double U, double V, double HeightM);

	/** 식생 규칙 하나를 뿌린 결과(월드 cm 변환). 지터 격자 + 칸마다 고정 시드 — 다시 돌려도 같다 */
	static void ScatterFoliage(const FGrimTerrainLayout& Layout, const FGrimTerrainMaps& Maps, const FGrimTerrainFoliageRule& Rule, TArray<FTransform>& OutWorld);

	static FGrimTerrainSummary MakeSummary(const FGrimTerrainLayout& Layout, const FGrimTerrainMaps& Maps);
};
