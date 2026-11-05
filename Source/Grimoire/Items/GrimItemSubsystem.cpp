#include "Items/GrimItemSubsystem.h"
#include "Items/GrimInventoryComponent.h"
#include "Items/GrimItemSettings.h"
#include "Core/GrimPlayerState.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"

bool UGrimItemSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

bool UGrimItemSubsystem::IsServer() const
{
	const UWorld* World = GetWorld();
	return World && World->GetNetMode() != NM_Client;
}

void UGrimItemSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (!IsServer())
	{
		return;
	}

	// 표를 미리 읽어 문제를 시작 로그에 남긴다.
	UGrimItemSettings::GetDatabase();

	ActorSpawnedHandle = InWorld.AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateWeakLambda(this, [this](AActor* Actor)
	{
		if (APlayerState* PlayerState = Cast<APlayerState>(Actor))
		{
			EnsureInventory(PlayerState);
		}
	}));
	if (const AGameStateBase* GameState = InWorld.GetGameState())
	{
		for (APlayerState* PlayerState : GameState->PlayerArray)
		{
			EnsureInventory(PlayerState);
		}
	}
}

void UGrimItemSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
	}
	Super::Deinitialize();
}

UGrimInventoryComponent* UGrimItemSubsystem::EnsureInventory(APlayerState* PlayerState)
{
	if (!PlayerState || !IsServer() || !Cast<AGrimPlayerState>(PlayerState))
	{
		return nullptr;
	}
	if (UGrimInventoryComponent* Existing = PlayerState->FindComponentByClass<UGrimInventoryComponent>())
	{
		return Existing;
	}
	UGrimInventoryComponent* Inventory = NewObject<UGrimInventoryComponent>(PlayerState, UGrimInventoryComponent::StaticClass(), UGrimInventoryComponent::ComponentName);
	PlayerState->AddInstanceComponent(Inventory);
	Inventory->RegisterComponent();
	UE_LOG(LogGrimItems, Log, TEXT("[Server] %s 에 가방을 붙였다(%d 칸)"), *GetNameSafe(PlayerState), UGrimItemSettings::Get()->PlayerSlotCount);
	return Inventory;
}
