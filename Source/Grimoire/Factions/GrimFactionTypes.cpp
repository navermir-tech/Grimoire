#include "Factions/GrimFactionTypes.h"
#include "Quest/GrimQuestSettings.h"
#include "Quest/GrimQuestTypes.h"
#include "UObject/Package.h"

namespace GrimTowerServices
{
	const FName Trial(TEXT("Trial"));
	const FName Purify(TEXT("Purify"));
	const FName Repair(TEXT("Repair"));
	const FName Erase(TEXT("Erase"));

	bool ParsePurify(FName Service, FName& OutTreeId)
	{
		const FString Text = Service.ToString();
		if (!Text.StartsWith(TEXT("Purify:")) || Text.Len() <= 7)
		{
			return false;
		}
		OutTreeId = FName(*Text.Mid(7));
		return true;
	}

	bool IsKnown(FName Service)
	{
		FName Tree;
		return Service == Trial || Service == Purify || Service == Repair || Service == Erase || ParsePurify(Service, Tree);
	}
}

namespace GrimDialogueContexts
{
	const FName Talk(TEXT("Talk"));
	const FName Menu(TEXT("Menu"));
	const FName TrialStarted(TEXT("Trial.Started"));
	const FName TrialNotReady(TEXT("Trial.NotReady"));
	const FName TrialLocked(TEXT("Trial.Locked"));
	const FName TrialNone(TEXT("Trial.None"));
	const FName TrialBusy(TEXT("Trial.Busy"));
	const FName Purify(TEXT("Purify"));
	const FName PurifyEmpty(TEXT("Purify.Empty"));
	const FName Repair(TEXT("Repair"));
	const FName Erase(TEXT("Erase"));
	const FName Refused(TEXT("Refused"));

	TArray<FName> RequiredForServices()
	{
		return { Menu, TrialStarted, TrialNotReady, TrialLocked, TrialNone, TrialBusy, Purify, PurifyEmpty, Repair, Erase, Refused };
	}
}

namespace GrimFactionTypesLocal
{
	FGrimDialogueDatabase GDatabase;
	bool GLoaded = false;

	TArray<FString> SplitBar(const FString& Text)
	{
		TArray<FString> Parts;
		Text.ParseIntoArray(Parts, TEXT("|"), true);
		for (FString& Part : Parts)
		{
			Part.TrimStartAndEndInline();
		}
		Parts.RemoveAll([](const FString& Part) { return Part.IsEmpty(); });
		return Parts;
	}

	UDataTable* MakeTable(UScriptStruct* RowStruct, const TCHAR* BaseName, const FString& CSV, TArray<FString>& OutProblems)
	{
		UDataTable* Table = NewObject<UDataTable>(GetTransientPackage(), MakeUniqueObjectName(GetTransientPackage(), UDataTable::StaticClass(), BaseName));
		Table->RowStruct = RowStruct;
		for (const FString& Problem : Table->CreateTableFromCSVString(CSV))
		{
			OutProblems.Add(FString::Printf(TEXT("%s: %s"), BaseName, *Problem));
		}
		return Table;
	}

	/** 행이 지금 진행에 맞으면 점수(클수록 구체적), 안 맞으면 INDEX_NONE */
	int32 Score(const FGrimDialogueEntry& Entry, const FGrimQuestLog& Log)
	{
		int32 Base = 0;
		if (!Entry.Quest.IsNone())
		{
			const FGrimQuestProgress* Progress = Log.FindActive(Entry.Quest);
			if (Entry.Step >= 0)
			{
				if (!Progress || Progress->Step != Entry.Step)
				{
					return INDEX_NONE;
				}
				Base = 3000;
			}
			else if (Entry.Step == -1)
			{
				if (!Progress)
				{
					return INDEX_NONE;
				}
				Base = 2000;
			}
			else
			{
				if (!Log.IsCompleted(Entry.Quest))
				{
					return INDEX_NONE;
				}
				Base = 1000;
			}
		}
		return Base + FMath::Clamp(Entry.Priority, -999, 999) + 999;
	}
}

const FGrimNpcDef* FGrimDialogueDatabase::FindNpc(FName NpcId) const
{
	return Npcs.FindByPredicate([NpcId](const FGrimNpcDef& Npc) { return Npc.Id == NpcId; });
}

