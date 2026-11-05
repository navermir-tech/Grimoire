#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "GrimTerrainMarker.generated.h"

class UDrawSphereComponent;

/**
 * 지형 표식 — 탑 자리 · 앞마당 · 거점 평지 · 길 꺾은점 · 생물 서식 구역(Habitat.*) 자리.
 * 지형 생성 스크립트(Scripts/Terrain/build_openbeta.py)가 놓는다. 게임 규칙은 없다 — 다른 시스템이 태그로 찾아 쓴다
 * (예: 생물 갈래의 서식 볼륨 · 스포너를 통합 때 Habitat.* 표식 자리에 놓는다).
 */
UCLASS(Blueprintable)
class GRIMOIRE_API AGrimTerrainMarker : public AActor
{
	GENERATED_BODY()

public:
	AGrimTerrainMarker();

	/** 무슨 자리인가(Terrain.Site.* · Terrain.Path · Habitat.*) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain")
	FGameplayTag MarkerTag;

	/** 자리의 반지름(cm) — 서식 구역이면 그 넓이 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Terrain", meta = (ClampMin = "0"))
	float Radius = 1000.f;

	/** 월드에서 그 태그(자식 태그 포함)를 가진 표식들 */
	UFUNCTION(BlueprintCallable, Category = "Terrain", meta = (WorldContext = "WorldContextObject"))
	static TArray<AGrimTerrainMarker*> FindMarkers(const UObject* WorldContextObject, FGameplayTag Tag);

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void GetAssetRegistryTags(FAssetRegistryTagsContext Context) const override;

	/** 에셋 레지스트리에 남기는 태그 이름(맵을 열지 않고 표식을 세는 테스트가 읽는다) */
	static const FName AssetRegistryMarkerTagName;

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UDrawSphereComponent> Area;
};
