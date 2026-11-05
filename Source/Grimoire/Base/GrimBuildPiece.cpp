#include "Base/GrimBuildPiece.h"
#include "Base/GrimBaseCore.h"
#include "Base/GrimBaseSettings.h"
#include "Base/GrimBaseSubsystem.h"
#include "Base/GrimBaseTags.h"
#include "Base/GrimBaseVisuals.h"
#include "Base/GrimBuilderComponent.h"
#include "Base/GrimStructureComponent.h"
#include "Interaction/GrimElementalStateComponent.h"
#include "Interaction/GrimInteractionTags.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "GrimBuildPiece"

AGrimBuildPiece::AGrimBuildPiece()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicatingMovement(false);
	SetNetUpdateFrequency(1.f);
	SetNetCullDistanceSquared(FMath::Square(60000.f));
	// 상태가 바뀔 때만 깨운다(구조 컴포넌트 · 문이 바꾸기 전에 FlushNetDormancy).
	SetNetDormancy(DORM_DormantAll);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Structure = CreateDefaultSubobject<UGrimStructureComponent>(TEXT("Structure"));
	ElementalState = CreateDefaultSubobject<UGrimElementalStateComponent>(TEXT("ElementalState"));
}

void AGrimBuildPiece::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGrimBuildPiece, Slot);
	DOREPLIFETIME(AGrimBuildPiece, bDoorOpen);
}

void AGrimBuildPiece::InitializePiece(FName PieceId, const FGrimBuildSlot& InSlot)
{
	Slot = InSlot;
	Structure->InitializePiece(PieceId);
}

FName AGrimBuildPiece::GetPieceId() const
{
	return Structure ? Structure->GetPieceId() : NAME_None;
}

const FGrimBuildPieceDefinition* AGrimBuildPiece::GetDefinition() const
{
	return UGrimBaseSettings::GetDatabase().FindPiece(GetPieceId());
}

AGrimBaseCore* AGrimBuildPiece::GetBaseCore() const
{
	const UGrimBaseSubsystem* Subsystem = UGrimBaseSubsystem::Get(this);
	return Subsystem ? Subsystem->FindBaseAt(GetActorLocation()) : nullptr;
}

void AGrimBuildPiece::BeginPlay()
{
	Super::BeginPlay();
	const FGrimBuildPieceDefinition* Def = GetDefinition();
	if (HasAuthority() && Def && Def->IsFlammable())
	{
		ElementalState->Material.AddTag(GrimBaseTags::Material_Wood);
	}
	Structure->OnStructureChanged.AddWeakLambda(this, [this](UGrimStructureComponent*)
	{
		if (BuiltForPiece != GetPieceId())
		{
			RebuildShape();
		}
		ApplyHealthTint();
	});
	RebuildShape();
	if (UGrimBaseSubsystem* Subsystem = UGrimBaseSubsystem::Get(this))
	{
		Subsystem->RegisterPiece(this);
		bRegistered = true;
	}
}

void AGrimBuildPiece::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGrimBaseSubsystem* Subsystem = UGrimBaseSubsystem::Get(this); Subsystem && bRegistered)
	{
		Subsystem->UnregisterPiece(this);
	}
	Super::EndPlay(EndPlayReason);
}

void AGrimBuildPiece::OnRep_Shape()
{
	RebuildShape();
}

void AGrimBuildPiece::RebuildShape()
{
	for (UStaticMeshComponent* Component : ShapeComponents)
	{
		if (Component)
		{
			// 같은 이름(Shape<n>)으로 다시 만들므로 지우기 전에 이름을 비킨다.
			Component->Rename(nullptr, nullptr, REN_DontCreateRedirectors | REN_ForceNoResetLoaders);
			Component->DestroyComponent();
		}
	}
	ShapeComponents.Reset();
	DoorLeaf = nullptr;

	const FGrimBuildPieceDefinition* Def = GetDefinition();
	BuiltForPiece = GetPieceId();
	if (!Def)
	{
		return;
	}
	const FGrimBuildGrid Grid = UGrimBaseSettings::Get()->MakeGrid(FVector::ZeroVector, 0.f);
	TArray<FGrimShapeBox> Boxes;
	Grid.GetShape(Def->Row.Category, Boxes);
	for (int32 Index = 0; Index < Boxes.Num(); ++Index)
	{
		const FGrimShapeBox& Box = Boxes[Index];
		const FLinearColor Color = Box.bDoorLeaf ? Def->Row.Color * 0.75f : Def->Row.Color;
		// 서버 · 클라이언트가 같은 순서 · 이름으로 만든다 — 이동 기반으로 복제될 수 있게(SetNetAddressable).
		UStaticMeshComponent* Component = GrimBaseVisuals::AddBox(this, Root, Box, Color, true, FName(*FString::Printf(TEXT("Shape%d"), Index)));
		ShapeComponents.Add(Component);
		if (Box.bDoorLeaf)
		{
			DoorLeaf = Component;
		}
	}
	ApplyDoorState();
	ApplyHealthTint();
}

