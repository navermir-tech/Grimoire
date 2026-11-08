#pragma once

#include "CoreMinimal.h"
#include "GrimTerrainTypes.generated.h"

GRIMOIRE_API DECLARE_LOG_CATEGORY_EXTERN(LogGrimTerrain, Log, All);

// 지형(M3 갈래 W, 2차 다시 만들기) — 높이 지도 생성기가 읽는 레이아웃. 정본은 Scripts/Terrain/openbeta_layout.json(이 구조체의 JSON).
// 단위: 거리 · 높이는 미터. 좌표는 "지도 좌표" — 왼쪽 아래(남서) 모서리가 (0, 0), +X(U) = 동쪽, +Y(V) = 북쪽.
// 게임 월드 좌표(cm): 월드 +X = 북쪽(지도 V), +Y = 동쪽(지도 U). 지도 좌표에서 WorldOrigin 을 뺀 뒤 100 을 곱한다(마법탑이 월드 원점).
// 하늘(GrimSkyController — 해돋이 Yaw 90 = +Y)과 언리얼 위에서 보기(위 = +X, 오른쪽 = +Y)가 북 · 동과 맞는다.
// 랜드스케이프는 월드 축을 따르므로 가져올 때 높이 지도를 바꿔 놓는다(로컬 X = 북쪽 4 km, Y = 동쪽 2 km).
// 설계 기준(왜 이 모양인가)은 docs/dev/level-design.md, 만드는 법은 docs/systems/terrain.md.

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