const FGrimDialogueEntry* FGrimDialogueDatabase::Select(FName NpcId, FName Context, const FGrimQuestLog& Log) const
{
	const FGrimDialogueEntry* Best = nullptr;
	int32 BestScore = INDEX_NONE;
	for (const FGrimDialogueEntry& Entry : Entries)
	{
		if (Entry.Context != Context || (!Entry.Npc.IsNone() && Entry.Npc != NpcId))
		{
			continue;
		}
		// 그 NPC 의 행이 모든 NPC 행보다 앞선다(같은 조건이면).
		const int32 RowScore = GrimFactionTypesLocal::Score(Entry, Log);
		if (RowScore == INDEX_NONE)
		{
			continue;
		}
		const int32 Total = RowScore * 2 + (Entry.Npc.IsNone() ? 0 : 1);
		if (Total > BestScore)
		{
			BestScore = Total;
			Best = &Entry;
		}
	}
	return Best;
}

bool FGrimDialogueDatabase::Build(const UDataTable* NpcTable, const UDataTable* DialogueTable, const FGrimQuestDatabase* Quests, FGrimDialogueDatabase& Out, TArray<FString>& OutProblems)
{
	using namespace GrimFactionTypesLocal;
	const int32 ProblemsBefore = OutProblems.Num();
	Out = FGrimDialogueDatabase();

	if (!NpcTable || NpcTable->GetRowStruct() != FGrimNpcRow::StaticStruct() || !DialogueTable || DialogueTable->GetRowStruct() != FGrimDialogueRow::StaticStruct())
	{
		OutProblems.Add(TEXT("NPC · 대사 표가 없거나 행 구조체가 다르다"));
		return false;
	}

	NpcTable->ForeachRow<FGrimNpcRow>(TEXT("GrimDialogue"), [&](const FName& Key, const FGrimNpcRow& Row)
	{
		FGrimNpcDef& Npc = Out.Npcs.AddDefaulted_GetRef();
		Npc.Id = Key;
		Npc.DisplayName = Row.DisplayName;
		Npc.Title = Row.Title;
		for (const FString& Service : SplitBar(Row.Services))
		{
			Npc.Services.Add(FName(*Service));
		}
		Npc.ServiceLabels = SplitBar(Row.ServiceLabels);
		if (Npc.DisplayName.IsEmpty())
		{
			OutProblems.Add(FString::Printf(TEXT("NPC %s: 이름이 비었다"), *Key.ToString()));
		}
		if (Npc.Services.Num() != Npc.ServiceLabels.Num())
		{
			OutProblems.Add(FString::Printf(TEXT("NPC %s: 서비스 %d 개와 글씨 %d 개가 다르다"), *Key.ToString(), Npc.Services.Num(), Npc.ServiceLabels.Num()));
		}
		for (const FName Service : Npc.Services)
		{
			if (!GrimTowerServices::IsKnown(Service))
			{
				OutProblems.Add(FString::Printf(TEXT("NPC %s: 모르는 서비스 %s"), *Key.ToString(), *Service.ToString()));
			}
		}
	});

	DialogueTable->ForeachRow<FGrimDialogueRow>(TEXT("GrimDialogue"), [&](const FName& Key, const FGrimDialogueRow& Row)
	{
		FGrimDialogueEntry& Entry = Out.Entries.AddDefaulted_GetRef();
		Entry.RowName = Key;
		Entry.Npc = Row.Npc;
		Entry.Context = Row.Context;
		Entry.Quest = Row.Quest;
		Entry.Step = Row.Step;
		Entry.Priority = Row.Priority;
		Entry.Lines = SplitBar(Row.Lines);
		if (!Row.Npc.IsNone() && !Out.FindNpc(Row.Npc))
		{
			OutProblems.Add(FString::Printf(TEXT("대사 %s: 모르는 NPC %s"), *Key.ToString(), *Row.Npc.ToString()));
		}
		if (Row.Context.IsNone())
		{
			OutProblems.Add(FString::Printf(TEXT("대사 %s: Context 가 비었다"), *Key.ToString()));
		}
		if (Entry.Lines.Num() == 0)
		{
			OutProblems.Add(FString::Printf(TEXT("대사 %s: 줄이 없다"), *Key.ToString()));
		}
		if (Row.Step < -2)
		{
			OutProblems.Add(FString::Printf(TEXT("대사 %s: Step 은 -2 이상"), *Key.ToString()));
		}
		if (!Row.Quest.IsNone() && Quests)
		{
			const FGrimQuestDef* Quest = Quests->Find(Row.Quest);
			if (!Quest)
			{
				OutProblems.Add(FString::Printf(TEXT("대사 %s: 모르는 퀘스트 %s"), *Key.ToString(), *Row.Quest.ToString()));
			}
			else if (Row.Step >= Quest->Steps.Num())
			{
				OutProblems.Add(FString::Printf(TEXT("대사 %s: %s 에 단계 %d 가 없다"), *Key.ToString(), *Row.Quest.ToString(), Row.Step));
			}
		}
	});

	// NPC 마다 언제나 고를 수 있는 기본 대화가 있어야 하고, 서비스가 있으면 결과 대사도 있어야 한다.
	const FGrimQuestLog EmptyLog;
	for (const FGrimNpcDef& Npc : Out.Npcs)
	{
		if (!Out.Select(Npc.Id, GrimDialogueContexts::Talk, EmptyLog))
		{
			OutProblems.Add(FString::Printf(TEXT("NPC %s: 기본 대화(Context=Talk, Quest 없음)가 없다"), *Npc.Id.ToString()));
		}
		if (Npc.Services.Num() > 0)
		{
			for (const FName Context : GrimDialogueContexts::RequiredForServices())
			{
				if (!Out.Select(Npc.Id, Context, EmptyLog))
				{
					OutProblems.Add(FString::Printf(TEXT("NPC %s: 맥락 %s 의 기본 대사가 없다"), *Npc.Id.ToString(), *Context.ToString()));
				}
			}
		}
	}

	return OutProblems.Num() == ProblemsBefore;
}

