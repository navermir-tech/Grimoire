#include "Quest/GrimQuestTypes.h"
#include "Environment/GrimTimeOfDayMath.h"
#include "GameplayTagsManager.h"
#include "UObject/Package.h"

DEFINE_LOG_CATEGORY(LogGrimQuest);

namespace GrimQuestTypesLocal
{
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

	bool NeedsTarget(EGrimQuestObjectiveType Type)
	{
		return Type != EGrimQuestObjectiveType::ReachCircle && Type != EGrimQuestObjectiveType::SleepThroughNight;
	}

	FString TypeName(EGrimQuestObjectiveType Type)
	{
		return StaticEnum<EGrimQuestObjectiveType>()->GetNameStringByValue(static_cast<int64>(Type));
	}
}

bool FGrimQuestObjectiveDef::IsSignalDriven() const
{
	return Type == EGrimQuestObjectiveType::TalkToNpc || Type == EGrimQuestObjectiveType::SleepThroughNight || Type == EGrimQuestObjectiveType::HitTarget
		|| Type == EGrimQuestObjectiveType::Event || Type == EGrimQuestObjectiveType::BreakTarget;
}

bool FGrimQuestGive::Parse(const FString& InText, FGrimQuestGive& Out)
{
	Out = FGrimQuestGive();
	FString Text = InText.TrimStartAndEnd();
	FString Where;
	if (Text.Split(TEXT("@"), &Text, &Where))
	{
		Text.TrimStartAndEndInline();
		Where.TrimStartAndEndInline();
		if (Where.IsEmpty())
		{
			return false;
		}
		Out.AtTag = FName(*Where);
	}
	FString Kind, Id;
	if (!Text.Split(TEXT(":"), &Kind, &Id))
	{
		Kind = Text;
	}
	Kind.TrimStartAndEndInline();
	Id.TrimStartAndEndInline();
	if (Kind.Equals(TEXT("Skillbook"), ESearchCase::IgnoreCase) && !Id.IsEmpty())
	{
		Out.Kind = EKind::Skillbook;
	}
	else if (Kind.Equals(TEXT("Item"), ESearchCase::IgnoreCase) && !Id.IsEmpty())
	{
		Out.Kind = EKind::Item;
	}
	else if (Kind.Equals(TEXT("Bedroll"), ESearchCase::IgnoreCase) && Id.IsEmpty())
	{
		Out.Kind = EKind::Bedroll;
	}
	else
	{
		return false;
	}
	Out.Id = Id.IsEmpty() ? NAME_None : FName(*Id);
	return true;
}

const FGrimQuestDef* FGrimQuestDatabase::Find(FName QuestId) const
{
	return Quests.FindByPredicate([QuestId](const FGrimQuestDef& Def) { return Def.Id == QuestId; });
}

const FGrimQuestDef* FGrimQuestDatabase::FindTrialForCircle(int32 TargetCircle) const
{
	return Quests.FindByPredicate([TargetCircle](const FGrimQuestDef& Def) { return Def.TrialCircle == TargetCircle; });
}

bool FGrimQuestDatabase::ParseTimeOfDay(const FString& InText, float DayRatio, float& OutTimeOfDay, bool& bOutValid)
{
	bOutValid = true;
	const FString Text = InText.TrimStartAndEnd();
	if (Text.IsEmpty())
	{
		return false;
	}

	// 기준 이름 + 선택적 오프셋(시)
	struct FAnchor { const TCHAR* Name; float TimeOfDay; };
	const FAnchor Anchors[] = {
		{ TEXT("Dusk"), FGrimTimeOfDayMath::GetDusk(DayRatio) },
		{ TEXT("Dawn"), FGrimTimeOfDayMath::GetDawn(DayRatio) },
		{ TEXT("Noon"), 0.5f },
		{ TEXT("Midnight"), 0.f },
	};
	for (const FAnchor& Anchor : Anchors)
	{
		if (Text.StartsWith(Anchor.Name, ESearchCase::IgnoreCase))
		{
			const FString Rest = Text.Mid(FCString::Strlen(Anchor.Name)).TrimStartAndEnd();
			float OffsetHours = 0.f;
			if (!Rest.IsEmpty())
			{
				if (!(Rest.StartsWith(TEXT("+")) || Rest.StartsWith(TEXT("-"))) || !Rest.Mid(1).TrimStart().IsNumeric())
				{
					bOutValid = false;
					return false;
				}
				OffsetHours = FCString::Atof(*Rest.Mid(1).TrimStart()) * (Rest.StartsWith(TEXT("-")) ? -1.f : 1.f);
			}
			OutTimeOfDay = FMath::Frac(Anchor.TimeOfDay + OffsetHours / 24.f + 1.f);
			return true;
		}
	}

	if (!Text.IsNumeric())
	{
		bOutValid = false;
		return false;
	}
	const float Hours = FCString::Atof(*Text);
	if (Hours < 0.f || Hours >= 24.f)
	{
		bOutValid = false;
		return false;
	}
	OutTimeOfDay = Hours / 24.f;
	return true;
}

