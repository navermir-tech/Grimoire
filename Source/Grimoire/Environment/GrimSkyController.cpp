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
#include "Components/PostProcessComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
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

	// 밤 화면(노출 하한 · 채도): 맵 전체에 걸리는 후처리. 값은 UGrimEnvironmentSettings, 가중치 · 값은 ApplyNightLook 이 정한다.
	NightLook = CreateDefaultSubobject<UPostProcessComponent>(TEXT("NightLook"));
	NightLook->SetupAttachment(Root);
	NightLook->bUnbound = true;
	NightLook->BlendWeight = 0.f;

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
	if (GetNetMode() != NM_DedicatedServer && GetViewLocation(ViewLocation))
	{
		if (Weather)
		{
			ApplyWeather(Weather->GetBlendedProfileAt(ViewLocation), ViewLocation);
		}
		UpdateIndoor(Weather, ViewLocation, DeltaSeconds);
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
		Moon->SetWorldRotation((-FGrimTimeOfDayMath::GetMoonDirection(SunDir, MoonMinElevationDegrees)).Rotation());
	}

	const float SunAlpha = FGrimTimeOfDayMath::GetDayBlend(static_cast<float>(SunDir.Z), HorizonFade);
	// 달빛 · 밤 화면은 해가 지평선 아래로 내려간 뒤에 들어온다(해넘이 순간은 해질녘 그대로 — Q101).
	const float MoonAlpha = FGrimTimeOfDayMath::GetNightBlend(static_cast<float>(SunDir.Z), HorizonFade);
	// 실내(등불 켠 탑 안)에서는 밤 화면을 뺀다 — 실내 화면은 밤에도 예전 그대로(Q101).
	ApplyNightLook(MoonAlpha * (1.f - IndoorAlpha));

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

void AGrimSkyController::ApplyNightLook(float NightBlend)
{
	// 밤 화면(Q101): 무한 범위 후처리. 낮 · 해넘이 순간(밤 섞임 0)에는 가중치 0 → 엔진이 아예 건너뛴다(낮 · 노을 화면 그대로).
	// 해가 진 뒤에는 가중치 1 로 걸고 값을 직접 섞는다 — 엔진 가중치에 맡기면 노출 하한이 -10 에서 선형으로 올라 어스름 초반에는
	// 거의 묶이지 않아, 막 진 뒤 장면을 자동 노출이 낮처럼 끌어올렸다(2026-11-18 시험: 평균 EV100 -6.5 인 장면이 한낮처럼 보임).
	const UGrimEnvironmentSettings* Settings = GetDefault<UGrimEnvironmentSettings>();
	const float Blend = FMath::Clamp(NightBlend, 0.f, 1.f);
	FPostProcessSettings& Look = NightLook->Settings;

	// 자동 노출 하한: 해넘이 노출(Twilight) → 밤 노출(Night). 장면이 하한보다 어두우면 더 끌어올리지 않는다(BUG-008). 하한보다 밝은 곳은 영향 없음.
	Look.bOverride_AutoExposureMinBrightness = true;
	Look.AutoExposureMinBrightness = FMath::Lerp(Settings->TwilightExposureMinEV100, Settings->NightExposureMinEV100, Blend);

	// 색: 전체를 조금, 어두운 영역(달빛만 받는 곳)을 많이 뺀다 — 불빛을 받은 밝은 곳은 색이 남는다. 채도 1 에서 시작해 튀지 않는다.
	const float Saturation = FMath::Lerp(1.f, Settings->NightSaturation, Blend);
	const float ShadowSaturation = FMath::Lerp(1.f, Settings->NightShadowSaturation, Blend);
	Look.bOverride_ColorSaturation = true;
	Look.ColorSaturation = FVector4(Saturation, Saturation, Saturation, 1.0);
	Look.bOverride_ColorSaturationShadows = true;
	Look.ColorSaturationShadows = FVector4(ShadowSaturation, ShadowSaturation, ShadowSaturation, 1.0);
	Look.bOverride_ColorCorrectionShadowsMax = true;
	Look.ColorCorrectionShadowsMax = Settings->NightShadowsMax;

	NightLook->BlendWeight = Blend > 0.f ? 1.f : 0.f;
}

