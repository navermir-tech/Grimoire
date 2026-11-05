#include "Creatures/GrimCreatureBox.h"
#include "Creatures/GrimCreature.h"
#include "Creatures/GrimCreatureSettings.h"
#include "Creatures/GrimCreatureSpecies.h"
#include "Creatures/GrimCreatureStorageMath.h"
#include "Creatures/GrimCreatureStorageSettings.h"
#include "Creatures/GrimCreatureTamingComponent.h"
#include "Creatures/GrimEcosystemSubsystem.h"
#include "Creatures/GrimMountComponent.h"
#include "Creatures/GrimTamerComponent.h"
#include "Creatures/GrimTamingSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace GrimCreatureBoxLocal // 유니티 빌드 이름 충돌 방지
{
	const FLinearColor WoodColor(0.36f, 0.22f, 0.12f);
	const FLinearColor LidColor(0.28f, 0.17f, 0.1f);
	const FLinearColor CrestColor(0.45f, 0.95f, 0.6f);
	/** 서버가 풀어놓은 생물을 맞추는 주기(주인이 나중에 접속 · 액터가 사라짐) */
	constexpr float SyncSeconds = 2.f;
}

AGrimCreatureBox::AGrimCreatureBox()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetNetUpdateFrequency(2.f);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	// 상자 몸(120 × 90 × 70cm) — 바닥이 액터 원점.
	Base = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Base"));
	SetRootComponent(Base);
	if (CubeMesh.Succeeded())
	{
		Base->SetStaticMesh(CubeMesh.Object);
	}
	Base->SetRelativeScale3D(FVector(1.2f, 0.9f, 0.7f));
	Base->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	// 뚜껑 · 문장(빛나는 구) — 장식, 충돌 없음. 크기는 몸 배율을 받지 않게 절대 배율.
	Lid = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Lid"));
	Lid->SetupAttachment(Base);
	if (CubeMesh.Succeeded())
	{
		Lid->SetStaticMesh(CubeMesh.Object);
	}
	Lid->SetUsingAbsoluteScale(true);
	Lid->SetRelativeLocation(FVector(0.f, 0.f, 60.f));
	Lid->SetRelativeScale3D(FVector(1.3f, 1.0f, 0.15f));
	Lid->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Crest = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Crest"));
	Crest->SetupAttachment(Base);
	if (SphereMesh.Succeeded())
	{
		Crest->SetStaticMesh(SphereMesh.Object);
	}
	Crest->SetUsingAbsoluteScale(true);
	Crest->SetRelativeLocation(FVector(0.f, 0.f, 85.f));
	Crest->SetRelativeScale3D(FVector(0.3f));
	Crest->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AGrimCreatureBox::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGrimCreatureBox, Entries);
}

void AGrimCreatureBox::BeginPlay()
{
	Super::BeginPlay();
	ApplyColors();
	if (HasAuthority())
	{
		SyncReleased();
		GetWorld()->GetTimerManager().SetTimer(SyncTimer, this, &AGrimCreatureBox::SyncReleased, GrimCreatureBoxLocal::SyncSeconds, true);
	}
}

void AGrimCreatureBox::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}
	// 상자가 게임 중 없어졌다(M4 철거 자리): 풀어놓은 생물도 함께 거둔다 — 기록은 상자와 함께 사라진다.
	if (HasAuthority() && EndPlayReason == EEndPlayReason::Destroyed)
	{
		for (const FGrimCreatureBoxEntry& Entry : Entries)
		{
			if (AGrimCreature* Creature = Entry.bReleased ? FindReleasedActor(Entry.Record.Instance.SpeciesId, Entry.Record.Instance.Seed) : nullptr)
			{
				Creature->Destroy();
			}
		}
	}
	Super::EndPlay(EndPlayReason);
}