bool FGrimQuestDatabase::Build(const UDataTable* QuestTable, const UDataTable* StepTable, const UDataTable* ObjectiveTable, FGrimQuestDatabase& Out, TArray<FString>& OutProblems)
{
	using namespace GrimQuestTypesLocal;
	const int32 ProblemsBefore = OutProblems.Num();
	Out.Quests.Reset();

	if (!QuestTable || QuestTable->GetRowStruct() != FGrimQuestRow::StaticStruct()
		|| !StepTable || StepTable->GetRowStruct() != FGrimQuestStepRow::StaticStruct()
		|| !ObjectiveTable || ObjectiveTable->GetRowStruct() != FGrimQuestObjectiveRow::StaticStruct())
	{
		OutProblems.Add(TEXT("표가 없거나 행 구조체가 다르다"));
		return false;
	}

	// 퀘스트(행 순서 유지)
	QuestTable->ForeachRow<FGrimQuestRow>(TEXT("GrimQuest"), [&Out](const FName& Key, const FGrimQuestRow& Row)
	{
		FGrimQuestDef& Def = Out.Quests.AddDefaulted_GetRef();
		Def.Id = Key;
		Def.Title = Row.Title;
		Def.Summary = Row.Summary;
		Def.Kind = Row.Kind;
		Def.bAutoStart = Row.AutoStart;
		Def.TrialCircle = Row.TrialCircle;
		Def.bRequireCircleConditions = Row.RequireCircleConditions;
		Def.RequiresQuest = Row.RequiresQuest;
		Def.RequiresStep = Row.RequiresStep;
		Def.RewardXP = Row.RewardXP;
		Def.RewardSpell = Row.RewardSpell;
		Def.NextQuest = Row.NextQuest;
	});

	// 단계: 퀘스트별로 모아 번호 순으로
	TMap<FName, TMap<int32, const FGrimQuestStepRow*>> StepsByQuest;
	StepTable->ForeachRow<FGrimQuestStepRow>(TEXT("GrimQuest"), [&](const FName& Key, const FGrimQuestStepRow& Row)
	{
		if (!Out.Find(Row.Quest))
		{
			OutProblems.Add(FString::Printf(TEXT("단계 %s: 모르는 퀘스트 %s"), *Key.ToString(), *Row.Quest.ToString()));
			return;
		}
		TMap<int32, const FGrimQuestStepRow*>& Steps = StepsByQuest.FindOrAdd(Row.Quest);
		if (Steps.Contains(Row.Step))
		{
			OutProblems.Add(FString::Printf(TEXT("단계 %s: %s 의 단계 %d 가 겹친다"), *Key.ToString(), *Row.Quest.ToString(), Row.Step));
			return;
		}
		Steps.Add(Row.Step, &Row);
	});

	for (FGrimQuestDef& Def : Out.Quests)
	{
		const TMap<int32, const FGrimQuestStepRow*>* Steps = StepsByQuest.Find(Def.Id);
		if (!Steps || Steps->Num() == 0)
		{
			OutProblems.Add(FString::Printf(TEXT("퀘스트 %s: 단계가 없다"), *Def.Id.ToString()));
			continue;
		}
		for (int32 Index = 0; Index < Steps->Num(); ++Index)
		{
			const FGrimQuestStepRow* const* Row = Steps->Find(Index);
			if (!Row)
			{
				OutProblems.Add(FString::Printf(TEXT("퀘스트 %s: 단계 %d 가 빠졌다(0 부터 빈틈 없이)"), *Def.Id.ToString(), Index));
				break;
			}
			FGrimQuestStepDef& Step = Def.Steps.AddDefaulted_GetRef();
			Step.Text = (*Row)->Text;
			Step.TimeLimitSeconds = (*Row)->TimeLimitSeconds;
			Step.TimeOfDayOnStart = (*Row)->TimeOfDayOnStart;
			Step.PropsTag = (*Row)->PropsTag;
			TArray<FString> GiveParts;
			(*Row)->Give.ParseIntoArray(GiveParts, TEXT("|"), true);
			for (const FString& Part : GiveParts)
			{
				if (Part.TrimStartAndEnd().IsEmpty())
				{
					continue;
				}
				FGrimQuestGive Give;
				if (FGrimQuestGive::Parse(Part, Give))
				{
					Step.Gives.Add(Give);
				}
				else
				{
					OutProblems.Add(FString::Printf(TEXT("퀘스트 %s 단계 %d: 줄 것 \"%s\" 을 못 읽는다(Skillbook:<ID> · Bedroll · Item:<ID>, 뒤에 @태그)"), *Def.Id.ToString(), Index, *Part));
				}
			}
			if (Step.TimeLimitSeconds < 0.f)
			{
				OutProblems.Add(FString::Printf(TEXT("퀘스트 %s 단계 %d: 시간 제한이 음수"), *Def.Id.ToString(), Index));
			}
			float Unused = 0.f;
			bool bValid = true;
			FGrimQuestDatabase::ParseTimeOfDay(Step.TimeOfDayOnStart, 0.625f, Unused, bValid);
			if (!bValid)
			{
				OutProblems.Add(FString::Printf(TEXT("퀘스트 %s 단계 %d: 시각 \"%s\" 을 못 읽는다"), *Def.Id.ToString(), Index, *Step.TimeOfDayOnStart));
			}
		}
	}

	// 목표
	ObjectiveTable->ForeachRow<FGrimQuestObjectiveRow>(TEXT("GrimQuest"), [&](const FName& Key, const FGrimQuestObjectiveRow& Row)
	{
		FGrimQuestDef* Def = Out.Quests.FindByPredicate([&Row](const FGrimQuestDef& Quest) { return Quest.Id == Row.Quest; });
		if (!Def)
		{
			OutProblems.Add(FString::Printf(TEXT("목표 %s: 모르는 퀘스트 %s"), *Key.ToString(), *Row.Quest.ToString()));
			return;
		}
		if (!Def->Steps.IsValidIndex(Row.Step))
		{
			OutProblems.Add(FString::Printf(TEXT("목표 %s: %s 에 단계 %d 가 없다"), *Key.ToString(), *Row.Quest.ToString(), Row.Step));
			return;
		}
		FGrimQuestObjectiveDef& Objective = Def->Steps[Row.Step].Objectives.AddDefaulted_GetRef();
		Objective.Type = Row.Type;
		Objective.Target = Row.Target;
		Objective.State = Row.State;
		Objective.Compare = Row.Compare;
		Objective.Value = Row.Value;
		Objective.Count = Row.Count;
		Objective.Text = Row.Text;

		const FString Where = FString::Printf(TEXT("목표 %s(%s)"), *Key.ToString(), *TypeName(Row.Type));
		if (Row.Count < 1)
		{
			OutProblems.Add(Where + TEXT(": Count 는 1 이상"));
		}
		if (NeedsTarget(Row.Type) && Row.Target.IsNone())
		{
			OutProblems.Add(Where + TEXT(": Target 이 비었다"));
		}
		switch (Row.Type)
		{
		case EGrimQuestObjectiveType::ElementalState:
			if (Row.State.IsNone() || !UGameplayTagsManager::Get().RequestGameplayTag(Row.State, false).IsValid())
			{
				OutProblems.Add(Where + FString::Printf(TEXT(": 상태 태그 %s 가 없다"), *Row.State.ToString()));
			}
			break;
		case EGrimQuestObjectiveType::ReachCircle:
			if (Row.Value < 1.f)
			{
				OutProblems.Add(Where + TEXT(": 서클(Value)은 1 이상"));
			}
			break;
		case EGrimQuestObjectiveType::ProficiencyGrade:
		case EGrimQuestObjectiveType::ReachLocation:
			if (Row.Value < 0.f)
			{
				OutProblems.Add(Where + TEXT(": Value 가 음수"));
			}
			break;
		default:
			break;
		}
	});

	// 서로 가리키는 것들
	TSet<int32> TrialCircles;
	for (const FGrimQuestDef& Def : Out.Quests)
	{
		for (int32 Index = 0; Index < Def.Steps.Num(); ++Index)
		{
			if (Def.Steps[Index].Objectives.Num() == 0)
			{
				OutProblems.Add(FString::Printf(TEXT("퀘스트 %s 단계 %d: 목표가 없다"), *Def.Id.ToString(), Index));
			}
			for (const FGrimQuestObjectiveDef& Objective : Def.Steps[Index].Objectives)
			{
				if (Objective.Type == EGrimQuestObjectiveType::CompleteQuest && (!Out.Find(Objective.Target) || Objective.Target == Def.Id))
				{
					OutProblems.Add(FString::Printf(TEXT("퀘스트 %s 단계 %d: CompleteQuest 대상 %s 가 없다(또는 자기 자신)"), *Def.Id.ToString(), Index, *Objective.Target.ToString()));
				}
			}
		}
		if (!Def.NextQuest.IsNone() && !Out.Find(Def.NextQuest))
		{
			OutProblems.Add(FString::Printf(TEXT("퀘스트 %s: NextQuest %s 가 없다"), *Def.Id.ToString(), *Def.NextQuest.ToString()));
		}
		if (!Def.RequiresQuest.IsNone() && (!Out.Find(Def.RequiresQuest) || Def.RequiresQuest == Def.Id))
		{
			OutProblems.Add(FString::Printf(TEXT("퀘스트 %s: RequiresQuest %s 가 없다(또는 자기 자신)"), *Def.Id.ToString(), *Def.RequiresQuest.ToString()));
		}
		if (Def.Kind == EGrimQuestKind::Trial)
		{
			if (Def.TrialCircle < 2)
			{
				OutProblems.Add(FString::Printf(TEXT("시험 %s: TrialCircle 은 2 이상"), *Def.Id.ToString()));
			}
			else if (TrialCircles.Contains(Def.TrialCircle))
			{
				OutProblems.Add(FString::Printf(TEXT("시험 %s: 서클 %d 시험이 둘이다"), *Def.Id.ToString(), Def.TrialCircle));
			}
			TrialCircles.Add(Def.TrialCircle);
		}
		else if (Def.TrialCircle != 0)
		{
			OutProblems.Add(FString::Printf(TEXT("퀘스트 %s: 시험(Kind=Trial)이 아닌데 TrialCircle 이 있다"), *Def.Id.ToString()));
		}
	}

	return OutProblems.Num() == ProblemsBefore;
}

