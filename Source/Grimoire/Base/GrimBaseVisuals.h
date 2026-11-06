#pragma once

#include "CoreMinimal.h"

class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class AActor;
class USceneComponent;
struct FGrimShapeBox;

/** 아트 전 임시 모양 — 엔진 기본 도형 + BasicShapeMaterial 색(지은 것 · 고스트 공통). */
namespace GrimBaseVisuals
{
	GRIMOIRE_API UStaticMesh* GetCubeMesh();
	GRIMOIRE_API UStaticMesh* GetCylinderMesh();
	GRIMOIRE_API UStaticMesh* GetSphereMesh();
	GRIMOIRE_API UStaticMesh* GetConeMesh();

	/** 메시 컴포넌트의 색(BasicShapeMaterial 의 Color 파라미터 — 동적 머티리얼을 만들거나 다시 쓴다) */
	GRIMOIRE_API UMaterialInstanceDynamic* SetColor(UStaticMeshComponent* Component, const FLinearColor& Color);

	/**
	 * 상자 하나를 런타임 컴포넌트로 만든다(엔진 큐브 100 cm 를 Extent 로 늘림). bCollision 이면 막는 콜리전(WorldStatic).
	 * StableName 을 주면(복제 액터) 서버 · 클라이언트가 같은 이름으로 만들고 SetNetAddressable — 캐릭터가 그 위에 서면 이동 기반(MovementBase)으로
	 * 복제되어야 해서다(이름 없는 런타임 컴포넌트는 「NOT Supported」로 이동 보정이 깨진다).
	 */
	GRIMOIRE_API UStaticMeshComponent* AddBox(AActor* Owner, USceneComponent* Parent, const FGrimShapeBox& Box, const FLinearColor& Color, bool bCollision, FName StableName = NAME_None);

	/** 고정 컴포넌트(생성자)를 상자 모양으로 맞춘다 */
	GRIMOIRE_API void ShapeAsBox(UStaticMeshComponent* Component, const FVector& Center, const FVector& Extent);
}
