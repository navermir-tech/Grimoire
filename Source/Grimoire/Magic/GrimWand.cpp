#include "Magic/GrimWand.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AGrimWand::AGrimWand()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	// 부착 정보(AttachmentReplication)는 움직임 복제가 켜져 있어야 클라이언트로 간다.
	SetReplicatingMovement(true);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCastShadow(false);
	RootComponent = Mesh;

	// M1 임시 모양: 엔진 기본 실린더를 가늘게. 손 소켓 기준 +X 로 뻗는다.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cylinder.Succeeded())
	{
		Mesh->SetStaticMesh(Cylinder.Object);
	}
	Mesh->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	Mesh->SetRelativeScale3D(FVector(0.03f, 0.03f, 0.35f));

	Muzzle = CreateDefaultSubobject<USceneComponent>(TEXT("Muzzle"));
	Muzzle->SetupAttachment(Mesh);
	// 실린더는 높이 100, 원점이 가운데 — 끝은 +Z 50.
	Muzzle->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
}

FTransform AGrimWand::GetMuzzleTransform() const
{
	return Muzzle ? Muzzle->GetComponentTransform() : GetActorTransform();
}

float AGrimWand::GetElementPower(FGameplayTag Element) const
{
	if (const float* Power = ElementPower.Find(Element))
	{
		return *Power;
	}
	return 1.f;
}
