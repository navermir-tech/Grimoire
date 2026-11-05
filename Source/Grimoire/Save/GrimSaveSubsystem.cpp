#include "Save/GrimSaveSubsystem.h"
#include "Save/GrimPlayerIdentity.h"
#include "Save/GrimPlayerSaveParticipants.h"
#include "Save/GrimSaveRecords.h"
#include "Save/GrimSaveSettings.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Info.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "HAL/FileManager.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

namespace GrimSaveSubsystemLocal // 유니티 빌드 이름 충돌 방지
{
	IGrimSaveParticipant* AsParticipant(UObject* Object, EGrimSaveScope Scope)
	{
		IGrimSaveParticipant* Participant = Cast<IGrimSaveParticipant>(Object);
		return (Participant && Participant->GetSaveScope() == Scope) ? Participant : nullptr;
	}

	void AddParticipant(UObject* Object, EGrimSaveScope Scope, TArray<FGrimSaveParticipantRef>& Out)
	{
		if (IGrimSaveParticipant* Participant = AsParticipant(Object, Scope))
		{
			FGrimSaveParticipantRef& Ref = Out.AddDefaulted_GetRef();
			Ref.Object = Object;
			Ref.Interface = Participant;
			Ref.SaveId = Participant->GetSaveId();
		}
	}

	/** 스폰됐어도 다시 스폰하지 않는 게임 프레임워크 액터(게임 모드가 만든다) */
	bool IsFrameworkActor(const AActor* Actor)
	{
		return Actor->IsA<AInfo>() || Actor->IsA<APawn>() || Actor->IsA<AController>();
	}

	FString ReadProjectVersion()
	{
		FString Version;
		if (GConfig)
		{
			GConfig->GetString(TEXT("/Script/EngineSettings.GeneralProjectSettings"), TEXT("ProjectVersion"), Version, GGameIni);
		}
		return Version;
	}

	TMap<FName, AActor*> MapActorsByName(UWorld* World)
	{
		TMap<FName, AActor*> Result;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (IsValid(*It))
			{
				Result.Add(It->GetFName(), *It);
			}
		}
		return Result;
	}

	const TCHAR* ScopeName(EGrimSaveScope Scope)
	{
		return Scope == EGrimSaveScope::Player ? TEXT("플레이어") : TEXT("세계");
	}
}

// ───────────────────────── 수명 ─────────────────────────

UGrimSaveSubsystem* UGrimSaveSubsystem::Get(const UWorld* World)
{
	return World ? World->GetSubsystem<UGrimSaveSubsystem>() : nullptr;
}

void UGrimSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LogoutHandle = FGameModeEvents::GameModeLogoutEvent.AddUObject(this, &ThisClass::HandleLogout);
	TearDownHandle = FWorldDelegates::OnWorldBeginTearDown.AddUObject(this, &ThisClass::HandleWorldBeginTearDown);
}

void UGrimSaveSubsystem::Deinitialize()
{
	FGameModeEvents::GameModeLogoutEvent.Remove(LogoutHandle);
	FWorldDelegates::OnWorldBeginTearDown.Remove(TearDownHandle);
	if (UWorld* World = GetWorld())
	{
		World->RemoveOnActorDestroyedHandler(ActorDestroyedHandle);
	}
	if (bEnabled && !bTornDown)
	{
		UE_LOG(LogGrimSave, Warning, TEXT("세이브 서브시스템이 종료 저장 없이 정리된다(월드 정리 순서가 예상과 다름)"));
	}
	Players.Reset();
	Super::Deinitialize();
}

TStatId UGrimSaveSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UGrimSaveSubsystem, STATGROUP_Tickables);
}

void UGrimSaveSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	DecideSlot();
	if (!bEnabled)
	{
		if (InWorld.GetNetMode() != NM_Client)
		{
			UE_LOG(LogGrimSave, Log, TEXT("세이브 꺼짐: %s"), *DisabledReason);
		}
		return;
	}

	ActorDestroyedHandle = InWorld.AddOnActorDestroyedHandler(FOnActorDestroyed::FDelegate::CreateUObject(this, &ThisClass::HandleActorDestroyed));
	// 놓인 액터의 BeginPlay(초기 상태)가 끝난 다음 틱에 세계를 읽는다 — 월드 서브시스템의 OnWorldBeginPlay 는 액터 BeginPlay 보다 먼저다.
	bWorldLoadPending = true;
	const float Interval = UGrimSaveSettings::Get()->AutoSaveIntervalSeconds;
	NextAutoSaveSeconds = Interval > 0.f ? InWorld.GetRealTimeSeconds() + Interval : 0.0;
	UE_LOG(LogGrimSave, Display, TEXT("[Server] 세이브 슬롯 %s (%s) — %s"), *Slot, *SlotSource, *GetSlotDirectoryFor(Slot));
}

