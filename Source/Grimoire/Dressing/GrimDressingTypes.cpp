#include "Dressing/GrimDressingTypes.h"
#include "Dressing/GrimDressingSettings.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/StrongObjectPtr.h"

DEFINE_LOG_CATEGORY(LogGrimDressing);

namespace GrimDressingDataLocal
{
	struct FCachedTable
	{
		TStrongObjectPtr<UDataTable> Table;
		bool bTried = false;
	};

	FCachedTable GProps;
	FCachedTable GHarvest;
	FCachedTable GJournal;

	const UDataTable* GetOrLoad(FCachedTable& Cache, const FString& Path, UScriptStruct* RowStruct, const TCHAR* What)
	{
		if (!Cache.bTried)
		{
			Cache.bTried = true;
			TArray<FString> Problems;
			UDataTable* Table = FGrimDressingData::LoadCSV(Path, RowStruct, Problems);
			for (const FString& Problem : Problems)
			{
				UE_LOG(LogGrimDressing, Warning, TEXT("%s 표 %s: %s"), What, *Path, *Problem);
			}
			if (Table)
			{
				UE_LOG(LogGrimDressing, Log, TEXT("%s 표 %d 행을 읽었다: %s"), What, Table->GetRowMap().Num(), *Path);
			}
			Cache.Table.Reset(Table);
		}
		return Cache.Table.Get();
	}

	template <typename RowType>
	const RowType* FindRow(const UDataTable* Table, FName RowName)
	{
		if (!Table || RowName.IsNone())
		{
			return nullptr;
		}
		const uint8* const* Found = Table->GetRowMap().Find(RowName);
		return Found ? reinterpret_cast<const RowType*>(*Found) : nullptr;
	}
}

UDataTable* FGrimDressingData::LoadCSV(const FString& ContentRelativePath, UScriptStruct* RowStruct, TArray<FString>& OutProblems)
{
	const FString FilePath = FPaths::Combine(FPaths::ProjectContentDir(), ContentRelativePath);
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *FilePath))
	{
		OutProblems.Add(FString::Printf(TEXT("파일을 열 수 없다: %s"), *FilePath));
		return nullptr;
	}
	UDataTable* Table = NewObject<UDataTable>(GetTransientPackage(), MakeUniqueObjectName(GetTransientPackage(), UDataTable::StaticClass(), FName(*FPaths::GetBaseFilename(FilePath))));
	Table->RowStruct = RowStruct;
	OutProblems.Append(Table->CreateTableFromCSVString(Text));
	if (Table->GetRowMap().Num() == 0)
	{
		OutProblems.Add(TEXT("행이 없다"));
		return nullptr;
	}
	return Table;
}

const UDataTable* FGrimDressingData::GetPropTable()
{
	return GrimDressingDataLocal::GetOrLoad(GrimDressingDataLocal::GProps, GetDefault<UGrimDressingSettings>()->ReactivePropsCSV, FGrimReactivePropRow::StaticStruct(), TEXT("반응 소품"));
}

const UDataTable* FGrimDressingData::GetHarvestTable()
{
	return GrimDressingDataLocal::GetOrLoad(GrimDressingDataLocal::GHarvest, GetDefault<UGrimDressingSettings>()->HarvestNodesCSV, FGrimHarvestNodeRow::StaticStruct(), TEXT("채집점"));
}

const UDataTable* FGrimDressingData::GetJournalTable()
{
	return GrimDressingDataLocal::GetOrLoad(GrimDressingDataLocal::GJournal, GetDefault<UGrimDressingSettings>()->JournalCSV, FGrimJournalRow::StaticStruct(), TEXT("도감"));
}

const FGrimReactivePropRow* FGrimDressingData::FindProp(FName PropId)
{
	return GrimDressingDataLocal::FindRow<FGrimReactivePropRow>(GetPropTable(), PropId);
}

const FGrimHarvestNodeRow* FGrimDressingData::FindHarvestNode(FName Kind)
{
	return GrimDressingDataLocal::FindRow<FGrimHarvestNodeRow>(GetHarvestTable(), Kind);
}

const FGrimJournalRow* FGrimDressingData::FindJournal(FName JournalId)
{
	return GrimDressingDataLocal::FindRow<FGrimJournalRow>(GetJournalTable(), JournalId);
}

void FGrimDressingData::Reset()
{
	using namespace GrimDressingDataLocal;
	for (FCachedTable* Cache : { &GProps, &GHarvest, &GJournal })
	{
		Cache->Table.Reset();
		Cache->bTried = false;
	}
}

void FGrimDressingData::SetTablesForTest(UDataTable* Props, UDataTable* Harvest, UDataTable* Journal)
{
	using namespace GrimDressingDataLocal;
	const TPair<FCachedTable*, UDataTable*> Pairs[] = { { &GProps, Props }, { &GHarvest, Harvest }, { &GJournal, Journal } };
	for (const TPair<FCachedTable*, UDataTable*>& Pair : Pairs)
	{
		if (Pair.Value)
		{
			Pair.Key->Table.Reset(Pair.Value);
			Pair.Key->bTried = true;
		}
	}
}

TArray<FGrimRewardEntry> FGrimDressingData::ParseRewards(const FString& Text, TArray<FString>* OutProblems)
{
	TArray<FGrimRewardEntry> Entries;
	TArray<FString> Parts;
	Text.ParseIntoArray(Parts, TEXT("|"), true);
	for (const FString& RawPart : Parts)
	{
		const FString Part = RawPart.TrimStartAndEnd();
		TArray<FString> Fields;
		Part.ParseIntoArray(Fields, TEXT(":"), false);
		if (Fields.Num() < 1 || Fields[0].TrimStartAndEnd().IsEmpty())
		{
			if (OutProblems) OutProblems->Add(FString::Printf(TEXT("보상 조각을 못 읽음: %s"), *Part));
			continue;
		}
		FGrimRewardEntry Entry;
		Entry.Item = FName(*Fields[0].TrimStartAndEnd());
		if (Fields.Num() >= 2)
		{
			FString MinText, MaxText;
			if (Fields[1].Split(TEXT("-"), &MinText, &MaxText))
			{
				Entry.Min = FCString::Atoi(*MinText);
				Entry.Max = FCString::Atoi(*MaxText);
			}
			else
			{
				Entry.Min = Entry.Max = FCString::Atoi(*Fields[1]);
			}
		}
		if (Fields.Num() >= 3)
		{
			Entry.Weight = FCString::Atof(*Fields[2]);
		}
		if (Entry.Min < 1 || Entry.Max < Entry.Min || Entry.Weight <= 0.f)
		{
			if (OutProblems) OutProblems->Add(FString::Printf(TEXT("보상 수 · 무게가 틀림: %s"), *Part));
			continue;
		}
		Entries.Add(Entry);
	}
	return Entries;
}

FGrimRewardEntry FGrimDressingData::PickReward(const TArray<FGrimRewardEntry>& Entries, float NothingWeight, float Roll)
{
	float Total = FMath::Max(0.f, NothingWeight);
	for (const FGrimRewardEntry& Entry : Entries)
	{
		Total += Entry.Weight;
	}
	FGrimRewardEntry Nothing;
	Nothing.Item = NAME_None;
	if (Total <= 0.f)
	{
		return Nothing;
	}
	float Pick = FMath::Clamp(Roll, 0.f, 0.99999f) * Total;
	for (const FGrimRewardEntry& Entry : Entries)
	{
		if (Pick < Entry.Weight)
		{
			return Entry;
		}
		Pick -= Entry.Weight;
	}
	return Nothing;
}
