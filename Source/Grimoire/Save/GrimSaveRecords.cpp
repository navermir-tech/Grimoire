#include "Save/GrimSaveRecords.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

DEFINE_LOG_CATEGORY(LogGrimSave);

// ───────────────────────── 레코드 ─────────────────────────

EGrimRecordReadDecision FGrimSaveRecordMath::DecideRead(int32 StoredVersion, int32 CodeVersion)
{
	if (StoredVersion <= 0)
	{
		return EGrimRecordReadDecision::SkipInvalid;
	}
	if (StoredVersion > CodeVersion)
	{
		return EGrimRecordReadDecision::SkipNewer;
	}
	return EGrimRecordReadDecision::Read;
}

const FGrimSaveRecord* FGrimSaveRecordMath::Find(const TArray<FGrimSaveRecord>& Records, FName SaveId)
{
	return Records.FindByPredicate([SaveId](const FGrimSaveRecord& Record) { return Record.SaveId == SaveId; });
}

TArray<FGrimSaveRecord> FGrimSaveRecordMath::Merge(const TArray<FGrimSaveRecord>& Existing, const TArray<FGrimSaveRecord>& Fresh,
	const TSet<FName>& Dropped, int32* OutKeptNewer)
{
	TArray<FGrimSaveRecord> Result;
	Result.Reserve(Existing.Num() + Fresh.Num());
	TSet<FName> Seen;
	int32 KeptNewer = 0;

	for (const FGrimSaveRecord& Old : Existing)
	{
		if (Old.SaveId.IsNone() || Dropped.Contains(Old.SaveId) || Seen.Contains(Old.SaveId))
		{
			continue;
		}
		Seen.Add(Old.SaveId);
		const FGrimSaveRecord* New = Find(Fresh, Old.SaveId);
		if (New && Old.Version > New->Version)
		{
			++KeptNewer;
			Result.Add(Old);
		}
		else
		{
			Result.Add(New ? *New : Old);
		}
	}

	for (const FGrimSaveRecord& New : Fresh)
	{
		if (New.SaveId.IsNone() || Dropped.Contains(New.SaveId) || Seen.Contains(New.SaveId))
		{
			continue;
		}
		Seen.Add(New.SaveId);
		Result.Add(New);
	}

	if (OutKeptNewer)
	{
		*OutKeptNewer = KeptNewer;
	}
	return Result;
}

TArray<int32> FGrimSaveRecordMath::MakeLoadOrder(const TArray<FName>& Ids, const TArray<FName>& Priority)
{
	TArray<int32> Order;
	Order.Reserve(Ids.Num());
	TArray<bool> Used;
	Used.SetNumZeroed(Ids.Num());

	for (const FName& First : Priority)
	{
		for (int32 i = 0; i < Ids.Num(); ++i)
		{
			if (!Used[i] && Ids[i] == First)
			{
				Used[i] = true;
				Order.Add(i);
			}
		}
	}
	for (int32 i = 0; i < Ids.Num(); ++i)
	{
		if (!Used[i])
		{
			Order.Add(i);
		}
	}
	return Order;
}

FString FGrimSaveRecordMath::SanitizeId(const FString& In, int32 MaxLen)
{
	FString Out;
	Out.Reserve(FMath::Min(In.Len(), MaxLen));
	for (const TCHAR Char : In)
	{
		if (Out.Len() >= MaxLen)
		{
			break;
		}
		if ((Char >= TEXT('a') && Char <= TEXT('z')) || (Char >= TEXT('A') && Char <= TEXT('Z')) || (Char >= TEXT('0') && Char <= TEXT('9'))
			|| Char == TEXT('-') || Char == TEXT('_'))
		{
			Out.AppendChar(Char);
		}
	}
	return Out;
}

// ───────────────────────── 파일 ─────────────────────────

void FGrimSaveFiles::Wrap(const TArray<uint8>& Payload, TArray<uint8>& OutFile)
{
	OutFile.Reset(HeaderSize + Payload.Num());
	FMemoryWriter Writer(OutFile, true);
	uint32 MagicValue = Magic;
	uint32 Version = EnvelopeVersion;
	uint32 Size = static_cast<uint32>(Payload.Num());
	uint32 Crc = FCrc::MemCrc32(Payload.GetData(), Payload.Num());
	Writer << MagicValue << Version << Size << Crc;
	OutFile.Append(Payload);
}

bool FGrimSaveFiles::Unwrap(const TArray<uint8>& File, TArray<uint8>& OutPayload, FString* OutError)
{
	auto Fail = [OutError](const TCHAR* Why)
	{
		if (OutError)
		{
			*OutError = Why;
		}
		return false;
	};

	if (File.Num() < HeaderSize)
	{
		return Fail(TEXT("파일이 머리보다 짧다"));
	}
	FMemoryReader Reader(File, true);
	uint32 MagicValue = 0, Version = 0, Size = 0, Crc = 0;
	Reader << MagicValue << Version << Size << Crc;
	if (MagicValue != Magic)
	{
		return Fail(TEXT("매직이 다르다(그리모리 세이브가 아님)"));
	}
	if (Version == 0 || Version > EnvelopeVersion)
	{
		return Fail(TEXT("모르는 봉투 버전"));
	}
	if (static_cast<int64>(Size) != static_cast<int64>(File.Num()) - HeaderSize)
	{
		return Fail(TEXT("크기가 맞지 않는다(잘렸거나 덧붙음)"));
	}
	const uint8* Data = File.GetData() + HeaderSize;
	if (FCrc::MemCrc32(Data, static_cast<int32>(Size)) != Crc)
	{
		return Fail(TEXT("CRC 가 맞지 않는다(깨짐)"));
	}
	OutPayload.Reset(Size);
	OutPayload.Append(Data, static_cast<int32>(Size));
	return true;
}

