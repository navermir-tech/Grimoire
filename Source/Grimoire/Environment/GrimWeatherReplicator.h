#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "Environment/GrimClimateTypes.h"
#include "GrimWeatherReplicator.generated.h"

/**
 * 지역별 날씨를 모두에게 복제하는 보이지 않는 액터. 서버의 UGrimWeatherSubsystem 이 만든다(맵에 놓지 않는다).
 * 시간 복제 액터(AGrimTimeOfDayReplicator)와 같은 방식 — GameState 를 고치지 않는다.
 * RegionWeather 는 날씨가 바뀔 때만 보낸다(블렌드 진행은 서버 월드 시각으로 각자 계산).
 */
UCLASS(NotPlaceable, Transient)
class GRIMOIRE_API AGrimWeatherReplicator : public AInfo
{
	GENERATED_BODY()

public:
	AGrimWeatherReplicator();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	/** 서버 전용. */
	void SetRegionWeather(const TArray<FGrimRegionWeather>& NewRegionWeather);
	const TArray<FGrimRegionWeather>& GetRegionWeather() const { return RegionWeather; }

	/** 서버 → 모두: 낙뢰 연출(섬광 · 이펙트). 게임플레이(자극)는 서버가 이미 했다. */
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastLightningStrike(FVector_NetQuantize Location);

private:
	UPROPERTY(ReplicatedUsing = OnRep_RegionWeather)
	TArray<FGrimRegionWeather> RegionWeather;

	UFUNCTION()
	void OnRep_RegionWeather();
};