void AGrimSkyController::UpdateIndoor(const UGrimWeatherSubsystem* Weather, const FVector& ViewLocation, float DeltaSeconds)
{
	const UGrimEnvironmentSettings* Settings = GetDefault<UGrimEnvironmentSettings>();
	IndoorProbeRemaining -= DeltaSeconds;
	if (IndoorProbeRemaining <= 0.f)
	{
		IndoorProbeRemaining = Settings->IndoorProbeIntervalSeconds;
		bIndoorTarget = false;
		// 지붕(날씨의 지붕 판정과 같은 위 트레이스) + 둘레 벽: 수평 방향 여럿 중 정한 수 이상이 막혀야 실내.
		// 나무 아래는 지붕은 있어도 둘레가 트여 실외로 남는다.
		const APlayerController* PC = GetWorld()->GetFirstPlayerController();
		const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
		if (Weather && Weather->IsSheltered(ViewLocation, Pawn))
		{
			FCollisionQueryParams Params(SCENE_QUERY_STAT(GrimSkyIndoor), false, Pawn);
			const FCollisionObjectQueryParams Objects(ECC_TO_BITFIELD(ECC_WorldStatic) | ECC_TO_BITFIELD(ECC_WorldDynamic));
			const int32 Directions = FMath::Max(1, Settings->IndoorProbeDirections);
			int32 Blocked = 0;
			for (int32 Index = 0; Index < Directions; ++Index)
			{
				const FVector Dir = FRotator(0.f, 360.f * Index / Directions, 0.f).Vector();
				if (GetWorld()->LineTraceTestByObjectType(ViewLocation, ViewLocation + Dir * Settings->IndoorProbeDistance, Objects, Params))
				{
					++Blocked;
				}
			}
			bIndoorTarget = Blocked >= Settings->IndoorWallsRequired;
		}
	}
	const float Rate = 1.f / FMath::Max(Settings->IndoorBlendSeconds, KINDA_SMALL_NUMBER);
	IndoorAlpha = FMath::FInterpConstantTo(IndoorAlpha, bIndoorTarget ? 1.f : 0.f, DeltaSeconds, Rate);
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

#if !UE_BUILD_SHIPPING

// Grim.Sky.Set <속성> <값> — 밤 화면 · 하늘 값을 실행 중에 바꿔 본다(개발 빌드, 이 프로세스에만 · 저장하지 않음).
// 환경 설정(UGrimEnvironmentSettings — NightExposureMinEV100 등)에 그 이름이 있으면 거기에, 없으면 월드의 하늘 컨트롤러(MoonIntensity 등)에 넣는다.
// 맞는 값을 찾으면 Config/DefaultGame.ini(설정) 나 헤더 기본값(하늘 컨트롤러)에 옮겨 적는다. Scripts/Environment/shoot_night.ps1 -ExtraCmds 가 쓴다.
static FAutoConsoleCommandWithWorldAndArgs GGrimSkySetCommand(
	TEXT("Grim.Sky.Set"),
	TEXT("<속성> <값> — 환경 설정(밤 화면) 또는 하늘 컨트롤러 값을 실행 중에 바꾼다. 예: Grim.Sky.Set NightExposureMinEV100 -1"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (Args.Num() < 2)
		{
			return;
		}
		const FName PropertyName(*Args[0]);
		// 값에 빈칸이 있으면(색 등) 나머지를 이어 붙인다
		FString Value = Args[1];
		for (int32 Index = 2; Index < Args.Num(); ++Index)
		{
			Value += TEXT(" ") + Args[Index];
		}
		auto SetOn = [&](UObject* Object)
		{
			FProperty* Property = Object ? Object->GetClass()->FindPropertyByName(PropertyName) : nullptr;
			if (!Property || !Property->ImportText_InContainer(*Value, Object, Object, PPF_None))
			{
				return false;
			}
			UE_LOG(LogGrimEnvironment, Display, TEXT("[Grim.Sky.Set] %s.%s = %s"), *Object->GetName(), *Args[0], *Value);
			return true;
		};
		if (SetOn(GetMutableDefault<UGrimEnvironmentSettings>()))
		{
			return;
		}
		bool bFound = false;
		if (World)
		{
			for (TActorIterator<AGrimSkyController> It(World); It; ++It)
			{
				bFound |= SetOn(*It);
			}
		}
		if (!bFound)
		{
			UE_LOG(LogGrimEnvironment, Warning, TEXT("[Grim.Sky.Set] 속성 %s 없음(환경 설정 · 하늘 컨트롤러)"), *Args[0]);
		}
	}));

#endif
