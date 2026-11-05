// 생물 디버그 콘솔 명령 — 헤드리스 리슨 서버 + 클라이언트로 스폰 · 사냥 · 죽음 · 전리품 복제를 확인할 때 쓴다(Scripts/Creatures/net_check_creatures.ps1).
// 지연(초)을 받는 건 -ExecCmds 가 맵 · 접속보다 먼저 실행되기 때문이다. 지연 뒤에 그 프로세스의 모든 게임 월드에 적용한다.

#include "Creatures/GrimCreature.h"
#include "Creatures/GrimCreatureAIController.h"
#include "Creatures/GrimCreatureMath.h"
#include "Creatures/GrimCreatureSettings.h"
#include "Creatures/GrimCreatureSpecies.h"
#include "Creatures/GrimEcosystemSubsystem.h"
#include "Creatures/GrimHabitatVolume.h"
#include "Core/GrimGameplayTags.h"
#include "Environment/GrimTimeOfDaySubsystem.h"
#include "Interaction/GrimElementalStateComponent.h"
#include "Magic/GrimSpellAbility.h"
#include "AbilitySystemComponent.h"
#include "Components/BoxComponent.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "HAL/IConsoleManager.h"

#if !UE_BUILD_SHIPPING

namespace GrimCreatureDebugLocal // 유니티 빌드 이름 충돌 방지
{
	const TCHAR* NetModeName(const UWorld* World)
	{
		switch (World->GetNetMode())
		{
		case NM_Client: return TEXT("Client");
		case NM_ListenServer: return TEXT("Server");
		case NM_DedicatedServer: return TEXT("Server");
		default: return TEXT("Standalone");
		}
	}

