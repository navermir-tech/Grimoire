#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Save/GrimPlayerIdentity.h"
#include "Save/GrimSaveGame.h"
#include "Save/GrimSaveRecords.h"
#include "Save/GrimSaveSubsystem.h"
#include "Core/GrimGameplayTags.h"
#include "Interaction/GrimElementalReactionTypes.h"
#include "Interaction/GrimElementalStateComponent.h"
#include "Interaction/GrimInteractionTags.h"
#include "Survival/GrimBed.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

namespace GrimSaveTestsLocal // 유니티 빌드에서 다른 테스트 파일과 이름이 부딪히지 않게
{
	constexpr EAutomationTestFlags SaveTestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	FGrimSaveRecord MakeRecord(const TCHAR* Id, int32 Version, uint8 Byte)
	{
		FGrimSaveRecord Record;
		Record.SaveId = Id;
		Record.Version = Version;
		Record.Bytes = { Byte, static_cast<uint8>(Byte + 1) };
		return Record;
	}

	TArray<FString> Ids(const TArray<FGrimSaveRecord>& Records)
	{
		TArray<FString> Out;
		for (const FGrimSaveRecord& Record : Records)
		{
			Out.Add(Record.SaveId.ToString());
		}
		return Out;
	}

	/** 테스트 폴더(Saved/Automation/GrimSave/<GUID>) — 소멸자가 지운다 */
	struct FScopedTempDir
	{
		FString Dir;
		FScopedTempDir() { Dir = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation/GrimSave"), FGuid::NewGuid().ToString())); }
		~FScopedTempDir() { IFileManager::Get().DeleteDirectory(*Dir, false, true); }
	};

	/** 게임 월드(게임 모드 없음). 소멸자가 정리한다 */
	struct FScopedSaveTestWorld
	{
		UWorld* World = nullptr;

		FScopedSaveTestWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("GrimSaveTestWorld"));
			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
			World->BeginPlay();
			World->GetWorldSettings()->NotifyBeginPlay();
		}

		~FScopedSaveTestWorld()
		{
			World->EndPlay(EEndPlayReason::Quit);
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}
	};
}

using namespace GrimSaveTestsLocal;

// 모르는 레코드 보존 · 순서 · 새 레코드는 끝에.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimSaveMergeTest, "Grimoire.Save.Records.Merge", GrimSaveTestsLocal::SaveTestFlags)
bool FGrimSaveMergeTest::RunTest(const FString& Parameters)
{
	const TArray<FGrimSaveRecord> Existing = { MakeRecord(TEXT("Magic.Spellbook"), 1, 10), MakeRecord(TEXT("Quest.Tower"), 3, 20), MakeRecord(TEXT("Survival.Vitals"), 1, 30) };
	const TArray<FGrimSaveRecord> Fresh = { MakeRecord(TEXT("Survival.Vitals"), 1, 31), MakeRecord(TEXT("Magic.Spellbook"), 1, 11), MakeRecord(TEXT("Player.Transform"), 1, 40) };

	int32 KeptNewer = -1;
	const TArray<FGrimSaveRecord> Merged = FGrimSaveRecordMath::Merge(Existing, Fresh, TSet<FName>(), &KeptNewer);
	TestEqual(TEXT("순서: 지난 파일 순서 + 새 id 는 끝"), Ids(Merged), TArray<FString>({ TEXT("Magic.Spellbook"), TEXT("Quest.Tower"), TEXT("Survival.Vitals"), TEXT("Player.Transform") }));
	TestEqual(TEXT("마법서는 새 바이트"), Merged[0].Bytes[0], static_cast<uint8>(11));
	TestEqual(TEXT("모르는 퀘스트 레코드는 그대로(버전)"), Merged[1].Version, 3);
	TestEqual(TEXT("모르는 퀘스트 레코드는 그대로(바이트)"), Merged[1].Bytes[0], static_cast<uint8>(20));
	TestEqual(TEXT("생존은 새 바이트"), Merged[2].Bytes[0], static_cast<uint8>(31));
	TestEqual(TEXT("새 버전 유지 없음"), KeptNewer, 0);

	// 빈 지난 파일 = Fresh 그대로, 빈 Fresh = 지난 파일 그대로.
	TestEqual(TEXT("처음 저장"), Ids(FGrimSaveRecordMath::Merge({}, Fresh, TSet<FName>())), Ids(Fresh));
	TestEqual(TEXT("참여자가 하나도 없을 때"), Ids(FGrimSaveRecordMath::Merge(Existing, {}, TSet<FName>())), Ids(Existing));

	// 중복 · 빈 id 는 하나만 / 버린다.
	const TArray<FGrimSaveRecord> Dupes = { MakeRecord(TEXT("A"), 1, 1), MakeRecord(TEXT("A"), 1, 2), FGrimSaveRecord() };
	const TArray<FGrimSaveRecord> Deduped = FGrimSaveRecordMath::Merge(Dupes, {}, TSet<FName>());
	TestEqual(TEXT("중복은 앞의 것 하나"), Deduped.Num(), 1);
	TestEqual(TEXT("앞의 것"), Deduped[0].Bytes[0], static_cast<uint8>(1));
	return true;
}