void AGrimCreatureBox::ApplyColors()
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	auto Tint = [](UStaticMeshComponent* Part, const FLinearColor& Color)
	{
		if (Part && Part->GetStaticMesh())
		{
			if (UMaterialInstanceDynamic* Material = Part->CreateAndSetMaterialInstanceDynamic(0))
			{
				Material->SetVectorParameterValue(TEXT("Color"), Color);
			}
		}
	};
	Tint(Base, GrimCreatureBoxLocal::WoodColor);
	Tint(Lid, GrimCreatureBoxLocal::LidColor);
	Tint(Crest, GrimCreatureBoxLocal::CrestColor);
}

// ───────────────────────── 읽기 ─────────────────────────

int32 AGrimCreatureBox::GetCapacity() const
{
	return CapacityOverride > 0 ? CapacityOverride : UGrimCreatureStorageSettings::Get()->BoxCapacity;
}

float AGrimCreatureBox::GetRoamRadius() const
{
	return RoamRadiusOverride > 0.f ? RoamRadiusOverride : UGrimCreatureStorageSettings::Get()->BoxRoamRadius;
}

int32 AGrimCreatureBox::GetReleasedCount() const
{
	int32 Count = 0;
	for (const FGrimCreatureBoxEntry& Entry : Entries)
	{
		Count += Entry.bReleased ? 1 : 0;
	}
	return Count;
}

int32 AGrimCreatureBox::CountOwnedBy(const FString& OwnerId) const
{
	int32 Count = 0;
	for (const FGrimCreatureBoxEntry& Entry : Entries)
	{
		Count += Entry.Record.Instance.OwnerId == OwnerId ? 1 : 0;
	}
	return Count;
}

bool AGrimCreatureBox::IsWithinManageRange(const APawn* Pawn, float Slack) const
{
	if (!Pawn)
	{
		return false;
	}
	const float Radius = UGrimCreatureStorageSettings::Get()->BoxManageRadius + FMath::Max(0.f, Slack);
	return FVector::DistSquared(Pawn->GetActorLocation(), GetActorLocation()) <= FMath::Square(Radius);
}

AGrimCreatureBox* AGrimCreatureBox::FindNearestUsable(const APawn* Pawn, float Slack)
{
	if (!Pawn || !Pawn->GetWorld())
	{
		return nullptr;
	}
	AGrimCreatureBox* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	for (TActorIterator<AGrimCreatureBox> It(Pawn->GetWorld()); It; ++It)
	{
		if (It->IsActorBeingDestroyed() || !It->IsWithinManageRange(Pawn, Slack))
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(Pawn->GetActorLocation(), It->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = *It;
		}
	}
	return Best;
}

AGrimCreatureBox* AGrimCreatureBox::FindNearestWithRoom(const UWorld* World, const FVector& Near)
{
	if (!World)
	{
		return nullptr;
	}
	AGrimCreatureBox* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();
	for (TActorIterator<AGrimCreatureBox> It(World); It; ++It)
	{
		if (It->IsActorBeingDestroyed() || !It->HasRoom())
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(Near, It->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = *It;
		}
	}
	return Best;
}

AGrimCreature* AGrimCreatureBox::FindReleasedActor(FName SpeciesId, int32 Seed) const
{
	for (TActorIterator<AGrimCreature> It(GetWorld()); It; ++It)
	{
		const UGrimCreatureTamingComponent* Taming = It->GetTaming();
		if (!It->IsDead() && !It->IsActorBeingDestroyed() && Taming && Taming->GetHomeBox() == this
			&& It->GetInstance().SpeciesId == SpeciesId && It->GetInstance().Seed == Seed)
		{
			return *It;
		}
	}
	return nullptr;
}

// ───────────────────────── 서버: 칸 ─────────────────────────

void AGrimCreatureBox::MarkChanged()
{
	ForceNetUpdate();
	OnBoxChanged.Broadcast(this);
}

void AGrimCreatureBox::OnRep_Entries()
{
	OnBoxChanged.Broadcast(this);
}