void UGrimSaveSubsystem::DecideSlot()
{
	const UWorld* World = GetWorld();
	const UGrimSaveSettings* Settings = UGrimSaveSettings::Get();
	bEnabled = false;

	if (!World || World->GetNetMode() == NM_Client)
	{
		DisabledReason = TEXT("클라이언트(세이브는 서버가 한다)");
		return;
	}
	if (!Settings->bEnabled)
	{
		DisabledReason = TEXT("설정 bEnabled=False");
		return;
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("NoGrimSave")))
	{
		DisabledReason = TEXT("명령줄 -NoGrimSave");
		return;
	}

	FString Requested;
	if (const TCHAR* Option = World->URL.GetOption(TEXT("SaveSlot="), nullptr))
	{
		Requested = Option;
		SlotSource = TEXT("URL ?SaveSlot=");
	}
	else if (World->WorldType == EWorldType::PIE)
	{
		Requested = Settings->PIESlot;
		SlotSource = TEXT("설정 PIESlot");
		if (Requested.IsEmpty())
		{
			DisabledReason = TEXT("PIE — 설정 PIESlot 이 비어 있다");
			return;
		}
	}
	else
	{
		if (FApp::IsUnattended() && !Settings->bSaveWhenUnattendedWithoutSlot)
		{
			DisabledReason = TEXT("무인 실행(-unattended)인데 URL 에 ?SaveSlot= 이 없다");
			return;
		}
		Requested = Settings->DefaultSlot;
		SlotSource = TEXT("설정 DefaultSlot");
	}

	Slot = FGrimSaveRecordMath::SanitizeId(Requested);
	if (Slot.IsEmpty())
	{
		Slot = TEXT("Default");
	}
	bEnabled = true;
	DisabledReason.Reset();
}

bool UGrimSaveSubsystem::IsActive() const
{
	const UWorld* World = GetWorld();
	return bEnabled && World && World->GetNetMode() != NM_Client;
}

void UGrimSaveSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UWorld* World = GetWorld();
	if (!IsActive() || bTornDown || !World || !World->HasBegunPlay())
	{
		return;
	}

	if (bWorldLoadPending)
	{
		bWorldLoadPending = false;
		LoadWorld(TEXT("시작"));
	}

	UpdatePlayers();

	if (NextAutoSaveSeconds > 0.0 && World->GetRealTimeSeconds() >= NextAutoSaveSeconds)
	{
		NextAutoSaveSeconds = World->GetRealTimeSeconds() + UGrimSaveSettings::Get()->AutoSaveIntervalSeconds;
		SaveAll(TEXT("자동"));
	}
}

// ───────────────────────── 경로 ─────────────────────────

FString UGrimSaveSubsystem::GetSlotDirectoryFor(const FString& InSlot)
{
	return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), UGrimSaveSettings::Get()->SaveDirectory, InSlot));
}

FString UGrimSaveSubsystem::GetWorldSavePath() const
{
	return FPaths::Combine(GetSlotDirectoryFor(Slot), FString::Printf(TEXT("WorldSave_%s.sav"), *Slot));
}

FString UGrimSaveSubsystem::GetPlayerSavePath(const FString& PlayerId) const
{
	return FPaths::Combine(GetSlotDirectoryFor(Slot), FString::Printf(TEXT("Player_%s.sav"), *PlayerId));
}

// ───────────────────────── 모으기 ─────────────────────────

bool UGrimSaveSubsystem::IsPlacedActor(const AActor* Actor)
{
	return Actor && (Actor->IsNetStartupActor() || Actor->HasAnyFlags(RF_WasLoaded));
}

void UGrimSaveSubsystem::GatherActorParticipants(AActor* Actor, EGrimSaveScope Scope, TArray<FGrimSaveParticipantRef>& Out)
{
	using namespace GrimSaveSubsystemLocal;
	if (!IsValid(Actor))
	{
		return;
	}
	AddParticipant(Actor, Scope, Out);
	for (UActorComponent* Component : Actor->GetComponents())
	{
		if (IsValid(Component))
		{
			AddParticipant(Component, Scope, Out);
		}
	}
}

void UGrimSaveSubsystem::GatherPlayerParticipants(APlayerState* PlayerState, TArray<FGrimSaveParticipantRef>& Out, TArray<TStrongObjectPtr<UObject>>& OutKeepAlive)
{
	using namespace GrimSaveSubsystemLocal;
	if (!IsValid(PlayerState))
	{
		return;
	}
	GatherActorParticipants(PlayerState, EGrimSaveScope::Player, Out);
	if (APawn* Pawn = PlayerState->GetPawn())
	{
		GatherActorParticipants(Pawn, EGrimSaveScope::Player, Out);
	}

	// 어느 시스템에도 속하지 않는 몫 — 세이브 시스템이 맡는다.
	for (UClass* HelperClass : { UGrimPlayerAttributesSaveParticipant::StaticClass(), UGrimPlayerTransformSaveParticipant::StaticClass() })
	{
		UGrimPlayerSaveParticipantBase* Helper = NewObject<UGrimPlayerSaveParticipantBase>(GetTransientPackage(), HelperClass);
		Helper->SetPlayerState(PlayerState);
		OutKeepAlive.Add(TStrongObjectPtr<UObject>(Helper));
		AddParticipant(Helper, EGrimSaveScope::Player, Out);
	}
}