// 지난 파일이 더 새 버전이면 옛 코드가 덮지 않는다.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimSaveKeepNewerTest, "Grimoire.Save.Records.KeepNewerVersion", GrimSaveTestsLocal::SaveTestFlags)
bool FGrimSaveKeepNewerTest::RunTest(const FString& Parameters)
{
	const TArray<FGrimSaveRecord> Existing = { MakeRecord(TEXT("Magic.Spellbook"), 2, 50), MakeRecord(TEXT("Survival.Vitals"), 1, 60) };
	const TArray<FGrimSaveRecord> Fresh = { MakeRecord(TEXT("Magic.Spellbook"), 1, 51), MakeRecord(TEXT("Survival.Vitals"), 2, 61) };
	int32 KeptNewer = 0;
	const TArray<FGrimSaveRecord> Merged = FGrimSaveRecordMath::Merge(Existing, Fresh, TSet<FName>(), &KeptNewer);
	TestEqual(TEXT("v2 마법서는 v1 이 덮지 않음"), Merged[0].Version, 2);
	TestEqual(TEXT("v2 바이트 그대로"), Merged[0].Bytes[0], static_cast<uint8>(50));
	TestEqual(TEXT("올린 버전(v1→v2)은 새 것으로"), Merged[1].Version, 2);
	TestEqual(TEXT("새 버전 유지 1"), KeptNewer, 1);

	TestTrue(TEXT("같은 버전 = 읽음"), FGrimSaveRecordMath::DecideRead(1, 1) == EGrimRecordReadDecision::Read);
	TestTrue(TEXT("옛 버전 = 읽음(참여자가 옮긴다)"), FGrimSaveRecordMath::DecideRead(1, 3) == EGrimRecordReadDecision::Read);
	TestTrue(TEXT("새 버전 = 건너뜀"), FGrimSaveRecordMath::DecideRead(4, 3) == EGrimRecordReadDecision::SkipNewer);
	TestTrue(TEXT("버전 0 = 잘못"), FGrimSaveRecordMath::DecideRead(0, 1) == EGrimRecordReadDecision::SkipInvalid);
	TestTrue(TEXT("음수 = 잘못"), FGrimSaveRecordMath::DecideRead(-2, 1) == EGrimRecordReadDecision::SkipInvalid);
	return true;
}

// 게임 중 없어진 액터의 몫은 다시 쓰지 않는다.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimSaveDroppedTest, "Grimoire.Save.Records.Dropped", GrimSaveTestsLocal::SaveTestFlags)
bool FGrimSaveDroppedTest::RunTest(const FString& Parameters)
{
	const TArray<FGrimSaveRecord> Existing = { MakeRecord(TEXT("Skillbook.Book_1"), 1, 1), MakeRecord(TEXT("Elemental.Candle_0"), 1, 2) };
	const TArray<FGrimSaveRecord> Fresh = { MakeRecord(TEXT("Elemental.Candle_0"), 1, 3), MakeRecord(TEXT("Skillbook.Book_1"), 1, 4) };
	TSet<FName> Dropped;
	Dropped.Add(TEXT("Skillbook.Book_1"));
	const TArray<FGrimSaveRecord> Merged = FGrimSaveRecordMath::Merge(Existing, Fresh, Dropped);
	TestEqual(TEXT("집은 책의 몫은 빠진다"), Ids(Merged), TArray<FString>({ TEXT("Elemental.Candle_0") }));
	return true;
}

