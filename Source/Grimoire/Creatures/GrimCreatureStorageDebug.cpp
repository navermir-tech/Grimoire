// 데리고 다니기 · 보관함 · 거점 생물 상자 콘솔 명령(R38) — 헤드리스 리슨 서버 + 클라이언트 확인(Scripts/Creatures/net_check_storage.ps1)과 플레이 테스트에 쓴다.
// 길들이기 명령(Grim.Taming.*)과 같은 방식: 지연(초)을 받는다(-ExecCmds 가 맵 · 접속보다 먼저 실행되기 때문).

#include "Creatures/GrimCreature.h"
#include "Creatures/GrimCreatureBox.h"
#include "Creatures/GrimCreatureStorageComponent.h"
#include "Creatures/GrimCreatureStorageMath.h"
#include "Creatures/GrimCreatureStorageSettings.h"
#include "Creatures/GrimCreatureStorageWidget.h"
#include "Creatures/GrimCreatureTamingComponent.h"
#include "Creatures/GrimEcosystemSubsystem.h"
#include "Creatures/GrimTamingTypes.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "HAL/IConsoleManager.h"

#if !UE_BUILD_SHIPPING

namespace GrimCreatureStorageDebugLocal // 유니티 빌드 이름 충돌 방지
{
	const TCHAR* SideName(const UWorld* World)
	{
		switch (World->GetNetMode())
		{
		case NM_Client: return TEXT("Client");
		case NM_ListenServer:
		case NM_DedicatedServer: return TEXT("Server");
		default: return TEXT("Standalone");
		}
	}

