#include "Environment/GrimSkyController.h"
#include "Environment/GrimEnvironmentSettings.h"
#include "Environment/GrimTimeOfDayMath.h"
#include "Environment/GrimTimeOfDaySubsystem.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/World.h"

AGrimSkyController::AGrimSkyController()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetupAttachment(Root);
	Sun->SetMobility(EComponentMobility::Movable);
	Sun->bAtmosphereSunLight = true;
	Sun->AtmosphereSunLightIndex = 0;
	Sun->ForwardShadingPriority = 1;
	Sun->Intensity = SunIntensity;

	Moon = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Moon"));
	Moon->SetupAttachment(Root);
	Moon->SetMobility(EComponentMobility::Movable);
	Moon->bAtmosphereSunLight = true;
	Moon->AtmosphereSunLightIndex = 1;
	Moon->ForwardShadingPriority = 0;
	Moon->Intensity = MoonIntensity;
	Moon->LightColor = MoonColor.ToFColor(true);

	SkyAtmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("SkyAtmosphere"));
	SkyAtmosphere->SetupAttachment(Root);

	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(Root);
	SkyLight->SetMobility(EComponentMobility::Movable);
	// 대기를 실시간으로 캡처한다 — 해가 움직여도 재캡처 호출이 필요 없다.
	SkyLight->bRealTimeCapture = true;

	HeightFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("HeightFog"));
	HeightFog->SetupAttachment(Root);
}

void AGrimSkyController::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyTimeOfDay(PreviewTimeOfDay, GetDefault<UGrimEnvironmentSettings>()->DayRatio);
}

void AGrimSkyController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (const UGrimTimeOfDaySubsystem* Time = GetWorld()->GetSubsystem<UGrimTimeOfDaySubsystem>())
	{
		ApplyTimeOfDay(Time->GetTimeOfDay(), Time->GetDayRatio());
	}
}

void AGrimSkyController::ApplyTimeOfDay(float TimeOfDay, float DayRatio)
{
	const float Angle = FMath::DegreesToRadians(FGrimTimeOfDayMath::GetSunAngleDegrees(TimeOfDay, DayRatio));

	// 해 궤도: 해돋이 방향 East 에서 시작해 기울어진 천정 Up 을 지나 반대편으로 진다.
	const FVector East = FRotator(0.f, SunriseYawDegrees, 0.f).Vector();
	const FVector Up = FVector::UpVector.RotateAngleAxis(OrbitTiltDegrees, East);
	const FVector SunDir = (East * FMath::Cos(Angle) + Up * FMath::Sin(Angle)).GetSafeNormal();

	// 빛은 해에서 땅으로 향한다. 달은 해의 맞은편.
	Sun->SetWorldRotation((-SunDir).Rotation());
	Moon->SetWorldRotation(SunDir.Rotation());

	const float Fade = FMath::Max(HorizonFade, KINDA_SMALL_NUMBER);
	const float SunAlpha = FMath::SmoothStep(-Fade, Fade, static_cast<float>(SunDir.Z));
	const float MoonAlpha = FMath::SmoothStep(-Fade, Fade, static_cast<float>(-SunDir.Z));

	Sun->SetIntensity(SunIntensity * SunAlpha);
	Sun->SetVisibility(SunAlpha > 0.f);
	Moon->SetIntensity(MoonIntensity * MoonAlpha);
	Moon->SetLightColor(MoonColor);
	Moon->SetVisibility(MoonAlpha > 0.f);

	SkyLight->SetIntensity(FMath::Lerp(NightSkyLightIntensity, DaySkyLightIntensity, SunAlpha));
}
