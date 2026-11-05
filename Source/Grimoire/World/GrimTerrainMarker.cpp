#include "World/GrimTerrainMarker.h"

#include "Components/DrawSphereComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "UObject/AssetRegistryTagsContext.h"

const FName AGrimTerrainMarker::AssetRegistryMarkerTagName(TEXT("GrimTerrainMarkerTag"));

AGrimTerrainMarker::AGrimTerrainMarker()
{
	PrimaryActorTick.bCanEverTick = false;
	SetCanBeDamaged(false);

	Area = CreateDefaultSubobject<UDrawSphereComponent>(TEXT("Area"));
	Area->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Area->SetGenerateOverlapEvents(false);
	Area->SetHiddenInGame(true);
	Area->ShapeColor = FColor(80, 200, 255);
	Area->SetSphereRadius(Radius);
	RootComponent = Area;
}

void AGrimTerrainMarker::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (Area)
	{
		Area->SetSphereRadius(Radius);
	}
}

void AGrimTerrainMarker::GetAssetRegistryTags(FAssetRegistryTagsContext Context) const
{
	Super::GetAssetRegistryTags(Context);
	Context.AddTag(FAssetRegistryTag(AssetRegistryMarkerTagName, MarkerTag.ToString(), FAssetRegistryTag::TT_Alphabetical));
}

TArray<AGrimTerrainMarker*> AGrimTerrainMarker::FindMarkers(const UObject* WorldContextObject, FGameplayTag Tag)
{
	TArray<AGrimTerrainMarker*> Result;
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World)
	{
		return Result;
	}
	for (TActorIterator<AGrimTerrainMarker> It(World); It; ++It)
	{
		if (!Tag.IsValid() || It->MarkerTag.MatchesTag(Tag))
		{
			Result.Add(*It);
		}
	}
	return Result;
}
