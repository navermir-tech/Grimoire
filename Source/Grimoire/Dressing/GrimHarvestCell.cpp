#include "Dressing/GrimHarvestCell.h"
#include "Dressing/GrimDressingSettings.h"
#include "Dressing/GrimDressingTypes.h"
#include "Environment/GrimTimeOfDaySubsystem.h"
#include "Items/GrimItemStatics.h"
#include "Save/GrimPlayerIdentity.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "GrimHarvestCell"

// ───── 복제 콜백 ─────

void FGrimHarvestedEntry::PostReplicatedAdd(const FGrimHarvestedArray& InArray)
{
	if (InArray.Owner)
	{
		InArray.Owner->RefreshPoint(PointIndex);
	}
}

void FGrimHarvestedEntry::PostReplicatedChange(const FGrimHarvestedArray& InArray)
{
	if (InArray.Owner)
	{
		InArray.Owner->RefreshPoint(PointIndex);
	}
}

void FGrimHarvestedEntry::PreReplicatedRemove(const FGrimHarvestedArray& InArray)
{
	if (InArray.Owner)
	{
		// 지워지는 중인 항목은 아직 배열에 있다 — 지워진 뒤 모습으로 보이게 다음 틱에 다시 본다
		const int32 Index = PointIndex;
		TWeakObjectPtr<AGrimHarvestCell> Weak(InArray.Owner);
		InArray.Owner->GetWorldTimerManager().SetTimerForNextTick([Weak, Index]()
		{
			if (AGrimHarvestCell* Cell = Weak.Get())
			{
				Cell->RefreshPoint(Index);
			}
		});
	}
}

// ───── 액터 ─────

AGrimHarvestCell::AGrimHarvestCell()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetNetDormancy(DORM_DormantAll);
	// 셀은 256 m — 셀 가운데에서 멀리 있어도 그 안의 플레이어에게 닿게
	SetNetCullDistanceSquared(FMath::Square(40000.f));
	Harvested.Owner = this;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SceneRoot->SetMobility(EComponentMobility::Static);
	SetRootComponent(SceneRoot);
#if WITH_EDITORONLY_DATA
	// 16 개뿐이고 휴면이다 — 늘 올려 두어 서버 · 클라이언트의 셀이 스트리밍에 따라 사라지지 않게
	bIsSpatiallyLoaded = false;
#endif
}

void AGrimHarvestCell::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGrimHarvestCell, Harvested);
	DOREPLIFETIME_CONDITION(AGrimHarvestCell, Points, COND_InitialOnly);
}

void AGrimHarvestCell::AddPoint(FName Kind, const FTransform& WorldTransform)
{
	FGrimHarvestPoint Point;
	Point.Kind = Kind;
	Point.Transform = WorldTransform;
	Points.Add(Point);
}

void AGrimHarvestCell::BeginPlay()
{
	Super::BeginPlay();
	Harvested.Owner = this;

	BuildInstances();

	UWorld* World = GetWorld();
	if (HasAuthority())
	{
		if (UGrimTimeOfDaySubsystem* Time = World->GetSubsystem<UGrimTimeOfDaySubsystem>())
		{
			Time->OnNightEnd.AddDynamic(this, &AGrimHarvestCell::HandleDawn);
			bBoundDawn = true;
		}
	}
	if (UGrimInteractableSubsystem* Interactables = World->GetSubsystem<UGrimInteractableSubsystem>())
	{
		Interactables->Register(this);
	}
}

