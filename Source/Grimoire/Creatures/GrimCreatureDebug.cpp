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
#include "DrawDebugHelpers.h"
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

	void Say(const FString& Text, const FColor Color = FColor::Cyan)
	{
		UE_LOG(LogGrimCreatures, Display, TEXT("%s"), *Text);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 8.f, Color, Text);
		}
	}

	/** 설정에 등록된 종 중 id 가 같거나(우선) 이름 일부가 맞는 것 */
	UGrimCreatureSpecies* FindSpeciesLoose(const FString& Query, FString& OutAllIds)
	{
		UGrimCreatureSpecies* Partial = nullptr;
		for (const TSoftObjectPtr<UGrimCreatureSpecies>& Soft : UGrimCreatureSettings::Get()->Species)
		{
			UGrimCreatureSpecies* Species = Soft.LoadSynchronous();
			if (!Species)
			{
				continue;
			}
			const FString Id = Species->SpeciesId.ToString();
			OutAllIds += OutAllIds.IsEmpty() ? Id : TEXT(" · ") + Id;
			if (Id.Equals(Query, ESearchCase::IgnoreCase))
			{
				return Species;
			}
			if (!Partial && !Query.IsEmpty() && Id.Contains(Query, ESearchCase::IgnoreCase))
			{
				Partial = Species;
			}
		}
		return Partial;
	}

	/** Grim.Spawn <종 이름 일부> [수=1] [거리m=6] — 플레이 테스트용: 화면에 결과를 띄우고, 이름 일부만 쳐도 된다 */
	void FriendlySpawnCommand(const TArray<FString>& Args)
	{
		FString AllIds;
		UGrimCreatureSpecies* Species = FindSpeciesLoose(Args.Num() > 0 ? Args[0] : FString(), AllIds);
		if (!Species)
		{
			Say(FString::Printf(TEXT("사용: Grim.Spawn <종> [수] [거리m] — 종: %s"), *AllIds), FColor::Yellow);
			return;
		}
		const int32 Count = FMath::Clamp(static_cast<int32>(ArgFloat(Args, 1, 1.f)), 1, 20);
		const float Distance = FMath::Max(1.f, ArgFloat(Args, 2, 6.f)) * 100.f;
		const FString SpeciesId = Species->SpeciesId.ToString();
		SpawnCommand({ SpeciesId, FString::FromInt(Count), TEXT("0"), FString::SanitizeFloat(Distance) });
		Say(FString::Printf(TEXT("%s × %d 를 앞 %.0fm 에 놓았다"), *SpeciesId, Count, Distance / 100.f));
	}

	/** Grim.Creature.Mark 표시 — 나온 생물은 굵은 기둥 · 이름 · 거리, 아직 접힌 무리(서버만 안다)는 노란 가는 기둥.
	 *  화면 위에는 가장 가까운 것의 이름 · 거리 · 방향. */
	FTSTicker::FDelegateHandle GMarkTicker;

	/** 보는 방향 기준 방향 말 — 앞 · 오른쪽 앞 · 오른쪽 … */
	const TCHAR* BearingWord(const FVector& From, const FVector& To, float ViewYaw)
	{
		static const TCHAR* Words[] = { TEXT("앞"), TEXT("오른쪽 앞"), TEXT("오른쪽"), TEXT("오른쪽 뒤"), TEXT("뒤"), TEXT("왼쪽 뒤"), TEXT("왼쪽"), TEXT("왼쪽 앞") };
		const float Relative = FRotator::NormalizeAxis((To - From).Rotation().Yaw - ViewYaw);
		const int32 Sector = FMath::RoundToInt((Relative < 0.f ? Relative + 360.f : Relative) / 45.f) % 8;
		return Words[Sector];
	}

	bool DrawCreatureMarks(float)
	{
		int32 WorldIndex = 0;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			++WorldIndex;
			UWorld* World = Context.World();
			APlayerController* Viewer = World && World->IsGameWorld() ? World->GetFirstPlayerController() : nullptr;
			const APawn* ViewerPawn = Viewer ? Viewer->GetPawn() : nullptr;
			if (!ViewerPawn || !Viewer->IsLocalController())
			{
				continue;
			}
			const FVector From = ViewerPawn->GetActorLocation();
			FString NearestName;
			FVector NearestLocation = FVector::ZeroVector;
			double NearestDistance = TNumericLimits<double>::Max();
			auto Consider = [&](const FString& Name, const FVector& Location, double Distance)
			{
				if (Distance < NearestDistance)
				{
					NearestDistance = Distance;
					NearestName = Name;
					NearestLocation = Location;
				}
			};
			for (TActorIterator<AGrimCreature> It(World); It; ++It)
			{
				if (It->IsDead())
				{
					continue;
				}
				const FVector Location = It->GetActorLocation();
				const double Distance = FVector::Dist(From, Location);
				const FString Name = It->GetInstance().SpeciesId.ToString();
				const FColor Color = It->GetSpecies() && It->GetSpecies()->Temperament == EGrimCreatureTemperament::Aggressive ? FColor::Red : FColor::Green;
				DrawDebugLine(World, Location, Location + FVector(0.f, 0.f, 2000.f), Color, false, 0.3f, 0, 12.f);
				DrawDebugString(World, Location + FVector(0.f, 0.f, 250.f), FString::Printf(TEXT("%s %.0fm"), *Name, Distance / 100.0), nullptr, Color, 0.3f, true, 1.3f);
				Consider(Name, Location, Distance);
			}
			// 접힌 무리 — 기록은 서버에만 있다(단독 · 리슨 호스트).
			if (const UGrimEcosystemSubsystem* Ecosystem = World->GetNetMode() != NM_Client ? World->GetSubsystem<UGrimEcosystemSubsystem>() : nullptr)
			{
				for (const FGrimHabitatRuntime& Runtime : Ecosystem->GetHabitats())
				{
					for (const FGrimHabitatPopulation& Population : Runtime.Populations)
					{
						TSet<int32> SeenHerds;
						for (const FGrimFoldedCreature& Record : Population.Folded)
						{
							if (!Population.Species || SeenHerds.Contains(Record.HerdId))
							{
								continue;
							}
							SeenHerds.Add(Record.HerdId);
							const double Distance = FVector::Dist(From, Record.Location);
							const FString Name = FString::Printf(TEXT("%s 무리(아직 안 나옴)"), *Population.Species->SpeciesId.ToString());
							DrawDebugLine(World, Record.Location, Record.Location + FVector(0.f, 0.f, 6000.f), FColor::Yellow, false, 0.3f, 0, 4.f);
							DrawDebugString(World, Record.Location + FVector(0.f, 0.f, 600.f), FString::Printf(TEXT("%s %.0fm"), *Name, Distance / 100.0), nullptr, FColor::Yellow, 0.3f, true, 1.3f);
							Consider(Name, Record.Location, Distance);
						}
					}
				}
			}
			const FString Text = NearestName.IsEmpty()
				? FString(TEXT("생물 없음 — Grim.Spawn <종> 으로 불러낸다"))
				: FString::Printf(TEXT("가장 가까운 생물: %s %.0fm %s"), *NearestName, NearestDistance / 100.0, BearingWord(From, NearestLocation, Viewer->GetControlRotation().Yaw));
			GEngine->AddOnScreenDebugMessage(static_cast<uint64>(0x6772696D00) + WorldIndex, 0.3f, FColor::Green, Text); // 같은 줄을 덮어 쓴다(월드마다 한 줄)
		}
		return true;
	}

	/** Grim.Creature.Mark [1/0] — 생물 위치 표시 켜기 · 끄기(인자 없으면 바꾸기) */
	void MarkCommand(const TArray<FString>& Args)
	{
		const bool bOn = Args.Num() > 0 ? FCString::Atoi(*Args[0]) != 0 : !GMarkTicker.IsValid();
		if (GMarkTicker.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(GMarkTicker);
			GMarkTicker.Reset();
		}
		if (bOn)
		{
			GMarkTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&DrawCreatureMarks), 0.25f);
		}
		Say(bOn ? TEXT("생물 표시 켬 — 초록 순함 · 빨강 사나움 · 노랑 아직 안 나온 무리(60m 안에 가면 나온다)") : TEXT("생물 표시 끔"));
	}

	FAutoConsoleCommand GFriendlySpawnCommand(TEXT("Grim.Spawn"), TEXT("생물 소환 치트: <종 이름 일부> [수] [거리m]. 예: Grim.Spawn Jack 3 / Grim.Spawn BlackDog"), FConsoleCommandWithArgsDelegate::CreateStatic(&FriendlySpawnCommand));
	FAutoConsoleCommand GMarkCommand(TEXT("Grim.Creature.Mark"), TEXT("생물 위치 표시(기둥 · 이름 · 거리). [1/0]"), FConsoleCommandWithArgsDelegate::CreateStatic(&MarkCommand));
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