bool AGrimCreatureBox::AddEntry(const FGrimOwnedCreatureRecord& Record, bool bRelease)
{
	if (!HasAuthority() || !Record.Instance.IsValid() || !HasRoom()
		|| GrimCreatureStorageMath::FindBoxEntry(Entries, Record.Instance.SpeciesId, Record.Instance.Seed) != INDEX_NONE)
	{
		return false;
	}
	FGrimCreatureBoxEntry& Entry = Entries.AddDefaulted_GetRef();
	Entry.Record = Record;
	Entry.bReleased = false;
	if (Entry.Record.SavedUtcSeconds <= 0.0)
	{
		Entry.Record.SavedUtcSeconds = FDateTime::UtcNow().ToUnixTimestamp();
	}
	UE_LOG(LogGrimTaming, Display, TEXT("[Server] 거점 상자 %s 에 넣음 %s seed=%d 주인=%s (%d/%d)"), *GetName(), *Record.Instance.SpeciesId.ToString(), Record.Instance.Seed,
		*Record.Instance.OwnerId, Entries.Num(), GetCapacity());
	if (bRelease)
	{
		// 풀어놓기 상한은 부르는 쪽이 본다.
		ReleaseEntry(Record.Instance.SpeciesId, Record.Instance.Seed);
	}
	MarkChanged();
	return true;
}

bool AGrimCreatureBox::TakeEntry(FName SpeciesId, int32 Seed, FGrimOwnedCreatureRecord& OutRecord)
{
	const int32 Index = GrimCreatureStorageMath::FindBoxEntry(Entries, SpeciesId, Seed);
	if (!HasAuthority() || Index == INDEX_NONE)
	{
		return false;
	}
	FGrimCreatureBoxEntry Entry = Entries[Index];
	if (Entry.bReleased)
	{
		if (AGrimCreature* Creature = FindReleasedActor(SpeciesId, Seed))
		{
			if (UGrimCreatureTamingComponent* Taming = Creature->GetTaming())
			{
				Entry.Record = Taming->MakeRecord();
				Taming->SetHomeBox(nullptr);
			}
			Creature->Destroy();
		}
	}
	Entries.RemoveAt(Index);
	OutRecord = Entry.Record;
	UE_LOG(LogGrimTaming, Display, TEXT("[Server] 거점 상자 %s 에서 꺼냄 %s seed=%d%s (%d/%d)"), *GetName(), *SpeciesId.ToString(), Seed, Entry.bReleased ? TEXT(" (풀어놓았던)") : TEXT(""),
		Entries.Num(), GetCapacity());
	MarkChanged();
	return true;
}

bool AGrimCreatureBox::ReleaseEntry(FName SpeciesId, int32 Seed)
{
	const int32 Index = GrimCreatureStorageMath::FindBoxEntry(Entries, SpeciesId, Seed);
	if (!HasAuthority() || Index == INDEX_NONE || Entries[Index].bReleased)
	{
		return false;
	}
	Entries[Index].bReleased = true;
	if (!SpawnReleased(Entries[Index], Index))
	{
		Entries[Index].bReleased = false;
		return false;
	}
	MarkChanged();
	return true;
}

bool AGrimCreatureBox::RecallEntry(FName SpeciesId, int32 Seed)
{
	const int32 Index = GrimCreatureStorageMath::FindBoxEntry(Entries, SpeciesId, Seed);
	if (!HasAuthority() || Index == INDEX_NONE || !Entries[Index].bReleased)
	{
		return false;
	}
	if (AGrimCreature* Creature = FindReleasedActor(SpeciesId, Seed))
	{
		if (UGrimCreatureTamingComponent* Taming = Creature->GetTaming())
		{
			Entries[Index].Record = Taming->MakeRecord();
			Taming->SetHomeBox(nullptr);
		}
		Creature->Destroy();
	}
	Entries[Index].bReleased = false;
	Entries[Index].Record.SavedUtcSeconds = FDateTime::UtcNow().ToUnixTimestamp();
	UE_LOG(LogGrimTaming, Display, TEXT("[Server] 거점 상자 %s 거둠 %s seed=%d"), *GetName(), *SpeciesId.ToString(), Seed);
	MarkChanged();
	return true;
}

