#include "Environment/GrimSkyController.h"
#include "Environment/GrimEnvironmentSettings.h"
#include "Environment/GrimTimeOfDayMath.h"
#include "Environment/GrimTimeOfDaySubsystem.h"
#include "Environment/GrimWeatherSettings.h"
#include "Environment/GrimWeatherSubsystem.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

namespace GrimSkyControllerLocal
{
	float GSunRotationStepOverride = -1.f;
	FAutoConsoleVariableRef CVarSunRotationStep(
		TEXT("Grim.Sky.SunRotationStep"),
		GSunRotationStepOverride,
		TEXT("해 · 달 빛을 돌리는 최소 각(도). 음수 = 액터 값(SunRotationStepDegrees), 0 = 매 프레임(그림자 캐시 무효 — 성능 비교용)"),
		ECVF_Default);
}

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

	// 강수 이펙트: 카메라를 따라 월드 좌표로 옮긴다. 에셋은 BeginPlay 에 설정에서 넣는다.
	auto MakePrecipitation = [this](const TCHAR* Name)
	{
		UNiagaraComponent* Component = CreateDefaultSubobject<UNiagaraComponent>(Name);
		Component->SetupAttachment(Root);
		Component->SetAutoActivate(false);
		Component->SetUsingAbsoluteLocation(true);
		Component->SetUsingAbsoluteRotation(true);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return Component;
	};
	RainFX = MakePrecipitation(TEXT("RainFX"));
	SnowFX = MakePrecipitation(TEXT("SnowFX"));
	DustFX = MakePrecipitation(TEXT("DustFX"));
}

void AGrimSkyController::BeginPlay()
{
	Super::BeginPlay();

	BaseFogDensity = HeightFog->FogDensity;

	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	const UGrimWeatherSettings* Settings = GetDefault<UGrimWeatherSettings>();
	RainFX->SetAsset(Settings->RainSystem.LoadSynchronous());
	SnowFX->SetAsset(Settings->SnowSystem.LoadSynchronous());
	DustFX->SetAsset(Settings->DustSystem.LoadSynchronous());
	LightningSystem = Settings->LightningSystem.LoadSynchronous();

	if (UGrimWeatherSubsystem* Weather = GetWorld()->GetSubsystem<UGrimWeatherSubsystem>())
	{
		Weather->OnLightningStrike.AddDynamic(this, &AGrimSkyController::HandleLightningStrike);
	}
}

void AGrimSkyController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UGrimWeatherSubsystem* Weather = World->GetSubsystem<UGrimWeatherSubsystem>())
		{
			Weather->OnLightningStrike.RemoveDynamic(this, &AGrimSkyController::HandleLightningStrike);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void AGrimSkyController::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyTimeOfDay(PreviewTimeOfDay, GetDefault<UGrimEnvironmentSettings>()->DayRatio);
}

