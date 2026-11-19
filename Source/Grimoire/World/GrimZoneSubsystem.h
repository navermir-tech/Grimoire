#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/GrimZoneTypes.h"
#include "GrimZoneSubsystem.generated.h"

class APlayerState;
class UGrimZoneDiscoveryComponent;

/**
 * 구역 월드 서브시스템(docs/systems/world-zones.md).
 *  - 조회(서버 · 클라이언트, 블루프린트): 한 자리의 구역, 구역 정의, 경계(월드 좌표) — 구역 이름 배너 · 나중의 미니맵 · 큰 지도가 같이 쓴다.
 *  - 서버: PlayerState 마다 UGrimZoneDiscoveryComponent 를 붙이고(Core 를 고치지 않으려고 런타임에),
 *    구역 맵(UGrimZoneSettings::Maps)이면 주기(PollIntervalSeconds)마다 들어섬을 보게 한다.
 */
UCLASS()
class GRIMOIRE_API UGrimZoneSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	/** 이 맵에 구역이 있나(설정 Maps — 비우면 모든 맵) */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Zones")
	bool IsZoneMap() const;

	/** 구역 표(구역 맵이 아니면 빈 표) */
	const FGrimZoneDatabase& GetDatabase() const;

	/** 월드 자리의 큰 구역 · 소구역(구역 밖이면 None) */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Zones")
	FGrimZoneHit FindZonesAt(const FVector& WorldLocation) const;

	/** 이름의 구역 정의(없으면 거짓) */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Zones")
	bool GetZone(FName Zone, FGrimZoneDef& OutZone) const;

	/** 한 층의 구역들(레이아웃 순서) — 지도 이름표 · 범례 */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Zones")
	TArray<FGrimZoneDef> GetZonesOfTier(EGrimZoneTier Tier) const;

	/** 큰 구역의 경계를 월드 좌표(cm, Z = 0)로 — 지도에 그리기. 소구역 · 없는 이름은 빈 배열 */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Zones")
	TArray<FVector> GetZoneWorldOutline(FName Zone) const;

	/** 구역 가운데(이름표 자리)의 월드 좌표(cm, Z = 0) */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Zones")
	FVector GetZoneWorldCenter(FName Zone) const;

	UFUNCTION(BlueprintPure, Category = "Grimoire|Zones")
	FVector2D WorldToMap(const FVector& WorldLocation) const;

	UFUNCTION(BlueprintPure, Category = "Grimoire|Zones")
	FVector MapToWorld(const FVector2D& MapPoint) const;

	/** 서버: PlayerState 에 구역 컴포넌트가 없으면 붙인다 */
	UGrimZoneDiscoveryComponent* EnsureComponent(APlayerState* PlayerState);

	/** 서버: 지금 모든 플레이어의 들어섬을 본다(주기 타이머) */
	void PollNow();

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	bool IsServer() const;

	FTimerHandle PollTimer;
	FDelegateHandle ActorSpawnedHandle;
};
