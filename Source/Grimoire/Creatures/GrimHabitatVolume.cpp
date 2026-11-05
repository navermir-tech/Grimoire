#include "Creatures/GrimHabitatVolume.h"
#include "Creatures/GrimEcosystemSubsystem.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"

AGrimHabitatVolume::AGrimHabitatVolume()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	SetCanBeDamaged(false);

	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	Bounds->SetBoxExtent(FVector(1500.f, 1500.f, 500.f));
	Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Bounds->SetGenerateOverlapEvents(false);
	Bounds->SetCanEverAffectNavigation(false);
	Bounds->bHiddenInGame = true;
	Bounds->ShapeColor = FColor(120, 220, 90);
	SetRootComponent(Bounds);
}

FVector AGrimHabitatVolume::GetRandomPoint(FRandomStream& Rng) const
{
	if (!Bounds)
	{
		return GetActorLocation();
	}
	const FTransform& Transform = Bounds->GetComponentTransform();
	const FVector Extent = Bounds->GetUnscaledBoxExtent();
	const FVector Local(Rng.FRandRange(-Extent.X, Extent.X), Rng.FRandRange(-Extent.Y, Extent.Y), 0.f);
	return Transform.TransformPosition(Local);
}

float AGrimHabitatVolume::GetDistanceTo(const FVector& Location) const
{
	if (!Bounds)
	{
		return FVector::Dist(Location, GetActorLocation());
	}
	const FTransform& Transform = Bounds->GetComponentTransform();
	const FVector Scale = Transform.GetScale3D().GetAbs();
	const FVector Local = Transform.GetRotation().UnrotateVector(Location - Transform.GetLocation());
	const FVector Extent = Bounds->GetUnscaledBoxExtent() * Scale;
	const FVector Outside(
		FMath::Max(0.0, FMath::Abs(Local.X) - Extent.X),
		FMath::Max(0.0, FMath::Abs(Local.Y) - Extent.Y),
		FMath::Max(0.0, FMath::Abs(Local.Z) - Extent.Z));
	return Outside.Size();
}

void AGrimHabitatVolume::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		if (UGrimEcosystemSubsystem* Ecosystem = GetWorld()->GetSubsystem<UGrimEcosystemSubsystem>())
		{
			Ecosystem->RegisterHabitat(this);
		}
	}
}

void AGrimHabitatVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UGrimEcosystemSubsystem* Ecosystem = World->GetSubsystem<UGrimEcosystemSubsystem>())
		{
			Ecosystem->UnregisterHabitat(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}