// 읽는 순서: 우선 목록 → 나머지는 원래 순서.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimSaveLoadOrderTest, "Grimoire.Save.Records.LoadOrder", GrimSaveTestsLocal::SaveTestFlags)
bool FGrimSaveLoadOrderTest::RunTest(const FString& Parameters)
{
	const TArray<FName> Participants = { TEXT("Magic.Spellbook"), TEXT("Survival.Vitals"), TEXT("Quest.Tower"), TEXT("Magic.Progression"), TEXT("Player.Transform") };
	const TArray<FName> Priority = { TEXT("Magic.Progression"), TEXT("Nope.Missing"), TEXT("Magic.Spellbook") };
	TestEqual(TEXT("서클 → 마법서 → 나머지 원래 순서"), FGrimSaveRecordMath::MakeLoadOrder(Participants, Priority), TArray<int32>({ 3, 0, 1, 2, 4 }));
	TestEqual(TEXT("우선 목록 없음 = 그대로"), FGrimSaveRecordMath::MakeLoadOrder(Participants, {}), TArray<int32>({ 0, 1, 2, 3, 4 }));
	TestEqual(TEXT("빈 참여자"), FGrimSaveRecordMath::MakeLoadOrder({}, Priority).Num(), 0);
	return true;
}

// 슬롯 · PlayerId 다듬기와 로그인 옵션.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimSaveIdentityTest, "Grimoire.Save.Identity", GrimSaveTestsLocal::SaveTestFlags)
bool FGrimSaveIdentityTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("허용 문자만"), FGrimSaveRecordMath::SanitizeId(TEXT("My World!/..\\x?y=z")), FString(TEXT("MyWorldxyz")));
	TestEqual(TEXT("- _ 유지"), FGrimSaveRecordMath::SanitizeId(TEXT("Host-PC_01")), FString(TEXT("Host-PC_01")));
	TestEqual(TEXT("한글은 빠진다"), FGrimSaveRecordMath::SanitizeId(TEXT("세계1")), FString(TEXT("1")));
	TestEqual(TEXT("길이 자름"), FGrimSaveRecordMath::SanitizeId(FString::ChrN(100, TEXT('a')), 64).Len(), 64);
	TestTrue(TEXT("빈 값"), FGrimSaveRecordMath::SanitizeId(TEXT("../..")).IsEmpty());

	TestEqual(TEXT("로그인 옵션"), FGrimPlayerIdentity::ParseLoginOption(TEXT("/Game/Grimoire/Maps/L_M1_Sandbox?Name=Kim?GrimPlayerId=ABC-123")), FString(TEXT("ABC-123")));
	TestEqual(TEXT("옵션 순서 무관"), FGrimPlayerIdentity::ParseLoginOption(TEXT("/Game/Map?GrimPlayerId=Zed_9?Name=Kim")), FString(TEXT("Zed_9")));
	TestTrue(TEXT("옵션 없음"), FGrimPlayerIdentity::ParseLoginOption(TEXT("/Game/Map?Name=Kim")).IsEmpty());
	TestTrue(TEXT("빈 URL"), FGrimPlayerIdentity::ParseLoginOption(FString()).IsEmpty());
	TestEqual(TEXT("경로 문자는 다듬는다"), FGrimPlayerIdentity::ParseLoginOption(TEXT("/Game/Map?GrimPlayerId=../../evil")), FString(TEXT("evil")));
	return true;
}

// 봉투: 온전 · 깨짐 · 잘림 · 매직.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimSaveEnvelopeTest, "Grimoire.Save.File.Envelope", GrimSaveTestsLocal::SaveTestFlags)
bool FGrimSaveEnvelopeTest::RunTest(const FString& Parameters)
{
	TArray<uint8> Payload;
	for (int32 i = 0; i < 300; ++i)
	{
		Payload.Add(static_cast<uint8>(i * 7));
	}
	TArray<uint8> File;
	FGrimSaveFiles::Wrap(Payload, File);
	TestEqual(TEXT("머리 16바이트"), File.Num(), Payload.Num() + FGrimSaveFiles::HeaderSize);

	TArray<uint8> Out;
	TestTrue(TEXT("온전하면 열린다"), FGrimSaveFiles::Unwrap(File, Out));
	TestEqual(TEXT("내용 같음"), Out, Payload);

	TArray<uint8> Flipped = File;
	Flipped[FGrimSaveFiles::HeaderSize + 123] ^= 0x40;
	FString Error;
	TestFalse(TEXT("한 비트 바뀌면 CRC 실패"), FGrimSaveFiles::Unwrap(Flipped, Out, &Error));
	TestTrue(TEXT("까닭: CRC"), Error.Contains(TEXT("CRC")));

	TArray<uint8> Truncated(File.GetData(), File.Num() - 10);
	TestFalse(TEXT("잘리면 실패"), FGrimSaveFiles::Unwrap(Truncated, Out));

	TArray<uint8> BadMagic = File;
	BadMagic[0] = 'X';
	TestFalse(TEXT("매직이 다르면 실패"), FGrimSaveFiles::Unwrap(BadMagic, Out));
	TestFalse(TEXT("너무 짧음"), FGrimSaveFiles::Unwrap(TArray<uint8>({ 1, 2, 3 }), Out));

	TArray<uint8> Empty;
	TArray<uint8> EmptyFile;
	FGrimSaveFiles::Wrap(Empty, EmptyFile);
	TestTrue(TEXT("빈 페이로드도 열린다"), FGrimSaveFiles::Unwrap(EmptyFile, Out) && Out.Num() == 0);
	return true;
}