void UGrimSaveSubsystem::GatherWorldParticipants(TArray<FGrimSaveParticipantRef>& OutTopLevel, TArray<AActor*>& OutSpawned) const
{
	using namespace GrimSaveSubsystemLocal;
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (UWorldSubsystem* Subsystem : World->GetSubsystemArrayCopy<UWorldSubsystem>())
	{
		AddParticipant(Subsystem, EGrimSaveScope::World, OutTopLevel);
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor) || Actor->IsActorBeingDestroyed())
		{
			continue;
		}
		if (IsPlacedActor(Actor))
		{
			GatherActorParticipants(Actor, EGrimSaveScope::World, OutTopLevel);
		}
		else if (AsParticipant(Actor, EGrimSaveScope::World))
		{
			if (IsFrameworkActor(Actor))
			{
				GatherActorParticipants(Actor, EGrimSaveScope::World, OutTopLevel);
			}
			else
			{
				OutSpawned.Add(Actor);
			}
		}
	}
}

void UGrimSaveSubsystem::WriteRecords(const TArray<FGrimSaveParticipantRef>& Participants, TArray<FGrimSaveRecord>& OutRecords)
{
	TSet<FName> Seen;
	for (const FGrimSaveParticipantRef& Ref : Participants)
	{
		if (!Ref.Object.IsValid() || !Ref.Interface)
		{
			continue;
		}
		if (Ref.SaveId.IsNone() || Seen.Contains(Ref.SaveId))
		{
			UE_LOG(LogGrimSave, Warning, TEXT("SaveId %s 가 비었거나 겹친다(%s) — 건너뜀"), *Ref.SaveId.ToString(), *GetPathNameSafe(Ref.Object.Get()));
			continue;
		}
		Seen.Add(Ref.SaveId);

		FGrimSaveRecord& Record = OutRecords.AddDefaulted_GetRef();
		Record.SaveId = Ref.SaveId;
		Record.Version = Ref.Interface->GetSaveVersion();
		FMemoryWriter Writer(Record.Bytes, true);
		Ref.Interface->WriteSaveData(Writer);
	}
}

int32 UGrimSaveSubsystem::ApplyRecords(const TArray<FGrimSaveParticipantRef>& Participants, const TArray<FGrimSaveRecord>& Records, const TArray<FName>& Priority,
	const FString& Context, const TPair<FString, FString>* IdRename)
{
	TArray<FName> Ids;
	for (const FGrimSaveParticipantRef& Ref : Participants)
	{
		Ids.Add(Ref.SaveId);
	}

	int32 Applied = 0;
	for (const int32 Index : FGrimSaveRecordMath::MakeLoadOrder(Ids, Priority))
	{
		const FGrimSaveParticipantRef& Ref = Participants[Index];
		if (!Ref.Object.IsValid() || !Ref.Interface)
		{
			continue;
		}

		const FGrimSaveRecord* Record = FGrimSaveRecordMath::Find(Records, Ref.SaveId);
		if (!Record && IdRename)
		{
			// 다시 스폰한 액터의 이름이 저장 때와 다르면 SaveId 의 이름 부분도 다르다.
			for (const FGrimSaveRecord& Candidate : Records)
			{
				if (Candidate.SaveId.ToString().Replace(*IdRename->Key, *IdRename->Value) == Ref.SaveId.ToString())
				{
					Record = &Candidate;
					break;
				}
			}
		}
		if (!Record)
		{
			continue;
		}

		const int32 CodeVersion = Ref.Interface->GetSaveVersion();
		switch (FGrimSaveRecordMath::DecideRead(Record->Version, CodeVersion))
		{
		case EGrimRecordReadDecision::SkipInvalid:
			UE_LOG(LogGrimSave, Warning, TEXT("%s: %s 레코드 버전 %d 이 잘못됨 — 읽지 않음"), *Context, *Ref.SaveId.ToString(), Record->Version);
			continue;
		case EGrimRecordReadDecision::SkipNewer:
			UE_LOG(LogGrimSave, Warning, TEXT("%s: %s 레코드가 더 새 버전(%d > 코드 %d) — 읽지 않고 파일에 그대로 둔다"), *Context, *Ref.SaveId.ToString(), Record->Version, CodeVersion);
			continue;
		default:
			break;
		}

		FMemoryReader Reader(Record->Bytes, true);
		Ref.Interface->ReadSaveData(Reader, Record->Version);
		if (Reader.IsError())
		{
			UE_LOG(LogGrimSave, Warning, TEXT("%s: %s 를 읽다가 바이트가 모자람(%d 바이트, 버전 %d)"), *Context, *Ref.SaveId.ToString(), Record->Bytes.Num(), Record->Version);
		}
		++Applied;
	}
	return Applied;
}

// ───────────────────────── 파일 ─────────────────────────

UGrimSaveGame* UGrimSaveSubsystem::NewSaveGame(EGrimSaveScope Scope) const
{
	UGrimSaveGame* SaveGame = NewObject<UGrimSaveGame>(GetTransientPackage());
	SaveGame->Scope = Scope;
	SaveGame->Slot = Slot;
	SaveGame->SavedAtUtc = FDateTime::UtcNow();
	SaveGame->ProjectVersion = GrimSaveSubsystemLocal::ReadProjectVersion();
	if (const UWorld* World = GetWorld())
	{
		SaveGame->MapName = UWorld::RemovePIEPrefix(World->GetMapName());
	}
	return SaveGame;
}

