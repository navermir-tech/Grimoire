#include "Environment/GrimGrassExclusionVolume.h"

#include "Components/BoxComponent.h"
#include "LandscapeProxy.h"

AGrimGrassExclusionVolume::AGrimGrassExclusionVolume()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	SetCanBeDamaged(false);

	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetBoxExtent(FVector(500.f, 500.f, 300.f));
	Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Box->SetGenerateOverlapEvents(false);
	Box->SetCanEverAffectNavigation(false);
	Box->ShapeColor = FColor(120, 200, 80);
	RootComponent = Box;
}

void AGrimGrassExclusionVolume::RegisterExclusion()
{
	if (!Box || !GetWorld())
	{
		return;
	}
	Box->UpdateBounds();
	// 같은 주인(이 액터)으로 다시 넣으면 엔진이 상자를 바꿔 낀다
	ALandscapeProxy::AddExclusionBox(FWeakObjectPtr(this), Box->Bounds.GetBox());
	bRegistered = true;
}

void AGrimGrassExclusionVolume::UnregisterExclusion()
{
	if (bRegistered)
	{
		ALandscapeProxy::RemoveExclusionBox(FWeakObjectPtr(this));
		bRegistered = false;
	}
}

void AGrimGrassExclusionVolume::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();
	RegisterExclusion();
}

void AGrimGrassExclusionVolume::UnregisterAllComponents(bool bForReregister)
{
	if (!bForReregister)
	{
		UnregisterExclusion();
	}
	Super::UnregisterAllComponents(bForReregister);
}

void AGrimGrassExclusionVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterExclusion();
	Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR
void AGrimGrassExclusionVolume::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);
	if (bFinished)
	{
		RegisterExclusion();
	}
}

void AGrimGrassExclusionVolume::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RegisterExclusion();
}
#endif