// SaveGame 직렬화 왕복: 레코드 · 스폰 액터 · 없앤 액터.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimSaveRoundTripTest, "Grimoire.Save.File.RoundTrip", GrimSaveTestsLocal::SaveTestFlags)
bool FGrimSaveRoundTripTest::RunTest(const FString& Parameters)
{
	UGrimSaveGame* Save = NewObject<UGrimSaveGame>();
	Save->Scope = EGrimSaveScope::World;
	Save->Slot = TEXT("RoundTrip");
	Save->SaveCount = 7;
	Save->SavedAtUtc = FDateTime(2026, 11, 5, 12, 0, 0);
	Save->Records = { MakeRecord(TEXT("Environment.TimeOfDay"), 1, 5), MakeRecord(TEXT("Quest.Future"), 9, 6) };
	Save->RemovedActors = { TEXT("GrimSkillbook_3") };
	FGrimSpawnedActorRecord& Spawned = Save->SpawnedActors.AddDefaulted_GetRef();
	Spawned.ActorName = TEXT("GrimBed_0");
	Spawned.ActorClass = FSoftClassPath(AGrimBed::StaticClass());
	Spawned.Transform = FTransform(FRotator(0.f, 90.f, 0.f), FVector(100.f, 200.f, 300.f));
	Spawned.Records = { MakeRecord(TEXT("Bed.GrimBed_0"), 1, 8) };

	TArray<uint8> Payload;
	if (!TestTrue(TEXT("직렬화"), FGrimSaveFiles::Serialize(Save, Payload)))
	{
		return false;
	}
	UGrimSaveGame* Loaded = FGrimSaveFiles::Deserialize(Payload);
	if (!TestNotNull(TEXT("역직렬화"), Loaded))
	{
		return false;
	}
	TestEqual(TEXT("슬롯"), Loaded->Slot, FString(TEXT("RoundTrip")));
	TestEqual(TEXT("저장 횟수"), Loaded->SaveCount, 7);
	TestEqual(TEXT("시각"), Loaded->SavedAtUtc, Save->SavedAtUtc);
	TestEqual(TEXT("레코드 id"), Ids(Loaded->Records), Ids(Save->Records));
	TestEqual(TEXT("모르는 레코드 버전 9"), Loaded->Records[1].Version, 9);
	TestEqual(TEXT("바이트"), Loaded->Records[1].Bytes, Save->Records[1].Bytes);
	TestEqual(TEXT("없앤 액터"), Loaded->RemovedActors.Num(), 1);
	if (TestEqual(TEXT("스폰 액터"), Loaded->SpawnedActors.Num(), 1))
	{
		TestEqual(TEXT("스폰 이름"), Loaded->SpawnedActors[0].ActorName, FName(TEXT("GrimBed_0")));
		TestEqual(TEXT("스폰 클래스"), Loaded->SpawnedActors[0].ActorClass.TryLoadClass<AActor>(), AGrimBed::StaticClass());
		TestTrue(TEXT("스폰 위치"), Loaded->SpawnedActors[0].Transform.GetLocation().Equals(FVector(100.f, 200.f, 300.f)));
		TestEqual(TEXT("스폰 레코드"), Ids(Loaded->SpawnedActors[0].Records), TArray<FString>({ TEXT("Bed.GrimBed_0") }));
	}
	// 쓰레기 페이로드는 시험하지 않는다 — LoadGameFromMemory 는 FName 을 읽다가 단언으로 멈춘다. 깨짐은 봉투의 CRC 가 먼저 거른다(File.Envelope).
	return true;
}