	void RunLaterInGameWorlds(float Delay, TFunction<void(UWorld*)> Fn)
	{
		auto Run = [Fn]()
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
			Run();
			return;
		}
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Run](float)
		{
			Run();
			return false;
		}), Delay);
	}

	float ArgNum(const TArray<FString>& Args, int32 Index, float Default)
	{
		return Args.IsValidIndex(Index) ? FCString::Atof(*Args[Index]) : Default;
	}

	bool MatchesFilter(FName SpeciesId, const FString& Filter)
	{
		return Filter.IsEmpty() || Filter == TEXT("*") || SpeciesId.ToString().Equals(Filter, ESearchCase::IgnoreCase);
	}

	APlayerController* LocalController(UWorld* World)
	{
		APlayerController* PC = World->GetFirstPlayerController();
		return PC && PC->IsLocalController() ? PC : nullptr;
	}

	/** 서버: 원격 클라이언트 하나(없으면 nullptr) */
	APlayerController* RemoteController(UWorld* World)
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if (APlayerController* PC = It->Get(); PC && !PC->IsLocalController())
			{
				return PC;
			}
		}
		return nullptr;
	}

	/** 그 자리에서 종이 맞는 첫 개체(종 + 씨앗). 없으면 false */
	bool FindInPlace(const UGrimCreatureStorageComponent* Storage, const AGrimCreatureBox* Box, const APawn* Pawn, EGrimCreaturePlace Place, const FString& Filter,
		FName& OutSpecies, int32& OutSeed)
	{
		float BestDistance = TNumericLimits<float>::Max();
		bool bFound = false;
		switch (Place)
		{
		case EGrimCreaturePlace::Party:
			for (const AGrimCreature* Creature : Storage->GetPartyCreatures())
			{
				const float Distance = Pawn ? FVector::Dist(Pawn->GetActorLocation(), Creature->GetActorLocation()) : 0.f;
				if (MatchesFilter(Creature->GetInstance().SpeciesId, Filter) && Distance < BestDistance)
				{
					BestDistance = Distance;
					OutSpecies = Creature->GetInstance().SpeciesId;
					OutSeed = Creature->GetInstance().Seed;
					bFound = true;
				}
			}
			return bFound;
		case EGrimCreaturePlace::Storage:
			for (const FGrimOwnedCreatureRecord& Record : Storage->GetStored())
			{
				if (MatchesFilter(Record.Instance.SpeciesId, Filter))
				{
					OutSpecies = Record.Instance.SpeciesId;
					OutSeed = Record.Instance.Seed;
					return true;
				}
			}
			return false;
		default:
			if (!Box)
			{
				return false;
			}
			for (const FGrimCreatureBoxEntry& Entry : Box->GetEntries())
			{
				if (Entry.Record.Instance.OwnerId == Storage->GetOwnerPlayerId() && Entry.bReleased == (Place == EGrimCreaturePlace::Released)
					&& MatchesFilter(Entry.Record.Instance.SpeciesId, Filter))
				{
					OutSpecies = Entry.Record.Instance.SpeciesId;
					OutSeed = Entry.Record.Instance.Seed;
					return true;
				}
			}
			return false;
		}
	}

	/** Grim.Taming.Storage <From> <To> [종 id|*] [지연초] — 로컬 플레이어: 그 자리의 생물을 옮긴다(상자가 끼면 곁의 상자) — RPC 그대로 */
	void StorageCommand(const TArray<FString>& Args)
	{
		EGrimCreaturePlace From = EGrimCreaturePlace::Party;
		EGrimCreaturePlace To = EGrimCreaturePlace::Storage;
		if (Args.Num() < 2 || !GrimCreatureStorageMath::ParsePlace(Args[0], From) || !GrimCreatureStorageMath::ParsePlace(Args[1], To))
		{
			UE_LOG(LogGrimTaming, Warning, TEXT("사용: Grim.Taming.Storage <Party|Storage|Box|Released> <Party|Storage|Box|Released> [종 id|*] [지연초]"));
			return;
		}
		const FString Filter = Args.IsValidIndex(2) ? Args[2] : FString();
		RunLaterInGameWorlds(ArgNum(Args, 3, 0.f), [From, To, Filter](UWorld* World)
		{
			APlayerController* PC = LocalController(World);
			UGrimCreatureStorageComponent* Storage = UGrimCreatureStorageComponent::FindForActor(PC);
			APawn* Pawn = PC ? PC->GetPawn() : nullptr;
			if (!Storage || !Pawn)
			{
				UE_LOG(LogGrimTaming, Warning, TEXT("[%s] Taming.Storage: 로컬 폰 · 보관함 컴포넌트 없음"), SideName(World));
				return;
			}
			AGrimCreatureBox* Box = AGrimCreatureBox::FindNearestUsable(Pawn, 100.f);
			FName Species;
			int32 Seed = 0;
			if (!FindInPlace(Storage, Box, Pawn, From, Filter, Species, Seed))
			{
				UE_LOG(LogGrimTaming, Warning, TEXT("[%s] Taming.Storage %s → %s: %s 이(가) 없다(상자 %s)"), SideName(World), GrimCreatureStorageMath::PlaceName(From),
					GrimCreatureStorageMath::PlaceName(To), *Filter, Box ? *Box->GetName() : TEXT("-"));
				return;
			}
			Storage->RequestTransfer(Box, Species, Seed, From, To);
			UE_LOG(LogGrimTaming, Display, TEXT("[%s] Taming.Storage %s → %s %s seed=%d 상자=%s"), SideName(World), GrimCreatureStorageMath::PlaceName(From),
				GrimCreatureStorageMath::PlaceName(To), *Species.ToString(), Seed, Box ? *Box->GetName() : TEXT("-"));
		});
	}

	/** Grim.Taming.SpawnBox [거리cm=500] [지연초] [Host|Client] — 서버: 그 플레이어 앞에 거점 생물 상자(기본 = 원격 클라이언트, 없으면 호스트) */
	void SpawnBoxCommand(const TArray<FString>& Args)
	{
		const float Distance = ArgNum(Args, 0, 500.f);
		const FString Who = Args.IsValidIndex(2) ? Args[2] : FString(TEXT("Client"));
		RunLaterInGameWorlds(ArgNum(Args, 1, 0.f), [Distance, Who](UWorld* World)
		{
			if (World->GetNetMode() == NM_Client)
			{
				return;
			}
			APlayerController* PC = Who.Equals(TEXT("Host"), ESearchCase::IgnoreCase) ? LocalController(World) : RemoteController(World);
			if (!PC)
			{
				PC = LocalController(World);
			}
			const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
			if (!Pawn)
			{
				UE_LOG(LogGrimTaming, Warning, TEXT("[%s] Taming.SpawnBox: 폰 없음"), SideName(World));
				return;
			}
			const FVector Forward = FRotator(0.f, PC->GetControlRotation().Yaw, 0.f).Vector();
			const FVector Point = Pawn->GetActorLocation() + Forward * Distance;
			FVector Ground = Point - FVector(0.f, 0.f, Pawn->GetSimpleCollisionHalfHeight());
			UGrimEcosystemSubsystem::FindGround(World, Point, Ground);
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			const AGrimCreatureBox* Box = World->SpawnActor<AGrimCreatureBox>(AGrimCreatureBox::StaticClass(), Ground, FRotator(0.f, Forward.Rotation().Yaw + 180.f, 0.f), Params);
			UE_LOG(LogGrimTaming, Display, TEXT("[%s] Taming.SpawnBox %s at (%.0f,%.0f,%.0f) 앞 %.0fcm"), SideName(World), *GetNameSafe(Box), Ground.X, Ground.Y, Ground.Z, Distance);
		});
	}

	/** Grim.Taming.StorageLearn [1|0] [지연초] — 서버: 모든 플레이어의 생물 보관함 배움을 정한다(스킬북 없이) */
	void LearnCommand(const TArray<FString>& Args)
	{
		const bool bLearned = ArgNum(Args, 0, 1.f) > 0.f;
		RunLaterInGameWorlds(ArgNum(Args, 1, 0.f), [bLearned](UWorld* World)
		{
			if (World->GetNetMode() == NM_Client || !World->GetGameState())
			{
				return;
			}
			for (APlayerState* PlayerState : World->GetGameState()->PlayerArray)
			{
				if (UGrimCreatureStorageComponent* Storage = UGrimCreatureStorageComponent::FindForActor(PlayerState))
				{
					Storage->DebugSetLearned(bLearned);
					UE_LOG(LogGrimTaming, Display, TEXT("[%s] Taming.StorageLearn %s → %d"), SideName(World), *PlayerState->GetPlayerName(), bLearned ? 1 : 0);
				}
			}
		});
	}

	/** Grim.Taming.Box <Open|Close> [지연초] — 로컬 플레이어: 곁의 거점 생물 상자 화면(헤드리스에서는 위젯 경로만) */
	void BoxScreenCommand(const TArray<FString>& Args)
	{
		const bool bOpen = !Args.IsValidIndex(0) || !Args[0].Equals(TEXT("Close"), ESearchCase::IgnoreCase);
		RunLaterInGameWorlds(ArgNum(Args, 1, 0.f), [bOpen](UWorld* World)
		{
			APlayerController* PC = LocalController(World);
			UGrimCreatureStorageComponent* Storage = UGrimCreatureStorageComponent::FindForActor(PC);
			AGrimCreatureBox* Box = PC && PC->GetPawn() ? AGrimCreatureBox::FindNearestUsable(PC->GetPawn(), 100.f) : nullptr;
			if (!Storage)
			{
				return;
			}
			if (bOpen && Box)
			{
				Storage->OpenBoxScreen(Box);
			}
			else
			{
				Storage->CloseBoxScreen();
			}
			UE_LOG(LogGrimTaming, Display, TEXT("[%s] Taming.Box %s 상자=%s 열림=%d"), SideName(World), bOpen ? TEXT("Open") : TEXT("Close"), Box ? *Box->GetName() : TEXT("-"),
				Storage->IsBoxScreenOpen() ? 1 : 0);
		});
	}

	/**
	 * Grim.Taming.StorageDump [지연초] [Local|Remote] — 이 프로세스가 보는 그 플레이어의 데리고 다님 · 보관함 · 상자 목록.
	 * Local = 로컬 플레이어(클라이언트는 자기 것만 복제받는다), Remote = 서버가 본 원격 클라이언트. 비교용 줄(storage · storagerow)은 역할을 빼고 같은 꼴.
	 */
	void DumpCommand(const TArray<FString>& Args)
	{
		const bool bRemote = Args.IsValidIndex(1) && Args[1].Equals(TEXT("Remote"), ESearchCase::IgnoreCase);
		RunLaterInGameWorlds(ArgNum(Args, 0, 0.f), [bRemote](UWorld* World)
		{
			APlayerController* PC = bRemote ? (World->GetNetMode() == NM_Client ? nullptr : RemoteController(World)) : LocalController(World);
			const UGrimCreatureStorageComponent* Storage = PC ? UGrimCreatureStorageComponent::FindForActor(PC) : nullptr;
			if (!Storage)
			{
				UE_LOG(LogGrimTaming, Display, TEXT("[%s] StorageDump %s 없음"), SideName(World), bRemote ? TEXT("Remote") : TEXT("Local"));
				return;
			}
			// 상자: 이 플레이어 곁 · 없으면 세계의 첫 상자
			const AGrimCreatureBox* Box = PC->GetPawn() ? AGrimCreatureBox::FindNearestUsable(PC->GetPawn(), 2000.f) : nullptr;
			if (!Box)
			{
				TActorIterator<AGrimCreatureBox> It(World);
				Box = It ? *It : nullptr;
			}
			TArray<FGrimCreatureRowView> Rows = UGrimCreatureStorageWidget::GatherRows(Storage, Box);
			Rows.Sort([](const FGrimCreatureRowView& A, const FGrimCreatureRowView& B)
			{
				return A.Place != B.Place ? static_cast<uint8>(A.Place) < static_cast<uint8>(B.Place) : A.Instance.Seed < B.Instance.Seed;
			});
			const TCHAR* Side = SideName(World);
			UE_LOG(LogGrimTaming, Display, TEXT("[%s] StorageDump %s 줄 %d"), Side, bRemote ? TEXT("Remote") : TEXT("Local"), Rows.Num());
			UE_LOG(LogGrimTaming, Display, TEXT("[%s] storage learned=%d party=%d/%d stored=%d/%d box=%s boxEntries=%d released=%d"), Side, Storage->IsStorageLearned() ? 1 : 0,
				Storage->GetPartyCount(), Storage->GetPartyLimit(), Storage->GetStored().Num(), Storage->GetStorageCapacity(), Box ? TEXT("yes") : TEXT("no"),
				Box ? Box->GetEntries().Num() : 0, Box ? Box->GetReleasedCount() : 0);
			for (const FGrimCreatureRowView& Row : Rows)
			{
				const FGrimCreatureInstance& I = Row.Instance;
				const FString FirstColor = I.Colors.Num() > 0 ? I.Colors[0].Color.ToFColor(true).ToHex() : FString(TEXT("-"));
				UE_LOG(LogGrimTaming, Display, TEXT("[%s] storagerow place=%s seed=%d species=%s sex=%s size=%.3f giant=%d mutant=%d personality=%d colors=%d c0=%s loyalty=%.0f nick=\"%s\""), Side,
					GrimCreatureStorageMath::PlaceName(Row.Place), I.Seed, *I.SpeciesId.ToString(), I.Sex == EGrimCreatureSex::Male ? TEXT("M") : TEXT("F"), I.SizeScale,
					I.bGiant ? 1 : 0, I.bMutant ? 1 : 0, static_cast<int32>(I.Personality), I.Colors.Num(), *FirstColor, I.Loyalty, *I.Nickname);
				// 체력 · 화면 글은 순간 다를 수 있어 따로(비교에서 뺀다).
				UE_LOG(LogGrimTaming, Display, TEXT("[%s] storagetext seed=%d health=%.2f \"%s\""), Side, I.Seed, Row.HealthRatio,
					*UGrimCreatureStorageWidget::DescribeRow(Row).Replace(TEXT("\n"), TEXT(" / ")));
			}
		});
	}

	/**
	 * Grim.Taming.AfterJoin <플레이어 수> <초> <명령 …> — 이 프로세스의 게임 세계에 플레이어가 그 수만큼 모이고(로컬 폰도 있고) 나서 몇 초 뒤 명령을 친다.
	 * 헤드리스 2인 확인의 시간표를 「접속한 때」에 맞춘다 — 기계가 바빠 클라이언트가 늦게 붙어도 서버 · 클라이언트 시간표가 어긋나지 않게.
	 */
	void AfterJoinCommand(const TArray<FString>& Args)
	{
		if (Args.Num() < 3)
		{
			UE_LOG(LogGrimTaming, Warning, TEXT("사용: Grim.Taming.AfterJoin <플레이어 수> <초> <명령 …>"));
			return;
		}
		const int32 Players = FMath::Max(1, FCString::Atoi(*Args[0]));
		const double Delay = FMath::Max(0.0, FCString::Atod(*Args[1]));
		TArray<FString> Rest(Args.GetData() + 2, Args.Num() - 2);
		const FString Command = FString::Join(Rest, TEXT(" "));
		TSharedRef<double> Anchor = MakeShared<double>(-1.0);
		const double GiveUpAt = FPlatformTime::Seconds() + 600.0;
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Players, Delay, Command, Anchor, GiveUpAt](float)
		{
			if (FPlatformTime::Seconds() > GiveUpAt)
			{
				return false;
			}
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				UWorld* World = Context.World();
				const AGameStateBase* GameState = World && World->IsGameWorld() ? World->GetGameState() : nullptr;
				const APlayerController* PC = World && World->IsGameWorld() ? LocalController(World) : nullptr;
				if (!GameState || !PC || !PC->GetPawn() || GameState->PlayerArray.Num() < Players)
				{
					continue;
				}
				if (*Anchor < 0.0)
				{
					*Anchor = FPlatformTime::Seconds();
				}
				if (FPlatformTime::Seconds() - *Anchor >= Delay)
				{
					UE_LOG(LogGrimTaming, Display, TEXT("[%s] AfterJoin +%.1fs: %s"), SideName(World), Delay, *Command);
					GEngine->Exec(World, *Command);
					return false;
				}
			}
			return true;
		}), 0.2f);
	}

	FAutoConsoleCommand GAfterJoinCommand(TEXT("Grim.Taming.AfterJoin"), TEXT("플레이어가 모이고 몇 초 뒤 명령. <플레이어 수> <초> <명령 …>"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&AfterJoinCommand));
	FAutoConsoleCommand GStorageCommand(TEXT("Grim.Taming.Storage"), TEXT("로컬 플레이어: 생물을 옮긴다. <Party|Storage|Box|Released> <Party|Storage|Box|Released> [종 id|*] [지연초]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&StorageCommand));
	FAutoConsoleCommand GSpawnBoxCommand(TEXT("Grim.Taming.SpawnBox"), TEXT("서버: 플레이어 앞에 거점 생물 상자. [거리cm] [지연초] [Host|Client]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&SpawnBoxCommand));
	FAutoConsoleCommand GLearnCommand(TEXT("Grim.Taming.StorageLearn"), TEXT("서버: 모든 플레이어 생물 보관함 배움. [1|0] [지연초]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&LearnCommand));
	FAutoConsoleCommand GBoxScreenCommand(TEXT("Grim.Taming.Box"), TEXT("로컬 플레이어: 곁의 거점 생물 상자 화면. <Open|Close> [지연초]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&BoxScreenCommand));
	FAutoConsoleCommand GStorageDumpCommand(TEXT("Grim.Taming.StorageDump"), TEXT("데리고 다님 · 보관함 · 상자 목록. [지연초] [Local|Remote]"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&DumpCommand));
}

#endif