bool FGrimQuestDatabase::BuildFromCSV(const FString& QuestsCSV, const FString& StepsCSV, const FString& ObjectivesCSV, FGrimQuestDatabase& Out, TArray<FString>& OutProblems)
{
	using namespace GrimQuestTypesLocal;
	const int32 ProblemsBefore = OutProblems.Num();
	const UDataTable* Quests = MakeTable(FGrimQuestRow::StaticStruct(), TEXT("DT_Quests"), QuestsCSV, OutProblems);
	const UDataTable* Steps = MakeTable(FGrimQuestStepRow::StaticStruct(), TEXT("DT_QuestSteps"), StepsCSV, OutProblems);
	const UDataTable* Objectives = MakeTable(FGrimQuestObjectiveRow::StaticStruct(), TEXT("DT_QuestObjectives"), ObjectivesCSV, OutProblems);
	return Build(Quests, Steps, Objectives, Out, OutProblems) && OutProblems.Num() == ProblemsBefore;
}

const FGrimQuestProgress* FGrimQuestLog::FindActive(FName QuestId) const
{
	return Active.FindByPredicate([QuestId](const FGrimQuestProgress& Progress) { return Progress.QuestId == QuestId; });
}

// ───── 모델 ─────

namespace GrimQuestModelLocal
{
	void BeginStep(const FGrimQuestDef& Def, FGrimQuestProgress& Progress, int32 StepIndex, float Now, TArray<FGrimQuestChange>& OutChanges)
	{
		Progress.Step = StepIndex;
		const FGrimQuestStepDef& Step = Def.Steps[StepIndex];
		Progress.Counts.Init(0, Step.Objectives.Num());
		Progress.Deadline = Step.TimeLimitSeconds > 0.f ? Now + Step.TimeLimitSeconds : 0.f;
		OutChanges.Add({ EGrimQuestChangeType::StepStarted, Def.Id, StepIndex });
	}
}

