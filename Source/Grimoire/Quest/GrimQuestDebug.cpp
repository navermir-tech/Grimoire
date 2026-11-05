// 퀘스트 디버그 콘솔 명령(개발 빌드) — 헤드리스로 튜토리얼을 끝까지 밟아 보는 데 쓴다(Scripts/Tutorial/check_tutorial.ps1).
// 지연(초)을 받는 건 -ExecCmds 가 맵 · 접속보다 먼저 실행되기 때문이다. 상태를 바꾸는 명령은 서버(권한) 월드에서만 돈다.

#include "Quest/GrimQuestComponent.h"
#include "Quest/GrimQuestSettings.h"
#include "Quest/GrimQuestSubsystem.h"
#include "Quest/GrimQuestTrackerWidget.h"
#include "Factions/GrimFactionTypes.h"
#include "Factions/GrimTowerNPC.h"
#include "Core/GrimElementalTypes.h"
#include "Core/GrimPlayerState.h"
#include "Environment/GrimTimeOfDaySubsystem.h"
#include "Magic/GrimProgressionComponent.h"
#include "Magic/GrimSpellbookComponent.h"
#include "Magic/GrimSpellDefinition.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagsManager.h"
#include "HAL/IConsoleManager.h"

#if !UE_BUILD_SHIPPING

namespace GrimQuestDebugLocal // 유니티 빌드에서 다른 디버그 파일의 같은 이름 도우미와 부딪히지 않게
{
	const TCHAR* NetModeName(const UWorld* World)
	{
		return World->GetNetMode() == NM_Client ? TEXT("Client") : TEXT("Server");
	}

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

