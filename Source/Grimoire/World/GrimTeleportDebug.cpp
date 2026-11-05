// 이동 치트(개발 빌드) — 2026-11-05 플레이 테스트 요청: 「마법 생물 어딨어? 이동 치트를 만들어줘」.
//   Grim.Go <이름 일부>        지형 표식(랜드마크 · POI · 서식 구역 · 탑 자리) · 생물 · 태그 붙은 액터 중 이름이 맞는 곳으로 순간이동
//   Grim.Go.List [이름 일부]   갈 수 있는 곳 목록(화면 · 로그)
// 예: Grim.Go Jackalope / Grim.Go Kelpie / Grim.Go BlackDog / Grim.Go GiantTree / Grim.Go Tower / Grim.Go Poi.Puzzle
// 서버(단독 · 리슨 호스트) 월드의 모든 플레이어 폰을 옮긴다 — 둘이 시험할 때 함께 간다.
#include "World/GrimTerrainMarker.h"
#include "Creatures/GrimCreature.h"
#include "Creatures/GrimCreatureSpecies.h"
#include "Creatures/GrimHabitatVolume.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
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

	/** 이 월드에서 갈 수 있는 곳 — 표식 · 서식 구역 · 살아 있는 생물 · 태그 있는 액터 */
	TArray<FDestination> Gather(UWorld* World)
	{
		TArray<FDestination> Out;
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
		for (TActorIterator<AGrimCreature> It(World); It; ++It)
		{
			if (const UGrimCreatureSpecies* Species = It->GetSpecies())
			{
				Out.Add({ FString::Printf(TEXT("Creature.%s"), *Species->SpeciesId.ToString()), TEXT("생물"), It->GetActorLocation() });
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

	/** 위에서 아래로 쏘아 땅 높이를 찾는다(없으면 그 높이 그대로) */
	FVector GroundAt(UWorld* World, const FVector& Location)
	{
		FHitResult Hit;
		const FVector Start = Location + FVector(0.f, 0.f, 50000.f);
		const FVector End = Location - FVector(0.f, 0.f, 50000.f);
		if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility))
		{
			return Hit.ImpactPoint;
		}
		return Location;
	}

	void Go(const TArray<FString>& Args)
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