bool FGrimQuestModel::Compare(EGrimQuestCompare InCompare, float Value, float Threshold)
{
	return InCompare == EGrimQuestCompare::AtMost ? Value <= Threshold + KINDA_SMALL_NUMBER : Value >= Threshold - KINDA_SMALL_NUMBER;
}

bool FGrimQuestModel::IsStepMet(const FGrimQuestStepDef& Step, const TArray<int32>& Counts)
{
	for (int32 Index = 0; Index < Step.Objectives.Num(); ++Index)
	{
		if (!Counts.IsValidIndex(Index) || !IsObjectiveMet(Step.Objectives[Index], Counts[Index]))
		{
			return false;
		}
	}
	return true;
}

EGrimQuestStartResult FGrimQuestModel::CanStart(const FGrimQuestDatabase& Database, const FGrimQuestLog& Log, FName QuestId)
{
	const FGrimQuestDef* Def = Database.Find(QuestId);
	if (!Def || Def->Steps.Num() == 0)
	{
		return EGrimQuestStartResult::UnknownQuest;
	}
	if (Log.IsActive(QuestId))
	{
		return EGrimQuestStartResult::AlreadyActive;
	}
	if (Log.IsCompleted(QuestId))
	{
		return EGrimQuestStartResult::AlreadyCompleted;
	}
	if (!Def->RequiresQuest.IsNone() && !Log.IsCompleted(Def->RequiresQuest))
	{
		const FGrimQuestProgress* Required = Log.FindActive(Def->RequiresQuest);
		if (!Required || Required->Step < Def->RequiresStep)
		{
			return EGrimQuestStartResult::PrerequisiteMissing;
		}
	}
	return EGrimQuestStartResult::Started;
}

