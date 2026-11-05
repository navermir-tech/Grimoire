#include "Magic/GrimSkillbook.h"
#include "Magic/GrimSpellDefinition.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GrimPlayerState.h"
#include "Magic/GrimSpellbookComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AGrimSkillbook::AGrimSkillbook()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicatingMovement(false);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// 책 크기: 가로 30 · 세로 22 · 두께 5cm(기본 상자는 100cm). 바닥에서 1m 위에 떠 있다.
	BookMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BookMesh"));
	BookMesh->SetupAttachment(Root);
	BookMesh->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
	BookMesh->SetRelativeScale3D(FVector(0.22f, 0.30f, 0.05f));
	BookMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		BookMesh->SetStaticMesh(CubeMesh.Object);
	}

	PickupZone = CreateDefaultSubobject<USphereComponent>(TEXT("PickupZone"));
	PickupZone->SetupAttachment(Root);
	PickupZone->SetRelativeLocation(FVector(0.f, 0.f, 90.f));
	PickupZone->SetSphereRadius(PickupRadius);
	PickupZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupZone->SetCollisionObjectType(ECC_WorldDynamic);
	PickupZone->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupZone->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	PickupZone->SetGenerateOverlapEvents(true);

	Spin = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("Spin"));
	Spin->RotationRate = FRotator(0.f, 30.f, 0.f);
	Spin->SetUpdatedComponent(BookMesh);
}

void AGrimSkillbook::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGrimSkillbook, TeachesSpell);
}

void AGrimSkillbook::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshCoverColor();
}

void AGrimSkillbook::BeginPlay()
{
	Super::BeginPlay();
	RefreshCoverColor();
	PickupZone->SetSphereRadius(PickupRadius);
	if (HasAuthority() && PickupRadius > 0.f)
	{
		PickupZone->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandlePickupOverlap);
	}
	else
	{
		PickupZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AGrimSkillbook::HandlePickupOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	const APawn* Pawn = Cast<APawn>(OtherActor);
	const AGrimPlayerState* PlayerState = Pawn ? Pawn->GetPlayerState<AGrimPlayerState>() : nullptr;
	if (!HasAuthority() || bConsumed || !PlayerState || !PlayerState->GetSpellbook())
	{
		return;
	}
	PlayerState->GetSpellbook()->PickUpSkillbook(this, Pawn);
}

void AGrimSkillbook::SetTeachesSpell(UGrimSpellDefinition* Spell)
{
	TeachesSpell = Spell;
	RefreshCoverColor();
}

void AGrimSkillbook::OnRep_TeachesSpell()
{
	RefreshCoverColor();
}

void AGrimSkillbook::RefreshCoverColor()
{
	if (!BookMesh || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	UMaterialInstanceDynamic* Material = Cast<UMaterialInstanceDynamic>(BookMesh->GetMaterial(0));
	if (!Material)
	{
		Material = BookMesh->CreateDynamicMaterialInstance(0);
	}
	if (Material)
	{
		Material->SetVectorParameterValue(TEXT("Color"), TeachesSpell ? TeachesSpell->Color : DefaultCoverColor);
	}
}

bool AGrimSkillbook::IsWithinReadRange(const APawn* Reader, float Slack) const
{
	if (!Reader || bConsumed)
	{
		return false;
	}
	const float Radius = ReadRadius + FMath::Max(Slack, 0.f);
	return FVector::DistSquared(Reader->GetActorLocation(), GetActorLocation()) <= FMath::Square(Radius);
}

void AGrimSkillbook::ConsumeBook()
{
	if (!HasAuthority() || bConsumed)
	{
		return;
	}
	bConsumed = true;
	Destroy();
}

// ───────────────────────── 세이브 (Skillbook.<이름> v1) ─────────────────────────

FName AGrimSkillbook::GetSaveId() const
{
	return FName(*FString::Printf(TEXT("Skillbook.%s"), *GetName()));
}

void AGrimSkillbook::WriteSaveData(FArchive& Ar)
{
	FString SpellPath = TeachesSpell ? TeachesSpell->GetPathName() : FString();
	bool bSavedConsumed = bConsumed;
	Ar << SpellPath << bSavedConsumed;
}

void AGrimSkillbook::ReadSaveData(FArchive& Ar, int32 Version)
{
	FString SpellPath;
	bool bSavedConsumed = false;
	Ar << SpellPath << bSavedConsumed;
	if (!HasAuthority() || Ar.IsError())
	{
		return;
	}
	if (!SpellPath.IsEmpty())
	{
		if (UGrimSpellDefinition* Spell = Cast<UGrimSpellDefinition>(FSoftObjectPath(SpellPath).TryLoad()); Spell && Spell != TeachesSpell)
		{
			SetTeachesSpell(Spell);
		}
	}
	if (bSavedConsumed)
	{
		ConsumeBook();
	}
}

AGrimSkillbook* AGrimSkillbook::FindNearestReadable(const APawn* Reader)
{
	UWorld* World = Reader ? Reader->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}
	AGrimSkillbook* Best = nullptr;
	double BestDistSq = TNumericLimits<double>::Max();
	for (TActorIterator<AGrimSkillbook> It(World); It; ++It)
	{
		if (!It->GetTeachesSpell() || !It->IsWithinReadRange(Reader))
		{
			continue;
		}
		const double DistSq = FVector::DistSquared(Reader->GetActorLocation(), It->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = *It;
		}
	}
	return Best;
}
