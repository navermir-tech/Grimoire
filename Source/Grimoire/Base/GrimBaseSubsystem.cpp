#include "Base/GrimBaseSubsystem.h"
#include "Base/GrimBaseCore.h"
#include "Base/GrimBaseSettings.h"
#include "Base/GrimBuildPiece.h"
#include "Base/GrimBuilderComponent.h"
#include "Base/GrimEquipmentComponent.h"
#include "Base/GrimStructureComponent.h"
#include "Core/GrimPlayerState.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "TimerManager.h"

UGrimBaseSubsystem* UGrimBaseSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UGrimBaseSubsystem>() : nullptr;
}

bool UGrimBaseSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

bool UGrimBaseSubsystem::IsServer() const
{
	const UWorld* World = GetWorld();
	return World && World->GetNetMode() != NM_Client;
}

void UGrimBaseSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (!IsServer())
	{
		return;
	}
	// 표를 미리 읽어 문제를 시작 로그에 남긴다.
	UGrimBaseSettings::GetDatabase();

	ActorSpawnedHandle = InWorld.AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateWeakLambda(this, [this](AActor* Actor)
	{
		if (APlayerState* PlayerState = Cast<APlayerState>(Actor))
		{
			EnsurePlayerComponents(PlayerState);
		}
	}));
	if (const AGameStateBase* GameState = InWorld.GetGameState())
	{
		for (APlayerState* PlayerState : GameState->PlayerArray)
		{
			EnsurePlayerComponents(PlayerState);
		}
	}
}

void UGrimBaseSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
		World->GetTimerManager().ClearTimer(CollapseTimer);
	}
	Super::Deinitialize();
}

void UGrimBaseSubsystem::EnsurePlayerComponents(APlayerState* PlayerState)
{
	if (!PlayerState || !IsServer() || !Cast<AGrimPlayerState>(PlayerState))
	{
		return;
	}
	if (!PlayerState->FindComponentByClass<UGrimBuilderComponent>())
	{
		UGrimBuilderComponent* Builder = NewObject<UGrimBuilderComponent>(PlayerState, UGrimBuilderComponent::StaticClass(), UGrimBuilderComponent::ComponentName);
		PlayerState->AddInstanceComponent(Builder);
		Builder->RegisterComponent();
	}
	if (!PlayerState->FindComponentByClass<UGrimEquipmentComponent>())
	{
		UGrimEquipmentComponent* Equipment = NewObject<UGrimEquipmentComponent>(PlayerState, UGrimEquipmentComponent::StaticClass(), UGrimEquipmentComponent::ComponentName);
		PlayerState->AddInstanceComponent(Equipment);
		Equipment->RegisterComponent();
	}
	UE_LOG(LogGrimBase, Log, TEXT("[Server] %s 에 건축 · 장비 컴포넌트를 붙였다"), *GetNameSafe(PlayerState));
}

// ───── 목록 ─────

void UGrimBaseSubsystem::RegisterCore(AGrimBaseCore* Core)
{
	Cores.AddUnique(Core);
}

void UGrimBaseSubsystem::UnregisterCore(AGrimBaseCore* Core)
{
	Cores.Remove(Core);
}

void UGrimBaseSubsystem::RegisterPiece(AGrimBuildPiece* Piece)
{
	Pieces.AddUnique(Piece);
}

void UGrimBaseSubsystem::UnregisterPiece(AGrimBuildPiece* Piece)
{
	Pieces.Remove(Piece);
}

void UGrimBaseSubsystem::RegisterStructure(UGrimStructureComponent* Structure)
{
	Structures.AddUnique(Structure);
}

void UGrimBaseSubsystem::UnregisterStructure(UGrimStructureComponent* Structure)
{
	Structures.Remove(Structure);
}