bool FGrimSaveFiles::Serialize(UGrimSaveGame* SaveGame, TArray<uint8>& OutPayload)
{
	OutPayload.Reset();
	return SaveGame && UGameplayStatics::SaveGameToMemory(SaveGame, OutPayload) && OutPayload.Num() > 0;
}

UGrimSaveGame* FGrimSaveFiles::Deserialize(const TArray<uint8>& Payload, FString* OutError)
{
	UGrimSaveGame* SaveGame = Cast<UGrimSaveGame>(UGameplayStatics::LoadGameFromMemory(Payload));
	if (!SaveGame)
	{
		if (OutError)
		{
			*OutError = TEXT("SaveGame 역직렬화 실패");
		}
		return nullptr;
	}
	if (SaveGame->FormatVersion <= 0 || SaveGame->FormatVersion > UGrimSaveGame::CurrentFormatVersion)
	{
		if (OutError)
		{
			*OutError = FString::Printf(TEXT("모르는 형식 버전 %d"), SaveGame->FormatVersion);
		}
		return nullptr;
	}
	return SaveGame;
}

bool FGrimSaveFiles::WriteWithBackup(const FString& Path, const TArray<uint8>& Payload, bool bKeepBackup, FString* OutError)
{
	IFileManager& Files = IFileManager::Get();
	Files.MakeDirectory(*FPaths::GetPath(Path), true);

	TArray<uint8> FileBytes;
	Wrap(Payload, FileBytes);

	const FString TempPath = GetTempPath(Path);
	if (!FFileHelper::SaveArrayToFile(FileBytes, *TempPath))
	{
		if (OutError)
		{
			*OutError = FString::Printf(TEXT("임시 파일을 쓸 수 없다: %s"), *TempPath);
		}
		return false;
	}

	if (Files.FileExists(*Path))
	{
		// 지금 파일이 온전할 때만 백업으로 돌린다 — 깨진 파일이 좋은 백업을 밀어내지 않게.
		TArray<uint8> Unused;
		const bool bCurrentValid = ReadPayload(Path, Unused);
		const FString BackupPath = GetBackupPath(Path);
		if (bKeepBackup && bCurrentValid)
		{
			Files.Move(*BackupPath, *Path, true, true);
		}
		else
		{
			Files.Delete(*Path, false, true, true);
		}
	}

	if (!Files.Move(*Path, *TempPath, true, true))
	{
		if (OutError)
		{
			*OutError = FString::Printf(TEXT("임시 파일을 제자리로 옮길 수 없다: %s"), *Path);
		}
		return false;
	}
	return true;
}

bool FGrimSaveFiles::ReadPayload(const FString& Path, TArray<uint8>& OutPayload, FString* OutError)
{
	TArray<uint8> FileBytes;
	if (!FFileHelper::LoadFileToArray(FileBytes, *Path, FILEREAD_Silent))
	{
		if (OutError)
		{
			*OutError = TEXT("파일 없음");
		}
		return false;
	}
	return Unwrap(FileBytes, OutPayload, OutError);
}

UGrimSaveGame* FGrimSaveFiles::LoadWithFallback(const FString& Path, bool& bOutUsedBackup, bool& bOutAnyFileExisted, TArray<FString>& OutProblems)
{
	bOutUsedBackup = false;
	bOutAnyFileExisted = false;
	IFileManager& Files = IFileManager::Get();

	const FString Candidates[] = { Path, GetBackupPath(Path) };
	for (int32 i = 0; i < UE_ARRAY_COUNT(Candidates); ++i)
	{
		const FString& Candidate = Candidates[i];
		if (!Files.FileExists(*Candidate))
		{
			continue;
		}
		bOutAnyFileExisted = true;

		FString Error;
		TArray<uint8> Payload;
		if (!ReadPayload(Candidate, Payload, &Error))
		{
			OutProblems.Add(FString::Printf(TEXT("%s: %s"), *FPaths::GetCleanFilename(Candidate), *Error));
			continue;
		}
		if (UGrimSaveGame* SaveGame = Deserialize(Payload, &Error))
		{
			bOutUsedBackup = (i > 0);
			return SaveGame;
		}
		OutProblems.Add(FString::Printf(TEXT("%s: %s"), *FPaths::GetCleanFilename(Candidate), *Error));
	}
	return nullptr;
}

void FGrimSaveFiles::DeleteAll(const FString& Path)
{
	IFileManager& Files = IFileManager::Get();
	Files.Delete(*Path, false, true, true);
	Files.Delete(*GetBackupPath(Path), false, true, true);
	Files.Delete(*GetTempPath(Path), false, true, true);
}
