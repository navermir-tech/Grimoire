#include "Dressing/GrimDressingSubsystem.h"
#include "Dressing/GrimDressingTypes.h"
#include "Dressing/GrimJournalComponent.h"
#include "Core/GrimPlayerState.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"

bool UGrimDressingSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UGrimDressingSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (InWorld.GetNetMode() == NM_Client)
	{
		return;
	}
	// 표를 미리 읽어 문제를 시작 로그에 남긴다
	FGrimDressingData::GetPropTable();
	FGrimDressingData::GetHarvestTable();
	FGrimDressingData::GetJournalTable();

	ActorSpawnedHandle = InWorld.AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateWeakLambda(this, [this](AActor* Actor)
	{
		if (APlayerState* PlayerState = Cast<APlayerState>(Actor))
		{
			EnsureJournal(PlayerState);
		}
	}));
	if (const AGameStateBase* GameState = InWorld.GetGameState())
	{
		for (APlayerState* PlayerState : GameState->PlayerArray)
		{
			EnsureJournal(PlayerState);
		}
	}
}

void UGrimDressingSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
	}
	Super::Deinitialize();
}

UGrimJournalComponent* UGrimDressingSubsystem::EnsureJournal(APlayerState* PlayerState)
{
	if (!PlayerState || !Cast<AGrimPlayerState>(PlayerState) || PlayerState->GetNetMode() == NM_Client)
	{
		return nullptr;
	}
	if (UGrimJournalComponent* Existing = PlayerState->FindComponentByClass<UGrimJournalComponent>())
	{
		return Existing;
	}
	UGrimJournalComponent* Journal = NewObject<UGrimJournalComponent>(PlayerState, UGrimJournalComponent::StaticClass(), UGrimJournalComponent::ComponentName);
	PlayerState->AddInstanceComponent(Journal);
	Journal->RegisterComponent();
	return Journal;
}