EGrimQuestStartResult FGrimQuestModel::StartQuest(const FGrimQuestDatabase& Database, FGrimQuestLog& Log, FName QuestId, float Now, TArray<FGrimQuestChange>& OutChanges)
{
	const EGrimQuestStartResult Result = CanStart(Database, Log, QuestId);
	if (Result != EGrimQuestStartResult::Started)
	{
		return Result;
	}
	const FGrimQuestDef& Def = *Database.Find(QuestId);
	FGrimQuestProgress& Progress = Log.Active.AddDefaulted_GetRef();
	Progress.QuestId = QuestId;
	OutChanges.Add({ EGrimQuestChangeType::QuestStarted, QuestId, 0 });
	GrimQuestModelLocal::BeginStep(Def, Progress, 0, Now, OutChanges);
	return Result;
}

bool FGrimQuestModel::ApplySignal(const FGrimQuestDatabase& Database, FGrimQuestLog& Log, const FGrimQuestSignal& Signal)
{
	bool bCounted = false;
	for (FGrimQuestProgress& Progress : Log.Active)
	{
		const FGrimQuestDef* Def = Database.Find(Progress.QuestId);
		if (!Def || !Def->Steps.IsValidIndex(Progress.Step))
		{
			continue;
		}
		const FGrimQuestStepDef& Step = Def->Steps[Progress.Step];
		Progress.Counts.SetNumZeroed(Step.Objectives.Num());
		for (int32 Index = 0; Index < Step.Objectives.Num(); ++Index)
		{
			const FGrimQuestObjectiveDef& Objective = Step.Objectives[Index];
			const bool bTargetMatches = Objective.Type == EGrimQuestObjectiveType::SleepThroughNight || Objective.Target == Signal.Target;
			if (Objective.IsSignalDriven() && Objective.Type == Signal.Type && bTargetMatches && !IsObjectiveMet(Objective, Progress.Counts[Index]))
			{
				Progress.Counts[Index] = FMath::Min(Progress.Counts[Index] + FMath::Max(1, Signal.Amount), FMath::Max(1, Objective.Count));
				bCounted = true;
			}
		}
	}
	return bCounted;
}

bool FGrimQuestModel::FailQuest(const FGrimQuestDatabase& Database, FGrimQuestLog& Log, FName QuestId, TArray<FGrimQuestChange>& OutChanges)
{
	const int32 Index = Log.Active.IndexOfByPredicate([QuestId](const FGrimQuestProgress& Progress) { return Progress.QuestId == QuestId; });
	if (Index == INDEX_NONE)
	{
		return false;
	}
	OutChanges.Add({ EGrimQuestChangeType::StepEnded, QuestId, Log.Active[Index].Step });
	OutChanges.Add({ EGrimQuestChangeType::QuestFailed, QuestId, Log.Active[Index].Step });
	Log.Active.RemoveAt(Index);
	return true;
}

