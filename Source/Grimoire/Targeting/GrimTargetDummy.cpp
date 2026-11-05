#include "Targeting/GrimTargetDummy.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

AGrimTargetDummy::AGrimTargetDummy()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;
	SetReplicatingMovement(false);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		Mesh->SetStaticMesh(SphereMesh.Object);
	}
	// 지름 60cm. 후보 오버랩(WorldDynamic) · 시야 트레이스에 걸린다.
	Mesh->SetRelativeScale3D(FVector(0.6f));
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
	Mesh->SetMobility(EComponentMobility::Movable);
}

void AGrimTargetDummy::BeginPlay()
{
	Super::BeginPlay();
	StartLocation = GetActorLocation();
	SetActorTickEnabled(!MoveAmplitude.IsNearlyZero());
}

void AGrimTargetDummy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const double Time = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	const double Phase = 2.0 * UE_DOUBLE_PI * Time / FMath::Max(MovePeriodSeconds, 0.1f);
	SetActorLocation(StartLocation + MoveAmplitude * FMath::Sin(Phase));
}

FVector AGrimTargetDummy::GetTargetPoint() const
{
	return GetActorLocation() + GetActorRotation().RotateVector(TargetPointOffset);
}
