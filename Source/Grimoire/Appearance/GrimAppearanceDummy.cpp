#include "Appearance/GrimAppearanceDummy.h"
#include "Appearance/GrimAppearanceComponent.h"
#include "Appearance/GrimAppearanceSettings.h"
#include "Animation/AnimationAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"

AGrimAppearanceDummy::AGrimAppearanceDummy()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetNetUpdateFrequency(1.f);

	Figure = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Figure"));
	Figure->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// 마네킹은 +Y 를 본다 — 액터 앞(+X)을 보게
	Figure->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	SetRootComponent(Figure);

	Appearance = CreateDefaultSubobject<UGrimAppearanceComponent>(TEXT("Appearance"));
}

void AGrimAppearanceDummy::BeginPlay()
{
	const UGrimAppearanceSettings* Settings = UGrimAppearanceSettings::Get();
	if (!Figure->GetSkeletalMeshAsset() && Settings->BodyMeshes.Num() > 0)
	{
		Figure->SetSkeletalMeshAsset(Settings->BodyMeshes[0].LoadSynchronous());
	}
	if (GetNetMode() != NM_DedicatedServer)
	{
		if (UAnimationAsset* Idle = Settings->NpcIdleAnimation.LoadSynchronous())
		{
			Figure->PlayAnimation(Idle, true);
			// 체형이 바뀌면 몸 메시가 바뀌며 애님이 다시 만들어진다 — 서 있는 애님을 다시 건다
			Appearance->OnAppearanceApplied.AddWeakLambda(this, [this, Idle](UGrimAppearanceComponent*)
			{
				if (!Figure->IsPlaying())
				{
					Figure->PlayAnimation(Idle, true);
				}
			});
		}
	}
	Appearance->SetLeaderMesh(Figure);
	Super::BeginPlay();
}

void AGrimAppearanceDummy::GenerateAppearance(FName Rule, int32 Seed)
{
	if (HasAuthority())
	{
		Appearance->SetAppearance(FGrimAppearanceMath::GenerateFromRule(UGrimAppearanceSettings::GetCatalog(), Rule, Seed));
	}
}