TArray<AGrimBaseCore*> UGrimBaseSubsystem::GetCores() const
{
	TArray<AGrimBaseCore*> Result;
	for (const TWeakObjectPtr<AGrimBaseCore>& Core : Cores)
	{
		if (Core.IsValid() && !Core->IsActorBeingDestroyed())
		{
			Result.Add(Core.Get());
		}
	}
	return Result;
}

TArray<AGrimBuildPiece*> UGrimBaseSubsystem::GetPieces(const AGrimBaseCore* Core) const
{
	TArray<AGrimBuildPiece*> Result;
	for (const TWeakObjectPtr<AGrimBuildPiece>& Piece : Pieces)
	{
		if (Piece.IsValid() && !Piece->IsActorBeingDestroyed() && (!Core || FindBaseAt(Piece->GetActorLocation()) == Core))
		{
			Result.Add(Piece.Get());
		}
	}
	return Result;
}

TArray<UGrimStructureComponent*> UGrimBaseSubsystem::GetStructures() const
{
	TArray<UGrimStructureComponent*> Result;
	for (const TWeakObjectPtr<UGrimStructureComponent>& Structure : Structures)
	{
		if (Structure.IsValid() && Structure->GetOwner() && !Structure->GetOwner()->IsActorBeingDestroyed())
		{
			Result.Add(Structure.Get());
		}
	}
	return Result;
}

AGrimBaseCore* UGrimBaseSubsystem::FindBaseAt(const FVector& Location) const
{
	AGrimBaseCore* Best = nullptr;
	double BestDistance = TNumericLimits<double>::Max();
	for (AGrimBaseCore* Core : GetCores())
	{
		if (Core->IsInside(Location))
		{
			const double Distance = FVector::DistSquared2D(Location, Core->GetActorLocation());
			if (Distance < BestDistance)
			{
				BestDistance = Distance;
				Best = Core;
			}
		}
	}
	return Best;
}

int32 UGrimBaseSubsystem::CountCoresOwnedBy(const FString& OwnerId) const
{
	int32 Count = 0;
	for (const AGrimBaseCore* Core : GetCores())
	{
		Count += Core->GetOwnerId() == OwnerId ? 1 : 0;
	}
	return Count;
}

// ───── 무너짐 ─────

void UGrimBaseSubsystem::NotifyStructureBroken(UGrimStructureComponent* Structure)
{
	if (!IsServer() || !Structure || !Cast<AGrimBuildPiece>(Structure->GetOwner()))
	{
		return;
	}
	UWorld* World = GetWorld();
	if (World && !World->GetTimerManager().IsTimerActive(CollapseTimer))
	{
		World->GetTimerManager().SetTimer(CollapseTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			CollapseUnsupported();
		}), 0.1f, false);
	}
}

int32 UGrimBaseSubsystem::CollapseUnsupported()
{
	if (!IsServer())
	{
		return 0;
	}
	// 거점(격자)마다 따로 — 같은 칸 번호가 다른 거점에도 있다.
	TMap<AGrimBaseCore*, TArray<AGrimBuildPiece*>> Groups;
	for (AGrimBuildPiece* Piece : GetPieces())
	{
		Groups.FindOrAdd(Piece->GetBaseCore()).Add(Piece);
	}
	int32 Collapsed = 0;
	for (TPair<AGrimBaseCore*, TArray<AGrimBuildPiece*>>& Group : Groups)
	{
		TArray<FGrimBuildSlot> Slots;
		for (const AGrimBuildPiece* Piece : Group.Value)
		{
			Slots.Add(Piece->GetSlot());
		}
		for (const int32 Index : FGrimBuildSupport::FindUnsupported(Slots))
		{
			if (UGrimStructureComponent* Structure = Group.Value[Index]->GetStructure())
			{
				Structure->Break(true, TEXT("받침이 사라짐"));
				++Collapsed;
			}
		}
	}
	if (Collapsed > 0)
	{
		UE_LOG(LogGrimBase, Log, TEXT("[Server] 받침 없는 조각 %d개가 무너졌다"), Collapsed);
	}
	return Collapsed;
}