void AGrimCreatureBox::ForgetCreature(FName SpeciesId, int32 Seed)
{
	const int32 Index = GrimCreatureStorageMath::FindBoxEntry(Entries, SpeciesId, Seed);
	if (!HasAuthority() || Index == INDEX_NONE)
	{
		return;
	}
	Entries.RemoveAt(Index);
	UE_LOG(LogGrimTaming, Display, TEXT("[Server] 거점 상자 %s 칸에서 지움 %s seed=%d(떠남 · 놓아줌 · 죽음)"), *GetName(), *SpeciesId.ToString(), Seed);
	MarkChanged();
}

void AGrimCreatureBox::RefreshReleasedRecords()
{
	if (!HasAuthority())
	{
		return;
	}
	for (FGrimCreatureBoxEntry& Entry : Entries)
	{
		if (!Entry.bReleased)
		{
			continue;
		}
		if (const AGrimCreature* Creature = FindReleasedActor(Entry.Record.Instance.SpeciesId, Entry.Record.Instance.Seed))
		{
			if (const UGrimCreatureTamingComponent* Taming = Creature->GetTaming())
			{
				Entry.Record = Taming->MakeRecord();
				Entry.Record.Command = EGrimTamedCommand::Roam;
			}
		}
	}
}

AGrimCreature* AGrimCreatureBox::SpawnReleased(FGrimCreatureBoxEntry& Entry, int32 Slot)
{
	UWorld* World = GetWorld();
	UGrimCreatureSpecies* Species = UGrimCreatureSettings::Get()->FindSpecies(Entry.Record.Instance.SpeciesId);
	if (!World || !Species)
	{
		UE_LOG(LogGrimTaming, Warning, TEXT("[Server] 거점 상자 %s: %s 종이 없어 풀어놓지 못한다"), *GetName(), *Entry.Record.Instance.SpeciesId.ToString());
		return nullptr;
	}
	// 상자 둘레 고르게(반경의 25~60% 사이 원 위) — 슬롯마다 각도를 나눠 겹치지 않게.
	const float Radius = GetRoamRadius();
	const float Angle = Slot * 137.5f + FMath::FRandRange(-15.f, 15.f);
	const float Distance = FMath::Clamp(Radius * FMath::FRandRange(0.25f, 0.6f), 250.f, FMath::Max(250.f, Radius));
	const FVector Point = GetActorLocation() + FVector::ForwardVector.RotateAngleAxis(Angle, FVector::UpVector) * Distance + FVector(0.f, 0.f, 100.f);
	FVector Ground = GetActorLocation();
	if (!UGrimEcosystemSubsystem::FindGround(World, Point, Ground))
	{
		Ground = FVector(Point.X, Point.Y, GetActorLocation().Z);
	}
	const UGrimCreatureStorageSettings* Settings = UGrimCreatureStorageSettings::Get();
	const float Health = GrimCreatureStorageMath::GetHealedRatio(Entry.Record.HealthRatio, Entry.Record.SavedUtcSeconds, FDateTime::UtcNow().ToUnixTimestamp(), Settings->StoredHealPerMinute);
	AGrimCreature* Creature = AGrimCreature::SpawnCreature(World, Species, Entry.Record.Instance, Ground, Angle + 180.f, Health);
	UGrimCreatureTamingComponent* Taming = Creature ? Creature->GetTaming() : nullptr;
	if (!Taming)
	{
		if (Creature)
		{
			Creature->Destroy();
		}
		return nullptr;
	}
	FGrimOwnedCreatureRecord Record = Entry.Record;
	Record.Command = EGrimTamedCommand::Roam;
	const UGrimTamingSubsystem* Subsystem = UGrimTamingSubsystem::Get(this);
	const UGrimTamerComponent* Tamer = Subsystem ? Subsystem->FindTamerById(Record.Instance.OwnerId) : nullptr;
	Taming->RestoreOwned(Tamer ? Tamer->GetPlayerState() : nullptr, Record, 0.0);
	Taming->SetHomeBox(this);
	Creature->SetHomeLocation(Ground);
	Entry.Record.HealthRatio = Health;
	Entry.Record.Command = EGrimTamedCommand::Roam;
	UE_LOG(LogGrimTaming, Display, TEXT("[Server] 거점 상자 %s 풀어놓음 %s seed=%d 주인=%s 체력 %.0f%%"), *GetName(), *Record.Instance.SpeciesId.ToString(), Record.Instance.Seed,
		*Record.Instance.OwnerId, Health * 100.f);
	return Creature;
}