void AGrimSkyController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FlashRemaining = FMath::Max(0.f, FlashRemaining - DeltaSeconds);

	FVector ViewLocation;
	const UGrimWeatherSubsystem* Weather = GetWorld()->GetSubsystem<UGrimWeatherSubsystem>();
	if (Weather && GetNetMode() != NM_DedicatedServer && GetViewLocation(ViewLocation))
	{
		ApplyWeather(Weather->GetBlendedProfileAt(ViewLocation), ViewLocation);
	}

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

	// 빛은 해에서 땅으로 향한다. 달은 해의 맞은편. 조금씩만 움직였으면 그대로 둔다(그림자 캐시 — SunRotationStepDegrees).
	const bool bWorld = GetWorld() && GetWorld()->IsGameWorld();
	const float Step = GrimSkyControllerLocal::GSunRotationStepOverride >= 0.f ? GrimSkyControllerLocal::GSunRotationStepOverride : SunRotationStepDegrees;
	if (!bWorld || AppliedSunDir.IsZero() || Step <= 0.f || FVector::DotProduct(AppliedSunDir, SunDir) < FMath::Cos(FMath::DegreesToRadians(Step)))
	{
		AppliedSunDir = SunDir;
		Sun->SetWorldRotation((-SunDir).Rotation());
		Moon->SetWorldRotation(SunDir.Rotation());
	}

	const float Fade = FMath::Max(HorizonFade, KINDA_SMALL_NUMBER);
	const float SunAlpha = FMath::SmoothStep(-Fade, Fade, static_cast<float>(SunDir.Z));
	const float MoonAlpha = FMath::SmoothStep(-Fade, Fade, static_cast<float>(-SunDir.Z));

	Sun->SetIntensity(SunIntensity * SunAlpha * WeatherSunScale);
	Sun->SetVisibility(SunAlpha > 0.f);
	Moon->SetIntensity(MoonIntensity * MoonAlpha * WeatherSunScale);
	Moon->SetLightColor(MoonColor);
	Moon->SetVisibility(MoonAlpha > 0.f);

	// 번개 섬광: 하늘빛에 잠깐 더한다(남은 시간 비율로 줄어든다).
	const UGrimWeatherSettings* WeatherSettings = GetDefault<UGrimWeatherSettings>();
	const float Flash = FlashRemaining > 0.f
		? WeatherSettings->LightningFlashSkyLight * FlashRemaining / FMath::Max(WeatherSettings->LightningFlashSeconds, KINDA_SMALL_NUMBER)
		: 0.f;
	SkyLight->SetIntensity(FMath::Lerp(NightSkyLightIntensity, DaySkyLightIntensity, SunAlpha) * WeatherSkyLightScale + Flash);
}

void AGrimSkyController::ApplyWeather(const FGrimWeatherProfile& Profile, const FVector& ViewLocation)
{
	WeatherSunScale = Profile.SunIntensityScale;
	WeatherSkyLightScale = Profile.SkyLightScale;
	if (!FMath::IsNearlyEqual(WeatherFogAdd, Profile.FogDensityAdd, 1e-4f))
	{
		WeatherFogAdd = Profile.FogDensityAdd;
		HeightFog->SetFogDensity(BaseFogDensity + WeatherFogAdd);
	}

	UpdatePrecipitation(RainFX, Profile.RainAmount, ViewLocation);
	UpdatePrecipitation(SnowFX, Profile.SnowAmount, ViewLocation);
	UpdatePrecipitation(DustFX, Profile.DustAmount, ViewLocation);
}

void AGrimSkyController::UpdatePrecipitation(UNiagaraComponent* Component, float Amount, const FVector& ViewLocation)
{
	if (!Component || !Component->GetAsset())
	{
		return;
	}
	const UGrimWeatherSettings* Settings = GetDefault<UGrimWeatherSettings>();
	if (Amount < Settings->PrecipitationVisibleThreshold)
	{
		if (Component->IsActive())
		{
			Component->Deactivate();
		}
		return;
	}
	// 임시: 위로 뿜는 엔진 기본 분수를 카메라 위에서 뒤집어 아래로 쏟는다.
	Component->SetWorldLocationAndRotation(ViewLocation + FVector(0.0, 0.0, Settings->PrecipitationHeight), FRotator(0.f, 0.f, 180.f));
	if (!Component->IsActive())
	{
		Component->Activate(true);
	}
}

bool AGrimSkyController::GetViewLocation(FVector& OutLocation) const
{
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !PC->IsLocalController())
	{
		return false;
	}
	if (PC->PlayerCameraManager)
	{
		OutLocation = PC->PlayerCameraManager->GetCameraLocation();
		return true;
	}
	if (const APawn* Pawn = PC->GetPawn())
	{
		OutLocation = Pawn->GetActorLocation();
		return true;
	}
	return false;
}

void AGrimSkyController::HandleLightningStrike(FVector Location)
{
	FlashRemaining = GetDefault<UGrimWeatherSettings>()->LightningFlashSeconds;
	if (LightningSystem)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, LightningSystem, Location);
	}
}
