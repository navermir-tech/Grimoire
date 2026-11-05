#include "Factions/GrimNoCombatVolume.h"
#include "Quest/GrimQuestTypes.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

namespace GrimFactionTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Zone_NoCombat, "Zone.NoCombat", "전투 금지 구역 안(마법탑 실내 — 중립 지대). 마법 쪽이 피해 · 적대 시전을 막는다");
}

namespace GrimNoCombatVolumeLocal
{
	UAbilitySystemComponent* FindASC(const APawn* Pawn)
	{
		if (!Pawn)
		{
			return nullptr;
		}
		if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn))
		{
			return ASC;
		}
		return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn->GetPlayerState());
	}
}

AGrimNoCombatVolume::AGrimNoCombatVolume()
{
	PrimaryActorTick.bCanEverTick = false;
	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	Bounds->SetBoxExtent(FVector(500.f));
	Bounds->SetCollisionProfileName(TEXT("Trigger"));
	Bounds->SetGenerateOverlapEvents(true);
	Bounds->SetCanEverAffectNavigation(false);
	SetRootComponent(Bounds);
	SetActorHiddenInGame(true);
}

void AGrimNoCombatVolume::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority())
	{
		return;
	}
	// World Partition 셀이 내렸다 다시 올라오면 BeginPlay 가 또 불린다 — 두 번 묶지 않게 AddUniqueDynamic.
	Bounds->OnComponentBeginOverlap.AddUniqueDynamic(this, &AGrimNoCombatVolume::HandleBeginOverlap);
	Bounds->OnComponentEndOverlap.AddUniqueDynamic(this, &AGrimNoCombatVolume::HandleEndOverlap);
	// 이미 안에 있는 폰(시작 지점이 탑 안)
	TArray<AActor*> Inside;
	Bounds->GetOverlappingActors(Inside, APawn::StaticClass());
	for (AActor* Actor : Inside)
	{
		RefreshPawn(Cast<APawn>(Actor));
	}
}

void AGrimNoCombatVolume::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	RefreshPawn(Cast<APawn>(OtherActor));
}

void AGrimNoCombatVolume::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	RefreshPawn(Cast<APawn>(OtherActor));
}

void AGrimNoCombatVolume::RefreshPawn(APawn* Pawn)
{
	UAbilitySystemComponent* ASC = GrimNoCombatVolumeLocal::FindASC(Pawn);
	if (!ASC || !Pawn->HasAuthority() || !Pawn->GetWorld())
	{
		return;
	}
	bool bInside = false;
	for (TActorIterator<AGrimNoCombatVolume> It(Pawn->GetWorld()); It && !bInside; ++It)
	{
		bInside = It->Bounds && It->Bounds->IsOverlappingActor(Pawn);
	}
	const bool bHad = ASC->GetTagCount(GrimFactionTags::Zone_NoCombat) > 0;
	if (bHad != bInside)
	{
		ASC->SetLooseGameplayTagCount(GrimFactionTags::Zone_NoCombat, bInside ? 1 : 0, EGameplayTagReplicationState::TagOnly);
		UE_LOG(LogGrimQuest, Display, TEXT("[Server] %s Zone.NoCombat %s"), *Pawn->GetName(), bInside ? TEXT("붙임(탑 실내)") : TEXT("뗌"));
	}
}
