// 초원 꾸밈 디버그 콘솔(개발 빌드) — docs/dev/cheats.md 「초원 꾸밈」. 지연은 Grim.At 으로(예: Grim.At 25 Grim.Meadow Dump).
//   Grim.Meadow Dump                      이 프로세스의 소품 · 셀 상태(서버 · 클라이언트 각자 — 복제 확인)
//   Grim.Meadow Spawn <PropId> [앞 cm] [Host|Client|All]    서버: 플레이어 앞에 소품(기본 Host = 서버의 로컬 플레이어)
//   Grim.Meadow SpawnCell <Kind> [앞 cm] [Host|Client|All] 서버: 플레이어마다 앞에 채집점 하나씩 든 셀 하나
//   Grim.Meadow Hit <Element> <세기> [반경 cm] [PropId]   서버: 플레이어 둘레 소품에 속성 자극(기본 반경 1500)
//   Grim.Meadow Dawn                      서버: 해 뜸 처리(되돌리기 · 다시 나기)
//   Grim.Meadow Interact                  F 와 같다(가장 가까운 소품 · 채집점 — 클라이언트면 서버 RPC)
//   Grim.Meadow Go <PropId|Cell> [번째] [거리 cm]   서버: 그 소품 · 셀 첫 점 앞으로 순간 이동(스크린숏)
//   Grim.Meadow GoNear <X> <Y> [거리 cm]             서버: 월드 (X, Y) 의 남쪽(−X) 으로 거리만큼 떨어진 땅 위로 순간 이동, 북쪽을 본다
//   Grim.Meadow Reload                    표 셋 다시 읽기
//   Grim.Journal                          내 도감 목록
#include "Dressing/GrimDressingTypes.h"
#include "Dressing/GrimHarvestCell.h"
#include "Dressing/GrimJournalComponent.h"
#include "Dressing/GrimReactiveProp.h"
#include "Core/GrimElementalTypes.h"
#include "Core/GrimInteractable.h"
#include "Interaction/GrimElementalStateComponent.h"
#include "Items/GrimInventoryComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "HAL/IConsoleManager.h"

#if !UE_BUILD_SHIPPING

namespace GrimDressingDebugLocal
{
	const TCHAR* NetModeName(const UWorld* World)
	{
		switch (World->GetNetMode())
		{
		case NM_Client: return TEXT("Client");
		case NM_ListenServer: return TEXT("ListenServer");
		case NM_DedicatedServer: return TEXT("DedicatedServer");
		default: return TEXT("Standalone");
		}
	}

