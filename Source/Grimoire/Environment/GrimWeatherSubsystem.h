#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GrimWeatherSubsystem.generated.h"

/**
 * 더위 · 추위 정수 등급 (R25). 지역 · 시간대 · 날씨가 정하고, 생존 쪽이 장비 저항 합과 비교한다
 * (docs/spec/survival 「체온 모델」). 둘 다 0 이상이고 보통 한쪽만 0 보다 크다.
 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimClimateExposure
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	int32 Heat = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weather")
	int32 Cold = 0;
};

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

	/**
	 * 그 위치의 더위 · 추위 등급(지역 · 시간대 · 날씨 몫만 — 젖음 · 열원 같은 개인 몫은 생존 쪽이 더한다).
	 * M2 기반: 지역 볼륨이 생기기 전까지는 주변 기온에서 바로 셈한다(쾌적 14~26℃, 4℃ 마다 한 등급).
	 */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Weather")
	FGrimClimateExposure GetClimateExposure(FVector Location) const;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
};
