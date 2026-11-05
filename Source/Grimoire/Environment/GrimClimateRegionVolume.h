#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GrimClimateRegionVolume.generated.h"

class UBoxComponent;

/**
 * 기후 지역 볼륨 (R04). 상자 안이 기후 표의 RegionId 지역이다. 볼륨 밖은 설정의 기본 지역(탑 주변 초원).
 * 경계에서 안쪽으로 BlendDistance 만큼 들어가며 기본 지역(또는 겹친 다른 볼륨)과 섞인다.
 * 충돌 없음 · 복제 없음 — 레벨에 놓인 그대로 서버 · 클라이언트가 같은 볼륨을 가진다.
 * 같은 RegionId 볼륨 여러 개는 날씨를 함께 쓴다(지역마다 상태 머신 하나).
 */
UCLASS(ClassGroup = (Grimoire), HideCategories = (Input, Physics, Collision, Replication, HLOD))
class GRIMOIRE_API AGrimClimateRegionVolume : public AActor
{
	GENERATED_BODY()

public:
	AGrimClimateRegionVolume();

	/** 기후 표의 행 이름(예: SnowPeak) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Climate")
	FName RegionId = TEXT("Meadow");

	/** 경계에서 안쪽으로 이 거리(cm)에 걸쳐 섞인다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Climate", meta = (ClampMin = "0"))
	float BlendDistance = 2000.f;

	/** 그 위치의 이 볼륨 가중치(0~1). 바깥 0, 경계에서 BlendDistance 들어가면 1 */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Weather")
	float GetBlendWeight(const FVector& WorldLocation) const;

	UBoxComponent* GetBounds() const { return Bounds; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UBoxComponent> Bounds;
};