void AGrimBuildPiece::ApplyHealthTint()
{
	const FGrimBuildPieceDefinition* Def = GetDefinition();
	if (!Def || !Structure)
	{
		return;
	}
	// 닳을수록 어두워진다(아트 전 내구도 표시). 절반 아래부터.
	const float Ratio = Structure->GetMaxHealth() > 0.f ? Structure->GetHealth() / Structure->GetMaxHealth() : 1.f;
	const float Shade = FMath::Lerp(0.35f, 1.f, FMath::Clamp(Ratio * 2.f, 0.f, 1.f));
	for (UStaticMeshComponent* Component : ShapeComponents)
	{
		if (Component)
		{
			const FLinearColor Base = Component == DoorLeaf ? Def->Row.Color * 0.75f : Def->Row.Color;
			GrimBaseVisuals::SetColor(Component, Base * Shade);
		}
	}
}

void AGrimBuildPiece::ApplyDoorState()
{
	if (DoorLeaf)
	{
		DoorLeaf->SetVisibility(!bDoorOpen);
		DoorLeaf->SetCollisionEnabled(bDoorOpen ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
	}
}

void AGrimBuildPiece::OnRep_DoorOpen()
{
	ApplyDoorState();
}

void AGrimBuildPiece::SetDoorOpen(bool bOpen)
{
	if (!HasAuthority() || !IsDoor() || bOpen == bDoorOpen)
	{
		return;
	}
	FlushNetDormancy();
	bDoorOpen = bOpen;
	ApplyDoorState();
}

FText AGrimBuildPiece::GetInteractPrompt(const APawn* Pawn) const
{
	return bDoorOpen ? LOCTEXT("CloseDoor", "문 닫기") : LOCTEXT("OpenDoor", "문 열기");
}

void AGrimBuildPiece::InteractLocal(UGrimBuilderComponent* Builder)
{
	if (Builder)
	{
		Builder->Server_Interact(this);
	}
}

void AGrimBuildPiece::InteractServer(UGrimBuilderComponent* Builder, APawn* Pawn)
{
	SetDoorOpen(!bDoorOpen);
}

// ── 세이브 (BuildPiece.<이름> v1) ──

FName AGrimBuildPiece::GetSaveId() const
{
	return FName(*FString::Printf(TEXT("BuildPiece.%s"), *GetName()));
}

void AGrimBuildPiece::WriteSaveData(FArchive& Ar)
{
	uint8 Category = static_cast<uint8>(Slot.Category);
	int32 X = Slot.X, Y = Slot.Y, Level = Slot.Level, Dir = Slot.Dir;
	bool bOpen = bDoorOpen;
	Ar << Category << X << Y << Level << Dir << bOpen;
}

void AGrimBuildPiece::ReadSaveData(FArchive& Ar, int32 Version)
{
	uint8 Category = 0;
	int32 X = 0, Y = 0, Level = 0, Dir = 0;
	bool bOpen = false;
	Ar << Category << X << Y << Level << Dir << bOpen;
	if (Ar.IsError())
	{
		return;
	}
	Slot.Category = static_cast<EGrimBuildCategory>(FMath::Min<uint8>(Category, static_cast<uint8>(EGrimBuildCategory::Station)));
	Slot.X = X;
	Slot.Y = Y;
	Slot.Level = Level;
	Slot.Dir = Dir;
	bDoorOpen = bOpen;
	if (HasActorBegunPlay())
	{
		FlushNetDormancy();
		RebuildShape();
	}
}

#undef LOCTEXT_NAMESPACE