// 백업: 직전 파일 하나 보관, 깨진 본 파일은 백업으로, 깨진 파일이 좋은 백업을 밀어내지 않음.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimSaveBackupTest, "Grimoire.Save.File.Backup", GrimSaveTestsLocal::SaveTestFlags)
bool FGrimSaveBackupTest::RunTest(const FString& Parameters)
{
	FScopedTempDir Temp;
	const FString Path = FPaths::Combine(Temp.Dir, TEXT("WorldSave_Test.sav"));
	IFileManager& Files = IFileManager::Get();

	auto MakePayload = [](int32 SaveCount)
	{
		UGrimSaveGame* Save = NewObject<UGrimSaveGame>();
		Save->SaveCount = SaveCount;
		TArray<uint8> Payload;
		FGrimSaveFiles::Serialize(Save, Payload);
		return Payload;
	};
	auto Load = [&Path](bool& bUsedBackup, bool& bAnyFile) -> int32
	{
		TArray<FString> Problems;
		const UGrimSaveGame* Save = FGrimSaveFiles::LoadWithFallback(Path, bUsedBackup, bAnyFile, Problems);
		return Save ? Save->SaveCount : -1;
	};
	bool bUsedBackup = false, bAnyFile = false;

	TestEqual(TEXT("파일 없음"), Load(bUsedBackup, bAnyFile), -1);
	TestFalse(TEXT("파일 없음 표시"), bAnyFile);

	TestTrue(TEXT("1번 쓰기"), FGrimSaveFiles::WriteWithBackup(Path, MakePayload(1), true));
	TestFalse(TEXT("처음엔 백업 없음"), Files.FileExists(*FGrimSaveFiles::GetBackupPath(Path)));
	TestTrue(TEXT("2번 쓰기"), FGrimSaveFiles::WriteWithBackup(Path, MakePayload(2), true));
	TestTrue(TEXT("백업 생김"), Files.FileExists(*FGrimSaveFiles::GetBackupPath(Path)));
	TestFalse(TEXT("tmp 안 남음"), Files.FileExists(*FGrimSaveFiles::GetTempPath(Path)));
	TestEqual(TEXT("본 파일 = 2"), Load(bUsedBackup, bAnyFile), 2);
	TestFalse(TEXT("본 파일을 씀"), bUsedBackup);

	// 본 파일을 깨뜨린다 → 백업(1).
	TArray<uint8> Bytes;
	FFileHelper::LoadFileToArray(Bytes, *Path);
	Bytes[Bytes.Num() / 2] ^= 0xFF;
	FFileHelper::SaveArrayToFile(Bytes, *Path);
	TestEqual(TEXT("깨지면 백업 = 1"), Load(bUsedBackup, bAnyFile), 1);
	TestTrue(TEXT("백업을 씀"), bUsedBackup);

	// 깨진 본 파일 위에 쓰면 깨진 것은 버리고 좋은 백업(1)을 지킨다.
	TestTrue(TEXT("3번 쓰기"), FGrimSaveFiles::WriteWithBackup(Path, MakePayload(3), true));
	TArray<uint8> BackupPayload;
	TestTrue(TEXT("백업은 온전"), FGrimSaveFiles::ReadPayload(FGrimSaveFiles::GetBackupPath(Path), BackupPayload));
	const UGrimSaveGame* Backup = FGrimSaveFiles::Deserialize(BackupPayload);
	TestTrue(TEXT("백업은 여전히 1"), Backup && Backup->SaveCount == 1);
	TestEqual(TEXT("본 파일 = 3"), Load(bUsedBackup, bAnyFile), 3);

	// 백업 끄기: 이전 파일을 남기지 않는다(옛 .bak 은 그대로 둔다 — 덮을 것이 없으니).
	FGrimSaveFiles::DeleteAll(Path);
	FGrimSaveFiles::WriteWithBackup(Path, MakePayload(4), false);
	FGrimSaveFiles::WriteWithBackup(Path, MakePayload(5), false);
	TestFalse(TEXT("백업 끄면 .bak 없음"), Files.FileExists(*FGrimSaveFiles::GetBackupPath(Path)));
	TestEqual(TEXT("본 파일 = 5"), Load(bUsedBackup, bAnyFile), 5);
	return true;
}