	void ForEachGameWorld(TFunctionRef<void(UWorld*)> Fn)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			UWorld* World = Context.World();
			if (World && World->IsGameWorld())
			{
				Fn(World);
			}
		}
	}

	APawn* LocalPawn(UWorld* World)
	{
		const APlayerController* PC = GEngine->GetFirstLocalPlayerController(World);
		return PC ? PC->GetPawn() : nullptr;
	}

	/** 서버: Host(로컬) · Client(원격) · All 플레이어 폰 */
	TArray<APawn*> PickPawns(UWorld* World, const FString& Who)
	{
		TArray<APawn*> Out;
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* PC = It->Get();
			if (!PC || !PC->GetPawn())
			{
				continue;
			}
			const bool bLocal = PC->IsLocalController();
			if (Who.Equals(TEXT("All"), ESearchCase::IgnoreCase) || (Who.Equals(TEXT("Client"), ESearchCase::IgnoreCase) ? !bLocal : bLocal))
			{
				Out.Add(PC->GetPawn());
			}
		}
		return Out;
	}

	FTransform InFrontOf(UWorld* World, const APawn* Pawn, float Ahead)
	{
		const FVector Forward = Pawn->GetActorForwardVector().GetSafeNormal2D();
		FVector Location = Pawn->GetActorLocation() + Forward * Ahead;
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GrimMeadowDebug), false, Pawn);
		if (World->LineTraceSingleByChannel(Hit, Location + FVector(0, 0, 500), Location - FVector(0, 0, 2000), ECC_Visibility, Params))
		{
			Location = Hit.ImpactPoint;
		}
		return FTransform(FRotator(0, Pawn->GetActorRotation().Yaw + 180.f, 0), Location);
	}

	FString DescribeStates(const UGrimElementalStateComponent* State)
	{
		TArray<FString> Parts;
		for (const FGrimElementalState& Item : State->GetStates().Items)
		{
			if (Item.Intensity > 0.f)
			{
				Parts.Add(FString::Printf(TEXT("%s=%.1f"), *Item.StateTag.ToString(), Item.Intensity));
			}
		}
		return Parts.Num() ? FString::Join(Parts, TEXT(",")) : TEXT("-");
	}

	void Dump(UWorld* World)
	{
		TMap<FName, int32> Counts;
		int32 Props = 0;
		for (TActorIterator<AGrimReactiveProp> It(World); It; ++It)
		{
			++Props;
			Counts.FindOrAdd(It->GetPropId())++;
			// 처음 상태가 아닌 것 · 주운 사람이 있는 것만 한 줄씩(수백 개라)
			if (!It->IsAtInitialState() || It->GetCollectedBy().Num() > 0 || It->GetRewardsSpawned() > 0)
			{
				UE_LOG(LogGrimDressing, Display, TEXT("[%s] MeadowDump prop %s %s states=%s alt=%d collected=%d mine=%d rewards=%d at=%s"), NetModeName(World),
					*It->GetName(), *It->GetPropId().ToString(), *DescribeStates(It->GetElementalState()), It->IsShowingAlt() ? 1 : 0,
					It->GetCollectedBy().Num(), It->IsCollectedByLocalPlayer() ? 1 : 0, It->GetRewardsSpawned(), *It->GetActorLocation().ToCompactString());
			}
		}
		TArray<FString> CountText;
		for (const TPair<FName, int32>& Pair : Counts)
		{
			CountText.Add(FString::Printf(TEXT("%s=%d"), *Pair.Key.ToString(), Pair.Value));
		}
		CountText.Sort();
		int32 Cells = 0, Points = 0, Harvested = 0;
		for (TActorIterator<AGrimHarvestCell> It(World); It; ++It)
		{
			++Cells;
			Points += It->GetPoints().Num();
			Harvested += It->GetHarvestedEntryCount();
			if (It->GetHarvestedEntryCount() > 0)
			{
				int32 Hidden = 0;
				for (int32 i = 0; i < It->GetPoints().Num(); ++i)
				{
					Hidden += (World->GetNetMode() != NM_DedicatedServer && !It->IsPointVisible(i)) ? 1 : 0;
				}
				UE_LOG(LogGrimDressing, Display, TEXT("[%s] MeadowDump cell %s points=%d harvested=%d hidden=%d"), NetModeName(World), *It->GetName(), It->GetPoints().Num(), It->GetHarvestedEntryCount(), Hidden);
			}
		}
		UE_LOG(LogGrimDressing, Display, TEXT("[%s] MeadowDump 합계 소품 %d (%s) · 셀 %d 점 %d 거둔 기록 %d"), NetModeName(World), Props, *FString::Join(CountText, TEXT(" ")), Cells, Points, Harvested);
		if (const APawn* Pawn = LocalPawn(World))
		{
			if (const UGrimJournalComponent* Journal = UGrimJournalComponent::Find(Pawn))
			{
				UE_LOG(LogGrimDressing, Display, TEXT("[%s] MeadowDump 도감 %d 줄"), NetModeName(World), Journal->GetEntries().Num());
			}
		}
	}

	void MeadowCommand(const TArray<FString>& Args)
	{
		const FString Verb = Args.IsValidIndex(0) ? Args[0] : TEXT("Dump");
		if (Verb.Equals(TEXT("Reload"), ESearchCase::IgnoreCase))
		{
			FGrimDressingData::Reset();
			ForEachGameWorld([](UWorld* World)
			{
				for (TActorIterator<AGrimReactiveProp> It(World); It; ++It)
				{
					It->SetPropId(It->GetPropId());
				}
			});
			UE_LOG(LogGrimDressing, Display, TEXT("Grim.Meadow Reload: 표를 다시 읽었다"));
			return;
		}
		ForEachGameWorld([&](UWorld* World)
		{
			APawn* Pawn = LocalPawn(World);
			const bool bServer = World->GetNetMode() != NM_Client;
			if (Verb.Equals(TEXT("Dump"), ESearchCase::IgnoreCase))
			{
				Dump(World);
			}
			else if (Verb.Equals(TEXT("Spawn"), ESearchCase::IgnoreCase) && bServer && Args.IsValidIndex(1))
			{
				const float Ahead = Args.IsValidIndex(2) ? FCString::Atof(*Args[2]) : 300.f;
				for (APawn* Who : PickPawns(World, Args.IsValidIndex(3) ? Args[3] : TEXT("Host")))
				{
					const FTransform Transform = InFrontOf(World, Who, Ahead);
					AGrimReactiveProp* Prop = World->SpawnActorDeferred<AGrimReactiveProp>(AGrimReactiveProp::StaticClass(), Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
					Prop->SetPropId(FName(*Args[1]));
					Prop->FinishSpawning(Transform);
					UE_LOG(LogGrimDressing, Display, TEXT("[%s] Grim.Meadow Spawn %s → %s (%s 앞)"), NetModeName(World), *Args[1], *Prop->GetName(), *GetNameSafe(Who));
				}
			}
			else if (Verb.Equals(TEXT("SpawnCell"), ESearchCase::IgnoreCase) && bServer && Args.IsValidIndex(1))
			{
				const float Ahead = Args.IsValidIndex(2) ? FCString::Atof(*Args[2]) : 150.f;
				AGrimHarvestCell* Cell = World->SpawnActorDeferred<AGrimHarvestCell>(AGrimHarvestCell::StaticClass(), FTransform::Identity);
				for (APawn* Who : PickPawns(World, Args.IsValidIndex(3) ? Args[3] : TEXT("Host")))
				{
					Cell->AddPoint(FName(*Args[1]), InFrontOf(World, Who, Ahead));
				}
				Cell->FinishSpawning(FTransform(Cell->GetPoints().Num() ? Cell->GetPoints()[0].Transform.GetLocation() : FVector::ZeroVector));
				UE_LOG(LogGrimDressing, Display, TEXT("[%s] Grim.Meadow SpawnCell %s → %s 점 %d"), NetModeName(World), *Args[1], *Cell->GetName(), Cell->GetPoints().Num());
			}
			else if (Verb.Equals(TEXT("Hit"), ESearchCase::IgnoreCase) && bServer && Args.IsValidIndex(2))
			{
				FGrimElementalStimulus Stimulus;
				Stimulus.Element = FGameplayTag::RequestGameplayTag(FName(*Args[1]), false);
				Stimulus.Intensity = FCString::Atof(*Args[2]);
				Stimulus.Source = EGrimStimulusSource::Spell;
				const float Radius = Args.IsValidIndex(3) ? FCString::Atof(*Args[3]) : 1500.f;
				const FName Filter = Args.IsValidIndex(4) ? FName(*Args[4]) : NAME_None;
				const FVector Center = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
				int32 Count = 0;
				for (TActorIterator<AGrimReactiveProp> It(World); It; ++It)
				{
					if ((Radius <= 0.f || FVector::Dist(It->GetActorLocation(), Center) <= Radius) && (Filter.IsNone() || It->GetPropId() == Filter))
					{
						Count += UGrimElementalStatics::ApplyStimulusToActor(*It, Stimulus, Pawn) ? 1 : 0;
					}
				}
				UE_LOG(LogGrimDressing, Display, TEXT("[%s] Grim.Meadow Hit %s %.2f → %d개"), NetModeName(World), *Stimulus.Element.ToString(), Stimulus.Intensity, Count);
			}
			else if (Verb.Equals(TEXT("Dawn"), ESearchCase::IgnoreCase) && bServer)
			{
				for (TActorIterator<AGrimReactiveProp> It(World); It; ++It)
				{
					if (const FGrimReactivePropRow* Row = It->GetRow(); Row && Row->ResetRule == EGrimPropReset::Dawn)
					{
						It->HandleDawn();
					}
				}
				for (TActorIterator<AGrimHarvestCell> It(World); It; ++It)
				{
					It->HandleDawn();
				}
				UE_LOG(LogGrimDressing, Display, TEXT("[%s] Grim.Meadow Dawn"), NetModeName(World));
			}
			else if (Verb.Equals(TEXT("Interact"), ESearchCase::IgnoreCase) && Pawn)
			{
				FGrimInteractionOffer Offer;
				const UGrimInteractableSubsystem* Interactables = World->GetSubsystem<UGrimInteractableSubsystem>();
				AActor* Target = Interactables ? Cast<AActor>(Interactables->FindBest(Pawn, Offer)) : nullptr;
				UGrimInventoryComponent* Inventory = Pawn->GetPlayerState() ? Pawn->GetPlayerState()->FindComponentByClass<UGrimInventoryComponent>() : nullptr;
				UE_LOG(LogGrimDressing, Display, TEXT("[%s] Grim.Meadow Interact → %s(%d) %s"), NetModeName(World), *GetNameSafe(Target), Offer.Token, *Offer.Prompt.ToString());
				if (Target && Inventory)
				{
					Inventory->Server_Interact(Target, Offer.Token);
				}
			}
			else if (Verb.Equals(TEXT("GoNear"), ESearchCase::IgnoreCase) && bServer && Pawn && Args.IsValidIndex(2))
			{
				const FVector Target(FCString::Atof(*Args[1]), FCString::Atof(*Args[2]), 0.f);
				const float Back = Args.IsValidIndex(3) ? FCString::Atof(*Args[3]) : 150.f;
				FVector From = Target - FVector(Back, 0.f, 0.f);
				FHitResult Hit;
				FCollisionQueryParams Params(SCENE_QUERY_STAT(GrimMeadowGoNear), false, Pawn);
				if (World->LineTraceSingleByChannel(Hit, From + FVector(0, 0, 100000), From - FVector(0, 0, 100000), ECC_Visibility, Params))
				{
					From.Z = Hit.ImpactPoint.Z + 100.f;
				}
				Pawn->TeleportTo(From, FRotator::ZeroRotator);
				if (AController* Controller = Pawn->GetController())
				{
					Controller->SetControlRotation(FRotator(-10.f, 0.f, 0.f));
				}
				UE_LOG(LogGrimDressing, Display, TEXT("[%s] Grim.Meadow GoNear → %s"), NetModeName(World), *From.ToCompactString());
			}
			else if (Verb.Equals(TEXT("Go"), ESearchCase::IgnoreCase) && bServer && Pawn && Args.IsValidIndex(1))
			{
				const int32 Nth = Args.IsValidIndex(2) ? FCString::Atoi(*Args[2]) : 0;
				const float Back = Args.IsValidIndex(3) ? FCString::Atof(*Args[3]) : 500.f;
				TOptional<FVector> Target;
				int32 Seen = 0;
				if (Args[1].Equals(TEXT("Cell"), ESearchCase::IgnoreCase))
				{
					for (TActorIterator<AGrimHarvestCell> It(World); It && !Target; ++It)
					{
						if (It->GetPoints().Num() > 0 && Seen++ == Nth)
						{
							Target = It->GetPoints()[0].Transform.GetLocation();
						}
					}
				}
				else
				{
					for (TActorIterator<AGrimReactiveProp> It(World); It && !Target; ++It)
					{
						if (It->GetPropId() == FName(*Args[1]) && Seen++ == Nth)
						{
							Target = It->GetActorLocation();
						}
					}
				}
				if (Target)
				{
					const FVector From = *Target + FVector(-Back, 0.f, 120.f);
					Pawn->TeleportTo(From, FRotator::ZeroRotator);
					if (AController* Controller = Pawn->GetController())
					{
						Controller->SetControlRotation((*Target - From).Rotation());
					}
					UE_LOG(LogGrimDressing, Display, TEXT("[%s] Grim.Meadow Go %s %d → %s"), NetModeName(World), *Args[1], Nth, *Target->ToCompactString());
				}
				else
				{
					UE_LOG(LogGrimDressing, Warning, TEXT("[%s] Grim.Meadow Go: %s %d 번째가 없다"), NetModeName(World), *Args[1], Nth);
				}
			}
		});
	}

	FAutoConsoleCommand MeadowCmd(TEXT("Grim.Meadow"), TEXT("초원 꾸밈: Dump · Spawn <PropId> · SpawnCell <Kind> · Hit <Element> <세기> [반경] [PropId] · Dawn · Interact · Go <PropId|Cell> [번째] [거리] · GoNear <X> <Y> [거리] · Reload"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&MeadowCommand));

	FAutoConsoleCommand JournalCmd(TEXT("Grim.Journal"), TEXT("내 도감(임시 목록)"), FConsoleCommandDelegate::CreateLambda([]()
	{
		ForEachGameWorld([](UWorld* World)
		{
			const UGrimJournalComponent* Journal = UGrimJournalComponent::Find(LocalPawn(World));
			if (!Journal)
			{
				return;
			}
			UE_LOG(LogGrimDressing, Display, TEXT("[%s] 도감 %d 줄"), NetModeName(World), Journal->GetEntries().Num());
			for (const FName& Id : Journal->GetEntries())
			{
				const FGrimJournalRow* Row = FGrimDressingData::FindJournal(Id);
				UE_LOG(LogGrimDressing, Display, TEXT("  %s — %s: %s"), *Id.ToString(), Row ? *Row->Title : TEXT("?"), Row ? *Row->Text : TEXT(""));
			}
		});
	}));
}

#endif