void AGrimHarvestCell::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RespawnTimer);
		if (bBoundDawn)
		{
			if (UGrimTimeOfDaySubsystem* Time = World->GetSubsystem<UGrimTimeOfDaySubsystem>())
			{
				Time->OnNightEnd.RemoveDynamic(this, &AGrimHarvestCell::HandleDawn);
			}
		}
		if (UGrimInteractableSubsystem* Interactables = World->GetSubsystem<UGrimInteractableSubsystem>())
		{
			Interactables->Unregister(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void AGrimHarvestCell::OnRep_Points()
{
	if (HasActorBegunPlay() && PointInstance.Num() != Points.Num())
	{
		for (TPair<FName, TObjectPtr<UInstancedStaticMeshComponent>>& Pair : KindMeshes)
		{
			if (Pair.Value)
			{
				Pair.Value->DestroyComponent();
			}
		}
		KindMeshes.Reset();
		BuildInstances();
	}
}

void AGrimHarvestCell::BuildInstances()
{
	// 경계(F 찾기에서 먼 셀을 바로 건너뛰려고)
	FBox Box(ForceInit);
	for (const FGrimHarvestPoint& Point : Points)
	{
		Box += Point.Transform.GetLocation();
	}
	BoundsCenter = Box.IsValid ? Box.GetCenter() : GetActorLocation();
	BoundsRadius = Box.IsValid ? Box.GetExtent().Size() : 0.f;

	PointInstance.Init(INDEX_NONE, Points.Num());
	if (GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	for (int32 Index = 0; Index < Points.Num(); ++Index)
	{
		const FGrimHarvestPoint& Point = Points[Index];
		const FGrimHarvestNodeRow* Row = FGrimDressingData::FindHarvestNode(Point.Kind);
		if (!Row)
		{
			UE_LOG(LogGrimDressing, Warning, TEXT("%s: HarvestNodes 표에 %s 가 없다"), *GetName(), *Point.Kind.ToString());
			continue;
		}
		TObjectPtr<UInstancedStaticMeshComponent>* Found = KindMeshes.Find(Point.Kind);
		UInstancedStaticMeshComponent* Mesh = Found ? Found->Get() : nullptr;
		if (!Mesh)
		{
			Mesh = NewObject<UInstancedStaticMeshComponent>(this, FName(*FString::Printf(TEXT("ISM_%s"), *Point.Kind.ToString())));
			Mesh->SetupAttachment(GetRootComponent());
			Mesh->SetMobility(EComponentMobility::Movable);
			Mesh->SetStaticMesh(Row->Mesh.LoadSynchronous());
			if (UMaterialInterface* Mat = Row->MeshMaterial.LoadSynchronous())
			{
				Mesh->SetMaterial(0, Mat);
			}
			Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Mesh->SetCastShadow(false);
			Mesh->SetCanEverAffectNavigation(false);
			Mesh->SetCullDistances(Row->CullDistance * 0.7f, Row->CullDistance);
			Mesh->RegisterComponent();
			AddInstanceComponent(Mesh);
			KindMeshes.Add(Point.Kind, Mesh);
		}
		PointInstance[Index] = Mesh->AddInstance(Point.Transform, true);
	}
	for (int32 Index = 0; Index < Points.Num(); ++Index)
	{
		RefreshPoint(Index);
	}
}

FString AGrimHarvestCell::LocalPlayerKey() const
{
	if (CachedLocalKey.IsEmpty())
	{
		if (const UWorld* World = GetWorld())
		{
			for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
			{
				const APlayerController* PC = It->Get();
				if (PC && PC->IsLocalController() && PC->GetLocalPlayer())
				{
					CachedLocalKey = FGrimPlayerIdentity::GetLocalPlayerId(PC->GetLocalPlayer());
					break;
				}
			}
		}
	}
	return CachedLocalKey;
}

int32 AGrimHarvestCell::FindEntry(int32 PointIndex, const FString& PlayerKey) const
{
	return Harvested.Items.IndexOfByPredicate([&](const FGrimHarvestedEntry& Entry) { return Entry.PointIndex == PointIndex && Entry.PlayerKey == PlayerKey; });
}

bool AGrimHarvestCell::IsHarvestedFor(int32 PointIndex, const FString& PlayerKey) const
{
	if (!Points.IsValidIndex(PointIndex))
	{
		return true;
	}
	const FGrimHarvestNodeRow* Row = FGrimDressingData::FindHarvestNode(Points[PointIndex].Kind);
	if (!Row)
	{
		return true;
	}
	const FString Key = Row->Ownership == EGrimHarvestOwnership::Shared ? FString() : PlayerKey;
	const int32 Entry = FindEntry(PointIndex, Key);
	return Entry != INDEX_NONE && Harvested.Items[Entry].Hits >= Row->HitsToHarvest;
}

void AGrimHarvestCell::RefreshPoint(int32 PointIndex)
{
	if (!PointInstance.IsValidIndex(PointIndex) || PointInstance[PointIndex] == INDEX_NONE)
	{
		return;
	}
	const FGrimHarvestPoint& Point = Points[PointIndex];
	TObjectPtr<UInstancedStaticMeshComponent>* Found = KindMeshes.Find(Point.Kind);
	if (!Found || !*Found)
	{
		return;
	}
	const bool bVisible = !IsHarvestedFor(PointIndex, LocalPlayerKey());
	FTransform Transform = Point.Transform;
	if (!bVisible)
	{
		Transform.SetScale3D(FVector::ZeroVector);
	}
	(*Found)->UpdateInstanceTransform(PointInstance[PointIndex], Transform, true, true, true);
}

bool AGrimHarvestCell::IsPointVisible(int32 PointIndex) const
{
	if (!PointInstance.IsValidIndex(PointIndex) || PointInstance[PointIndex] == INDEX_NONE)
	{
		return false;
	}
	const TObjectPtr<UInstancedStaticMeshComponent>* Found = KindMeshes.Find(Points[PointIndex].Kind);
	if (!Found || !*Found)
	{
		return false;
	}
	FTransform Transform;
	(*Found)->GetInstanceTransform(PointInstance[PointIndex], Transform, true);
	return !Transform.GetScale3D().IsNearlyZero();
}

double AGrimHarvestCell::GetServerSeconds() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.0;
}

// ───── F ─────

bool AGrimHarvestCell::GetInteractionOffer(const APawn* Pawn, FGrimInteractionOffer& OutOffer) const
{
	if (!Pawn || Points.Num() == 0)
	{
		return false;
	}
	const FVector PawnLocation = Pawn->GetActorLocation();
	if (FVector::Dist(PawnLocation, BoundsCenter) > BoundsRadius + 1000.f)
	{
		return false;
	}
	const FString Key = Pawn->IsLocallyControlled() ? LocalPlayerKey() : FGrimPlayerIdentity::ResolveServerSide(Pawn->GetPlayerState());
	bool bFound = false;
	for (int32 Index = 0; Index < Points.Num(); ++Index)
	{
		const FGrimHarvestNodeRow* Row = FGrimDressingData::FindHarvestNode(Points[Index].Kind);
		if (!Row)
		{
			continue;
		}
		const float Distance = FVector::Dist(PawnLocation, Points[Index].Transform.GetLocation());
		if (Distance > Row->InteractRadius || Distance >= OutOffer.Distance || IsHarvestedFor(Index, Key))
		{
			continue;
		}
		OutOffer.Distance = Distance;
		OutOffer.Token = Index;
		OutOffer.Prompt = FText::Format(LOCTEXT("Harvest", "줍기 — {0}"), FText::FromString(Row->DisplayName.IsEmpty() ? Points[Index].Kind.ToString() : Row->DisplayName));
		bFound = true;
	}
	return bFound;
}

bool AGrimHarvestCell::Interact(APawn* Pawn, int32 Token)
{
	if (!HasAuthority() || !Pawn || !Points.IsValidIndex(Token))
	{
		return false;
	}
	const FGrimHarvestNodeRow* Row = FGrimDressingData::FindHarvestNode(Points[Token].Kind);
	if (!Row)
	{
		return false;
	}
	const float Slack = GetDefault<UGrimDressingSettings>()->ServerReachSlack;
	if (FVector::Dist(Pawn->GetActorLocation(), Points[Token].Transform.GetLocation()) > Row->InteractRadius + Slack)
	{
		UE_LOG(LogGrimDressing, Warning, TEXT("[Server] %s: 점 %d 이(가) %s 에게서 너무 멀다"), *GetName(), Token, *GetNameSafe(Pawn));
		return false;
	}
	const FString Key = FGrimPlayerIdentity::ResolveServerSide(Pawn->GetPlayerState());
	if (Row->Ownership == EGrimHarvestOwnership::PerPlayer && Key.IsEmpty())
	{
		return false;
	}
	if (IsHarvestedFor(Token, Key))
	{
		return false;
	}
	HarvestPoint(Token, Key, Pawn);
	return true;
}

int32 AGrimHarvestCell::HarvestPoint(int32 PointIndex, const FString& PlayerKey, APawn* Receiver)
{
	if (!HasAuthority() || !Points.IsValidIndex(PointIndex))
	{
		return 0;
	}
	const FGrimHarvestNodeRow* Row = FGrimDressingData::FindHarvestNode(Points[PointIndex].Kind);
	if (!Row || IsHarvestedFor(PointIndex, PlayerKey))
	{
		return 0;
	}
	const FString Key = Row->Ownership == EGrimHarvestOwnership::Shared ? FString() : PlayerKey;

	FlushNetDormancy();
	int32 EntryIndex = FindEntry(PointIndex, Key);
	if (EntryIndex == INDEX_NONE)
	{
		FGrimHarvestedEntry& NewEntry = Harvested.Items.AddDefaulted_GetRef();
		NewEntry.PointIndex = PointIndex;
		NewEntry.PlayerKey = Key;
		EntryIndex = Harvested.Items.Num() - 1;
	}
	FGrimHarvestedEntry& Entry = Harvested.Items[EntryIndex];
	++Entry.Hits;
	const bool bDone = Entry.Hits >= Row->HitsToHarvest;
	Entry.RespawnAt = (bDone && Row->ResetRule == EGrimPropReset::Seconds) ? GetServerSeconds() + Row->ResetSeconds : -1.0;
	Harvested.MarkItemDirty(Entry);

	int32 Given = 0;
	if (bDone && !Row->Item.IsNone())
	{
		const int32 Count = FMath::RandRange(Row->CountMin, FMath::Max(Row->CountMin, Row->CountMax));
		Given = Receiver ? UGrimItemStatics::GiveItem(Receiver, Row->Item, Count) : 0;
		if (Receiver && Given < Count)
		{
			// 가방이 차면 그 자리에 떨군다
			UGrimItemStatics::SpawnPickup(this, Row->Item, Count - Given, Points[PointIndex].Transform.GetLocation() + FVector(0.f, 0.f, 30.f));
		}
		UE_LOG(LogGrimDressing, Log, TEXT("[Server] %s: 점 %d(%s) 거둠 — %s ×%d(가방 %d)%s"), *GetName(), PointIndex, *Points[PointIndex].Kind.ToString(),
			*Row->Item.ToString(), Count, Given, Key.IsEmpty() ? TEXT("") : *FString::Printf(TEXT(" · %s 몫"), *Key));
	}
	RefreshPoint(PointIndex);
	UpdateRespawnTimer();
	return Given;
}

// ───── 다시 나기 ─────

void AGrimHarvestCell::HandleDawn()
{
	if (!HasAuthority())
	{
		return;
	}
	TArray<int32> Refresh;
	FlushNetDormancy();
	const int32 Before = Harvested.Items.Num();
	Harvested.Items.RemoveAll([&](const FGrimHarvestedEntry& Entry)
	{
		const FGrimHarvestNodeRow* Row = Points.IsValidIndex(Entry.PointIndex) ? FGrimDressingData::FindHarvestNode(Points[Entry.PointIndex].Kind) : nullptr;
		const bool bRemove = !Row || Row->ResetRule == EGrimPropReset::Dawn;
		if (bRemove)
		{
			Refresh.Add(Entry.PointIndex);
		}
		return bRemove;
	});
	if (Harvested.Items.Num() != Before)
	{
		Harvested.MarkArrayDirty();
		UE_LOG(LogGrimDressing, Log, TEXT("[Server] %s: 아침 — %d 점이 다시 났다"), *GetName(), Before - Harvested.Items.Num());
	}
	for (int32 Index : Refresh)
	{
		RefreshPoint(Index);
	}
}

void AGrimHarvestCell::RespawnDue(double NowSeconds)
{
	if (!HasAuthority())
	{
		return;
	}
	TArray<int32> Refresh;
	const int32 Before = Harvested.Items.Num();
	FlushNetDormancy();
	Harvested.Items.RemoveAll([&](const FGrimHarvestedEntry& Entry)
	{
		const bool bRemove = Entry.RespawnAt >= 0.0 && Entry.RespawnAt <= NowSeconds;
		if (bRemove)
		{
			Refresh.Add(Entry.PointIndex);
		}
		return bRemove;
	});
	if (Harvested.Items.Num() != Before)
	{
		Harvested.MarkArrayDirty();
	}
	for (int32 Index : Refresh)
	{
		RefreshPoint(Index);
	}
	UpdateRespawnTimer();
}

void AGrimHarvestCell::HandleRespawnTimer()
{
	RespawnDue(GetServerSeconds());
}

void AGrimHarvestCell::UpdateRespawnTimer()
{
	UWorld* World = GetWorld();
	if (!World || !HasAuthority())
	{
		return;
	}
	double Next = TNumericLimits<double>::Max();
	for (const FGrimHarvestedEntry& Entry : Harvested.Items)
	{
		if (Entry.RespawnAt >= 0.0)
		{
			Next = FMath::Min(Next, Entry.RespawnAt);
		}
	}
	FTimerManager& Timers = World->GetTimerManager();
	Timers.ClearTimer(RespawnTimer);
	if (Next < TNumericLimits<double>::Max())
	{
		Timers.SetTimer(RespawnTimer, this, &AGrimHarvestCell::HandleRespawnTimer, FMath::Max(0.1f, static_cast<float>(Next - GetServerSeconds())), false);
	}
}

void AGrimHarvestCell::ApplyReplicatedForTest(const FGrimHarvestedArray& FromServer)
{
	TArray<int32> Removed;
	for (const FGrimHarvestedEntry& Old : Harvested.Items)
	{
		if (!FromServer.Items.ContainsByPredicate([&](const FGrimHarvestedEntry& E) { return E.PointIndex == Old.PointIndex && E.PlayerKey == Old.PlayerKey; }))
		{
			Removed.Add(Old.PointIndex);
		}
	}
	Harvested.Items = FromServer.Items;
	Harvested.Owner = this;
	for (FGrimHarvestedEntry& Entry : Harvested.Items)
	{
		Entry.PostReplicatedAdd(Harvested);
	}
	for (int32 Index : Removed)
	{
		RefreshPoint(Index);
	}
}

// ───── 세이브 ─────

FName AGrimHarvestCell::GetSaveId() const
{
	return FName(*FString::Printf(TEXT("Dressing.Harvest.%s"), *GetName()));
}

void AGrimHarvestCell::WriteSaveData(FArchive& Ar)
{
	int32 Count = Harvested.Items.Num();
	Ar << Count;
	const double Now = GetServerSeconds();
	for (FGrimHarvestedEntry& Entry : Harvested.Items)
	{
		float Remaining = Entry.RespawnAt >= 0.0 ? static_cast<float>(FMath::Max(0.0, Entry.RespawnAt - Now)) : -1.f;
		Ar << Entry.PointIndex << Entry.PlayerKey << Entry.Hits << Remaining;
	}
}

void AGrimHarvestCell::ReadSaveData(FArchive& Ar, int32 Version)
{
	int32 Count = 0;
	Ar << Count;
	FlushNetDormancy();
	Harvested.Items.Reset();
	const double Now = GetServerSeconds();
	for (int32 i = 0; i < Count && !Ar.IsError(); ++i)
	{
		FGrimHarvestedEntry Entry;
		float Remaining = -1.f;
		Ar << Entry.PointIndex << Entry.PlayerKey << Entry.Hits << Remaining;
		Entry.RespawnAt = Remaining >= 0.f ? Now + Remaining : -1.0;
		if (Points.IsValidIndex(Entry.PointIndex))
		{
			Harvested.Items.Add(Entry);
		}
	}
	Harvested.MarkArrayDirty();
	for (int32 Index = 0; Index < Points.Num(); ++Index)
	{
		RefreshPoint(Index);
	}
	UpdateRespawnTimer();
}

#undef LOCTEXT_NAMESPACE
