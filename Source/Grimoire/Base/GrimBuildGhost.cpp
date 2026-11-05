#include "Base/GrimBuildGhost.h"
#include "Base/GrimBaseSettings.h"
#include "Base/GrimBaseVisuals.h"
#include "Components/StaticMeshComponent.h"

namespace GrimBuildGhostLocal // 유니티 빌드에서 이름이 부딪히지 않게
{
	const FLinearColor ValidColor(0.2f, 0.9f, 0.3f);
	const FLinearColor InvalidColor(0.95f, 0.15f, 0.1f);
}

AGrimBuildGhost::AGrimBuildGhost()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	SetActorEnableCollision(false);
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void AGrimBuildGhost::ShowPiece(FName PieceId)
{
	if (PieceId == ShownPiece)
	{
		return;
	}
	ShownPiece = PieceId;
	ShownValid = -1;
	for (UStaticMeshComponent* Box : Boxes)
	{
		if (Box)
		{
			Box->DestroyComponent();
		}
	}
	Boxes.Reset();

	const FGrimBuildPieceDefinition* Def = UGrimBaseSettings::GetDatabase().FindPiece(PieceId);
	if (!Def)
	{
		return;
	}
	TArray<FGrimShapeBox> Shape;
	if (Def->IsStructural())
	{
		UGrimBaseSettings::Get()->MakeGrid(FVector::ZeroVector, 0.f).GetShape(Def->Row.Category, Shape);
	}
	else
	{
		// 시설은 바닥 위에 놓이는 상자.
		FGrimShapeBox& Box = Shape.AddDefaulted_GetRef();
		Box.Extent = Def->Row.Extent;
		Box.Center = FVector(0.f, 0.f, Def->Row.Extent.Z);
	}
	for (const FGrimShapeBox& Box : Shape)
	{
		Boxes.Add(GrimBaseVisuals::AddBox(this, GetRootComponent(), Box, GrimBuildGhostLocal::ValidColor, false));
	}
}

void AGrimBuildGhost::SetValid(bool bValid)
{
	if (ShownValid == (bValid ? 1 : 0))
	{
		return;
	}
	ShownValid = bValid ? 1 : 0;
	for (UStaticMeshComponent* Box : Boxes)
	{
		GrimBaseVisuals::SetColor(Box, bValid ? GrimBuildGhostLocal::ValidColor : GrimBuildGhostLocal::InvalidColor);
	}
}