bool FGrimDialogueDatabase::BuildFromCSV(const FString& NpcsCSV, const FString& DialogueCSV, const FGrimQuestDatabase* Quests, FGrimDialogueDatabase& Out, TArray<FString>& OutProblems)
{
	using namespace GrimFactionTypesLocal;
	const int32 ProblemsBefore = OutProblems.Num();
	const UDataTable* Npcs = MakeTable(FGrimNpcRow::StaticStruct(), TEXT("DT_TowerNpcs"), NpcsCSV, OutProblems);
	const UDataTable* Dialogue = MakeTable(FGrimDialogueRow::StaticStruct(), TEXT("DT_TowerDialogue"), DialogueCSV, OutProblems);
	return Build(Npcs, Dialogue, Quests, Out, OutProblems) && OutProblems.Num() == ProblemsBefore;
}

const FGrimDialogueDatabase& FGrimDialogueDatabase::Get()
{
	using namespace GrimFactionTypesLocal;
	if (GLoaded)
	{
		return GDatabase;
	}
	GLoaded = true;

	const UGrimQuestSettings* Settings = UGrimQuestSettings::Get();
	FString Npcs, Dialogue;
	if (!UGrimQuestSettings::LoadContentFile(Settings->NpcsCSV, Npcs) || !UGrimQuestSettings::LoadContentFile(Settings->DialogueCSV, Dialogue))
	{
		UE_LOG(LogGrimQuest, Error, TEXT("NPC · 대사 표를 열 수 없다: %s · %s"), *Settings->NpcsCSV, *Settings->DialogueCSV);
		GDatabase = FGrimDialogueDatabase();
		return GDatabase;
	}
	TArray<FString> Problems;
	BuildFromCSV(Npcs, Dialogue, &UGrimQuestSettings::GetQuestDatabase(), GDatabase, Problems);
	for (const FString& Problem : Problems)
	{
		UE_LOG(LogGrimQuest, Error, TEXT("대사 표: %s"), *Problem);
	}
	UE_LOG(LogGrimQuest, Log, TEXT("NPC %d · 대사 %d 개를 읽었다(문제 %d)"), GDatabase.Npcs.Num(), GDatabase.Entries.Num(), Problems.Num());
	return GDatabase;
}

void FGrimDialogueDatabase::ResetCache()
{
	GrimFactionTypesLocal::GLoaded = false;
}