bool UGrimSaveSubsystem::WriteSaveGame(UGrimSaveGame* SaveGame, const FString& Path, const FString& What) const
{
	TArray<uint8> Payload;
	if (!FGrimSaveFiles::Serialize(SaveGame, Payload))
	{
		UE_LOG(LogGrimSave, Error, TEXT("%s: 직렬화 실패"), *What);
		return false;
	}
	FString Error;
	if (!FGrimSaveFiles::WriteWithBackup(Path, Payload, UGrimSaveSettings::Get()->bKeepBackup, &Error))
	{
		UE_LOG(LogGrimSave, Error, TEXT("%s: 쓰기 실패 — %s"), *What, *Error);
		return false;
	}
	return true;
}

void UGrimSaveSubsystem::QuarantineCorrupt(const FString& Path)
{
	IFileManager& Files = IFileManager::Get();
	const FString Stamp = FDateTime::Now().ToString(TEXT("%Y%m%d-%H%M%S"));
	for (const FString& Candidate : { Path, FGrimSaveFiles::GetBackupPath(Path) })
	{
		if (Files.FileExists(*Candidate))
		{
			const FString Aside = FString::Printf(TEXT("%s.corrupt-%s"), *Candidate, *Stamp);
			Files.Move(*Aside, *Candidate, true, true);
			UE_LOG(LogGrimSave, Error, TEXT("깨진 세이브를 옆으로 치움: %s"), *Aside);
		}
	}
}

// ───────────────────────── 세계 ─────────────────────────

bool UGrimSaveSubsystem::SaveWorld(const FString& Reason)
{
	if (!IsActive())
	{
		return false;
	}
	if (!bWorldLoaded)
	{
		UE_LOG(LogGrimSave, Log, TEXT("[Server] 세계 저장 건너뜀(%s): 아직 불러오기 전 — 기본 상태가 세이브를 덮지 않게"), *Reason);
		return false;
	}

	TArray<FGrimSaveParticipantRef> TopLevel;
	TArray<AActor*> Spawned;
	GatherWorldParticipants(TopLevel, Spawned);

	TArray<FGrimSaveRecord> Fresh;
	WriteRecords(TopLevel, Fresh);
	int32 KeptNewer = 0;
	TArray<FGrimSaveRecord> Merged = FGrimSaveRecordMath::Merge(WorldStoredRecords, Fresh, DroppedWorldIds, &KeptNewer);

	UGrimSaveGame* SaveGame = NewSaveGame(EGrimSaveScope::World);
	SaveGame->SaveCount = WorldSaveCount + 1;
	SaveGame->Records = Merged;
	SaveGame->RemovedActors = RemovedActors;
	for (AActor* Actor : Spawned)
	{
		FGrimSpawnedActorRecord& Record = SaveGame->SpawnedActors.AddDefaulted_GetRef();
		Record.ActorName = Actor->GetFName();
		Record.ActorClass = FSoftClassPath(Actor->GetClass());
		Record.Transform = Actor->GetActorTransform();
		TArray<FGrimSaveParticipantRef> Own;
		GatherActorParticipants(Actor, EGrimSaveScope::World, Own);
		WriteRecords(Own, Record.Records);
	}

	const FString Path = GetWorldSavePath();
	if (!WriteSaveGame(SaveGame, Path, TEXT("세계 저장")))
	{
		return false;
	}
	WorldStoredRecords = MoveTemp(Merged);
	WorldSaveCount = SaveGame->SaveCount;
	UE_LOG(LogGrimSave, Display, TEXT("[Server] 세계 저장(%s) #%d: 레코드 %d(지금 %d · 보존 %d · 새 버전 유지 %d), 스폰 액터 %d, 없앤 액터 %d → %s"),
		*Reason, WorldSaveCount, WorldStoredRecords.Num(), Fresh.Num(), WorldStoredRecords.Num() - Fresh.Num() + KeptNewer, KeptNewer,
		SaveGame->SpawnedActors.Num(), RemovedActors.Num(), *Path);
	return true;
}