bool FGrimQuestModel::Advance(const FGrimQuestDatabase& Database, FGrimQuestLog& Log, const IGrimQuestFacts* Facts, float Now, TArray<FGrimQuestChange>& OutChanges)
{
	bool bAnyChange = false;
	// 끝낸 시험이 다른 퀘스트의 CompleteQuest 를 채우는 것처럼 이어지는 변화는 더 바뀌지 않을 때까지 본다(넉넉한 상한).
	// 단, 이번 호출에서 새로 시작한 단계는 다음 호출까지 보지 않는다 — 단계 시작의 세계 쪽 일(소품 되돌리기 · 시각)이
	// 부른 쪽에서 처리된 뒤에 조회해야 한다(되돌리기 전의 켜진 양초로 시험이 바로 끝나지 않게).
	TSet<FName> StartedThisCall;
	for (int32 Pass = 0; Pass < 64; ++Pass)
	{
		bool bChanged = false;
		for (int32 ActiveIndex = 0; ActiveIndex < Log.Active.Num(); ++ActiveIndex)
		{
			const FName QuestId = Log.Active[ActiveIndex].QuestId;
			if (StartedThisCall.Contains(QuestId))
			{
				continue;
			}
			const FGrimQuestDef* Def = Database.Find(QuestId);
			if (!Def || !Def->Steps.IsValidIndex(Log.Active[ActiveIndex].Step))
			{
				// 표에서 사라진 퀘스트(옛 세이브) — 조용히 뺀다.
				Log.Active.RemoveAt(ActiveIndex--);
				bChanged = true;
				continue;
			}

			FGrimQuestProgress& Progress = Log.Active[ActiveIndex];
			if (Progress.Deadline > 0.f && Now > Progress.Deadline)
			{
				FailQuest(Database, Log, QuestId, OutChanges);
				--ActiveIndex;
				bChanged = true;
				continue;
			}

			const FGrimQuestStepDef& Step = Def->Steps[Progress.Step];
			Progress.Counts.SetNumZeroed(Step.Objectives.Num());
			for (int32 Index = 0; Index < Step.Objectives.Num(); ++Index)
			{
				const FGrimQuestObjectiveDef& Objective = Step.Objectives[Index];
				if (Objective.IsSignalDriven() || IsObjectiveMet(Objective, Progress.Counts[Index]))
				{
					continue;
				}
				int32 Value = 0;
				if (Objective.Type == EGrimQuestObjectiveType::CompleteQuest)
				{
					Value = Log.IsCompleted(Objective.Target) ? 1 : 0;
				}
				else if (Facts)
				{
					Value = Facts->QueryObjective(Objective);
				}
				const int32 Clamped = FMath::Clamp(Value, 0, FMath::Max(1, Objective.Count));
				if (Clamped > Progress.Counts[Index])
				{
					Progress.Counts[Index] = Clamped;
					bChanged = true;
				}
			}

			if (!IsStepMet(Step, Progress.Counts))
			{
				continue;
			}

			OutChanges.Add({ EGrimQuestChangeType::StepEnded, QuestId, Progress.Step });
			bChanged = true;
			if (Progress.Step + 1 < Def->Steps.Num())
			{
				GrimQuestModelLocal::BeginStep(*Def, Progress, Progress.Step + 1, Now, OutChanges);
				StartedThisCall.Add(QuestId);
				continue;
			}

			// 끝 — 진행에서 빼고 끝낸 목록에, 이어지는 퀘스트를 시작.
			const int32 LastStep = Progress.Step;
			Log.Active.RemoveAt(ActiveIndex--);
			Log.Completed.AddUnique(QuestId);
			OutChanges.Add({ EGrimQuestChangeType::QuestCompleted, QuestId, LastStep });
			if (!Def->NextQuest.IsNone() && StartQuest(Database, Log, Def->NextQuest, Now, OutChanges) == EGrimQuestStartResult::Started)
			{
				StartedThisCall.Add(Def->NextQuest);
			}
		}
		bAnyChange |= bChanged;
		if (!bChanged)
		{
			break;
		}
	}
	return bAnyChange;
}
