#include "World/GrimZoneDiscoveryComponent.h"

#include "World/GrimZoneSettings.h"
#include "Save/GrimSaveSubsystem.h"
#include "UI/GrimZoneBannerWidget.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

const FName UGrimZoneDiscoveryComponent::ComponentName(TEXT("GrimZones"));

UGrimZoneDiscoveryComponent::UGrimZoneDiscoveryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UGrimZoneDiscoveryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UGrimZoneDiscoveryComponent, Discovered, COND_OwnerOnly);
}

UGrimZoneDiscoveryComponent* UGrimZoneDiscoveryComponent::FindForPlayerState(const APlayerState* PlayerState)
{
	return PlayerState ? PlayerState->FindComponentByClass<UGrimZoneDiscoveryComponent>() : nullptr;
}

UGrimZoneDiscoveryComponent* UGrimZoneDiscoveryComponent::FindForActor(const AActor* Actor)
{
	if (const APlayerState* PlayerState = Cast<APlayerState>(Actor))
	{
		return FindForPlayerState(PlayerState);
	}
	if (const AController* Controller = Cast<AController>(Actor))
	{
		return FindForPlayerState(Controller->PlayerState);
	}
	if (const APawn* Pawn = Cast<APawn>(Actor))
	{
		return FindForPlayerState(Pawn->GetPlayerState());
	}
	return nullptr;
}

void UGrimZoneDiscoveryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Banner)
	{
		Banner->RemoveFromParent();
		Banner = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

// ───────────────────────── 서버 ─────────────────────────

void UGrimZoneDiscoveryComponent::ServerPoll(const FGrimZoneDatabase& Database, double Now)
{
	const APlayerState* PlayerState = Cast<APlayerState>(GetOwner());
	const APawn* Pawn = PlayerState ? PlayerState->GetPawn() : nullptr;
	if (!Pawn || Database.IsEmpty())
	{
		return;
	}
	// 세이브가 이 플레이어 몫을 읽기 전이면 기다린다 — 이미 본 구역을 「처음」으로 띄우지 않게
	if (const UGrimSaveSubsystem* Save = GetWorld() ? GetWorld()->GetSubsystem<UGrimSaveSubsystem>() : nullptr)
	{
		if (Save->IsActive() && !Save->IsPlayerLoaded(PlayerState))
		{
			return;
		}
	}

	const UGrimZoneSettings* Settings = UGrimZoneSettings::Get();
	const FGrimZoneHit Hit = FGrimZoneMath::FindAt(Database, FGrimZoneMath::WorldToMap(Database, Pawn->GetActorLocation()));
	TArray<FGrimZoneEntered> Entered;
	if (Tracker.Step(Hit, Now, Settings->StableSeconds, Settings->RevisitCooldownSeconds, Entered))
	{
		MarkDiscoveredChanged();
	}
	for (const FGrimZoneEntered& Entry : Entered)
	{
		UE_LOG(LogGrimZone, Log, TEXT("[Server] %s 가 %s 에 들어섬(%s)"), *GetNameSafe(PlayerState), *Entry.Zone.ToString(), Entry.bFirstVisit ? TEXT("처음") : TEXT("다시"));
		Client_ZoneEntered(Entry.Zone, Entry.bFirstVisit);
	}
}

void UGrimZoneDiscoveryComponent::ServerResetDiscovered()
{
	Tracker.Reset();
	MarkDiscoveredChanged();
}

void UGrimZoneDiscoveryComponent::MarkDiscoveredChanged()
{
	Discovered = Tracker.Discovered;
	// 서버(리슨 호스트 화면)는 복제 콜백이 없으니 직접 알린다
	OnDiscoveredChanged.Broadcast(this);
	if (AActor* Owner = GetOwner())
	{
		Owner->ForceNetUpdate();
	}
}

void UGrimZoneDiscoveryComponent::OnRep_Discovered()
{
	OnDiscoveredChanged.Broadcast(this);
}

// ───────────────────────── 세이브 ─────────────────────────

void UGrimZoneDiscoveryComponent::WriteSaveData(FArchive& Ar)
{
	TArray<FString> Names;
	for (const FName& Name : Tracker.Discovered)
	{
		Names.Add(Name.ToString());
	}
	Ar << Names;
}

void UGrimZoneDiscoveryComponent::ReadSaveData(FArchive& Ar, int32 Version)
{
	TArray<FString> Names;
	Ar << Names;
	if (Ar.IsError())
	{
		return;
	}
	TArray<FName> Loaded;
	for (const FString& Name : Names)
	{
		Loaded.Add(FName(*Name));
	}
	if (Tracker.MergeDiscovered(Loaded))
	{
		MarkDiscoveredChanged();
	}
}

// ───────────────────────── 클라이언트 ─────────────────────────

void UGrimZoneDiscoveryComponent::Client_ZoneEntered_Implementation(FName Zone, bool bFirstVisit)
{
	ShowBannerLocal(Zone, bFirstVisit);
}

APlayerController* UGrimZoneDiscoveryComponent::GetLocalOwningController() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
		if (PlayerController && PlayerController->IsLocalController() && PlayerController->PlayerState == GetOwner())
		{
			return PlayerController;
		}
	}
	return nullptr;
}

void UGrimZoneDiscoveryComponent::ShowBannerLocal(FName Zone, bool bFirstVisit)
{
	const UGrimZoneSettings* Settings = UGrimZoneSettings::Get();
	const FGrimZoneDef* Def = UGrimZoneSettings::GetDatabase().Find(Zone);
	if (!Def || !Settings->bShowBanners || GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	if (!Settings->GetStyle(Def->Tier, bFirstVisit).bEnabled)
	{
		return;
	}
	UE_LOG(LogGrimZone, Log, TEXT("[%s] 배너: %s 「%s」 (%s)"), GetNetMode() == NM_Client ? TEXT("Client") : TEXT("Server"),
		*Zone.ToString(), *Def->Label.ToString(), bFirstVisit ? TEXT("처음") : TEXT("다시"));
	if (!Banner)
	{
		APlayerController* PlayerController = GetLocalOwningController();
		if (!PlayerController)
		{
			return;
		}
		Banner = CreateWidget<UGrimZoneBannerWidget>(PlayerController, UGrimZoneBannerWidget::StaticClass());
		if (!Banner)
		{
			return;
		}
		// HUD(0) · 퀘스트 추적(5) 위, 메뉴 아래
		Banner->AddToViewport(6);
	}
	Banner->Enqueue(*Def, bFirstVisit);
}
