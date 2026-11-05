#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "World/GrimTerrainTypes.h"
#include "GrimTerrainEditorLibrary.generated.h"

class UMaterialInterface;
class UStaticMesh;

/**
 * 지형 1차 에디터 도구 — Scripts/Terrain/build_openbeta.py 가 헤드리스 에디터 파이썬에서 부른다.
 * 무거운 계산(높이 지도 · 식생 뿌리기)과 파이썬이 못 하는 일(랜드스케이프 가져오기 · World Partition 격자 ·
 * 브러시 볼륨)만 여기 둔다. 에디터 밖(게임 빌드)에서는 아무것도 하지 않는다.
 * GenerateTerrain 의 결과를 정적 캐시에 두고 나머지 함수가 그것을 쓴다(파이썬 한 번 실행 = 레이아웃 하나).
 */
UCLASS()
class GRIMOIRE_API UGrimTerrainEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 레이아웃 JSON 읽기. 상대 경로면 프로젝트 폴더 기준 */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Terrain")
	static bool LoadLayoutFromJson(const FString& JsonPath, FGrimTerrainLayout& OutLayout);

	/** 높이 지도 · 무게 · 물을 계산해 캐시에 둔다 */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Terrain")
	static bool GenerateTerrain(const FGrimTerrainLayout& Layout, FGrimTerrainSummary& OutSummary);

	/** 캐시한 지형의 땅 높이(월드 cm) */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Terrain")
	static float GetGroundHeight(float WorldX, float WorldY);

	/** 캐시한 지형의 경사(도) */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Terrain")
	static float GetSlopeDegrees(float WorldX, float WorldY);

	/** 캐시한 지형의 설계 검사(경사 분포 · 구역 높이 차 · 길 위 시선 · POI 밀도 · 길 경사) — 사람이 읽는 글 */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Terrain")
	static FString RunTerrainAnalysis();

	/** 캐시한 지형에서 그 지도 자리(m, 땅 + 눈높이)에 서면 보이는 POI · 랜드마크 이름 */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Terrain")
	static TArray<FName> GetVisibleFrom(float MapU, float MapV, float EyeHeight = 1.7f);

	/** 캐시한 지형에서 (From + 눈높이) → (To + 목표 높이) 시선이 땅에 막히지 않는가(지도 좌표 m) — POI 자리를 고를 때 */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Terrain")
	static bool IsMapPointVisible(float FromU, float FromV, float EyeHeight, float ToU, float ToV, float TargetHeight);

	/** 16비트 raw 높이 지도(리틀 엔디언, SizeX × SizeY) — 다른 도구 · 손 다듬기용 참고 파일 */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Terrain")
	static bool ExportHeightmapR16(const FString& FilePath);

	/** 사람이 보는 미리보기 그림(PNG, 1/Downsample 크기): 높이 색 + 음영 + 물 · 모래 · 길 · 계곡 숲 */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Terrain")
	static bool ExportPreviewImage(const FString& FilePath, int32 Downsample = 4);

	/** 랜드스케이프 레이어 정보 에셋(무게 섞지 않음 — 레이어마다 따로 0~1) */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Terrain")
	static UObject* CreateLayerInfoAsset(const FString& PackagePath, FName LayerName, FLinearColor DebugColor);

	/** 폴리지 타입 에셋(엔진 기본 도형 + 색 머티리얼) */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Terrain")
	static UObject* CreateFoliageTypeAsset(const FString& PackagePath, UStaticMesh* Mesh, UMaterialInterface* Material, int32 CullStartCm, int32 CullEndCm, bool bBlockCollision, bool bCastShadow);

	/**
	 * 캐시한 높이 지도로 랜드스케이프를 만든다(지금 열린 월드). LayerInfos 는 Shore · Path · Forest 이름의 레이어 정보.
	 * World Partition 월드면 GridSizeInComponents 로 스트리밍 프록시로 나누고, bSpatiallyLoaded = false 면 늘 로드되게 둔다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Terrain", meta = (WorldContext = "WorldContextObject"))
	static AActor* CreateLandscape(const UObject* WorldContextObject, UMaterialInterface* Material, const TArray<UObject*>& LayerInfos, int32 GridSizeInComponents, bool bSpatiallyLoaded);

	/**
	 * 식생 규칙(레이아웃 Foliage 의 이름) 하나를 뿌린 변환들(월드 cm). 인스턴스마다 PartOffset(크기 1 기준 부품 자리 · 크기)을 먼저 적용한다
	 * — 나무 = 줄기(원통) + 잎(원뿔)처럼 같은 자리에 부품 여럿. 폴리지 추가는 파이썬이 InstancedFoliageActor.add_instances 로 한다
	 * (그 함수는 Foliage 모듈 밖으로 내보내지 않아 C++ 에서 부를 수 없다).
	 */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Terrain")
	static TArray<FTransform> GetFoliageTransforms(FName RuleName, const FTransform& PartOffset);

	/**
	 * World Partition 런타임 격자(셀 크기 · 스트리밍 거리, cm)를 바꾼다. 해시 종류(RuntimeHashSet · RuntimeSpatialHash)에 맞춰
	 * 리플렉션으로 첫 격자(메인 파티션)를 고친다. 바꾼 결과를 사람이 읽는 글로 돌려준다(못 바꾸면 빈 글).
	 */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Terrain", meta = (WorldContext = "WorldContextObject"))
	static FString ConfigureWorldPartition(const UObject* WorldContextObject, int32 CellSizeCm, int32 LoadingRangeCm);

	/** 상자 브러시 볼륨(물리 볼륨 · 막는 볼륨 등)을 놓는다. Extent = 반 크기(cm) */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Terrain", meta = (WorldContext = "WorldContextObject"))
	static AActor* SpawnBoxVolume(const UObject* WorldContextObject, TSubclassOf<AActor> VolumeClass, FVector Center, FVector Extent, float Yaw);
};
