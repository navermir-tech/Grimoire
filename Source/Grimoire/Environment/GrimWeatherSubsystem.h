#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GrimWeatherSubsystem.generated.h"

/**
 * 기후 · 날씨 (R04). M1 은 전역 기온 하나 — 시간대 곡선(밤에 낮음)만 있다.
 * 지역 볼륨 · 날씨 상태 머신은 M2(docs/systems/environment).
 */
UCLASS()
class GRIMOIRE_API UGrimWeatherSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** 그 위치의 주변 기온(섭씨). M1 은 위치와 무관하게 시간대 곡선. 서버 · 클라이언트 어디서나 부를 수 있다. */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Weather")
	float GetAmbientTemperature(FVector Location) const;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
};
