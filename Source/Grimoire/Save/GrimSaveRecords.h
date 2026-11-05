#pragma once

#include "CoreMinimal.h"
#include "Save/GrimSaveGame.h"

GRIMOIRE_API DECLARE_LOG_CATEGORY_EXTERN(LogGrimSave, Log, All);

/** 저장된 레코드를 지금 코드가 읽어도 되나. */
enum class EGrimRecordReadDecision : uint8
{
	Read,
	/** 버전 0 이하 — 잘못 쓰인 레코드 */
	SkipInvalid,
	/** 지금 코드보다 새 버전 — 읽지 않고, 다시 쓸 때도 덮지 않는다 */
	SkipNewer
};

/** 레코드 병합 · 버전 처리 · 이름 다듬기 — 순수 계산(Grimoire.Save.* 테스트). */
struct GRIMOIRE_API FGrimSaveRecordMath
{
	static EGrimRecordReadDecision DecideRead(int32 StoredVersion, int32 CodeVersion);

	/**
	 * 다시 쓸 레코드 목록. Existing(지난 파일) 순서를 지키며 같은 SaveId 는 Fresh(지금 참여자가 쓴 것)로 바꾼다.
	 *  - Existing 쪽 버전이 Fresh 보다 높으면 Existing 을 남긴다(옛 코드가 새 데이터를 덮지 않게).
	 *  - Fresh 에만 있는 id 는 끝에 Fresh 순서로 더한다. Fresh 에 없는 Existing 레코드(모르는 시스템 · 안 올라온 액터)는 그대로.
	 *  - Dropped 의 id(게임 중 없어진 액터의 몫)는 뺀다. 결과에 같은 id 는 하나뿐(Existing 안의 중복은 앞의 것).
	 */
	static TArray<FGrimSaveRecord> Merge(const TArray<FGrimSaveRecord>& Existing, const TArray<FGrimSaveRecord>& Fresh,
		const TSet<FName>& Dropped, int32* OutKeptNewer = nullptr);

	static const FGrimSaveRecord* Find(const TArray<FGrimSaveRecord>& Records, FName SaveId);

	/** Ids 를 읽을 순서(인덱스): Priority 에 있는 id 가 그 순서로 먼저, 나머지는 원래 순서. */
	static TArray<int32> MakeLoadOrder(const TArray<FName>& Ids, const TArray<FName>& Priority);

	/** 파일 이름 · 슬롯 · PlayerId 에 쓸 수 있게: 영문 · 숫자 · '-' · '_' 만 남기고 MaxLen 자로 자른다. */
	static FString SanitizeId(const FString& In, int32 MaxLen = 64);
};

/**
 * 세이브 파일 읽고 쓰기. 파일 = 봉투(매직 'GRMS' · 봉투 버전 · 페이로드 크기 · CRC32) + SaveGameToMemory 결과.
 * 쓸 때: <파일>.tmp 에 쓰고 → 지금 파일이 온전하면 <파일>.bak 으로(직전 파일 하나 보관) → tmp 를 제자리로.
 * 읽을 때: 본 파일이 깨졌으면(봉투 · CRC · 역직렬화 실패) .bak 으로.
 */
struct GRIMOIRE_API FGrimSaveFiles
{
	static constexpr uint32 Magic = 0x534D5247; // 'G' 'R' 'M' 'S'
	static constexpr uint32 EnvelopeVersion = 1;
	static constexpr int32 HeaderSize = 16;

	static void Wrap(const TArray<uint8>& Payload, TArray<uint8>& OutFile);
	static bool Unwrap(const TArray<uint8>& File, TArray<uint8>& OutPayload, FString* OutError = nullptr);

	static FString GetBackupPath(const FString& Path) { return Path + TEXT(".bak"); }
	static FString GetTempPath(const FString& Path) { return Path + TEXT(".tmp"); }

	static bool Serialize(UGrimSaveGame* SaveGame, TArray<uint8>& OutPayload);
	static UGrimSaveGame* Deserialize(const TArray<uint8>& Payload, FString* OutError = nullptr);

	/** 봉투로 싸서 Path 에 쓴다(백업 회전 포함). */
	static bool WriteWithBackup(const FString& Path, const TArray<uint8>& Payload, bool bKeepBackup, FString* OutError = nullptr);

	/** Path 를 읽어 봉투를 연다. */
	static bool ReadPayload(const FString& Path, TArray<uint8>& OutPayload, FString* OutError = nullptr);

	/** 본 파일 → 백업 순으로 처음 온전한 세이브. 없거나 둘 다 깨졌으면 nullptr. */
	static UGrimSaveGame* LoadWithFallback(const FString& Path, bool& bOutUsedBackup, bool& bOutAnyFileExisted, TArray<FString>& OutProblems);

	/** 본 파일 · 백업 · tmp 를 지운다. */
	static void DeleteAll(const FString& Path);
};