	void RunLater(float Delay, TFunction<void()> Fn)
	{
		if (Delay <= 0.f)
		{
			Fn();
			return;
		}
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Fn](float)
		{
			Fn();
			return false;
		}), Delay);
	}

	UGrimQuestComponent* FindLocalQuest(UWorld* World)
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if (const APlayerController* PlayerController = It->Get(); PlayerController && PlayerController->IsLocalController())
			{
				return UGrimQuestComponent::FindForController(PlayerController);
			}
		}
		return nullptr;
	}

	/** 태그로 찾고, 없으면 NPC ID 로 */
	AActor* FindTarget(UWorld* World, FName TagOrNpc)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->ActorHasTag(TagOrNpc) && !It->IsActorBeingDestroyed())
			{
				return *It;
			}
		}
		return AGrimTowerNPC::FindById(World, TagOrNpc);
	}

	void Dump(UWorld* World)
	{
		const AGameStateBase* GameState = World->GetGameState();
		if (!GameState)
		{
			return;
		}
		const UGrimTimeOfDaySubsystem* TimeOfDay = World->GetSubsystem<UGrimTimeOfDaySubsystem>();
		const FGrimQuestDatabase& Database = UGrimQuestSettings::GetQuestDatabase();
		for (APlayerState* PlayerState : GameState->PlayerArray)
		{
			const UGrimQuestComponent* Quest = UGrimQuestComponent::FindForPlayerState(PlayerState);
			const AGrimPlayerState* GrimPlayerState = Cast<AGrimPlayerState>(PlayerState);
			if (!Quest || !GrimPlayerState)
			{
				continue;
			}
			FString Spells;
			if (const UGrimSpellbookComponent* Spellbook = GrimPlayerState->GetSpellbook())
			{
				for (const UGrimSpellDefinition* Spell : Spellbook->GetKnownSpells())
				{
					Spells += GetNameSafe(Spell).Replace(*UGrimQuestSettings::Get()->SpellAssetPrefix, TEXT("")) + TEXT(" ");
				}
			}
			const UGrimProgressionComponent* Progression = GrimPlayerState->GetProgression();
			FString Completed;
			for (const FName Id : Quest->GetLog().Completed)
			{
				Completed += Id.ToString() + TEXT(" ");
			}
			UE_LOG(LogGrimQuest, Display, TEXT("[QuestDump %s] %s circle=%d time=%.3f day=%d spells={%s} completed={%s} active=%d"),
				NetModeName(World), *PlayerState->GetPlayerName(), Progression ? Progression->GetCircle() : -1,
				TimeOfDay ? TimeOfDay->GetTimeOfDay() : -1.f, TimeOfDay ? TimeOfDay->GetDayIndex() : -1,
				*Spells.TrimEnd(), *Completed.TrimEnd(), Quest->GetLog().Active.Num());
			for (const FGrimQuestProgress& Progress : Quest->GetLog().Active)
			{
				FString Title, Body;
				UGrimQuestTrackerWidget::DescribeProgress(Database, Progress, Quest->GetServerTime(), Title, Body);
				FString Counts;
				for (const int32 Count : Progress.Counts)
				{
					Counts += FString::FromInt(Count) + TEXT(" ");
				}
				UE_LOG(LogGrimQuest, Display, TEXT("[QuestDump %s]   %s step=%d counts={%s} %s"), NetModeName(World),
					*Progress.QuestId.ToString(), Progress.Step, *Counts.TrimEnd(), *Body.Replace(TEXT("\n"), TEXT(" / ")));
			}
		}
	}

	/** 지금 바로 한 명령(서브 명령 + 인자) */
	void RunSub(const FString& Sub, const TArray<FString>& Args)
	{
		auto Arg = [&Args](int32 Index) { return Args.IsValidIndex(Index) ? Args[Index] : FString(); };

		ForEachGameWorld([&](UWorld* World)
		{
			const bool bServer = World->GetNetMode() != NM_Client;

			if (Sub.Equals(TEXT("Dump"), ESearchCase::IgnoreCase))
			{
				Dump(World);
			}
			else if (Sub.Equals(TEXT("Talk"), ESearchCase::IgnoreCase) || Sub.Equals(TEXT("Service"), ESearchCase::IgnoreCase))
			{
				// 로컬 플레이어가 NPC 에게(대화를 마침 · 서비스). 거리 검사는 서버가 한다 — 먼저 Teleport.
				UGrimQuestComponent* Quest = FindLocalQuest(World);
				AGrimTowerNPC* Npc = AGrimTowerNPC::FindById(World, FName(*Arg(0)));
				const FName Request = Sub.Equals(TEXT("Talk"), ESearchCase::IgnoreCase) ? GrimDialogueContexts::Talk : FName(*Arg(1));
				if (!Quest || !Npc)
				{
					UE_LOG(LogGrimQuest, Warning, TEXT("[%s] %s: 퀘스트 · NPC 없음 (%s)"), NetModeName(World), *Sub, *Arg(0));
					return;
				}
				UE_LOG(LogGrimQuest, Display, TEXT("[%s] 콘솔: %s 에게 %s"), NetModeName(World), *Npc->GetNpcId().ToString(), *Request.ToString());
				Quest->RequestNpc(Npc, Request);
			}
			else if (!bServer)
			{
				return;
			}
			else if (Sub.Equals(TEXT("Teleport"), ESearchCase::IgnoreCase))
			{
				AActor* Target = FindTarget(World, FName(*Arg(0)));
				const float Distance = Args.IsValidIndex(1) ? FCString::Atof(*Args[1]) : 120.f;
				if (!Target)
				{
					UE_LOG(LogGrimQuest, Warning, TEXT("[Server] Teleport: 대상 없음 %s"), *Arg(0));
					return;
				}
				int32 Index = 0;
				for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It, ++Index)
				{
					APlayerController* PlayerController = It->Get();
					APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
					if (!Pawn)
					{
						continue;
					}
					FVector Location = Target->GetActorLocation() + Target->GetActorForwardVector() * Distance + FVector(0.f, 80.f * Index, 0.f);
					Location.Z = Pawn->GetActorLocation().Z;
					Pawn->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
					PlayerController->ClientSetLocation(Location, (Target->GetActorLocation() - Location).Rotation());
					UE_LOG(LogGrimQuest, Display, TEXT("[Server] Teleport %s → %s 곁 %s"), *Pawn->GetName(), *Target->GetName(), *Location.ToCompactString());
				}
			}
			else if (Sub.Equals(TEXT("Stimulus"), ESearchCase::IgnoreCase))
			{
				// 태그의 사물에 마법 자극(쏜 쪽 = 첫 플레이어 폰)
				FGrimElementalStimulus Stimulus;
				Stimulus.Element = UGameplayTagsManager::Get().RequestGameplayTag(FName(*Arg(1)), false);
				Stimulus.Intensity = Args.IsValidIndex(2) ? FCString::Atof(*Args[2]) : 1.f;
				Stimulus.Source = EGrimStimulusSource::Spell;
				const APlayerController* First = World->GetFirstPlayerController();
				AActor* Instigator = First ? First->GetPawn() : nullptr;
				int32 Count = 0;
				TArray<AActor*> Targets;
				for (TActorIterator<AActor> It(World); It; ++It)
				{
					if (It->ActorHasTag(FName(*Arg(0))) && !It->IsActorBeingDestroyed())
					{
						Targets.Add(*It);
					}
				}
				for (AActor* Target : Targets)
				{
					Count += UGrimElementalStatics::ApplyStimulusToActor(Target, Stimulus, Instigator) ? 1 : 0;
				}
				UE_LOG(LogGrimQuest, Display, TEXT("[Server] Stimulus %s %s %.2f → %d 개"), *Arg(0), *Stimulus.Element.ToString(), Stimulus.Intensity, Count);
			}
			else
			{
				const AGameStateBase* GameState = World->GetGameState();
				if (!GameState)
				{
					return;
				}
				for (APlayerState* PlayerState : GameState->PlayerArray)
				{
					UGrimQuestComponent* Quest = UGrimQuestComponent::FindForPlayerState(PlayerState);
					if (!Quest)
					{
						continue;
					}
					if (Sub.Equals(TEXT("Start"), ESearchCase::IgnoreCase))
					{
						Quest->StartQuest(FName(*Arg(0)));
					}
					else if (Sub.Equals(TEXT("Skip"), ESearchCase::IgnoreCase))
					{
						Quest->DebugCompleteStep(Args.IsValidIndex(0) ? FName(*Arg(0)) : NAME_None);
					}
					else if (Sub.Equals(TEXT("Fail"), ESearchCase::IgnoreCase))
					{
						Quest->FailQuest(FName(*Arg(0)));
					}
					else if (Sub.Equals(TEXT("Event"), ESearchCase::IgnoreCase))
					{
						// 아직 없는 시스템(생물 · 요리 · 거점 …)이 보낼 이벤트를 대신 보낸다
						UE_LOG(LogGrimQuest, Display, TEXT("[Server] 콘솔: 이벤트 %s"), *Arg(0));
						Quest->NotifyEvent(FName(*Arg(0)), Args.IsValidIndex(1) ? FMath::Max(1, FCString::Atoi(*Args[1])) : 1);
					}
					else
					{
						UE_LOG(LogGrimQuest, Warning, TEXT("모르는 퀘스트 명령 %s"), *Sub);
						return;
					}
				}
			}
		});
	}

	/** Grim.Quest.At <초> <명령> [인자…] */
	void AtCommand(const TArray<FString>& Args)
	{
		if (Args.Num() < 2)
		{
			UE_LOG(LogGrimQuest, Warning, TEXT("사용: Grim.Quest.At <초> <Dump|Talk|Service|Teleport|Stimulus|Start|Skip|Fail|Event> [인자…]"));
			return;
		}
		const float Delay = FCString::Atof(*Args[0]);
		const FString Sub = Args[1];
		TArray<FString> Rest(Args.GetData() + 2, Args.Num() - 2);
		RunLater(Delay, [Sub, Rest]() { RunSub(Sub, Rest); });
	}

	/** Grim.Quest.Run <명령> [인자…] — 바로 */
	void RunCommand(const TArray<FString>& Args)
	{
		if (Args.Num() < 1)
		{
			return;
		}
		TArray<FString> Rest(Args.GetData() + 1, Args.Num() - 1);
		RunSub(Args[0], Rest);
	}

	FAutoConsoleCommand GAtCommand(
		TEXT("Grim.Quest.At"),
		TEXT("몇 초 뒤 퀘스트 명령. <초> <Dump | Talk <NPC> | Service <NPC> <서비스> | Teleport <태그|NPC> [거리] | Stimulus <태그> <Element.X> [세기] | Start <퀘스트> | Skip [퀘스트] | Fail <퀘스트> | Event <이벤트 ID> [수]>"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&AtCommand));

	FAutoConsoleCommand GRunCommand(
		TEXT("Grim.Quest.Run"),
		TEXT("퀘스트 명령을 바로. <명령> [인자…] (Grim.Quest.At 과 같은 명령)"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&RunCommand));

	FAutoConsoleCommand GDumpCommand(
		TEXT("Grim.Quest.Dump"),
		TEXT("이 프로세스에서 본 각 플레이어의 퀘스트 진행을 로그로"),
		FConsoleCommandDelegate::CreateLambda([]() { RunSub(TEXT("Dump"), {}); }));
}

#endif
