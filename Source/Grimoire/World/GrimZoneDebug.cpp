// 구역 이름 표시 개발 콘솔(개발 빌드) — docs/systems/world-zones.md 「콘솔」.
// 지연(초)을 받는 Grim.Zones.At 은 -ExecCmds 가 맵 · 접속보다 먼저 실행되기 때문이다. 본 구역을 바꾸는 명령은 서버(권한) 월드에서만 돈다.

#include "World/GrimZoneDiscoveryComponent.h"
#include "World/GrimZoneSettings.h"
#include "World/GrimZoneSubsystem.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "HAL/IConsoleManager.h"

#if !UE_BUILD_SHIPPING

namespace GrimZoneDebugLocal
{
	void ForEachGameWorld(TFunction<void(UWorld*)> Fn)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (UWorld* World = Context.World(); World && World->IsGameWorld())
			{
				Fn(World);
			}
		}
	}

	APlayerController* FindLocalController(UWorld* World)
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if (APlayerController* PlayerController = It->Get(); PlayerController && PlayerController->IsLocalController())
			{
				return PlayerController;
			}
		}
		return nullptr;
	}

	FString ZoneLabel(FName Zone)
	{
		if (Zone.IsNone())
		{
			return TEXT("(없음)");
		}
		const FGrimZoneDef* Def = UGrimZoneSettings::GetDatabase().Find(Zone);
		return Def ? FString::Printf(TEXT("%s 「%s」"), *Zone.ToString(), *Def->Label.ToString()) : Zone.ToString();
	}

	void Run(const TArray<FString>& Args)
	{
		if (Args.Num() == 0)
		{
			UE_LOG(LogGrimZone, Display, TEXT("Grim.Zones: Where | Dump | Reset | Show <구역> [처음=1] | Reload"));
			return;
		}
		const FString Sub = Args[0];
		if (Sub.Equals(TEXT("Reload"), ESearchCase::IgnoreCase))
		{
			UGrimZoneSettings::ReloadData();
			return;
		}
		ForEachGameWorld([&](UWorld* World)
		{
			const TCHAR* Net = World->GetNetMode() == NM_Client ? TEXT("Client") : TEXT("Server");
			const UGrimZoneSubsystem* Subsystem = World->GetSubsystem<UGrimZoneSubsystem>();
			APlayerController* Local = FindLocalController(World);
			if (Sub.Equals(TEXT("Where"), ESearchCase::IgnoreCase))
			{
				const APawn* Pawn = Local ? Local->GetPawn() : nullptr;
				if (!Pawn || !Subsystem)
				{
					UE_LOG(LogGrimZone, Display, TEXT("[%s] 구역: 로컬 폰 없음"), Net);
					return;
				}
				const FGrimZoneHit Hit = Subsystem->FindZonesAt(Pawn->GetActorLocation());
				const FVector2D Map = Subsystem->WorldToMap(Pawn->GetActorLocation());
				UE_LOG(LogGrimZone, Display, TEXT("[%s] 구역: 지도 (%.0f, %.0f) m · 큰 구역 %s · 소구역 %s · 구역 맵 %d"),
					Net, Map.X, Map.Y, *ZoneLabel(Hit.Region), *ZoneLabel(Hit.Sector), Subsystem->IsZoneMap() ? 1 : 0);
			}
			else if (Sub.Equals(TEXT("Show"), ESearchCase::IgnoreCase) && Args.Num() >= 2)
			{
				UGrimZoneDiscoveryComponent* Zones = Local ? UGrimZoneDiscoveryComponent::FindForActor(Local) : nullptr;
				if (Zones)
				{
					const bool bFirst = Args.Num() < 3 || FCString::Atoi(*Args[2]) != 0;
					Zones->ShowBannerLocal(FName(*Args[1]), bFirst);
					UE_LOG(LogGrimZone, Display, TEXT("[%s] 배너 %s (%s)"), Net, *ZoneLabel(FName(*Args[1])), bFirst ? TEXT("처음") : TEXT("다시"));
				}
			}
			else if (World->GetNetMode() != NM_Client && World->GetGameState())
			{
				for (APlayerState* PlayerState : World->GetGameState()->PlayerArray)
				{
					UGrimZoneDiscoveryComponent* Zones = UGrimZoneDiscoveryComponent::FindForPlayerState(PlayerState);
					if (!Zones)
					{
						continue;
					}
					if (Sub.Equals(TEXT("Reset"), ESearchCase::IgnoreCase))
					{
						Zones->ServerResetDiscovered();
						UE_LOG(LogGrimZone, Display, TEXT("[%s] %s: 본 구역을 잊었다"), Net, *GetNameSafe(PlayerState));
					}
					else if (Sub.Equals(TEXT("Dump"), ESearchCase::IgnoreCase))
					{
						TArray<FString> Names;
						for (const FName& Name : Zones->GetDiscovered())
						{
							Names.Add(ZoneLabel(Name));
						}
						UE_LOG(LogGrimZone, Display, TEXT("[%s] %s: 지금 %s / %s · 본 구역 %d — %s"), Net, *GetNameSafe(PlayerState),
							*ZoneLabel(Zones->GetCurrent().Region), *ZoneLabel(Zones->GetCurrent().Sector), Names.Num(), *FString::Join(Names, TEXT(", ")));
					}
				}
			}
		});
	}

	void AtCommand(const TArray<FString>& Args)
	{
		if (Args.Num() < 2)
		{
			UE_LOG(LogGrimZone, Display, TEXT("Grim.Zones.At <초> <명령> [인자…]"));
			return;
		}
		const float Delay = FCString::Atof(*Args[0]);
		TArray<FString> Rest(Args.GetData() + 1, Args.Num() - 1);
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Rest](float)
		{
			Run(Rest);
			return false;
		}), FMath::Max(0.f, Delay));
	}

	FAutoConsoleCommand GRunCommand(
		TEXT("Grim.Zones"),
		TEXT("구역 이름 표시. Where(지금 자리의 구역) | Dump(본 구역 — 서버) | Reset(본 구역을 잊는다 — 서버) | Show <구역> [처음=1](배너만 띄우기) | Reload(구역 표 다시 읽기)"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&Run));

	FAutoConsoleCommand GAtCommand(
		TEXT("Grim.Zones.At"),
		TEXT("몇 초 뒤 Grim.Zones 명령. <초> <명령> [인자…]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&AtCommand));
}

#endif