// 속성 상태 참여자: 지속 상태(켜진 양초 + 전파)만 저장, 시간 제한 상태(젖음)는 빼고, 읽으면 없는 지속 상태는 지운다.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimSaveElementalTest, "Grimoire.Save.Participant.Elemental", GrimSaveTestsLocal::SaveTestFlags)
bool FGrimSaveElementalTest::RunTest(const FString& Parameters)
{
	FScopedSaveTestWorld TestWorld;
	auto MakeActor = [&TestWorld]()
	{
		AActor* Actor = TestWorld.World->SpawnActor<AActor>();
		UGrimElementalStateComponent* State = NewObject<UGrimElementalStateComponent>(Actor, TEXT("ElementalState"));
		State->RegisterComponent();
		return State;
	};

	UGrimElementalStateComponent* Source = MakeActor();
	TArray<FGrimElementalStateChange> Changes;
	FGrimElementalStateArray& SourceStates = const_cast<FGrimElementalStateArray&>(Source->GetStates());
	SourceStates.SetIntensity(GrimTags::State_Burning, 2.f, Changes);
	SourceStates.Find(GrimTags::State_Burning)->Propagation.Element = GrimTags::Element_Fire;
	SourceStates.Find(GrimTags::State_Burning)->Propagation.Radius = 120.f;
	SourceStates.Find(GrimTags::State_Burning)->Propagation.IntensityScale = 0.5f;
	SourceStates.SetIntensity(GrimTags::State_Wet, 1.f, Changes);
	SourceStates.Find(GrimTags::State_Wet)->RemainingTime = 20.f;

	TestTrue(TEXT("SaveId 에 액터 이름"), Source->GetSaveId().ToString().StartsWith(TEXT("Elemental.")));
	TestTrue(TEXT("세계 몫"), Source->GetSaveScope() == EGrimSaveScope::World);

	TArray<uint8> Bytes;
	{
		FMemoryWriter Writer(Bytes, true);
		Source->WriteSaveData(Writer);
	}

	// 대상: 처음부터 다른 지속 상태(탄 자국)와 시간 제한 젖음을 가진다.
	UGrimElementalStateComponent* Target = MakeActor();
	FGrimElementalStateArray& TargetStates = const_cast<FGrimElementalStateArray&>(Target->GetStates());
	TargetStates.SetIntensity(GrimInteractionTags::State_Charred, 1.f, Changes);
	TargetStates.SetIntensity(GrimTags::State_Wet, 2.f, Changes);
	TargetStates.Find(GrimTags::State_Wet)->RemainingTime = 5.f;
	{
		FMemoryReader Reader(Bytes, true);
		Target->ReadSaveData(Reader, Target->GetSaveVersion());
		TestFalse(TEXT("다 읽음"), Reader.IsError());
	}

	TestEqual(TEXT("불꽃 세기 이어짐"), Target->GetStateIntensity(GrimTags::State_Burning), 2.f);
	const FGrimElementalState* Burning = Target->GetStates().Find(GrimTags::State_Burning);
	if (TestNotNull(TEXT("불 상태"), Burning))
	{
		TestFalse(TEXT("불은 무한"), Burning->IsTimed());
		TestTrue(TEXT("전파 이어짐"), Burning->Propagation.IsActive());
		TestEqual(TEXT("전파 반경"), Burning->Propagation.Radius, 120.f);
		TestEqual(TEXT("전파 세기 배율"), Burning->Propagation.IntensityScale, 0.5f);
	}
	TestFalse(TEXT("세이브에 없는 지속 상태(탄 자국)는 지움"), Target->HasState(GrimInteractionTags::State_Charred));
	TestEqual(TEXT("시간 제한 상태(젖음)는 세이브와 무관 — 대상 것 그대로"), Target->GetStateIntensity(GrimTags::State_Wet), 2.f);
	TestTrue(TEXT("전파 등록"), Target->HasActivePropagation());

	// 깨진 바이트: 크래시 없이 오류만.
	TArray<uint8> Garbage = { 0xFF, 0xFF, 0xFF, 0x7F };
	FMemoryReader GarbageReader(Garbage, true);
	Target->ReadSaveData(GarbageReader, 1);
	TestTrue(TEXT("말도 안 되는 개수 = 오류"), GarbageReader.IsError());
	TestEqual(TEXT("오류면 그대로"), Target->GetStateIntensity(GrimTags::State_Burning), 2.f);
	return true;
}

