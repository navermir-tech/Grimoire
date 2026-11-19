#include "World/GrimZoneSubsystem.h"

#include "World/GrimZoneDiscoveryComponent.h"
#include "World/GrimZoneSettings.h"
#include "Core/GrimPlayerState.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "TimerManager.h"

namespace GrimZoneSubsystemLocal
{
	const FGrimZoneDatabase& EmptyDatabase()
	{
		static const FGrimZoneDatabase Empty;
		return Empty;
	}
}

bool UGrimZoneSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

bool UGrimZoneSubsystem::IsServer() const
{
	const UWorld* World = GetWorld();
	return World && World->GetNetMode() != NM_Client;
}

bool UGrimZoneSubsystem::IsZoneMap() const
{
	const TArray<FString>& Maps = UGrimZoneSettings::Get()->Maps;
	if (Maps.Num() == 0)
	{
		return true;
	}
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	const FString MapName = UWorld::RemovePIEPrefix(World->GetMapName());
	return Maps.ContainsByPredicate([&MapName](const FString& Name) { return MapName.Equals(Name, ESearchCase::IgnoreCase); });
}

const FGrimZoneDatabase& UGrimZoneSubsystem::GetDatabase() const
{
	return IsZoneMap() ? UGrimZoneSettings::GetDatabase() : GrimZoneSubsystemLocal::EmptyDatabase();
}

void UGrimZoneSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (!IsServer())
	{
		return;
	}
	// 본 구역 기록은 구역이 없는 맵에서도 세이브에 남아야 하니 컴포넌트는 늘 붙인다(살피기만 구역 맵에서).
	// PlayerState 가 생기자마자 — 세이브가 플레이어 몫을 읽기 전에 컴포넌트가 있어야 본 구역도 되돌려진다.
	ActorSpawnedHandle = InWorld.AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateWeakLambda(this, [this](AActor* Actor)
	{
		if (APlayerState* PlayerState = Cast<APlayerState>(Actor))
		{
			EnsureComponent(PlayerState);
		}
	}));
	if (const AGameStateBase* GameState = InWorld.GetGameState())
	{
		for (APlayerState* PlayerState : GameState->PlayerArray)
		{
			EnsureComponent(PlayerState);
		}
	}
	if (IsZoneMap())
	{
		UE_LOG(LogGrimZone, Log, TEXT("[Server] 구역 맵 — 구역 %d, %.2f 초마다 들어섬을 본다"), UGrimZoneSettings::GetDatabase().Zones.Num(), UGrimZoneSettings::Get()->PollIntervalSeconds);
		InWorld.GetTimerManager().SetTimer(PollTimer, FTimerDelegate::CreateUObject(this, &UGrimZoneSubsystem::PollNow),
			UGrimZoneSettings::Get()->PollIntervalSeconds, true);
	}
}

void UGrimZoneSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PollTimer);
		World->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
	}
	Super::Deinitialize();
}

UGrimZoneDiscoveryComponent* UGrimZoneSubsystem::EnsureComponent(APlayerState* PlayerState)
{
	if (!PlayerState || !IsServer() || !Cast<AGrimPlayerState>(PlayerState))
	{
		return nullptr;
	}
	if (UGrimZoneDiscoveryComponent* Existing = UGrimZoneDiscoveryComponent::FindForPlayerState(PlayerState))
	{
		return Existing;
	}
	UGrimZoneDiscoveryComponent* Zones = NewObject<UGrimZoneDiscoveryComponent>(PlayerState, UGrimZoneDiscoveryComponent::StaticClass(), UGrimZoneDiscoveryComponent::ComponentName);
	PlayerState->AddInstanceComponent(Zones);
	Zones->RegisterComponent();
	return Zones;
}

void UGrimZoneSubsystem::PollNow()
{
	UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	if (!GameState || !IsServer())
	{
		return;
	}
	const FGrimZoneDatabase& Database = GetDatabase();
	const double Now = World->GetTimeSeconds();
	const TArray<TObjectPtr<APlayerState>> Players = GameState->PlayerArray;
	for (APlayerState* PlayerState : Players)
	{
		if (UGrimZoneDiscoveryComponent* Zones = UGrimZoneDiscoveryComponent::FindForPlayerState(PlayerState))
		{
			Zones->ServerPoll(Database, Now);
		}
	}
}

FGrimZoneHit UGrimZoneSubsystem::FindZonesAt(const FVector& WorldLocation) const
{
	const FGrimZoneDatabase& Database = GetDatabase();
	return FGrimZoneMath::FindAt(Database, FGrimZoneMath::WorldToMap(Database, WorldLocation));
}

bool UGrimZoneSubsystem::GetZone(FName Zone, FGrimZoneDef& OutZone) const
{
	if (const FGrimZoneDef* Def = GetDatabase().Find(Zone))
	{
		OutZone = *Def;
		return true;
	}
	return false;
}

TArray<FGrimZoneDef> UGrimZoneSubsystem::GetZonesOfTier(EGrimZoneTier Tier) const
{
	TArray<FGrimZoneDef> Out;
	for (const FGrimZoneDef& Zone : GetDatabase().Zones)
	{
		if (Zone.Tier == Tier)
		{
			Out.Add(Zone);
		}
	}
	return Out;
}

TArray<FVector> UGrimZoneSubsystem::GetZoneWorldOutline(FName Zone) const
{
	TArray<FVector> Out;
	const FGrimZoneDatabase& Database = GetDatabase();
	if (const FGrimZoneDef* Def = Database.Find(Zone))
	{
		for (const FVector2D& Point : Def->Polygon)
		{
			Out.Add(FGrimZoneMath::MapToWorld(Database, Point));
		}
	}
	return Out;
}

FVector UGrimZoneSubsystem::GetZoneWorldCenter(FName Zone) const
{
	const FGrimZoneDatabase& Database = GetDatabase();
	const FGrimZoneDef* Def = Database.Find(Zone);
	return Def ? FGrimZoneMath::MapToWorld(Database, Def->Center) : FVector::ZeroVector;
}

FVector2D UGrimZoneSubsystem::WorldToMap(const FVector& WorldLocation) const
{
	return FGrimZoneMath::WorldToMap(GetDatabase(), WorldLocation);
}

FVector UGrimZoneSubsystem::MapToWorld(const FVector2D& MapPoint) const
{
	return FGrimZoneMath::MapToWorld(GetDatabase(), MapPoint);
}