	void RunInGameWorlds(float Delay, TFunction<void(UWorld*)> Fn)
	{
		auto RunNow = [Fn]()
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				UWorld* World = Context.World();
				if (World && World->IsGameWorld())
				{
					Fn(World);
				}
			}
		};
		if (Delay <= 0.f)
		{
			RunNow();
			return;
		}
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([RunNow](float)
		{
			RunNow();
			return false;
		}), Delay);
	}

	float ArgFloat(const TArray<FString>& Args, int32 Index, float Default)
	{
		return Args.IsValidIndex(Index) ? FCString::Atof(*Args[Index]) : Default;
	}

	/** 서버의 첫 플레이어 폰(없으면 nullptr) */
	APawn* FirstPlayerPawn(UWorld* World)
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if (APawn* Pawn = It->Get() ? It->Get()->GetPawn() : nullptr)
			{
				return Pawn;
			}
		}
		return nullptr;
	}

	bool MatchesSpecies(const AGrimCreature* Creature, const FString& Filter)
	{
		return Filter.IsEmpty() || Filter == TEXT("*") || Creature->GetInstance().SpeciesId.ToString().Equals(Filter, ESearchCase::IgnoreCase);
	}

	/** 플레이어 앞(시선 방향) Distance 지점 — 없으면 원점 */
	FVector PointNearPlayer(UWorld* World, float Distance, float SideOffset)
	{
		const APawn* Pawn = FirstPlayerPawn(World);
		if (!Pawn)
		{
			return FVector(Distance, SideOffset, 0.f);
		}
		FVector Forward = Pawn->GetActorForwardVector().GetSafeNormal2D();
		if (Forward.IsNearlyZero())
		{
			Forward = FVector::ForwardVector;
		}
		const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward);
		return Pawn->GetActorLocation() + Forward * Distance + Right * SideOffset;
	}

	/** Grim.Creature.Spawn <SpeciesId> [수=1] [지연초] [거리=800] — 서버: 플레이어 앞에 개체를 굴려 놓는다(서식 구역 밖 — 접히지 않음) */
	void SpawnCommand(const TArray<FString>& Args)
	{
		if (Args.Num() < 1)
		{
			UE_LOG(LogGrimCreatures, Warning, TEXT("사용: Grim.Creature.Spawn <종 id> [수] [지연초] [거리cm]"));
			return;
		}
		const FName SpeciesId(*Args[0]);
		const int32 Count = FMath::Max(1, static_cast<int32>(ArgFloat(Args, 1, 1.f)));
		const float Distance = ArgFloat(Args, 3, 800.f);
		RunInGameWorlds(ArgFloat(Args, 2, 0.f), [SpeciesId, Count, Distance](UWorld* World)
		{
			if (World->GetNetMode() == NM_Client)
			{
				return;
			}
			UGrimCreatureSpecies* Species = UGrimCreatureSettings::Get()->FindSpecies(SpeciesId);
			if (!Species)
			{
				UE_LOG(LogGrimCreatures, Warning, TEXT("Grim.Creature.Spawn: 종 없음 %s (설정 Species 를 보라)"), *SpeciesId.ToString());
				return;
			}
			int32 Spawned = 0;
			for (int32 i = 0; i < Count; ++i)
			{
				const FVector Point = PointNearPlayer(World, Distance, (i - (Count - 1) * 0.5f) * 150.f);
				FVector Ground = Point;
				UGrimEcosystemSubsystem::FindGround(World, Point, Ground);
				const FGrimCreatureInstance Instance = GrimCreatureMath::RollInstance(*Species, FMath::Rand() + 1, UGrimCreatureSettings::Get()->Roll);
				if (AGrimCreature::SpawnCreature(World, Species, Instance, Ground, FMath::FRandRange(0.f, 360.f)))
				{
					++Spawned;
				}
			}
			UE_LOG(LogGrimCreatures, Display, TEXT("[%s] Spawn %s x%d"), NetModeName(World), *SpeciesId.ToString(), Spawned);
		});
	}

	/** Grim.Creature.Habitat <SpeciesId> [상한=4] [지연초] [거리=1500] [반크기=600] — 서버: 플레이어 앞에 서식 구역을 만든다 */
	void HabitatCommand(const TArray<FString>& Args)
	{
		if (Args.Num() < 1)
		{
			UE_LOG(LogGrimCreatures, Warning, TEXT("사용: Grim.Creature.Habitat <종 id> [상한] [지연초] [거리cm] [반크기cm]"));
			return;
		}
		const FName SpeciesId(*Args[0]);
		const int32 Cap = FMath::Max(1, static_cast<int32>(ArgFloat(Args, 1, 4.f)));
		const float Distance = ArgFloat(Args, 3, 1500.f);
		const float HalfSize = ArgFloat(Args, 4, 600.f);
		RunInGameWorlds(ArgFloat(Args, 2, 0.f), [SpeciesId, Cap, Distance, HalfSize](UWorld* World)
		{
			if (World->GetNetMode() == NM_Client)
			{
				return;
			}
			UGrimCreatureSpecies* Species = UGrimCreatureSettings::Get()->FindSpecies(SpeciesId);
			if (!Species)
			{
				UE_LOG(LogGrimCreatures, Warning, TEXT("Grim.Creature.Habitat: 종 없음 %s"), *SpeciesId.ToString());
				return;
			}
			const FTransform Transform(PointNearPlayer(World, Distance, 0.f));
			AGrimHabitatVolume* Volume = World->SpawnActorDeferred<AGrimHabitatVolume>(AGrimHabitatVolume::StaticClass(), Transform);
			if (!Volume)
			{
				return;
			}
			FGrimHabitatSpeciesEntry& Entry = Volume->Species.AddDefaulted_GetRef();
			Entry.Species = Species;
			Entry.MaxCount = Cap;
			Volume->GetBounds()->SetBoxExtent(FVector(HalfSize, HalfSize, 500.f));
			Volume->FinishSpawning(Transform);
			UE_LOG(LogGrimCreatures, Display, TEXT("[%s] Habitat %s 상한 %d"), NetModeName(World), *SpeciesId.ToString(), Cap);
		});
	}

	/** Grim.Creature.Damage <양> [지연초] [종 id|*] [Element.*] — 서버: 첫 플레이어가 쏜 것처럼 살아 있는 생물에 마법 피해 */
	void DamageCommand(const TArray<FString>& Args)
	{
		if (Args.Num() < 1)
		{
			UE_LOG(LogGrimCreatures, Warning, TEXT("사용: Grim.Creature.Damage <양> [지연초] [종 id|*] [Element.*]"));
			return;
		}
		const float Amount = ArgFloat(Args, 0, 0.f);
		const FString Filter = Args.IsValidIndex(2) ? Args[2] : FString();
		const FGameplayTag Element = Args.IsValidIndex(3) ? FGameplayTag::RequestGameplayTag(FName(*Args[3]), false) : FGameplayTag();
		RunInGameWorlds(ArgFloat(Args, 1, 0.f), [Amount, Filter, Element](UWorld* World)
		{
			if (World->GetNetMode() == NM_Client)
			{
				return;
			}
			UAbilitySystemComponent* SourceASC = UGrimSpellAbility::FindTargetASC(FirstPlayerPawn(World));
			int32 Count = 0;
			for (TActorIterator<AGrimCreature> It(World); It; ++It)
			{
				if (!It->IsDead() && MatchesSpecies(*It, Filter))
				{
					It->ApplyDebugDamage(Amount, Element, SourceASC);
					++Count;
				}
			}
			UE_LOG(LogGrimCreatures, Display, TEXT("[%s] Damage %.1f %s -> %d마리"), NetModeName(World), Amount, *Element.ToString(), Count);
		});
	}

	/** Grim.Creature.Stimulus <Element.*> <세기> [지연초] [종 id|*] — 서버: 생물에 속성 자극(마법 출처, 첫 플레이어가 쏜 것처럼) */
	void StimulusCommand(const TArray<FString>& Args)
	{
		if (Args.Num() < 2)
		{
			UE_LOG(LogGrimCreatures, Warning, TEXT("사용: Grim.Creature.Stimulus <Element.*> <세기> [지연초] [종 id|*]"));
			return;
		}
		FGrimElementalStimulus Stimulus;
		Stimulus.Element = FGameplayTag::RequestGameplayTag(FName(*Args[0]), false);
		Stimulus.Intensity = ArgFloat(Args, 1, 1.f);
		Stimulus.Source = EGrimStimulusSource::Spell;
		const FString Filter = Args.IsValidIndex(3) ? Args[3] : FString();
		RunInGameWorlds(ArgFloat(Args, 2, 0.f), [Stimulus, Filter](UWorld* World)
		{
			if (World->GetNetMode() == NM_Client || !Stimulus.Element.IsValid())
			{
				return;
			}
			APawn* Player = FirstPlayerPawn(World);
			int32 Count = 0;
			for (TActorIterator<AGrimCreature> It(World); It; ++It)
			{
				if (!It->IsDead() && MatchesSpecies(*It, Filter))
				{
					UGrimElementalStatics::ApplyStimulusToActor(*It, Stimulus, Player);
					++Count;
				}
			}
			UE_LOG(LogGrimCreatures, Display, TEXT("[%s] Stimulus %s %.1f -> %d마리"), NetModeName(World), *Stimulus.Element.ToString(), Stimulus.Intensity, Count);
		});
	}

	/** Grim.Creature.Kill [지연초] [종 id|*] — 서버: 첫 플레이어가 죽인 것으로 */
	void KillCommand(const TArray<FString>& Args)
	{
		const FString Filter = Args.IsValidIndex(1) ? Args[1] : FString();
		RunInGameWorlds(ArgFloat(Args, 0, 0.f), [Filter](UWorld* World)
		{
			if (World->GetNetMode() == NM_Client)
			{
				return;
			}
			APawn* Player = FirstPlayerPawn(World);
			AActor* Killer = Player && Player->GetPlayerState() ? static_cast<AActor*>(Player->GetPlayerState()) : Player;
			int32 Count = 0;
			for (TActorIterator<AGrimCreature> It(World); It; ++It)
			{
				if (!It->IsDead() && MatchesSpecies(*It, Filter))
				{
					It->DebugKill(Killer);
					++Count;
				}
			}
			UE_LOG(LogGrimCreatures, Display, TEXT("[%s] Kill -> %d마리"), NetModeName(World), Count);
		});
	}

	/** Grim.Creature.Time <0~1> [지연초] — 서버: 시각(0 = 자정, 0.5 = 정오) */
	void TimeCommand(const TArray<FString>& Args)
	{
		const float TimeOfDay = ArgFloat(Args, 0, 0.5f);
		RunInGameWorlds(ArgFloat(Args, 1, 0.f), [TimeOfDay](UWorld* World)
		{
			if (World->GetNetMode() == NM_Client)
			{
				return;
			}
			if (UGrimTimeOfDaySubsystem* Time = World->GetSubsystem<UGrimTimeOfDaySubsystem>())
			{
				Time->SetTimeOfDay(TimeOfDay);
				UE_LOG(LogGrimCreatures, Display, TEXT("[%s] Time %.2f (밤=%d)"), NetModeName(World), TimeOfDay, Time->IsNight() ? 1 : 0);
			}
		});
	}

	/** Grim.Creature.Dump [지연초] — 이 프로세스에서 본 생물(seed 로 서버 · 클라이언트를 잇는다) */
	void DumpCommand(const TArray<FString>& Args)
	{
		RunInGameWorlds(ArgFloat(Args, 0, 0.f), [](UWorld* World)
		{
			TArray<AGrimCreature*> List;
			for (TActorIterator<AGrimCreature> It(World); It; ++It)
			{
				List.Add(*It);
			}
			List.Sort([](const AGrimCreature& A, const AGrimCreature& B) { return A.GetInstance().Seed < B.GetInstance().Seed; });
			UE_LOG(LogGrimCreatures, Display, TEXT("[%s] Dump 생물 %d"), NetModeName(World), List.Num());
			for (const AGrimCreature* Creature : List)
			{
				const FGrimCreatureInstance& Instance = Creature->GetInstance();
				FString Colors;
				for (const FGrimCreatureRegionColor& Color : Instance.Colors)
				{
					Colors += FString::Printf(TEXT("%s%s"), Colors.IsEmpty() ? TEXT("") : TEXT(","), *Color.Color.ToFColor(true).ToHex());
				}
				FString Drops;
				for (const FGrimCreatureLootDrop& Drop : Creature->GetDeath().Drops)
				{
					Drops += FString::Printf(TEXT("%s%s x%d"), Drops.IsEmpty() ? TEXT("") : TEXT(","), *Drop.ItemId.ToString(), Drop.Count);
				}
				const UEnum* StateEnum = StaticEnum<EGrimCreatureAIState>();
				UE_LOG(LogGrimCreatures, Display, TEXT("[%s] seed=%d %s %s size=%.2f%s%s colors=%s hp=%.0f/%.0f dead=%d hidden=%d killer=%s drops={%s}"),
					NetModeName(World), Instance.Seed, *Instance.SpeciesId.ToString(),
					Instance.Sex == EGrimCreatureSex::Male ? TEXT("M") : TEXT("F"), Instance.SizeScale,
					Instance.bGiant ? TEXT(" giant") : TEXT(""), Instance.bMutant ? TEXT(" mutant") : TEXT(""), *Colors,
					Creature->GetHealth(), Creature->GetMaxHealth(), Creature->IsDead() ? 1 : 0, Creature->IsShadowHidden() ? 1 : 0,
					*Creature->GetDeath().KillerName, *Drops);
				// AI 상태는 판단 주기 차이로 순간 다를 수 있어 따로 찍는다(비교에서 뺀다).
				UE_LOG(LogGrimCreatures, Display, TEXT("[%s] ai seed=%d state=%s seeds=%d"), NetModeName(World), Instance.Seed,
					StateEnum ? *StateEnum->GetNameStringByValue(static_cast<int64>(Creature->GetAIState())) : TEXT("?"), Creature->GetSeedSpreadCount());
			}
		});
	}

	/** Grim.Creature.Eco [지연초] — 서버: 서식 구역 개체군 */
	void EcoCommand(const TArray<FString>& Args)
	{
		RunInGameWorlds(ArgFloat(Args, 0, 0.f), [](UWorld* World)
		{
			const UGrimEcosystemSubsystem* Ecosystem = World->GetSubsystem<UGrimEcosystemSubsystem>();
			if (!Ecosystem || World->GetNetMode() == NM_Client)
			{
				return;
			}
			UE_LOG(LogGrimCreatures, Display, TEXT("[%s] Eco 펼친 생물 %d (상한 %d)"), NetModeName(World), Ecosystem->GetActiveCount(), UGrimCreatureSettings::Get()->Ecosystem.MaxActiveCreatures);
			for (const FGrimHabitatRuntime& Runtime : Ecosystem->GetHabitats())
			{
				for (const FGrimHabitatPopulation& Population : Runtime.Populations)
				{
					UE_LOG(LogGrimCreatures, Display, TEXT("[%s] 구역 %s %s 현재 %d/%d (펼침 %d · 접힘 %d · 죽음 %d · 회복 %d)"), NetModeName(World),
						Runtime.Volume.IsValid() ? *Runtime.Volume->GetName() : TEXT("?"), Population.Species ? *Population.Species->SpeciesId.ToString() : TEXT("?"),
						Population.GetCurrent(), Population.Cap, Population.Active.Num(), Population.Folded.Num(), Population.Killed, Population.Regrown);
				}
			}
		});
	}

	FAutoConsoleCommand GSpawnCommand(TEXT("Grim.Creature.Spawn"), TEXT("서버: 플레이어 앞에 생물. <종 id> [수] [지연초] [거리cm]"), FConsoleCommandWithArgsDelegate::CreateStatic(&SpawnCommand));
	FAutoConsoleCommand GHabitatCommand(TEXT("Grim.Creature.Habitat"), TEXT("서버: 플레이어 앞에 서식 구역. <종 id> [상한] [지연초] [거리cm] [반크기cm]"), FConsoleCommandWithArgsDelegate::CreateStatic(&HabitatCommand));
	FAutoConsoleCommand GDamageCommand(TEXT("Grim.Creature.Damage"), TEXT("서버: 첫 플레이어가 쏜 마법 피해. <양> [지연초] [종 id|*] [Element.*]"), FConsoleCommandWithArgsDelegate::CreateStatic(&DamageCommand));
	FAutoConsoleCommand GStimulusCommand(TEXT("Grim.Creature.Stimulus"), TEXT("서버: 생물에 속성 자극. <Element.*> <세기> [지연초] [종 id|*]"), FConsoleCommandWithArgsDelegate::CreateStatic(&StimulusCommand));
	FAutoConsoleCommand GKillCommand(TEXT("Grim.Creature.Kill"), TEXT("서버: 첫 플레이어가 죽인 것으로. [지연초] [종 id|*]"), FConsoleCommandWithArgsDelegate::CreateStatic(&KillCommand));
	FAutoConsoleCommand GTimeCommand(TEXT("Grim.Creature.Time"), TEXT("서버: 시각 0~1(0 자정 · 0.5 정오). [지연초]"), FConsoleCommandWithArgsDelegate::CreateStatic(&TimeCommand));
	FAutoConsoleCommand GDumpCommand(TEXT("Grim.Creature.Dump"), TEXT("이 프로세스에서 본 생물. [지연초]"), FConsoleCommandWithArgsDelegate::CreateStatic(&DumpCommand));
	FAutoConsoleCommand GEcoCommand(TEXT("Grim.Creature.Eco"), TEXT("서버: 서식 구역 개체군. [지연초]"), FConsoleCommandWithArgsDelegate::CreateStatic(&EcoCommand));
}

#endif
