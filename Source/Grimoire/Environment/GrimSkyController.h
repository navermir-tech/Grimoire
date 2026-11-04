#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GrimSkyController.generated.h"

class UDirectionalLightComponent;
class USkyAtmosphereComponent;
class USkyLightComponent;
class UExponentialHeightFogComponent;

/**
 * 하늘 (R04). 레벨에 하나 놓으면 해 · 달 · 대기 · 하늘빛 · 높이 안개를 스스로 갖고
 * UGrimTimeOfDaySubsystem 의 시각에 맞춰 해 · 달을 돌리고 밝기를 바꾼다(서버 · 클라이언트 각자, 복제 없음).
 * 레벨의 다른 Directional Light · Sky Atmosphere · Sky Light · 높이 안개와 겹치므로 그것들은 레벨에서 빼야 한다.
 */
UCLASS(ClassGroup = (Grimoire), HideCategories = (Input, Collision, Physics))
class GRIMOIRE_API AGrimSkyController : public AActor
{
	GENERATED_BODY()

public:
	AGrimSkyController();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	/** 지정한 시각(0~1)의 하늘로 맞춘다. */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Sky")
	void ApplyTimeOfDay(float TimeOfDay, float DayRatio);

	/** 에디터 미리보기용 시각(게임 중에는 서브시스템 시각을 쓴다). */
	UPROPERTY(EditAnywhere, Category = "Sky", meta = (ClampMin = "0", ClampMax = "1"))
	float PreviewTimeOfDay = 0.4f;

	/** 해가 뜨는 방향(도, 월드 Yaw). */
	UPROPERTY(EditAnywhere, Category = "Sky")
	float SunriseYawDegrees = 90.f;

	/** 해 궤도를 천정에서 기울이는 각(도). 0 이면 정오에 바로 머리 위. */
	UPROPERTY(EditAnywhere, Category = "Sky", meta = (ClampMin = "0", ClampMax = "80"))
	float OrbitTiltDegrees = 25.f;

	/** 해의 최대 조도(lux). */
	UPROPERTY(EditAnywhere, Category = "Sky", meta = (ClampMin = "0"))
	float SunIntensity = 10.f;

	/** 달의 최대 조도(lux). */
	UPROPERTY(EditAnywhere, Category = "Sky", meta = (ClampMin = "0"))
	float MoonIntensity = 0.4f;

	UPROPERTY(EditAnywhere, Category = "Sky")
	FLinearColor MoonColor = FLinearColor(0.55f, 0.65f, 1.f);

	/** 하늘빛 세기: 낮 · 밤. */
	UPROPERTY(EditAnywhere, Category = "Sky", meta = (ClampMin = "0"))
	float DaySkyLightIntensity = 1.f;

	UPROPERTY(EditAnywhere, Category = "Sky", meta = (ClampMin = "0"))
	float NightSkyLightIntensity = 0.25f;

	/** 해 · 달이 지평선 아래 이 높이(사인 값)부터 서서히 켜진다. */
	UPROPERTY(EditAnywhere, Category = "Sky", meta = (ClampMin = "0", ClampMax = "0.5"))
	float HorizonFade = 0.08f;

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UDirectionalLightComponent> Sun;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UDirectionalLightComponent> Moon;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USkyAtmosphereComponent> SkyAtmosphere;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USkyLightComponent> SkyLight;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UExponentialHeightFogComponent> HeightFog;
};
