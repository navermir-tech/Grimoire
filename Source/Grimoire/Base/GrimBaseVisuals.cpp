#include "Base/GrimBaseVisuals.h"
#include "Base/GrimBaseTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace GrimBaseVisualsLocal // 유니티 빌드에서 이름이 부딪히지 않게
{
	UStaticMesh* LoadBasicShape(const TCHAR* Path)
	{
		return LoadObject<UStaticMesh>(nullptr, Path);
	}
}

UStaticMesh* GrimBaseVisuals::GetCubeMesh()
{
	static TWeakObjectPtr<UStaticMesh> Cached;
	if (!Cached.IsValid())
	{
		Cached = GrimBaseVisualsLocal::LoadBasicShape(TEXT("/Engine/BasicShapes/Cube.Cube"));
	}
	return Cached.Get();
}

UStaticMesh* GrimBaseVisuals::GetCylinderMesh()
{
	static TWeakObjectPtr<UStaticMesh> Cached;
	if (!Cached.IsValid())
	{
		Cached = GrimBaseVisualsLocal::LoadBasicShape(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	}
	return Cached.Get();
}

UStaticMesh* GrimBaseVisuals::GetSphereMesh()
{
	static TWeakObjectPtr<UStaticMesh> Cached;
	if (!Cached.IsValid())
	{
		Cached = GrimBaseVisualsLocal::LoadBasicShape(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	}
	return Cached.Get();
}

UStaticMesh* GrimBaseVisuals::GetConeMesh()
{
	static TWeakObjectPtr<UStaticMesh> Cached;
	if (!Cached.IsValid())
	{
		Cached = GrimBaseVisualsLocal::LoadBasicShape(TEXT("/Engine/BasicShapes/Cone.Cone"));
	}
	return Cached.Get();
}

UMaterialInstanceDynamic* GrimBaseVisuals::SetColor(UStaticMeshComponent* Component, const FLinearColor& Color)
{
	if (!Component || !Component->GetStaticMesh())
	{
		return nullptr;
	}
	UMaterialInstanceDynamic* Material = Cast<UMaterialInstanceDynamic>(Component->GetMaterial(0));
	if (!Material)
	{
		Material = Component->CreateDynamicMaterialInstance(0);
	}
	if (Material)
	{
		Material->SetVectorParameterValue(TEXT("Color"), Color);
	}
	return Material;
}

void GrimBaseVisuals::ShapeAsBox(UStaticMeshComponent* Component, const FVector& Center, const FVector& Extent)
{
	if (!Component)
	{
		return;
	}
	// 엔진 큐브는 한 변 100 cm, 피벗이 가운데.
	Component->SetRelativeLocation(Center);
	Component->SetRelativeScale3D(Extent / 50.f);
}

UStaticMeshComponent* GrimBaseVisuals::AddBox(AActor* Owner, USceneComponent* Parent, const FGrimShapeBox& Box, const FLinearColor& Color, bool bCollision, FName StableName)
{
	if (!Owner || !Parent)
	{
		return nullptr;
	}
	UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(Owner, StableName, RF_Transient);
	if (!StableName.IsNone())
	{
		Component->SetNetAddressable();
	}
	Component->SetStaticMesh(GetCubeMesh());
	Component->SetMobility(EComponentMobility::Movable);
	Component->SetupAttachment(Parent);
	Component->SetRelativeRotation(Box.Rotation);
	ShapeAsBox(Component, Box.Center, Box.Extent);
	if (bCollision)
	{
		Component->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Component->SetCollisionObjectType(ECC_WorldStatic);
		Component->SetCanEverAffectNavigation(true);
	}
	else
	{
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetCanEverAffectNavigation(false);
		Component->SetCastShadow(false);
	}
	Component->SetGenerateOverlapEvents(false);
	Component->RegisterComponent();
	Owner->AddInstanceComponent(Component);
	SetColor(Component, Color);
	return Component;
}