bool UGrimSaveSubsystem::LoadWorld(const FString& Reason)
{
	using namespace GrimSaveSubsystemLocal;
	UWorld* World = GetWorld();
	if (!IsActive() || !World)
	{
		return false;
	}

	const FString Path = GetWorldSavePath();
	bool bUsedBackup = false, bAnyFile = false;
	TArray<FString> Problems;
	UGrimSaveGame* SaveGame = FGrimSaveFiles::LoadWithFallback(Path, bUsedBackup, bAnyFile, Problems);
	for (const FString& Problem : Problems)
	{
		UE_LOG(LogGrimSave, Warning, TEXT("[Server] 세계 세이브 문제: %s"), *Problem);
	}

	if (!SaveGame)
	{
		if (bAnyFile)
		{
			UE_LOG(LogGrimSave, Error, TEXT("[Server] 세계 세이브를 읽을 수 없다(본 파일 · 백업 모두) — 새 세계로 시작한다"));
			QuarantineCorrupt(Path);
		}
		else
		{
			UE_LOG(LogGrimSave, Display, TEXT("[Server] 세계 세이브 없음(%s) — 새 세계"), *Reason);
		}
		bWorldLoaded = true;
		return false;
	}

	// 1) 게임 중 없어진 놓인 액터(집은 스킬북 …)를 다시 없앤다.
	RemovedActors = SaveGame->RemovedActors;
	TMap<FName, AActor*> ByName = MapActorsByName(World);
	int32 Removed = 0;
	for (const FName& Name : RemovedActors)
	{
		if (AActor** Found = ByName.Find(Name); Found && IsPlacedActor(*Found))
		{
			TArray<FGrimSaveParticipantRef> Own;
			GatherActorParticipants(*Found, EGrimSaveScope::World, Own);
			for (const FGrimSaveParticipantRef& Ref : Own)
			{
				DroppedWorldIds.Add(Ref.SaveId);
			}
			(*Found)->Destroy();
			++Removed;
		}
	}

	// 2) 스폰된 저장 대상 액터를 다시 스폰한다(이미 있으면 — 게임 중 Grim.Save.Load — 그 액터에 읽힌다).
	ByName = MapActorsByName(World);
	int32 Respawned = 0;
	for (const FGrimSpawnedActorRecord& Record : SaveGame->SpawnedActors)
	{
		AActor** Existing = ByName.Find(Record.ActorName);
		AActor* Actor = (Existing && !IsPlacedActor(*Existing)) ? *Existing : nullptr;
		bool bDeferred = false;
		if (!Actor)
		{
			UClass* Class = Record.ActorClass.TryLoadClass<AActor>();
			if (!Class)
			{
				UE_LOG(LogGrimSave, Warning, TEXT("[Server] 스폰 액터 %s: 클래스 %s 없음 — 건너뜀"), *Record.ActorName.ToString(), *Record.ActorClass.ToString());
				continue;
			}
			FActorSpawnParameters Params;
			Params.Name = Record.ActorName;
			Params.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;
			Params.bDeferConstruction = true;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Actor = World->SpawnActor(Class, &Record.Transform, Params);
			if (!Actor)
			{
				UE_LOG(LogGrimSave, Warning, TEXT("[Server] 스폰 액터 %s 를 만들지 못함"), *Record.ActorName.ToString());
				continue;
			}
			bDeferred = true;
		}

		TArray<FGrimSaveParticipantRef> Own;
		GatherActorParticipants(Actor, EGrimSaveScope::World, Own);
		const TPair<FString, FString> Rename(Record.ActorName.ToString(), Actor->GetName());
		ApplyRecords(Own, Record.Records, UGrimSaveSettings::Get()->WorldLoadOrder, FString::Printf(TEXT("세계 %s"), *Actor->GetName()),
			Actor->GetFName() != Record.ActorName ? &Rename : nullptr);
		if (bDeferred)
		{
			Actor->FinishSpawning(Record.Transform);
			++Respawned;
		}
	}

	// 3) 월드 서브시스템 · 놓인 액터.
	TArray<FGrimSaveParticipantRef> TopLevel;
	TArray<AActor*> Spawned;
	GatherWorldParticipants(TopLevel, Spawned);
	const int32 Applied = ApplyRecords(TopLevel, SaveGame->Records, UGrimSaveSettings::Get()->WorldLoadOrder, TEXT("세계"));

	WorldStoredRecords = SaveGame->Records;
	WorldSaveCount = SaveGame->SaveCount;
	bWorldLoaded = true;
	UE_LOG(LogGrimSave, Display, TEXT("[Server] 세계 불러옴(%s)%s: #%d (%s UTC, %s), 레코드 %d 중 %d 적용(나머지 보존), 스폰 액터 %d 다시 만듦, 없앤 액터 %d"),
		*Reason, bUsedBackup ? TEXT(" [백업에서]") : TEXT(""), SaveGame->SaveCount, *SaveGame->SavedAtUtc.ToString(), *SaveGame->MapName,
		SaveGame->Records.Num(), Applied, Respawned, Removed);
	return true;
}

void UGrimSaveSubsystem::HandleActorDestroyed(AActor* Actor)
{
	const UWorld* World = GetWorld();
	if (!IsActive() || bTornDown || !World || World->bIsTearingDown || !IsPlacedActor(Actor))
	{
		return;
	}
	TArray<FGrimSaveParticipantRef> Own;
	GatherActorParticipants(Actor, EGrimSaveScope::World, Own);
	if (Own.IsEmpty())
	{
		return;
	}
	RemovedActors.AddUnique(Actor->GetFName());
	for (const FGrimSaveParticipantRef& Ref : Own)
	{
		DroppedWorldIds.Add(Ref.SaveId);
	}
	UE_LOG(LogGrimSave, Log, TEXT("[Server] 놓인 액터 %s 가 없어짐 — 세이브에 기록(다시 열어도 안 나온다)"), *Actor->GetName());
}

// ───────────────────────── 플레이어 ─────────────────────────

UGrimSaveSubsystem::FPlayerEntry* UGrimSaveSubsystem::FindEntry(const APlayerState* PlayerState)
{
	return Players.FindByPredicate([PlayerState](const FPlayerEntry& Entry) { return Entry.PlayerState.Get() == PlayerState; });
}

const UGrimSaveSubsystem::FPlayerEntry* UGrimSaveSubsystem::FindEntry(const APlayerState* PlayerState) const
{
	return Players.FindByPredicate([PlayerState](const FPlayerEntry& Entry) { return Entry.PlayerState.Get() == PlayerState; });
}

FString UGrimSaveSubsystem::GetPlayerId(const APlayerState* PlayerState) const
{
	const FPlayerEntry* Entry = FindEntry(PlayerState);
	return Entry ? Entry->PlayerId : FString();
}

