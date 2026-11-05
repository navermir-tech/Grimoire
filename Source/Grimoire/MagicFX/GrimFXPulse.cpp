#include "MagicFX/GrimFXPulse.h"
#include "MagicFX/GrimMagicFXStatics.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	UStaticMesh* LoadShapeMesh(EGrimFXPulseShape Shape)
	{
		const TCHAR* Path = Shape == EGrimFXPulseShape::Cone ? TEXT("/Engine/BasicShapes/Cone.Cone") : TEXT("/Engine/BasicShapes/Sphere.Sphere");
		return LoadObject<UStaticMesh>(nullptr, Path);
	}
}

AGrimFXPulse::AGrimFXPulse()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	bReplicates = false;
	SetCanBeDamaged(false);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCastShadow(false);
	Mesh->bReceivesDecals = false;

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Root);
	Light->SetCastShadows(false);
	Light->SetIntensityUnits(ELightUnits::Unitless);
	Light->SetIntensity(0.f);
}

AGrimFXPulse* AGrimFXPulse::Spawn(UWorld* World, const FGrimFXPulseSpec& Spec, const FVector& Location, const FVector& Direction, const FLinearColor& Color, const FVector& SizeOverride)
{
	if (!World || !Spec.IsEnabled() || World->GetNetMode() == NM_DedicatedServer)
	{
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	AGrimFXPulse* Pulse = World->SpawnActor<AGrimFXPulse>(AGrimFXPulse::StaticClass(), FTransform(Location), Params);
	if (Pulse)
	{
		Pulse->Init(Spec, Direction, Color, SizeOverride);
	}
	return Pulse;
}

void AGrimFXPulse::Init(const FGrimFXPulseSpec& InSpec, const FVector& Direction, const FLinearColor& InColor, const FVector& SizeOverride)
{
	Spec = InSpec;
	Color = InColor;
	StartSize = Spec.StartSize;
	EndSize = SizeOverride.IsNearlyZero() ? Spec.EndSize : SizeOverride;
	if (!SizeOverride.IsNearlyZero())
	{
		// 끝 크기를 바꾸면 시작 크기도 같은 비율로.
		const FVector Ratio(
			Spec.EndSize.X > 0.f ? Spec.StartSize.X / Spec.EndSize.X : 0.1f,
			Spec.EndSize.Y > 0.f ? Spec.StartSize.Y / Spec.EndSize.Y : 0.1f,
			Spec.EndSize.Z > 0.f ? Spec.StartSize.Z / Spec.EndSize.Z : 0.1f);
		StartSize = EndSize * Ratio;
	}

	UStaticMesh* ShapeMesh = LoadShapeMesh(Spec.Shape);
	Mesh->SetStaticMesh(ShapeMesh);

	const FVector Axis = Direction.IsNearlyZero() ? FVector::UpVector : Direction.GetSafeNormal();
	if (Spec.Shape == EGrimFXPulseShape::Cone)
	{
		// 원뿔 메시의 꼭짓점(+Z 끝)을 시작점에 두고 몸통이 Direction 으로 퍼진다: 메시 +Z = −Direction.
		SetActorRotation(FRotationMatrix::MakeFromZ(-Axis).Rotator());
	}
	else if (Spec.Shape == EGrimFXPulseShape::Disc)
	{
		SetActorRotation(FRotationMatrix::MakeFromZ(Axis).Rotator());
	}

	if (UMaterialInterface* BaseMaterial = UGrimMagicFXStatics::GetGlowMaterial())
	{
		Material = Mesh->CreateDynamicMaterialInstance(0, BaseMaterial);
	}
	if (Material)
	{
		Material->SetVectorParameterValue(TEXT("Color"), Color);
		Material->SetScalarParameterValue(TEXT("Glow"), Spec.Glow);
		Material->SetScalarParameterValue(TEXT("RimMix"), Spec.RimMix);
	}

	Light->SetLightColor(Color);
	Light->SetAttenuationRadius(Spec.LightRadius);
	Light->SetVisibility(Spec.LightIntensity > 0.f);

	Apply(0.f);
}

void AGrimFXPulse::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	Elapsed += DeltaSeconds;
	const float Alpha = Spec.Lifetime > 0.f ? Elapsed / Spec.Lifetime : 1.f;
	if (Alpha >= 1.f)
	{
		Destroy();
		return;
	}
	Apply(Alpha);
}

void AGrimFXPulse::Apply(float Alpha)
{
	// 빨리 커지고(ease-out) 천천히 사라진다.
	const float Grow = 1.f - FMath::Square(1.f - Alpha);
	const FVector Size = FMath::Lerp(StartSize, EndSize, Grow);
	const FVector Scale = Size / 100.f; // 엔진 기본 도형은 100cm.
	Mesh->SetRelativeScale3D(Scale.ComponentMax(FVector(0.001f)));

	if (Spec.Shape == EGrimFXPulseShape::Cone && Mesh->GetStaticMesh())
	{
		const FBoxSphereBounds Bounds = Mesh->GetStaticMesh()->GetBounds();
		const float TopZ = Bounds.Origin.Z + Bounds.BoxExtent.Z;
		Mesh->SetRelativeLocation(FVector(0.f, 0.f, -TopZ * Scale.Z));
	}

	const float Fade = FMath::Pow(1.f - Alpha, 1.5f);
	if (Material)
	{
		Material->SetScalarParameterValue(TEXT("Opacity"), Fade);
	}
	if (Spec.LightIntensity > 0.f)
	{
		Light->SetIntensity(Spec.LightIntensity * FMath::Square(1.f - Alpha));
	}
}
