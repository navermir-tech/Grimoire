#include "Base/GrimBaseCore.h"
#include "Base/GrimBaseSettings.h"
#include "Base/GrimBaseSubsystem.h"
#include "Base/GrimBaseVisuals.h"
#include "Base/GrimBuilderComponent.h"
#include "Base/GrimStructureComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "GrimBaseCore"

const FName AGrimBaseCore::TabId(TEXT("Base"));

AGrimBaseCore::AGrimBaseCore()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicatingMovement(false);
	SetNetUpdateFrequency(1.f);
	SetNetCullDistanceSquared(FMath::Square(60000.f));

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));

	// 받침돌: 지름 120 · 높이 40, 위에 떠 있는 결정, 바닥 마법진 고리(지름 300).
	Pedestal = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pedestal"));
	Pedestal->SetupAttachment(Root);
	Pedestal->SetStaticMesh(CylinderMesh.Object);
	Pedestal->SetRelativeLocation(FVector(0.f, 0.f, 20.f));
	Pedestal->SetRelativeScale3D(FVector(1.2f, 1.2f, 0.4f));
	Pedestal->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	Crystal = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Crystal"));
	Crystal->SetupAttachment(Root);
	Crystal->SetStaticMesh(ConeMesh.Object);
	Crystal->SetRelativeLocation(FVector(0.f, 0.f, 120.f));
	Crystal->SetRelativeScale3D(FVector(0.35f, 0.35f, 0.9f));
	Crystal->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Crystal->SetCollisionResponseToAllChannels(ECR_Ignore);
	Crystal->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Crystal->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);

	Ring = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ring"));
	Ring->SetupAttachment(Root);
	Ring->SetStaticMesh(CylinderMesh.Object);
	Ring->SetRelativeLocation(FVector(0.f, 0.f, 1.f));
	Ring->SetRelativeScale3D(FVector(3.f, 3.f, 0.02f));
	Ring->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(Crystal);
	Glow->SetRelativeLocation(FVector(0.f, 0.f, 40.f));
	Glow->SetIntensityUnits(ELightUnits::Candelas);
	Glow->SetIntensity(40.f);
	Glow->SetAttenuationRadius(900.f);
	Glow->SetLightColor(FLinearColor(0.45f, 0.65f, 1.f));
	Glow->SetCastShadows(false);

	Structure = CreateDefaultSubobject<UGrimStructureComponent>(TEXT("Structure"));
}

void AGrimBaseCore::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGrimBaseCore, OwnerId);
	DOREPLIFETIME(AGrimBaseCore, OwnerName);
}

void AGrimBaseCore::BeginPlay()
{
	Super::BeginPlay();
	GrimBaseVisuals::SetColor(Pedestal, FLinearColor(0.35f, 0.33f, 0.38f));
	GrimBaseVisuals::SetColor(Crystal, FLinearColor(0.4f, 0.7f, 1.f));
	GrimBaseVisuals::SetColor(Ring, FLinearColor(0.25f, 0.45f, 0.9f));
	if (UGrimBaseSubsystem* Subsystem = UGrimBaseSubsystem::Get(this))
	{
		Subsystem->RegisterCore(this);
	}
}

void AGrimBaseCore::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGrimBaseSubsystem* Subsystem = UGrimBaseSubsystem::Get(this))
	{
		Subsystem->UnregisterCore(this);
	}
	Super::EndPlay(EndPlayReason);
}

FGrimBuildGrid AGrimBaseCore::GetGrid() const
{
	return UGrimBaseSettings::Get()->MakeGrid(GetActorLocation(), GetActorRotation().Yaw);
}

bool AGrimBaseCore::IsInside(const FVector& Location, float Slack) const
{
	const float Radius = UGrimBaseSettings::Get()->BaseRadius + Slack;
	return FVector::DistSquared2D(Location, GetActorLocation()) <= FMath::Square(Radius);
}

void AGrimBaseCore::SetOwnerInfo(const FString& InOwnerId, const FString& InOwnerName)
{
	OwnerId = InOwnerId;
	OwnerName = InOwnerName;
}

FText AGrimBaseCore::GetInteractPrompt(const APawn* Pawn) const
{
	return OwnerName.IsEmpty()
		? LOCTEXT("PromptNoOwner", "거점 핵 — 거점 살피기 · 거점 수리")
		: FText::Format(LOCTEXT("Prompt", "{0} 의 거점 핵 — 거점 살피기 · 거점 수리"), FText::FromString(OwnerName));
}

void AGrimBaseCore::InteractLocal(UGrimBuilderComponent* Builder)
{
	if (Builder)
	{
		Builder->OpenStation(this);
	}
}

// ── 세이브 (BaseCore.<이름> v1) ──

FName AGrimBaseCore::GetSaveId() const
{
	return FName(*FString::Printf(TEXT("BaseCore.%s"), *GetName()));
}

void AGrimBaseCore::WriteSaveData(FArchive& Ar)
{
	FString SavedId = OwnerId;
	FString SavedName = OwnerName;
	Ar << SavedId;
	Ar << SavedName;
}

void AGrimBaseCore::ReadSaveData(FArchive& Ar, int32 Version)
{
	FString SavedId;
	FString SavedName;
	Ar << SavedId;
	Ar << SavedName;
	if (!Ar.IsError())
	{
		OwnerId = SavedId;
		OwnerName = SavedName;
	}
}

#undef LOCTEXT_NAMESPACE