bool UGrimSaveSubsystem::IsPlayerLoaded(const APlayerState* PlayerState) const
{
	const FPlayerEntry* Entry = FindEntry(PlayerState);
	return Entry && Entry->bLoaded;
}

bool UGrimSaveSubsystem::ResolveEntryId(FPlayerEntry& Entry, APlayerState* PlayerState)
{
	if (!Entry.PlayerId.IsEmpty())
	{
		return true;
	}
	FString Source;
	FString Id = FGrimPlayerIdentity::ResolveServerSide(PlayerState, &Source);
	if (Id.IsEmpty())
	{
		return false;
	}

	// 같은 PC 의 두 인스턴스처럼 id 가 겹치면 뒤에 온 쪽을 갈라 둔다(서로의 세이브를 덮지 않게).
	const FString Base = Id;
	for (int32 Suffix = 2; Players.ContainsByPredicate([&Id, &Entry](const FPlayerEntry& Other)
		{ return &Other != &Entry && Other.PlayerState.IsValid() && Other.PlayerId == Id; }); ++Suffix)
	{
		Id = FString::Printf(TEXT("%s_%d"), *Base, Suffix);
	}
	if (Id != Base)
	{
		UE_LOG(LogGrimSave, Warning, TEXT("[Server] PlayerId %s 가 이미 접속 중 — %s 로 저장한다(같은 PC 의 두 번째 인스턴스면 -GrimPlayerId= 를 주자)"), *Base, *Id);
	}

	Entry.PlayerId = Id;
	Entry.IdSource = Source;
	Entry.PlayerName = PlayerState->GetPlayerName();
	return true;
}

void UGrimSaveSubsystem::UpdatePlayers()
{
	UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	if (!GameState)
	{
		return;
	}

	Players.RemoveAll([](const FPlayerEntry& Entry) { return !Entry.PlayerState.IsValid(); });

	const int32 DelayTicks = FMath::Max(1, UGrimSaveSettings::Get()->PlayerLoadDelayTicks);
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		if (!IsValid(PlayerState) || PlayerState->IsABot() || PlayerState->IsInactive())
		{
			continue;
		}
		FPlayerEntry* Entry = FindEntry(PlayerState);
		if (!Entry)
		{
			Entry = &Players.AddDefaulted_GetRef();
			Entry->PlayerState = PlayerState;
		}
		if (Entry->bLoaded)
		{
			if (const APawn* Pawn = PlayerState->GetPawn())
			{
				Entry->bHasLastTransform = true;
				Entry->LastLocation = Pawn->GetActorLocation();
				const AController* Controller = Pawn->GetController();
				Entry->LastYaw = Controller ? Controller->GetControlRotation().Yaw : Pawn->GetActorRotation().Yaw;
			}
			continue;
		}
		if (!ResolveEntryId(*Entry, PlayerState))
		{
			continue;
		}

		// 폰이 붙고 ASC 가 그 폰으로 초기화된 뒤(마법서 · 생존이 OnAbilityActorInfoReady 에서 시작한 뒤).
		const APawn* Pawn = PlayerState->GetPawn();
		const IAbilitySystemInterface* AbilityOwner = Cast<IAbilitySystemInterface>(PlayerState);
		const UAbilitySystemComponent* ASC = AbilityOwner ? AbilityOwner->GetAbilitySystemComponent() : nullptr;
		const bool bReady = Pawn && (!ASC || ASC->GetAvatarActor() == Pawn);
		if (!bReady)
		{
			Entry->ReadyTicks = 0;
			continue;
		}
		if (++Entry->ReadyTicks < DelayTicks)
		{
			continue;
		}
		ApplyPlayerFile(*Entry, PlayerState, TEXT("접속"));
	}
}

bool UGrimSaveSubsystem::ApplyPlayerFile(FPlayerEntry& Entry, APlayerState* PlayerState, const FString& Reason)
{
	const FString Path = GetPlayerSavePath(Entry.PlayerId);
	bool bUsedBackup = false, bAnyFile = false;
	TArray<FString> Problems;
	UGrimSaveGame* SaveGame = FGrimSaveFiles::LoadWithFallback(Path, bUsedBackup, bAnyFile, Problems);
	for (const FString& Problem : Problems)
	{
		UE_LOG(LogGrimSave, Warning, TEXT("[Server] 플레이어 세이브 %s 문제: %s"), *Entry.PlayerId, *Problem);
	}
	Entry.bLoaded = true;

	if (!SaveGame)
	{
		if (bAnyFile)
		{
			UE_LOG(LogGrimSave, Error, TEXT("[Server] 플레이어 %s(%s) 세이브를 읽을 수 없다 — 새 캐릭터로 시작한다"), *Entry.PlayerName, *Entry.PlayerId);
			QuarantineCorrupt(Path);
		}
		else
		{
			UE_LOG(LogGrimSave, Display, TEXT("[Server] 플레이어 %s — PlayerId %s(%s), 세이브 없음(%s) — 새 캐릭터"), *Entry.PlayerName, *Entry.PlayerId, *Entry.IdSource, *Reason);
		}
		return false;
	}

	TArray<FGrimSaveParticipantRef> Participants;
	TArray<TStrongObjectPtr<UObject>> KeepAlive;
	GatherPlayerParticipants(PlayerState, Participants, KeepAlive);
	const int32 Applied = ApplyRecords(Participants, SaveGame->Records, UGrimSaveSettings::Get()->PlayerLoadOrder,
		FString::Printf(TEXT("플레이어 %s"), *Entry.PlayerId));

	Entry.StoredRecords = SaveGame->Records;
	Entry.SaveCount = SaveGame->SaveCount;
	UE_LOG(LogGrimSave, Display, TEXT("[Server] 플레이어 %s — PlayerId %s(%s) 불러옴(%s)%s: #%d (%s UTC), 레코드 %d 중 %d 적용"),
		*Entry.PlayerName, *Entry.PlayerId, *Entry.IdSource, *Reason, bUsedBackup ? TEXT(" [백업에서]") : TEXT(""), SaveGame->SaveCount,
		*SaveGame->SavedAtUtc.ToString(), SaveGame->Records.Num(), Applied);
	return true;
}

