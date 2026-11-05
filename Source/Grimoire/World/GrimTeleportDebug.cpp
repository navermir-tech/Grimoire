// 이동 치트(개발 빌드) — 2026-11-05 플레이 테스트 요청: 「마법 생물 어딨어? 이동 치트를 만들어줘」.
//   Grim.Go <이름 일부> [지연초] 지형 표식(랜드마크 · POI · 서식 구역 · 탑 자리) · 생물 · 태그 붙은 액터 중 이름이 맞는 곳으로 순간이동
//   Grim.Go.List [이름 일부]   갈 수 있는 곳 목록(화면 · 로그)
// 예: Grim.Go Jackalope / Grim.Go Kelpie / Grim.Go BlackDog / Grim.Go GiantTree / Grim.Go Tower / Grim.Go Poi.Puzzle
// 서버(단독 · 리슨 호스트) 월드의 모든 플레이어 폰을 옮긴다 — 둘이 시험할 때 함께 간다.
#include "World/GrimTerrainMarker.h"
#include "Creatures/GrimCreature.h"
#include "Creatures/GrimCreatureSpecies.h"
#include "Creatures/GrimHabitatVolume.h"
#include "Creatures/GrimEcosystemSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Containers/Ticker.h"
#include "HAL/IConsoleManager.h"

#if !UE_BUILD_SHIPPING

namespace GrimTeleportDebugLocal
{
	struct FDestination
	{
		FString Name;
		FString Kind;
		FVector Location = FVector::ZeroVector;
	};

