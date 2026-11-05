#include "Interaction/GrimElementalObject.h"
#include "Interaction/GrimElementalStateComponent.h"
#include "Interaction/GrimInteractionTags.h"
#include "Core/GrimGameplayTags.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	UStaticMesh* FindBasicShape(const TCHAR* Path)
	{
		ConstructorHelpers::FObjectFinder<UStaticMesh> Finder(Path);
		return Finder.Succeeded() ? Finder.Object : nullptr;
	}
}

AGrimElementalObject::AGrimElementalObject()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetNetDormancy(DORM_DormantAll);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(Root);

	FlameMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FlameMesh"));
	FlameMesh->SetupAttachment(Root);
	FlameMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FlameMesh->SetCastShadow(false);
	FlameMesh->SetHiddenInGame(true);
	static UStaticMesh* const ConeMesh = FindBasicShape(TEXT("/Engine/BasicShapes/Cone.Cone"));
	FlameMesh->SetStaticMesh(ConeMesh);

	FlameLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FlameLight"));
	FlameLight->SetupAttachment(Root);
	FlameLight->SetIntensityUnits(ELightUnits::Candelas);
	FlameLight->SetIntensity(0.f);
	FlameLight->SetLightColor(FLinearColor(1.f, 0.6f, 0.25f));
	FlameLight->SetVisibility(false);

	ElementalState = CreateDefaultSubobject<UGrimElementalStateComponent>(TEXT("ElementalState"));
}

void AGrimElementalObject::BeginPlay()
{
	// 컴포넌트 BeginPlay(초기 상태) 전에 묶어야 처음 상태도 연출된다.
	ElementalState->OnStateChanged.AddDynamic(this, &AGrimElementalObject::HandleElementalStateChanged);

	if (GetNetMode() != NM_DedicatedServer)
	{
		BodyMaterial = BodyMesh->CreateDynamicMaterialInstance(0);
		FlameMaterial = FlameMesh->CreateDynamicMaterialInstance(0);
		if (FlameMaterial)
		{
			FlameMaterial->SetVectorParameterValue(ColorParameterName, FlameColor);
		}
	}

	Super::BeginPlay();
	RefreshElementalVisuals();
}

FVector AGrimElementalObject::GetTargetPoint() const
{
	const FBoxSphereBounds Bounds = BodyMesh->Bounds;
	return Bounds.Origin + FVector(0.f, 0.f, Bounds.BoxExtent.Z * TargetPointHeightRatio);
}

void AGrimElementalObject::HandleElementalStateChanged(UGrimElementalStateComponent* Component, FGameplayTag StateTag, float OldIntensity, float NewIntensity)
{
	RefreshElementalVisuals();
}

void AGrimElementalObject::RefreshElementalVisuals()
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	const float Burning = ElementalState->GetStateIntensity(GrimTags::State_Burning);
	const bool bBurning = Burning > 0.f;
	FlameMesh->SetHiddenInGame(!bBurning);
	FlameLight->SetVisibility(bBurning);
	FlameLight->SetIntensity(Burning * LightCandelaPerIntensity);
	if (bBurning)
	{
		FlameMesh->SetRelativeScale3D(FlameScaleAtOne * FMath::Sqrt(Burning));
	}

	if (BodyMaterial)
	{
		FLinearColor Color = BodyColor;
		if (ElementalState->HasState(GrimInteractionTags::State_Charred))
		{
			Color = CharredColor;
		}
		const float Wet = ElementalState->GetStateIntensity(GrimTags::State_Wet);
		Color = FLinearColor::LerpUsingHSV(Color, WetColor, FMath::Clamp(Wet / WetIntensityForFullColor, 0.f, 1.f));
		BodyMaterial->SetVectorParameterValue(ColorParameterName, Color);
	}
}

// ---------------------------------------------------------------------------

AGrimCandle::AGrimCandle()
{
	static UStaticMesh* const CylinderMesh = FindBasicShape(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	BodyMesh->SetStaticMesh(CylinderMesh);
	// 기본 도형은 100cm, 가운데가 원점 → 지름 6cm · 높이 20cm 양초가 바닥에 서도록 올린다.
	BodyMesh->SetRelativeScale3D(FVector(0.06f, 0.06f, 0.2f));
	BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, 10.f));

	FlameMesh->SetRelativeLocation(FVector(0.f, 0.f, 24.f));
	FlameScaleAtOne = FVector(0.03f, 0.03f, 0.07f);

	FlameLight->SetRelativeLocation(FVector(0.f, 0.f, 30.f));
	FlameLight->SetAttenuationRadius(500.f);
	LightCandelaPerIntensity = 15.f;

	BodyColor = FLinearColor(0.95f, 0.92f, 0.8f);
	ElementalState->Material.AddTag(GrimInteractionTags::Material_Flammable);
}

AGrimCampfire::AGrimCampfire()
{
	static UStaticMesh* const CylinderMesh = FindBasicShape(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	BodyMesh->SetStaticMesh(CylinderMesh);
	// 장작 더미: 지름 80cm · 높이 15cm
	BodyMesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 0.15f));
	BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, 7.5f));

	FlameMesh->SetRelativeLocation(FVector(0.f, 0.f, 45.f));
	FlameScaleAtOne = FVector(0.4f, 0.4f, 0.6f);

	FlameLight->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
	FlameLight->SetAttenuationRadius(1500.f);
	LightCandelaPerIntensity = 120.f;

	BodyColor = FLinearColor(0.3f, 0.18f, 0.08f);
	ElementalState->Material.AddTag(GrimInteractionTags::Material_Flammable);
}

AGrimDryGrass::AGrimDryGrass()
{
	static UStaticMesh* const CubeMesh = FindBasicShape(TEXT("/Engine/BasicShapes/Cube.Cube"));
	BodyMesh->SetStaticMesh(CubeMesh);
	// 1m × 1m × 10cm 풀 덤불. 막지 않고 겹치기만 한다(전파 쿼리 · 마법 판정은 받는다).
	BodyMesh->SetRelativeScale3D(FVector(1.f, 1.f, 0.1f));
	BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, 5.f));
	BodyMesh->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
	BodyMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	FlameMesh->SetRelativeLocation(FVector(0.f, 0.f, 30.f));
	FlameScaleAtOne = FVector(0.6f, 0.6f, 0.4f);

	FlameLight->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
	FlameLight->SetAttenuationRadius(800.f);
	LightCandelaPerIntensity = 60.f;

	BodyColor = FLinearColor(0.75f, 0.65f, 0.3f);
	ElementalState->Material.AddTag(GrimInteractionTags::Material_Flammable);
	ElementalState->Material.AddTag(GrimInteractionTags::Material_Consumable);
}