bool UGrimSaveSubsystem::SavePlayer(APlayerState* PlayerState, const FString& Reason)
{
	FPlayerEntry* Entry = FindEntry(PlayerState);
	if (!IsActive() || !IsValid(PlayerState) || !Entry)
	{
		return false;
	}
	if (!Entry->bLoaded)
	{
		UE_LOG(LogGrimSave, Log, TEXT("[Server] 플레이어 %s 저장 건너뜀(%s): 아직 불러오기 전"), *PlayerState->GetPlayerName(), *Reason);
		return false;
	}

	TArray<FGrimSaveParticipantRef> Participants;
	TArray<TStrongObjectPtr<UObject>> KeepAlive;
	GatherPlayerParticipants(PlayerState, Participants, KeepAlive);
	if (Entry->bHasLastTransform)
	{
		for (const TStrongObjectPtr<UObject>& Helper : KeepAlive)
		{
			if (UGrimPlayerTransformSaveParticipant* Transform = Cast<UGrimPlayerTransformSaveParticipant>(Helper.Get()))
			{
				Transform->SetFallback(Entry->LastLocation, Entry->LastYaw);
			}
		}
	}
	TArray<FGrimSaveRecord> Fresh;
	WriteRecords(Participants, Fresh);
	int32 KeptNewer = 0;
	TArray<FGrimSaveRecord> Merged = FGrimSaveRecordMath::Merge(Entry->StoredRecords, Fresh, TSet<FName>(), &KeptNewer);

	UGrimSaveGame* SaveGame = NewSaveGame(EGrimSaveScope::Player);
	SaveGame->PlayerId = Entry->PlayerId;
	SaveGame->PlayerName = PlayerState->GetPlayerName();
	SaveGame->SaveCount = Entry->SaveCount + 1;
	SaveGame->Records = Merged;

	const FString Path = GetPlayerSavePath(Entry->PlayerId);
	if (!WriteSaveGame(SaveGame, Path, FString::Printf(TEXT("플레이어 %s 저장"), *Entry->PlayerId)))
	{
		return false;
	}
	Entry->StoredRecords = MoveTemp(Merged);
	Entry->SaveCount = SaveGame->SaveCount;
	Entry->PlayerName = SaveGame->PlayerName;
	UE_LOG(LogGrimSave, Display, TEXT("[Server] 플레이어 %s — PlayerId %s 저장(%s) #%d: 레코드 %d(지금 %d · 보존 %d) → %s"),
		*SaveGame->PlayerName, *Entry->PlayerId, *Reason, Entry->SaveCount, Entry->StoredRecords.Num(), Fresh.Num(),
		Entry->StoredRecords.Num() - Fresh.Num() + KeptNewer, *Path);
	return true;
}

bool UGrimSaveSubsystem::LoadPlayer(APlayerState* PlayerState, const FString& Reason)
{
	FPlayerEntry* Entry = FindEntry(PlayerState);
	if (!IsActive() || !IsValid(PlayerState) || !Entry || Entry->PlayerId.IsEmpty())
	{
		return false;
	}
	return ApplyPlayerFile(*Entry, PlayerState, Reason);
}

// ───────────────────────── 묶음 ─────────────────────────

bool UGrimSaveSubsystem::SaveAll(const FString& Reason)
{
	if (!IsActive())
	{
		return false;
	}
	bool bOk = SaveWorld(Reason);
	TArray<APlayerState*> Loaded;
	for (const FPlayerEntry& Entry : Players)
	{
		if (Entry.bLoaded && Entry.PlayerState.IsValid())
		{
			Loaded.Add(Entry.PlayerState.Get());
		}
	}
	for (APlayerState* PlayerState : Loaded)
	{
		bOk &= SavePlayer(PlayerState, Reason);
	}
	return bOk;
}

bool UGrimSaveSubsystem::LoadAll(const FString& Reason)
{
	if (!IsActive())
	{
		return false;
	}
	bWorldLoadPending = false;
	bool bOk = LoadWorld(Reason);
	for (FPlayerEntry& Entry : Players)
	{
		if (Entry.bLoaded && Entry.PlayerState.IsValid())
		{
			bOk &= ApplyPlayerFile(Entry, Entry.PlayerState.Get(), Reason);
		}
	}
	return bOk;
}