/** 구역(큰 모양) — 다각형 안의 바닥 높이. 레이아웃 순서대로 덮는다(뒤 구역이 앞 구역을 덮는다) */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimTerrainZone
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FName Name;
	/** 꼭짓점(지도 좌표). 시계 · 반시계 상관없음 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") TArray<FVector2D> Polygon;
	/** 바닥 높이(다각형 남쪽 끝) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float BaseHeight = 26.f;
	/** 다각형 남쪽 끝 → 북쪽 끝으로 더 오르는 높이 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float NorthRise = 0.f;
	/** 경계에서 둘레와 섞는 폭(경계선 안팎으로 절반씩) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float BlendWidth = 200.f;
	/** 경계선을 들쭉날쭉하게 미는 거리(±m) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float EdgeWobble = 40.f;
	/** 이 구역에만 더하는 잔물결(±m) · 파장(m) — 언덕 사이 바닥이 평평하지 않게(마탑 구간). 0 이면 없음. 덮는 순서와 상관없이 가장 센 구역 것을 쓴다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float ExtraRollingAmplitude = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float ExtraRollingScale = 60.f;
	/**
	 * 레벨 디자인 2차: 이 구역에만 더하는 「뒤튼 능선 노이즈」(±m 의 대략 두 배 폭) · 파장(m) · 뒤틀기(0~1.5).
	 * 능선형 다중 프랙탈(1 − |펄린|)² 을 저주파 노이즈로 뒤틀어(domain warp) 둥근 언덕이 줄지어 늘어서지 않고 등성이 · 골 · 안부가 이어진 땅을 만든다.
	 * 0 이면 없음. 잔물결처럼 덮는 순서와 상관없이 가장 센 구역 것을 쓴다. docs/systems/terrain.md 「높이를 만드는 순서」
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float RidgedAmplitude = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float RidgedScale = 220.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float WarpStrength = 0.6f;
	/** 레이아웃 둔턱(BumpAmplitude)에 곱하는 값 — 바닥 높이처럼 뒤 구역이 덮으며 섞인다. 작으면 큰 땅 모양이 잔 울퉁불퉁에 묻히지 않는다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float BumpMultiplier = 1.f;
};

/**
 * 능선(레벨 디자인 2차) — 꺾은선을 따라 둥근 등성이가 이어진다. 점마다 높이를 주면 낮은 점이 안부(고갯마루)가 된다.
 * 양쪽 밑동 폭이 달라(비대칭) 한쪽은 가파르고 한쪽은 완만하다. 언덕과 같은 버퍼에 그려 겹치면 언덕처럼 섞인다(가장 큰 것 + 나머지 × HillOverlapBlend).
 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimTerrainRidgeline
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FName Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") TArray<FVector2D> Points;
	/** 점마다 등성이 높이(구역 바닥 위, m) — Points 와 같은 수. 비면 Height 하나 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") TArray<float> Heights;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float Height = 20.f;
	/** 진행 방향 왼쪽 · 오른쪽 밑동까지(m) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float LeftWidth = 120.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float RightWidth = 120.f;
	/** 단면 실루엣 k(언덕과 같은 식). 0 이면 레이아웃 HillSharpness */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float Sharpness = 0.f;
	/** 등성이 높이 · 폭이 길이를 따라 들쭉날쭉한 정도(±비율) · 마디 길이(m) — 곧은 둑이 되지 않게 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float Variation = 0.25f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float VariationScale = 70.f;

	/** 레벨 디자인 3차: 날카로운 능선 단면(꼭대기 기울기가 0 이 아니다 — 둥근 돔 = 「고분」을 피한다). 거짓이면 2차 언덕 단면 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") bool bSharpCrest = false;
	/** 단면 지수 (1 − t)^p — 왼쪽 · 오른쪽. 1 에 가까우면 곧은 급사면, 클수록 밑이 오목하게 퍼지는 완사면 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float LeftPower = 1.3f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float RightPower = 1.3f;
	/** 등성이 꼭대기를 둥글리는 반지름(m) — 칼날이 메시에서 깨지지 않을 만큼만 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float CrestRound = 2.5f;
	/** 등성이 선이 좌우로 흔들리는 크기(m) · 파장(m) — 자로 그은 둑이 되지 않게 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float CrestWobble = 6.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float CrestWobbleScale = 45.f;
	/** 등성이 높이의 작은 톱니(±비율) · 파장(m) — 이어진 선 위의 작은 봉우리 · 안부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float CrestJag = 0.08f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float CrestJagScale = 32.f;
	/** 바위 띠가 나는 급사면 쪽: 1 = 왼쪽, −1 = 오른쪽, 0 = 없음 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") int32 ScarpSide = 0;
	/** 바위 띠 높이(m) · 세기(0~1) — 급사면의 지층 턱 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float ScarpBandHeight = 6.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float ScarpBandStrength = 0.6f;
	/** 비탈의 골짜기 세기(0~1) · 간격(m) — 등성이에서 내려오는 지맥과 골이 번갈아 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float GullyStrength = 0.25f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float GullySpacing = 60.f;
};

/** 손으로 놓는 언덕 하나(중간 모양) — 둥근 삼각형 실루엣 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimTerrainHillSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FName Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FVector2D Center = FVector2D::ZeroVector;
	/** 구역 바닥 위로 솟는 높이 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float Height = 20.f;
	/** 밑동 지름(긴 쪽) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float Diameter = 250.f;
	/** 긴 쪽 / 짧은 쪽(1 = 원) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float Aspect = 1.f;
	/** 긴 쪽 방향(도, 지도 +X 에서 반시계) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float Yaw = 0.f;
	/** 실루엣 뾰족함(0 이면 레이아웃 HillSharpness). 클수록 꼭대기가 좁고 밑동이 넓게 퍼진다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float Sharpness = 0.f;
};

/** 시드로 뿌리는 언덕 군 — 다각형 안에 Count 개, 작은 것이 많고 큰 것이 적게(크기 = 난수^SizeBias) */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimTerrainHillField
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FName Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") TArray<FVector2D> Polygon;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") int32 Count = 20;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float MinHeight = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float MaxHeight = 30.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float MinDiameter = 150.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float MaxDiameter = 350.f;
	/** 크기 쏠림: 0~1 난수를 이 거듭제곱 — 1 이면 고르게, 2~3 이면 작은 것이 많다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float SizeBias = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float MaxAspect = 1.5f;
	/** 길쭉함 하한(1 = 원도 나올 수 있음) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float MinAspect = 1.f;
	/** 긴 쪽 방향(도, 지도 +X 에서 반시계) 가운데 · 흩어짐(±도). 흩어짐 ≥ 90 이면 아무 방향 — 작으면 언덕이 한 방향으로 줄지어 능선 · 골 결이 생긴다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float YawCenter = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float YawSpread = 90.f;
	/** 다른 언덕과 가운데 거리 ≥ (반지름 합) × 이 값 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float MinSpacing = 0.5f;
	/** 0 이면 레이아웃 HillSharpness */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float Sharpness = 0.f;
	/** 가장 가파른 곳(도) 상한 — 0 이면 레이아웃 HillMaxSlopeDeg. 작은 언덕이 촘촘한 마탑 구간은 조금 더 가파르게 둔다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float MaxSlopeDeg = 0.f;
	/** 길 · 개울 가운데선에서 떨어지는 여유(반지름 배수) — 음수면 레이아웃 HillLineClearance. 작으면 길이 언덕 사이 골로 지난다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float LineClearance = -1.f;
	/** 손 언덕(랜드마크 언덕)과 가운데 거리 ≥ (반지름 합) × 이 값 — 0 이면 MinSpacing. 크게 두면 큰 언덕 비탈이 깨끗해 랜드마크가 멀리서 보인다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float HandHillSpacing = 0.f;
	/** 참이면 다각형 대신 이미 놓인 언덕 꼭대기 둘레(반지름 40% 안)에 놓는다 — 바위 노두용 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") bool bNearHillTops = false;
	/** 참이면 언덕 수 집계(「언덕 군」)에서 뺀다 — 바위 노두 같은 작은 모양 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") bool bSmallShape = false;
};

/** 바위 능선 — 꺾은선을 따라 솟은 날카로운 등성이(북부 고지) */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimTerrainRidge
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FName Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") TArray<FVector2D> Points;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float Height = 25.f;
	/** 가운데선에서 밑동까지 반폭 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float HalfWidth = 60.f;
};

/** 가장자리 산등성이의 틈(고개) — 북쪽 산등성이를 이 U 둘레에서 낮춘다 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimTerrainGap
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FName Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float CenterU = 1000.f;
	/** 바닥 폭 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float Width = 70.f;
	/** 바닥에서 산등성이 높이로 돌아가는 폭 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float BlendWidth = 120.f;
};

/** 계곡 — 바닥은 상류 → 하류로 내려가기만, 한쪽은 절벽 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimTerrainValley
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FName Name;
	/** 위(상류) → 아래(하류) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") TArray<FVector2D> Points;
	/** 바닥(평평한 곳) 폭 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float FloorWidth = 90.f;
	/** 가운데선 둘레 땅보다 바닥이 낮은 깊이 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float Depth = 38.f;
	/** 완만한 쪽 비탈 폭 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float GentleWidth = 110.f;
	/** 절벽 쪽 비탈 폭 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float CliffWidth = 28.f;
	/** 절벽 쪽: 1 = 하류를 볼 때 왼쪽, -1 = 오른쪽 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") int32 CliffSide = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float MinFloorHeight = 3.f;
	/** 바닥이 하류로 1 m 갈 때 적어도 내려가는 높이 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float FloorGrade = 0.004f;
	/** 바닥 높이를 고르는 폭(이동 평균, m) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float FloorSmoothing = 120.f;
	/** 바닥 폭 · 완만한 비탈 폭이 계곡을 따라 들쭉날쭉한 정도(±비율) — 곧은 도랑이 되지 않게 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float WidthVariation = 0.3f;
	/** 절벽 폭이 들쭉날쭉한 정도(±비율) — 절벽 위 끝선이 곧은 벽이 되지 않게 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float CliffVariation = 0.6f;
	/** 들쭉날쭉한 마디 길이(m) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float VariationScale = 90.f;
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
	/** 레벨 디자인 3차: 물가선이 둥글지 않게 흔들리는 정도(±비율). 0 보다 크면 원판 대신 물 표면 메시(BuildWaterSurfaceMesh)를 쓴다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float Irregularity = 0.f;
};

/** 강 · 개울 · 길처럼 꺾은선을 따라가는 지형 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimTerrainChannel
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FName Name;
	/** 위(상류) → 아래(하류) 순서. 길은 시작 → 끝 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") TArray<FVector2D> Points;
	/** 바닥(물) 폭 · 길 폭 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float Width = 6.f;
	/** 물 높이 아래로 판 깊이 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float Depth = 1.f;
	/** 둑 · 비탈 폭. 길은 길 높이에서 둘레 땅으로 이어 주는 폭 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float BankWidth = 8.f;
	/** 개울: 둘레 땅보다 물이 얼마나 낮은가. 강은 해수면을 쓴다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float WaterBelowGround = 1.f;

	/** 레벨 디자인 3차: 자연 개울 — 지형이 판 V 자 골(상류) → 넓은 바닥(하류), 폭이 변하고 소 · 여울 · 물 표면 메시. 거짓이면 2차 도랑 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") bool bNatural = false;
	/** 하류 끝 물 폭(0 이면 Width 그대로). Width 는 상류 물 폭 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float WidthEnd = 0.f;
	/** 골 바닥 반폭(m) — 상류 · 하류 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float ValleyHalfStart = 1.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float ValleyHalfEnd = 10.f;
	/** 골 옆면 경사(도) — 상류(V 자) · 하류(완만) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float ValleySlopeStart = 38.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float ValleySlopeEnd = 14.f;
	/** 물머리(샘) 웅덩이 반지름(m). 0 이면 없음 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float HeadPoolRadius = 0.f;
	/** 소 깊이(m) · 간격(물 폭의 배수 — 실제 하천 5~7) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float PoolDepth = 1.1f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float PoolSpacingWidths = 6.f;
	/** 종단 곡선 지수 — 1 보다 크면 위가 가파르고 아래가 완만(오목), 1 보다 작으면 물이 높이 머물다 끝에서 떨어진다(길이 깊은 협곡에 빠지지 않게) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float LongProfilePower = 1.5f;
	/** 물이 둘레 땅보다 몇 m 아래로 파여 흐르나 — 상류 · 하류(이것이 V 자 골의 깊이가 된다) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float ValleyDepthStart = 0.4f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float ValleyDepthEnd = 0.4f;
	/** 이 연못으로 흘러든다 — 끝 물 높이 = 연못 물 높이(틈 없이 이어짐) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FName JoinPond;
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

/** 랜드마크(방향을 잡는 기준) — 그레이박스 모양은 Kind 로 build_openbeta.py 가 고른다. 표식 태그 Landmark.<Kind> */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimTerrainLandmark
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FName Name;
	/** Tower · GiantTree · RockGate · StoneCircle · Waterfall · Watchtower · Windmill · FloatingRock */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FName Kind;
	/** Global(지역 어디서나) · District(그 구역 안에서) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FName Tier = TEXT("District");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FVector2D Center = FVector2D::ZeroVector;
	/** 모양이 보는 방향(도, 지도 +X 에서 반시계) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float Yaw = 0.f;
	/** 땅 위로 보이는 꼭대기 높이 — 시선 검사가 이 점을 본다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float Height = 20.f;
	/** 시선 검사에서 「보인다」로 셀 수 있는 거리 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float ViewDistance = 900.f;
};

/** POI(들를 곳) — 자리 · 실루엣만(그레이박스 소품). 표식 태그 Poi.<Kind> */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimTerrainPoi
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FName Name;
	/** Puzzle · Resource · Creature · Ruin · Viewpoint · Story · BaseSite · Dungeon */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FName Kind;
	/** 묶음(미니 클러스터) 이름 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FName Cluster;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FVector2D Center = FVector2D::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float Yaw = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float Radius = 12.f;
	/** 사람이 읽는 설명(무엇이 놓일 자리인가) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FString Note;
};

/** 이름 붙은 시선 — 이 자리에 서면 이 랜드마크들이 보여야 한다(예: 탑 문 앞 → 고목 · 풍차 · 선돌 고리) */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimTerrainViewCheck
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FName Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FVector2D From = FVector2D::ZeroVector;
	/** 땅 위 눈높이 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float EyeHeight = 1.7f;
	/** 랜드마크 이름(Landmarks[].Name) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") TArray<FName> MustSee;
};

/**
 * 먼저 완성하는 구간(마탑 구간 — docs/dev/level-design.md 「7」)의 검사 기준. 다각형 안만 본다.
 * 큰 땅(2 × 4 km) 기준(FGrimTerrainAnalysisSpec)보다 촘촘하다: POI 간격 60~120 m, 사람 눈높이(1.8 m)에서 다음 언덕 너머가 가려져야 한다.
 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimTerrainSectorSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FName Name;
	/** 구간 경계(지도 좌표) — 바다 · 강 쪽은 물 가운데까지. 3 개보다 적으면 구간 검사를 하지 않는다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") TArray<FVector2D> Polygon;
	/** 경사: 구간 땅의 이 비율 이상이 Analysis.RollingMinDeg~RollingMaxDeg */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float RollingMinFraction = 0.7f;
	/** 길 위 샘플 간격(m) — 시선 · 가림 · 랜드마크 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float PathSampleStep = 25.f;
	/** 사람 눈높이(m) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float EyeHeight = 1.8f;
	/** 길 위 어느 점에서도 이 거리 안에 POI */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float PoiMaxDistance = 120.f;
	/** 구간 POI 마다 가장 가까운 다른 POI 까지 거리가 이 범위 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float PoiSpacingMin = 60.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float PoiSpacingMax = 120.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") int32 MinPois = 15;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") int32 MaxPois = 25;
	/** 가림: 길 위 점에서 OcclusionDirections 방향 × (Min~Max 거리, Step 마다) 땅 + TargetHeight 가 언덕에 가려진 비율 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") int32 OcclusionDirections = 24;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float OcclusionMinDistance = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float OcclusionMaxDistance = 300.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float OcclusionStep = 50.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float OcclusionTargetHeight = 1.f;
	/** 전체 가림 비율 하한 · 길 위 점 하나의 가림 비율 하한(탁 트인 곳이 없게) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float OcclusionMinFraction = 0.6f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float OcclusionPointMinFraction = 0.25f;
	/** 길 위 어디서나 보여야 하는 랜드마크 수(가까운 것 · 먼 것 모두, 랜드마크 ViewDistance 안) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") int32 MinLandmarksVisible = 2;
	/** 길 위 점 가운데 MinLandmarksVisible 개 이상 보이는 점의 비율 하한(언덕 골 안은 가려지는 것이 맞다). 어느 점에서도 하나는 보여야 한다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float LandmarkMinFraction = 1.f;
	/** 구간 언덕(작은 모양 빼고) — 가장 가까운 언덕까지 거리(가운데끼리)의 가운데값이 이 범위 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") int32 MinHills = 30;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float HillSpacingMin = 50.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float HillSpacingMax = 120.f;
	/** 시드 언덕 크기(손 언덕 빼고) — 지름 · 높이 범위 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FVector2D HillDiameterRange = FVector2D(40.0, 150.0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FVector2D HillHeightRange = FVector2D(8.0, 30.0);
};

/** 다 만든 뒤 확인할 것(docs/dev/level-design.md 「6」) — 기준값 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimTerrainAnalysisSpec
{
	GENERATED_BODY()

	/** 경사 분포를 재는 영역(지도 좌표 사각형) — 가장자리 산등성이 · 바다 · 강 건너는 뺀다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FVector2D BoundsMin = FVector2D(230.0, 280.0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FVector2D BoundsMax = FVector2D(1700.0, 3800.0);
	/** 경사 샘플 간격(m) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float SlopeSampleStep = 4.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float RollingMinDeg = 5.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float RollingMaxDeg = 25.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float FlatMaxDeg = 3.f;
	/** 땅의 이 비율 이상이 RollingMinDeg~RollingMaxDeg */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float RollingMinFraction = 0.7f;
	/** 평지(FlatMaxDeg 이하)는 이 비율 이하 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float FlatMaxFraction = 0.3f;
	/** 높이 차를 볼 구역(남 → 북 순서). 이웃끼리 평균 높이 차가 ZoneStepMin 이상 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") TArray<FName> ZoneSteps;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float ZoneStepMin = 25.f;
	/** 길 위 샘플 간격(m) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float PathSampleStep = 50.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float EyeHeight = 1.7f;
	/** POI 소품이 보이는 높이 · 거리 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float PoiViewHeight = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float PoiViewDistance = 450.f;
	/** 길 위 어디서나 수평선에 보여야 하는 POI · 랜드마크 수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") int32 MinVisible = 3;
	/** 길 위 어느 점에서도 이 거리 안에 POI */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float PoiMaxDistance = 300.f;
	/** 길 경사 상한(도) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float PathMaxGradeDeg = 15.f;
	/** 이름 붙은 시선(사람 눈 확인 자리와 같은 곳) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") TArray<FGrimTerrainViewCheck> ViewChecks;
	/** 먼저 완성하는 구간(마탑 구간)의 더 촘촘한 기준 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FGrimTerrainSectorSpec Sector;
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

	// ── 레벨 디자인 2차: 자연스러운 분포(docs/systems/terrain.md 「식생」) ──
	/** 자리끼리 최소 거리(m) — 포아송 원판(격자 느낌을 없앤다). 0 이면 가장 높은 밀도의 평균 간격 × 0.45 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float MinSpacing = 0.f;
	/** 작은 무리(초원의 숲덩이 · 바위 무더기): 같은 채널끼리 같은 노이즈. 파장(m, 0 이면 없음) · 문턱(-1~1) · 부드러움 · 무리 밖 밀도 배수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") int32 ClumpChannel = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float ClumpScale = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float ClumpThreshold = 0.1f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float ClumpSoftness = 0.15f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float ClumpOutside = 0.15f;
	/** 숲 안 덩이(m, 0 이면 없음) — 숲 안에서도 몇 그루씩 모이고 작은 빈터가 생기게(고른 간격의 「과수원」 느낌을 없앤다). 덩이 밖 밀도 배수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float ForestPatchScale = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float ForestPatchMin = 0.2f;
	/** 숲 가장자리(숲 무늬 무게 0.5 언저리) 밀도 배수 — 덤불이 숲 가장자리에 모인다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float ForestEdgeFactor = 1.f;
	/** 등성이(볼록) · 골(오목) 밀도 배수 — 나무는 등성이에서 성기고 바위는 등성이 · 비탈에 모인다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float RidgeFactor = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float ValleyFactor = 1.f;
	/** 경사 RollingSlopeDeg 언저리(언덕 비탈) 밀도 배수 — 바위가 비탈에 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float SlopeFactor = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float SlopeFactorDeg = 18.f;
	/** 물가(MinWaterDistance ~ + WaterEdgeDistance m — 개울 · 연못 · 강) 밀도 배수 — 개울을 따라 늘어선 나무 · 덤불 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float WaterEdgeFactor = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float WaterEdgeDistance = 20.f;
	/** 길 가장자리(MinPathDistance ~ + PathEdgeDistance m) 밀도 배수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float PathEdgeFactor = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float PathEdgeDistance = 12.f;
	/** 무리 · 숲 가운데일수록 크다(0~1) — 가운데는 늙은 나무, 가장자리는 어린 나무 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float GroveScaleBoost = 0.f;
	/** 막 기울임(도, 피치 · 롤) · 비탈에 맞추는 정도(0~1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float MaxTiltDeg = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float AlignToSlope = 0.f;
	/** 종(메시) 고르기 노이즈 파장(m) — 같은 종끼리 무리 지어 섞인다. 파이썬이 GetFoliageInstances 의 Variant 로 고른다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float VariantScale = 140.f;
	/** 먼저 뿌린 이 규칙들의 자리에서 AvoidDistance(m) 안에는 놓지 않는다(바위가 나무 줄기 속에 들지 않게) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") TArray<FName> AvoidRules;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float AvoidDistance = 3.f;
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

	// ── 바탕(구역에 들지 않는 곳의 바닥) · 큰 노이즈 ──
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float BaseHeight = 22.f;
	/** 북쪽으로 갈수록 오르는 높이(NorthRiseStart~End 사이에서) — 구역이 높이를 맡으면 0 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float NorthRise = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float NorthRiseStart = 600.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float NorthRiseEnd = 3600.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float NoiseAmplitude = 7.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float NoiseScale = 380.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") int32 NoiseOctaves = 5;
	/** 중간 크기 물결 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float RollingAmplitude = 3.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float RollingScale = 150.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float DetailAmplitude = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float DetailScale = 24.f;

	// ── 큰 모양: 구역 ──
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") TArray<FGrimTerrainZone> Zones;

	// ── 중간 모양: 언덕(둥근 삼각형) ──
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") TArray<FGrimTerrainHillSpec> Hills;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") TArray<FGrimTerrainHillField> HillFields;
	/** 실루엣: 높이 = H (1 − t)^k (1 + k t), t = 가운데 거리 / 반지름. k = 2 는 종 모양, 3 이면 뾰족한 산 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float HillSharpness = 2.4f;
	/** 밑동 반지름의 들쭉날쭉한 정도(0~0.4) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float HillIrregularity = 0.18f;
	/** 겹친 언덕: 높이 = 가장 큰 것 + (합 − 가장 큰 것) × 이 값 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float HillOverlapBlend = 0.3f;
	/** 시드 언덕의 가장 가파른 곳(도) — 넘으면 높이를 줄인다(손 언덕은 레이아웃 값 그대로) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float HillMaxSlopeDeg = 22.f;
	/** 시드 언덕이 길 · 개울 · 강 · 계곡 가운데선에서 떨어지는 여유(반지름 배수) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float HillLineClearance = 0.6f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") TArray<FGrimTerrainRidge> Ridges;
	/** 능선 · 안부(레벨 디자인 2차) — 둥근 등성이가 이어지고 길은 그 사이 골과 안부로 지난다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") TArray<FGrimTerrainRidgeline> Ridgelines;

	// ── 침식(레벨 디자인 2차): 물방울 침식 — 비탈에 가지 친 물길 · 골 바닥에 쌓인 흙. 사각형 안만(비싸다) ──
	/** 칸(1 m²)당 물방울 수. 0 이면 끔 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float ErosionDropletsPerCell = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FVector2D ErosionBoundsMin = FVector2D::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FVector2D ErosionBoundsMax = FVector2D::ZeroVector;
	/** 깎는 · 쌓는 세기(0~1) · 실어 나르는 양 배수 · 물방울 수명(걸음) · 붓 반지름(칸) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float ErosionErodeSpeed = 0.3f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float ErosionDepositSpeed = 0.3f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float ErosionCapacity = 4.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") int32 ErosionLifetime = 40;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") int32 ErosionRadius = 3;
	/** 한 걸음에 깎는 깊이 상한(m) — 뾰족한 구덩이가 생기지 않게 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float ErosionMaxStep = 0.25f;

	// ── 작은 모양: 둔턱 · 침식 골 ──
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float BumpAmplitude = 1.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float BumpScale = 40.f;
	/** 비탈에 패인 골의 깊이(m) · 골 사이(가로) · 골 길이(세로) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float GullyDepth = 1.4f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float GullyAcrossScale = 16.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float GullyAlongScale = 140.f;
	/** 이 경사(도)부터 골이 생겨 GullySlopeFull 에서 다 깊어진다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float GullySlopeStart = 7.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float GullySlopeFull = 18.f;

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
	/** 북쪽 산등성이의 틈(고개) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") TArray<FGrimTerrainGap> PassGaps;

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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FGrimTerrainValley Valley;
	/** 큰 강 — 물은 해수면, 바다로 이어진다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FGrimTerrainChannel River;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") TArray<FGrimTerrainChannel> Creeks;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") TArray<FGrimTerrainPond> Ponds;
	/** 길 — 길 높이는 땅을 따라가되 경사를 PathGradeDeg 아래로 고른 뒤 깎고 메운다. 뒤의 길(갈림길)은 앞의 길 위에서 시작한다 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") TArray<FGrimTerrainChannel> Paths;
	/** 길을 만들 때 쓰는 경사 상한(도) — 검사 상한(Analysis.PathMaxGradeDeg)보다 여유 있게 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float PathGradeDeg = 11.f;
	/** 길 높이를 고르는 폭(이동 평균, m) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float PathSmoothing = 30.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") TArray<FGrimTerrainSite> BaseSites;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") TArray<FGrimTerrainMarkerSpec> Markers;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") TArray<FGrimTerrainLandmark> Landmarks;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") TArray<FGrimTerrainPoi> Pois;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") FGrimTerrainAnalysisSpec Analysis;

	// ── 식생 ──
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") TArray<FGrimTerrainFoliageRule> Foliage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float ForestNoiseScale = 340.f;
	/** 숲 노이즈(-1~1)가 이 값을 넘으면 숲 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float ForestThreshold = 0.18f;
	/** 숲 경계를 부드럽게 섞는 노이즈 폭(문턱 ± 이 값) — 0 이면 칼로 자른 경계(1 · 2차) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float ForestSoftness = 0.f;
	/** 숲 노이즈를 뒤트는 세기(0~1.5) — 둥근 얼룩이 아니라 길쭉 · 불규칙한 숲 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float ForestWarp = 0.f;
	/** 등성이 · 골을 읽는 반지름(m) — 둘레 평균 높이 − 가운데 높이 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float CurvatureRadius = 20.f;
	/** 탑 평지 둘레로 식생을 비우는 여유 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float TowerClearance = 40.f;
	/** 랜드마크 · POI 둘레로 식생을 비우는 거리(POI 는 반지름 + 이 값) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain") float LandmarkClearance = 25.f;
};

/** 생성기가 계산한 물 하나(연못 · 개울 · 강 · 바다 · 폭포) — 물 평면 · 물 볼륨을 놓는 데 쓴다 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimTerrainWaterBody
{
	GENERATED_BODY()

	/** Pond · Creek · River · Sea · Falls */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain") FName Kind;
	UPROPERTY(BlueprintReadOnly, Category = "Terrain") FName Name;
	/** 월드 좌표(cm). 연못은 가운데 하나, 개울 · 강은 상류 → 하류 점들, 폭포는 위 · 아래 두 점. Z = 물 높이 */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain") TArray<FVector> Points;
	/** 연못 = 물 반지름, 개울 · 강 · 폭포 = 물 폭(cm) */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain") float Size = 0.f;
	/** 물 높이 아래 가장 깊은 곳까지(cm). 폭포는 떨어지는 높이 */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain") float Depth = 0.f;
	/** 레벨 디자인 3차 자연 개울: 점마다 물 폭(cm) — Points 와 같은 수. 그 밖은 비어 있다 */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain") TArray<float> Widths;
	/** 점마다 소인 정도(0 = 여울, 1 = 소) — 돌 · 물풀 놓기에 쓴다 */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain") TArray<float> Pools;
	/** 물 표면 메시(BuildWaterSurfaceMesh)로 그린다 — 평면 조각 · 원판을 놓지 않는다 */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain") bool bSurfaceMesh = false;
};

