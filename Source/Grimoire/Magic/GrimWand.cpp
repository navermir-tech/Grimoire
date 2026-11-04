#include "Magic/GrimWand.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// 마네킹(SKM_Manny_Simple) hand_r 공간: 손가락 = −X, 손바닥 = +Y, 엄지 쪽 = +Z(차렷 자세에서 폰 앞).
	// 주먹 가운데(손가락 쪽 9cm, 손바닥 쪽 3.5cm)에 두고 Grip +X 를 손 +Z 로 — 완드가 주먹에서 엄지 쪽으로 나온다.
	// Grim.Magic.DumpWands 가 이 축들을 로그로 보여 준다.
	const FVector GripLocation = FVector(-9.f, 3.5f, 0.f);
	const FRotator GripRotation = FRotator(90.f, 0.f, 0.f);
}

AGrimWand::AGrimWand()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	// 부착 정보(AttachmentReplication)는 움직임 복제가 켜져 있어야 클라이언트로 간다.
	SetReplicatingMovement(true);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// 손 소켓(hand_r) 공간에서 쥐는 자세. +X = 완드 끝. 값은 마네킹 hand_r 축을 재서 정했다(docs/systems/magic 「완드」).
	Grip = CreateDefaultSubobject<USceneComponent>(TEXT("Grip"));
	Grip->SetupAttachment(Root);
	Grip->SetRelativeLocationAndRotation(GripLocation, GripRotation);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Grip);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCastShadow(false);

	// M1 임시 모양: 엔진 기본 실린더(높이 100, 원점이 가운데, +Z 축)를 가늘게 해 Grip +X 로 눕힌다.
	// 길이 35cm 중 7.5cm 는 주먹 뒤로, 27.5cm 는 앞으로 나온다.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (Cylinder.Succeeded())
	{
		Mesh->SetStaticMesh(Cylinder.Object);
	}
	Mesh->SetRelativeLocationAndRotation(FVector(10.f, 0.f, 0.f), FRotator(-90.f, 0.f, 0.f));
	Mesh->SetRelativeScale3D(FVector(0.03f, 0.03f, 0.35f));

	Muzzle = CreateDefaultSubobject<USceneComponent>(TEXT("Muzzle"));
	Muzzle->SetupAttachment(Grip);
	Muzzle->SetRelativeLocation(FVector(27.5f, 0.f, 0.f));
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
