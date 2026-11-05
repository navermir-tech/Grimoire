#include "Environment/GrimClimateRegionVolume.h"
#include "Environment/GrimWeatherMath.h"
#include "Environment/GrimWeatherSubsystem.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"

AGrimClimateRegionVolume::AGrimClimateRegionVolume()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	SetCanBeDamaged(false);

	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	Bounds->SetBoxExtent(FVector(5000.f, 5000.f, 5000.f));
	Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Bounds->SetGenerateOverlapEvents(false);
	Bounds->SetCanEverAffectNavigation(false);
	Bounds->bHiddenInGame = true;
	Bounds->ShapeColor = FColor(80, 200, 255);
	SetRootComponent(Bounds);
}

float AGrimClimateRegionVolume::GetBlendWeight(const FVector& WorldLocation) const
{
	if (!Bounds)
	{
		return 0.f;
	}
	const FTransform& Transform = Bounds->GetComponentTransform();
	// 크기 배율까지 푼 로컬 좌표 · 반 크기(스케일된 상자도 맞게).
	const FVector Scale = Transform.GetScale3D().GetAbs();
	const FVector Local = Transform.GetRotation().UnrotateVector(WorldLocation - Transform.GetLocation());
	const FVector Extent = Bounds->GetUnscaledBoxExtent() * Scale;
	return FGrimWeatherMath::GetBoxBlendWeight(Local, Extent, BlendDistance);
}

void AGrimClimateRegionVolume::BeginPlay()
{
	Super::BeginPlay();
	if (UGrimWeatherSubsystem* Weather = GetWorld()->GetSubsystem<UGrimWeatherSubsystem>())
	{
		Weather->RegisterRegionVolume(this);
	}
}

void AGrimClimateRegionVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UGrimWeatherSubsystem* Weather = World->GetSubsystem<UGrimWeatherSubsystem>())
		{
			Weather->UnregisterRegionVolume(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}