void AGrimCreatureBox::SyncReleased()
{
	if (!HasAuthority() || !GetWorld())
	{
		return;
	}
	bool bChanged = false;
	// 1) 칸에 없는 액터는 거둔다(상자에서 지워졌는데 남은 것).
	for (TActorIterator<AGrimCreature> It(GetWorld()); It; ++It)
	{
		const UGrimCreatureTamingComponent* Taming = It->GetTaming();
		if (!Taming || Taming->GetHomeBox() != this || It->IsActorBeingDestroyed())
		{
			continue;
		}
		const int32 Index = GrimCreatureStorageMath::FindBoxEntry(Entries, It->GetInstance().SpeciesId, It->GetInstance().Seed);
		if (Index == INDEX_NONE || !Entries[Index].bReleased)
		{
			It->Destroy();
		}
	}
	// 2) 풀어놓은 칸마다 액터 — 없으면 내놓고(불러오기 · 죽음 뒤), 있으면 접속한 주인을 잇는다.
	const UGrimTamingSubsystem* Subsystem = UGrimTamingSubsystem::Get(this);
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		FGrimCreatureBoxEntry& Entry = Entries[Index];
		if (!Entry.bReleased)
		{
			continue;
		}
		AGrimCreature* Creature = FindReleasedActor(Entry.Record.Instance.SpeciesId, Entry.Record.Instance.Seed);
		if (!Creature)
		{
			Creature = SpawnReleased(Entry, Index);
			bChanged |= Creature != nullptr;
		}
		UGrimCreatureTamingComponent* Taming = Creature ? Creature->GetTaming() : nullptr;
		if (Taming && !IsValid(Taming->GetOwnerPlayerState()) && Subsystem)
		{
			if (const UGrimTamerComponent* Tamer = Subsystem->FindTamerById(Entry.Record.Instance.OwnerId))
			{
				Taming->SetOwnerPlayerState(Tamer->GetPlayerState());
			}
		}
	}
	if (bChanged)
	{
		MarkChanged();
	}
}

// ───────────────────────── 세이브 ─────────────────────────

FName AGrimCreatureBox::GetSaveId() const
{
	return FName(*FString::Printf(TEXT("CreatureBox.%s"), *GetName()));
}

void AGrimCreatureBox::WriteSaveData(FArchive& Ar)
{
	RefreshReleasedRecords();
	GrimCreatureStorageMath::SerializeBoxEntries(Ar, Entries);
}

void AGrimCreatureBox::ReadSaveData(FArchive& Ar, int32 Version)
{
	TArray<FGrimCreatureBoxEntry> Loaded;
	if (!GrimCreatureStorageMath::SerializeBoxEntries(Ar, Loaded))
	{
		UE_LOG(LogGrimTaming, Warning, TEXT("[Server] 거점 상자 %s 세이브를 읽지 못했다(v%d)"), *GetName(), Version);
		return;
	}
	Entries = MoveTemp(Loaded);
	UE_LOG(LogGrimTaming, Display, TEXT("[Server] 거점 상자 %s: 칸 %d(풀어놓음 %d) 을 읽었다"), *GetName(), Entries.Num(), GetReleasedCount());
	// 이미 시작한 상자(놓인 상자 · 다시 불러오기)는 바로 맞춘다. 다시 스폰된 상자는 BeginPlay 에서.
	if (HasActorBegunPlay())
	{
		SyncReleased();
	}
	MarkChanged();
}