	/** 이 월드에서 갈 수 있는 곳 — 생물(나온 것 · 아직 접힌 무리) · 표식 · 서식 구역 · 태그 있는 액터.
	 *  생물을 앞에 둔다: 「Grim.Go Jackalope」 처럼 이름 일부만 치면 서식 구역 한가운데가 아니라 실제 무리로 간다
	 *  (구역은 수백 m 이고 무리는 플레이어 60m 안에서만 나와, 한가운데에 서면 아무것도 안 보였다 — 2026-11-05 플레이 테스트). */
	TArray<FDestination> Gather(UWorld* World)
	{
		TArray<FDestination> Out;
		for (TActorIterator<AGrimCreature> It(World); It; ++It)
		{
			if (const UGrimCreatureSpecies* Species = It->GetSpecies(); Species && !It->IsDead())
			{
				Out.Add({ FString::Printf(TEXT("Creature.%s"), *Species->SpeciesId.ToString()), TEXT("생물"), It->GetActorLocation() });
			}
		}
		if (const UGrimEcosystemSubsystem* Ecosystem = World->GetSubsystem<UGrimEcosystemSubsystem>())
		{
			for (const FGrimHabitatRuntime& Runtime : Ecosystem->GetHabitats())
			{
				for (const FGrimHabitatPopulation& Population : Runtime.Populations)
				{
					TSet<int32> SeenHerds;
					for (const FGrimFoldedCreature& Record : Population.Folded)
					{
						if (Population.Species && !SeenHerds.Contains(Record.HerdId))
						{
							SeenHerds.Add(Record.HerdId);
							Out.Add({ FString::Printf(TEXT("Creature.%s"), *Population.Species->SpeciesId.ToString()), TEXT("무리 — 가면 나온다"), Record.Location });
						}
					}
				}
			}
		}
		for (TActorIterator<AGrimTerrainMarker> It(World); It; ++It)
		{
			Out.Add({ It->MarkerTag.ToString(), TEXT("표식"), It->GetActorLocation() });
		}
		for (TActorIterator<AGrimHabitatVolume> It(World); It; ++It)
		{
			for (const FGrimHabitatSpeciesEntry& Entry : It->Species)
			{
				if (Entry.Species)
				{
					Out.Add({ FString::Printf(TEXT("Habitat.%s"), *Entry.Species->SpeciesId.ToString()), TEXT("서식 구역"), It->GetActorLocation() });
				}
			}
		}
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			for (const FName& Tag : It->Tags)
			{
				const FString TagString = Tag.ToString();
				if (TagString.StartsWith(TEXT("Tut_")) || TagString.StartsWith(TEXT("Npc_")) || TagString.StartsWith(TEXT("Hunter_")))
				{
					Out.Add({ TagString, TEXT("태그"), It->GetActorLocation() });
				}
			}
		}
		return Out;
	}

	UWorld* FindAuthorityWorld()
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			UWorld* World = Context.World();
			if (World && World->IsGameWorld() && World->GetNetMode() != NM_Client)
			{
				return World;
			}
		}
		return nullptr;
	}

	void Say(const FString& Text, const FColor Color = FColor::Cyan)
	{
		UE_LOG(LogTemp, Display, TEXT("[Grim.Go] %s"), *Text);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 8.f, Color, Text);
		}
	}

	/** 땅 높이를 찾는다 — 먼저 대상 바로 위(3m)에서 쏜다: 탑 안 대상이면 지붕이 아니라 그 층 바닥에 서게.
	 *  거기서 못 찾으면(언덕 속에서 쏜 경우 등) 하늘 높이에서 다시. 둘 다 없으면 그 높이 그대로. */
	FVector GroundAt(UWorld* World, const FVector& Location)
	{
		FHitResult Hit;
		const FVector End = Location - FVector(0.f, 0.f, 50000.f);
		for (const float StartHeight : { 300.f, 50000.f })
		{
			if (World->LineTraceSingleByChannel(Hit, Location + FVector(0.f, 0.f, StartHeight), End, ECC_Visibility))
			{
				return Hit.ImpactPoint;
			}
		}
		return Location;
	}

	void GoNow(const TArray<FString>& Args);

	/** Grim.Go <이름> [지연초] — 지연은 -ExecCmds 로 맵이 뜬 뒤 옮길 때(점검 스크립트) */
	void Go(const TArray<FString>& Args)
	{
		const float Delay = Args.IsValidIndex(1) ? FCString::Atof(*Args[1]) : 0.f;
		if (Delay <= 0.f)
		{
			GoNow(Args);
			return;
		}
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Args](float)
		{
			GoNow(Args);
			return false;
		}), Delay);
	}

	void GoNow(const TArray<FString>& Args)
	{
		UWorld* World = FindAuthorityWorld();
		if (!World || Args.Num() == 0)
		{
			Say(TEXT("사용: Grim.Go <이름 일부> — 목록은 Grim.Go.List"), FColor::Yellow);
			return;
		}
		const FString Query = Args[0];
		const TArray<FDestination> All = Gather(World);
		const FDestination* Best = nullptr;
		for (const FDestination& Destination : All)
		{
			if (Destination.Name.Equals(Query, ESearchCase::IgnoreCase))
			{
				Best = &Destination;
				break;
			}
			if (!Best && Destination.Name.Contains(Query, ESearchCase::IgnoreCase))
			{
				Best = &Destination;
			}
		}
		if (!Best)
		{
			Say(FString::Printf(TEXT("「%s」 를 찾지 못했다 — Grim.Go.List 로 이름을 보라"), *Query), FColor::Red);
			return;
		}

		// 대상에서 남쪽으로 4m 떨어져(겹치지 않게) 땅 위에 서서 대상을 본다.
		const FVector Ground = GroundAt(World, Best->Location - FVector(400.f, 0.f, 0.f));
		int32 Index = 0;
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It, ++Index)
		{
			APlayerController* PlayerController = It->Get();
			APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
			if (!Pawn)
			{
				continue;
			}
			const FVector Spot = GroundAt(World, Ground + FVector(0.f, 200.f * Index, 0.f)) + FVector(0.f, 0.f, 120.f);
			const FRotator Facing = (Best->Location - Spot).GetSafeNormal2D().Rotation();
			Pawn->TeleportTo(Spot, Facing, false, true);
			PlayerController->SetControlRotation(Facing);
			PlayerController->ClientSetLocation(Spot, Facing);
		}
		Say(FString::Printf(TEXT("%s(%s) 곁으로 이동 — %s"), *Best->Name, *Best->Kind, *Best->Location.ToCompactString()));
	}

	void List(const TArray<FString>& Args)
	{
		UWorld* World = FindAuthorityWorld();
		if (!World)
		{
			return;
		}
		const FString Filter = Args.Num() > 0 ? Args[0] : FString();
		TArray<FDestination> All = Gather(World);
		All.Sort([](const FDestination& A, const FDestination& B) { return A.Name < B.Name; });
		int32 Count = 0;
		for (const FDestination& Destination : All)
		{
			if (!Filter.IsEmpty() && !Destination.Name.Contains(Filter, ESearchCase::IgnoreCase))
			{
				continue;
			}
			UE_LOG(LogTemp, Display, TEXT("[Grim.Go.List] %s (%s) %s"), *Destination.Name, *Destination.Kind, *Destination.Location.ToCompactString());
			if (GEngine && Count < 40)
			{
				GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Cyan, FString::Printf(TEXT("%s  (%s)"), *Destination.Name, *Destination.Kind));
			}
			++Count;
		}
		Say(FString::Printf(TEXT("갈 수 있는 곳 %d 개(화면엔 40개까지, 전체는 로그)"), Count));
	}
}

static FAutoConsoleCommand GGrimGoCommand(
	TEXT("Grim.Go"),
	TEXT("이동 치트: 이름이 맞는 표식 · 서식 구역 · 생물 · 태그 액터 곁으로. 예: Grim.Go Jackalope / Grim.Go GiantTree / Grim.Go Tut_Candle"),
	FConsoleCommandWithArgsDelegate::CreateStatic(&GrimTeleportDebugLocal::Go));

static FAutoConsoleCommand GGrimGoListCommand(
	TEXT("Grim.Go.List"),
	TEXT("이동 치트로 갈 수 있는 곳 목록. 예: Grim.Go.List / Grim.Go.List Landmark"),
	FConsoleCommandWithArgsDelegate::CreateStatic(&GrimTeleportDebugLocal::List));

#endif