// 모으기 · 적용: 모르는 레코드 무시, 새 버전 건너뜀, 스폰 액터는 컴포넌트만 참여자면 세계 몫이 아님.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimSaveApplyTest, "Grimoire.Save.Participant.Apply", GrimSaveTestsLocal::SaveTestFlags)
bool FGrimSaveApplyTest::RunTest(const FString& Parameters)
{
	FScopedSaveTestWorld TestWorld;
	AActor* Actor = TestWorld.World->SpawnActor<AActor>();
	UGrimElementalStateComponent* State = NewObject<UGrimElementalStateComponent>(Actor, TEXT("ElementalState"));
	State->RegisterComponent();
	FActorSpawnParameters BedParams;
	BedParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGrimBed* Bed = TestWorld.World->SpawnActor<AGrimBed>(FVector::ZeroVector, FRotator::ZeroRotator, BedParams);

	TestFalse(TEXT("스폰한 액터는 놓인 액터가 아님"), UGrimSaveSubsystem::IsPlacedActor(Actor));

	TArray<FGrimSaveParticipantRef> Participants;
	UGrimSaveSubsystem::GatherActorParticipants(Actor, EGrimSaveScope::World, Participants);
	UGrimSaveSubsystem::GatherActorParticipants(Bed, EGrimSaveScope::World, Participants);
	TestEqual(TEXT("속성 상태 + 침대"), Participants.Num(), 2);
	TArray<FGrimSaveParticipantRef> PlayerOnly;
	UGrimSaveSubsystem::GatherActorParticipants(Actor, EGrimSaveScope::Player, PlayerOnly);
	TestEqual(TEXT("범위가 다르면 안 모음"), PlayerOnly.Num(), 0);

	if (UGrimSaveSubsystem* Save = UGrimSaveSubsystem::Get(TestWorld.World))
	{
		TArray<FGrimSaveParticipantRef> TopLevel;
		TArray<AActor*> Spawned;
		Save->GatherWorldParticipants(TopLevel, Spawned);
		TestTrue(TEXT("스폰 침대(스스로 참여자)는 다시 스폰할 몫"), Spawned.Contains(Bed));
		TestFalse(TEXT("컴포넌트만 참여자인 스폰 액터는 저장 안 함"), Spawned.Contains(Actor)
			|| TopLevel.ContainsByPredicate([State](const FGrimSaveParticipantRef& Ref) { return Ref.Object.Get() == State; }));
		TestTrue(TEXT("시간 서브시스템은 세계 몫"), TopLevel.ContainsByPredicate([](const FGrimSaveParticipantRef& Ref) { return Ref.SaveId == FName(TEXT("Environment.TimeOfDay")); }));
	}

	TArray<FGrimSaveRecord> Records;
	UGrimSaveSubsystem::WriteRecords(Participants, Records);
	TestEqual(TEXT("레코드 둘"), Records.Num(), 2);
	TestTrue(TEXT("버전 1"), Records.Num() == 2 && Records[0].Version == 1 && Records[1].Version == 1);

	// 침대 레코드를 침낭으로 바꾸고, 속성 레코드는 더 새 버전으로, 모르는 레코드 하나 더.
	TArray<FGrimSaveRecord> Incoming;
	{
		FGrimSaveRecord BedRecord;
		BedRecord.SaveId = Bed->GetSaveId();
		BedRecord.Version = 1;
		FMemoryWriter Writer(BedRecord.Bytes, true);
		bool bBedroll = true;
		Writer << bBedroll;
		Incoming.Add(BedRecord);
	}
	{
		FGrimSaveRecord Newer = Records[0];
		Newer.Version = 99;
		Newer.Bytes = { 0xFF, 0xFF, 0xFF, 0x7F };
		Incoming.Add(Newer);
	}
	Incoming.Add(MakeRecord(TEXT("Quest.NotHere"), 1, 1));

	AddExpectedMessage(TEXT("더 새 버전"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	const int32 Applied = UGrimSaveSubsystem::ApplyRecords(Participants, Incoming, {}, TEXT("테스트"));
	TestEqual(TEXT("침대만 적용(새 버전 · 모르는 레코드 제외)"), Applied, 1);
	TestTrue(TEXT("침낭이 됨"), Bed->IsBedroll());
	return true;
}

#endif
