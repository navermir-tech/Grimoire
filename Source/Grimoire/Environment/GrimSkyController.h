#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GrimSkyController.generated.h"

class UDirectionalLightComponent;
class USkyAtmosphereComponent;
class USkyLightComponent;
class UExponentialHeightFogComponent;
class UPostProcessComponent;
class UNiagaraComponent;
class UNiagaraSystem;
struct FGrimWeatherProfile;

/**
 * 하늘 (R04). 레벨에 하나 놓으면 해 · 달 · 대기 · 하늘빛 · 높이 안개를 스스로 갖고
 * UGrimTimeOfDaySubsystem 의 시각에 맞춰 해 · 달을 돌리고 밝기를 바꾼다(서버 · 클라이언트 각자, 복제 없음).
 * 레벨의 다른 Directional Light · Sky Atmosphere · Sky Light · 높이 안개와 겹치므로 그것들은 레벨에서 빼야 한다.
 * 날씨(M2): 카메라 위치의 섞인 날씨로 해 · 하늘빛 세기, 안개 밀도를 바꾸고, 카메라 둘레에 비 · 눈 · 모래 이펙트,
 * 낙뢰 때 섬광 · 이펙트를 낸다(UGrimWeatherSubsystem 조회 · 이벤트만 — 게임플레이 없음, 전용 서버에선 연출을 건너뛴다).
 */
UCLASS(ClassGroup = (Grimoire), HideCategories = (Input, Collision, Physics))
class GRIMOIRE_API AGrimSkyController : public AActor
{
	GENERATED_BODY()

public:
	AGrimSkyController();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 지정한 시각(0~1)의 하늘로 맞춘다. */
	UFUNCTION(BlueprintCallable, Category = "Grimoire|Sky")
	void ApplyTimeOfDay(float TimeOfDay, float DayRatio);

	/** 날씨 연출 배율을 정한다(다음 ApplyTimeOfDay 부터 적용). 섞인 날씨 값 · 강수 이펙트 위치(카메라). */
	void ApplyWeather(const FGrimWeatherProfile& Profile, const FVector& ViewLocation);

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

	/** 달빛의 최저 높이(도). 달은 해의 맞은편이지만 이보다 낮으면 같은 방위에서 이 높이로 비춘다 —
	 *  해가 막 진 뒤에 달빛이 땅을 스치기만 해 한밤보다 어두워지지 않게(Q101). 0 = 늘 해의 맞은편 */
	UPROPERTY(EditAnywhere, Category = "Sky", meta = (ClampMin = "0", ClampMax = "80"))
	float MoonMinElevationDegrees = 30.f;

	/** 하늘빛 세기: 낮 · 밤. */
	UPROPERTY(EditAnywhere, Category = "Sky", meta = (ClampMin = "0"))
	float DaySkyLightIntensity = 1.f;

	UPROPERTY(EditAnywhere, Category = "Sky", meta = (ClampMin = "0"))
	float NightSkyLightIntensity = 0.25f;

	/** 해 · 달이 지평선 아래 이 높이(사인 값)부터 서서히 켜진다. */
	UPROPERTY(EditAnywhere, Category = "Sky", meta = (ClampMin = "0", ClampMax = "0.5"))
	float HorizonFade = 0.08f;

	/** 해 · 달 방향을 이 각(도)만큼 움직였을 때만 빛을 돌린다. 방향광이 돌면 가상 섀도 맵 캐시가 통째로 무효가 돼
	 *  그림자를 매 프레임 다시 그린다(2026-11-06 측정: GPU 그림자 2.6 ms) — 48분 하루면 0.1도 ≈ 0.8초에 한 번. 0 이면 매 프레임 */
	UPROPERTY(EditAnywhere, Category = "Sky", meta = (ClampMin = "0", ClampMax = "5"))
	float SunRotationStepDegrees = 0.1f;

private:
	UFUNCTION()
	void HandleLightningStrike(FVector Location);

	/** 밤 화면(자동 노출 하한 · 채도)을 밤 섞임(0 = 낮, 1 = 밤)만큼 건다. 값은 UGrimEnvironmentSettings 「Night」 */
	void ApplyNightLook(float NightBlend);

	/** 카메라가 실내(지붕 + 둘레 벽)인지 몇 번에 한 번 트레이스로 보고 IndoorAlpha 를 부드럽게 옮긴다(로컬 화면 전용) */
	void UpdateIndoor(const class UGrimWeatherSubsystem* Weather, const FVector& ViewLocation, float DeltaSeconds);

	void UpdatePrecipitation(UNiagaraComponent* Component, float Amount, const FVector& ViewLocation);
	bool GetViewLocation(FVector& OutLocation) const;

	/** 날씨 배율(1 = 맑음). ApplyWeather 가 정하고 ApplyTimeOfDay 가 쓴다 */
	float WeatherSunScale = 1.f;
	float WeatherSkyLightScale = 1.f;
	float WeatherFogAdd = 0.f;
	float BaseFogDensity = 0.02f;
	float FlashRemaining = 0.f;
	/** 실내 정도(0 = 바깥, 1 = 실내) — 밤 화면을 이만큼 뺀다 */
	float IndoorAlpha = 0.f;
	bool bIndoorTarget = false;
	float IndoorProbeRemaining = 0.f;
	/** 마지막으로 빛에 준 해 방향(영 벡터 = 아직 안 줌) — SunRotationStepDegrees */
	FVector AppliedSunDir = FVector::ZeroVector;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> LightningSystem;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UNiagaraComponent> RainFX;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UNiagaraComponent> SnowFX;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UNiagaraComponent> DustFX;

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

	/** 밤 화면 후처리(무한 범위). 해가 진 뒤 바깥에서만 가중치 1(값은 ApplyNightLook 이 섞는다), 낮 · 노을 · 실내는 0 이라 걸리지 않는다 */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPostProcessComponent> NightLook;
};