bool UGrimSaveSubsystem::DeleteSlot(const FString& InSlot)
{
	const FString Target = FGrimSaveRecordMath::SanitizeId(InSlot.IsEmpty() ? Slot : InSlot);
	if (Target.IsEmpty())
	{
		return false;
	}
	const FString Directory = GetSlotDirectoryFor(Target);
	const bool bDeleted = IFileManager::Get().DeleteDirectory(*Directory, false, true);
	if (Target == Slot)
	{
		WorldStoredRecords.Reset();
		RemovedActors.Reset();
		DroppedWorldIds.Reset();
		WorldSaveCount = 0;
		for (FPlayerEntry& Entry : Players)
		{
			Entry.StoredRecords.Reset();
			Entry.SaveCount = 0;
		}
	}
	UE_LOG(LogGrimSave, Display, TEXT("[Server] 슬롯 %s 지움%s: %s"), *Target, bDeleted ? TEXT("") : TEXT("(폴더 없음)"), *Directory);
	return bDeleted;
}

void UGrimSaveSubsystem::HandleLogout(AGameModeBase* GameMode, AController* Exiting)
{
	if (!GameMode || GameMode->GetWorld() != GetWorld() || !IsActive() || bTornDown || !Exiting)
	{
		return;
	}
	if (APlayerState* PlayerState = Exiting->PlayerState)
	{
		SavePlayer(PlayerState, TEXT("퇴장"));
		Players.RemoveAll([PlayerState](const FPlayerEntry& Entry) { return Entry.PlayerState.Get() == PlayerState; });
	}
}

void UGrimSaveSubsystem::HandleWorldBeginTearDown(UWorld* World)
{
	if (World != GetWorld() || bTornDown)
	{
		return;
	}
	if (IsActive())
	{
		SaveAll(TEXT("종료"));
	}
	bTornDown = true;
}

// ───────────────────────── 덤프 ─────────────────────────

void UGrimSaveSubsystem::Dump() const
{
	using namespace GrimSaveSubsystemLocal;
	if (!IsActive())
	{
		UE_LOG(LogGrimSave, Display, TEXT("[Save] 꺼짐: %s"), *DisabledReason);
		return;
	}

	UE_LOG(LogGrimSave, Display, TEXT("[Save] 슬롯 %s (%s), 폴더 %s, 세계 불러옴=%d, 저장 #%d, 기억 레코드 %d, 없앤 액터 %d"),
		*Slot, *SlotSource, *GetSlotDirectoryFor(Slot), bWorldLoaded ? 1 : 0, WorldSaveCount, WorldStoredRecords.Num(), RemovedActors.Num());

	TArray<FGrimSaveParticipantRef> TopLevel;
	TArray<AActor*> Spawned;
	GatherWorldParticipants(TopLevel, Spawned);
	for (const FGrimSaveRecord& Record : WorldStoredRecords)
	{
		const bool bLive = TopLevel.ContainsByPredicate([&Record](const FGrimSaveParticipantRef& Ref) { return Ref.SaveId == Record.SaveId; });
		UE_LOG(LogGrimSave, Display, TEXT("[Save]   세계 %s v%d %d바이트 %s"), *Record.SaveId.ToString(), Record.Version, Record.Bytes.Num(), bLive ? TEXT("") : TEXT("(보존 — 지금 참여자 없음)"));
	}
	for (const FGrimSaveParticipantRef& Ref : TopLevel)
	{
		if (!FGrimSaveRecordMath::Find(WorldStoredRecords, Ref.SaveId))
		{
			UE_LOG(LogGrimSave, Display, TEXT("[Save]   세계 %s (아직 저장 안 됨)"), *Ref.SaveId.ToString());
		}
	}
	for (const AActor* Actor : Spawned)
	{
		UE_LOG(LogGrimSave, Display, TEXT("[Save]   스폰 액터 %s (%s) at %s"), *Actor->GetName(), *Actor->GetClass()->GetName(), *Actor->GetActorLocation().ToCompactString());
	}
	for (const FName& Name : RemovedActors)
	{
		UE_LOG(LogGrimSave, Display, TEXT("[Save]   없앤 놓인 액터 %s"), *Name.ToString());
	}
	for (const FPlayerEntry& Entry : Players)
	{
		UE_LOG(LogGrimSave, Display, TEXT("[Save]   플레이어 %s PlayerId=%s(%s) 불러옴=%d 저장 #%d 레코드 %d → %s"),
			*Entry.PlayerName, *Entry.PlayerId, *Entry.IdSource, Entry.bLoaded ? 1 : 0, Entry.SaveCount, Entry.StoredRecords.Num(), *GetPlayerSavePath(Entry.PlayerId));
		for (const FGrimSaveRecord& Record : Entry.StoredRecords)
		{
			UE_LOG(LogGrimSave, Display, TEXT("[Save]     %s v%d %d바이트"), *Record.SaveId.ToString(), Record.Version, Record.Bytes.Num());
		}
	}

	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *FPaths::Combine(GetSlotDirectoryFor(Slot), TEXT("*")), true, false);
	for (const FString& File : Files)
	{
		const FString Full = FPaths::Combine(GetSlotDirectoryFor(Slot), File);
		TArray<uint8> Payload;
		FString Error;
		const bool bValid = FGrimSaveFiles::ReadPayload(Full, Payload, &Error);
		UE_LOG(LogGrimSave, Display, TEXT("[Save]   파일 %s %lld바이트 %s"), *File, IFileManager::Get().FileSize(*Full), bValid ? TEXT("온전") : *Error);
	}
}