/** 길이 자연 개울을 건너는 자리(생성 결과) — 다리는 실제 곡선에서 계산한 이 값대로 놓는다 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimTerrainBridge
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Terrain") FName Name;
	UPROPERTY(BlueprintReadOnly, Category = "Terrain") FName PathName;
	UPROPERTY(BlueprintReadOnly, Category = "Terrain") FName CreekName;
	/** 다리 가운데(월드 cm), Z = 상판 윗면 */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain") FVector Center = FVector::ZeroVector;
	/** 길 방향(월드 Yaw, 도) */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain") float Yaw = 0.f;
	/** 양쪽 둑 사이 길이 · 길 폭(cm) */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain") float Span = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "Terrain") float Width = 0.f;
	/** 물 높이(월드 cm) */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain") float WaterZ = 0.f;
	/** 길과 개울이 만나는 각(도, 90 = 직각) */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain") float CrossingAngle = 90.f;
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
	/** 길이 자연 개울을 건너는 자리 */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain") TArray<FGrimTerrainBridge> Bridges;
	/** 놓인 언덕 수(손 + 시드, 작은 모양 빼고) · 바위 노두 수 */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain") int32 HillCount = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Terrain") int32 OutcropCount = 0;
};

/** 놓인 언덕 하나(생성 결과) */
struct FGrimTerrainPlacedHill
{
	FName Name;
	FVector2D Center = FVector2D::ZeroVector;
	float Height = 0.f;
	/** 긴 쪽 반지름 */
	float Radius = 0.f;
	float Aspect = 1.f;
	float YawRad = 0.f;
	float Sharpness = 2.4f;
	bool bSmallShape = false;
	/** 레이아웃 Hills(손으로 놓음) — 랜드마크 자리라 바위 노두를 얹지 않는다 */
	bool bHandPlaced = false;
};
