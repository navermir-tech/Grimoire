#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Quest/GrimQuestComponent.h"
#include "Quest/GrimQuestSettings.h"
#include "Quest/GrimQuestTypes.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Factions/GrimFactionTypes.h"
#include "Environment/GrimTimeOfDayMath.h"

namespace GrimQuestTestsLocal // 유니티 빌드에서 다른 테스트 파일과 이름이 부딪히지 않게
{
	constexpr EAutomationTestFlags QuestTestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	/** 조회형 목표를 함수로 답하는 가짜 세계 */
	class FFakeFacts final : public IGrimQuestFacts
	{
	public:
		TFunction<int32(const FGrimQuestObjectiveDef&)> Query;
		virtual int32 QueryObjective(const FGrimQuestObjectiveDef& Objective) const override { return Query ? Query(Objective) : 0; }
	};

	bool LoadRepoQuests(FAutomationTestBase& Test, FGrimQuestDatabase& Out)
	{
		const UGrimQuestSettings* Settings = UGrimQuestSettings::Get();
		FString Quests, Steps, Objectives;
		if (!UGrimQuestSettings::LoadContentFile(Settings->QuestsCSV, Quests) || !UGrimQuestSettings::LoadContentFile(Settings->QuestStepsCSV, Steps)
			|| !UGrimQuestSettings::LoadContentFile(Settings->QuestObjectivesCSV, Objectives))
		{
			Test.AddError(TEXT("저장소의 퀘스트 CSV 를 열 수 없다"));
			return false;
		}
		TArray<FString> Problems;
		const bool bOk = FGrimQuestDatabase::BuildFromCSV(Quests, Steps, Objectives, Out, Problems);
		for (const FString& Problem : Problems)
		{
			Test.AddError(Problem);
		}
		return bOk;
	}

	bool HasChange(const TArray<FGrimQuestChange>& Changes, EGrimQuestChangeType Type, FName QuestId)
	{
		return Changes.ContainsByPredicate([&](const FGrimQuestChange& Change) { return Change.Type == Type && Change.QuestId == QuestId; });
	}

	int32 StepOf(const FGrimQuestLog& Log, FName QuestId)
	{
		const FGrimQuestProgress* Progress = Log.FindActive(QuestId);
		return Progress ? Progress->Step : INDEX_NONE;
	}

	// 작은 표: A(두 단계) → B. A 의 1단계는 시간 제한.
	const TCHAR* MiniQuests =
		TEXT("Name,Title,Summary,Kind,AutoStart,TrialCircle,RequireCircleConditions,RequiresQuest,RequiresStep,RewardXP,RewardSpell,NextQuest\n")
		TEXT("A,\"가\",\"\",Main,True,0,False,None,0,10,None,B\n")
		TEXT("B,\"나\",\"\",Main,False,0,False,A,0,0,None,None\n")
		TEXT("T,\"시험\",\"\",Trial,False,2,True,B,1,0,None,None\n");
	const TCHAR* MiniSteps =
		TEXT("Name,Quest,Step,Text,TimeLimitSeconds,TimeOfDayOnStart,PropsTag,Give\n")
		TEXT("A0,A,0,\"말 걸기\",0,\"\",None,\"Skillbook:Ember\"\n")
		TEXT("A1,A,1,\"양초 둘\",10,\"Dusk+0.5\",Props,\"Bedroll@Camp|Item:Meat\"\n")
		TEXT("B0,B,0,\"시험 통과\",0,\"\",None,\"\"\n")
		TEXT("B1,B,1,\"자기\",0,\"\",None,\"\"\n")
		TEXT("T0,T,0,\"표적\",0,\"\",None,\"\"\n");
	const TCHAR* MiniObjectives =
		TEXT("Name,Quest,Step,Type,Target,State,Compare,Value,Count,Text\n")
		TEXT("A0a,A,0,TalkToNpc,Master,None,AtLeast,0,1,\"\"\n")
		TEXT("A1a,A,1,ElementalState,Candle,State.Burning,AtLeast,1,2,\"\"\n")
		TEXT("B0a,B,0,CompleteQuest,T,None,AtLeast,0,1,\"\"\n")
		TEXT("B1a,B,1,SleepThroughNight,None,None,AtLeast,0,1,\"\"\n")
		TEXT("T0a,T,0,HitTarget,Dummy,None,AtLeast,0,3,\"\"\n");
}

using namespace GrimQuestTestsLocal;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimQuestTableTest, "Grimoire.Quest.Table", QuestTestFlags)
bool FGrimQuestTableTest::RunTest(const FString& Parameters)
{
	// 저장소의 표가 문제 없이 읽히고, 튜토리얼 흐름의 뼈대가 있다.
	FGrimQuestDatabase Database;
	if (!LoadRepoQuests(*this, Database))
	{
		return false;
	}
	const FGrimQuestDef* Tutorial = Database.Find(TEXT("Tutorial"));
	if (!TestNotNull(TEXT("Tutorial 퀘스트"), Tutorial))
	{
		return false;
	}
	TestTrue(TEXT("Tutorial 은 저절로 시작"), Tutorial->bAutoStart);
	TestTrue(TEXT("Tutorial 은 열 단계 넘게"), Tutorial->Steps.Num() >= 10);
	const FGrimQuestDef* Trial2 = Database.FindTrialForCircle(2);
	const FGrimQuestDef* Trial3 = Database.FindTrialForCircle(3);
	TestTrue(TEXT("2서클 · 3서클 시험이 있다"), Trial2 && Trial3);
	int32 NightStep = INDEX_NONE;
	for (int32 Index = 0; Index < Tutorial->Steps.Num(); ++Index)
	{
		for (const FGrimQuestObjectiveDef& Objective : Tutorial->Steps[Index].Objectives)
		{
			if (Objective.Type == EGrimQuestObjectiveType::SleepThroughNight)
			{
				NightStep = Index;
			}
		}
	}
	if (Trial2 && Trial3)
	{
		TestTrue(TEXT("시험은 Tutorial 진행에 묶인다"), Trial2->RequiresQuest == Tutorial->Id && Trial3->RequiresQuest == Tutorial->Id);
		TestTrue(TEXT("시험은 첫 밤을 넘긴 뒤"), NightStep != INDEX_NONE && Trial2->RequiresStep > NightStep && Trial3->RequiresStep > NightStep);
		TestTrue(TEXT("2서클 시험 보상 = 이그니스 스킬북"), Trial2->RewardSpell == TEXT("Fireball"));
		const bool bTimed = Trial2->Steps.ContainsByPredicate([](const FGrimQuestStepDef& Step) { return Step.TimeLimitSeconds > 0.f && !Step.PropsTag.IsNone(); });
		TestTrue(TEXT("2서클 시험에 시간 제한 · 소품 단계"), bTimed);
	}
	bool bHasNight = false;
	for (const FGrimQuestStepDef& Step : Tutorial->Steps)
	{
		for (const FGrimQuestObjectiveDef& Objective : Step.Objectives)
		{
			bHasNight |= Objective.Type == EGrimQuestObjectiveType::SleepThroughNight;
		}
	}
	TestTrue(TEXT("첫 밤 넘기기 목표"), bHasNight);
	// 시작할 때 아는 마법이 없다 — 모두 스킬북으로 준다
	TSet<FName> GivenBooks;
	bool bGivesBedroll = false;
	for (const FGrimQuestStepDef& Step : Tutorial->Steps)
	{
		for (const FGrimQuestGive& Give : Step.Gives)
		{
			if (Give.Kind == FGrimQuestGive::EKind::Skillbook)
			{
				GivenBooks.Add(Give.Id);
			}
			bGivesBedroll |= Give.Kind == FGrimQuestGive::EKind::Bedroll;
		}
	}
	for (const TCHAR* Spell : { TEXT("BasicAttack"), TEXT("Ember"), TEXT("WaterJet"), TEXT("Breeze"), TEXT("Pactum") })
	{
		TestTrue(FString::Printf(TEXT("스킬북으로 주는 마법 %s"), Spell), GivenBooks.Contains(FName(Spell)));
	}
	TestTrue(TEXT("침낭을 준다"), bGivesBedroll);
	const FGrimQuestDef* Hunter = Database.Find(TEXT("Hunter_FirstEncounter"));
	TestTrue(TEXT("사냥꾼 첫 등장 퀘스트는 저절로 시작하지 않고 튜토리얼 뒤"), Hunter && !Hunter->bAutoStart && Hunter->RequiresQuest == Tutorial->Id);

	// NPC · 대사 표도 함께(대사가 가리키는 퀘스트 · 단계까지).
	FString Npcs, Dialogue;
	if (!UGrimQuestSettings::LoadContentFile(UGrimQuestSettings::Get()->NpcsCSV, Npcs) || !UGrimQuestSettings::LoadContentFile(UGrimQuestSettings::Get()->DialogueCSV, Dialogue))
	{
		AddError(TEXT("저장소의 NPC · 대사 CSV 를 열 수 없다"));
		return false;
	}
	FGrimDialogueDatabase Dialogues;
	TArray<FString> Problems;
	const bool bDialogueOk = FGrimDialogueDatabase::BuildFromCSV(Npcs, Dialogue, &Database, Dialogues, Problems);
	for (const FString& Problem : Problems)
	{
		AddError(Problem);
	}
	TestTrue(TEXT("대사 표 문제 없음"), bDialogueOk);
	const FGrimNpcDef* Master = Dialogues.FindNpc(TEXT("Master"));
	TestTrue(TEXT("스승은 서비스 일곱(시험 · 정화 넷 · 수리 · 지우기)"), Master && Master->Services.Num() == 7);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimQuestTableProblemsTest, "Grimoire.Quest.TableProblems", QuestTestFlags)
bool FGrimQuestTableProblemsTest::RunTest(const FString& Parameters)
{
	FGrimQuestDatabase Database;
	TArray<FString> Problems;
	TestTrue(TEXT("작은 표는 문제 없음"), FGrimQuestDatabase::BuildFromCSV(MiniQuests, MiniSteps, MiniObjectives, Database, Problems));
	for (const FString& Problem : Problems)
	{
		AddError(Problem);
	}

	auto ExpectProblem = [this](const TCHAR* What, const FString& Quests, const FString& Steps, const FString& Objectives)
	{
		FGrimQuestDatabase Bad;
		TArray<FString> BadProblems;
		const bool bOk = FGrimQuestDatabase::BuildFromCSV(Quests, Steps, Objectives, Bad, BadProblems);
		TestFalse(What, bOk || BadProblems.Num() == 0);
	};
	const FString Q = MiniQuests;
	const FString S = MiniSteps;
	const FString O = MiniObjectives;
	ExpectProblem(TEXT("단계가 빠짐(0 다음 2)"), Q, S.Replace(TEXT("A1,A,1,"), TEXT("A1,A,2,")), O.Replace(TEXT("A1a,A,1,"), TEXT("A1a,A,0,")));
	ExpectProblem(TEXT("모르는 퀘스트의 단계"), Q, S + TEXT("Z0,Zzz,0,\"\",0,\"\",None\n"), O);
	ExpectProblem(TEXT("없는 상태 태그"), Q, S, O.Replace(TEXT("State.Burning"), TEXT("State.NoSuchThing")));
	ExpectProblem(TEXT("Count 0"), Q, S, O.Replace(TEXT("Dummy,None,AtLeast,0,3"), TEXT("Dummy,None,AtLeast,0,0")));
	ExpectProblem(TEXT("Target 이 빔"), Q, S, O.Replace(TEXT("TalkToNpc,Master"), TEXT("TalkToNpc,None")));
	ExpectProblem(TEXT("목표 없는 단계"), Q, S, O.Replace(TEXT("B1a,B,1,SleepThroughNight,None,None,AtLeast,0,1,\"\"\n"), TEXT("")));
	ExpectProblem(TEXT("못 읽는 시각"), Q, S.Replace(TEXT("Dusk+0.5"), TEXT("Sunset")), O);
	ExpectProblem(TEXT("없는 NextQuest"), Q.Replace(TEXT(",None,B\n"), TEXT(",None,Nope\n")), S, O);
	ExpectProblem(TEXT("없는 CompleteQuest 대상"), Q, S, O.Replace(TEXT("CompleteQuest,T,"), TEXT("CompleteQuest,Nope,")));
	ExpectProblem(TEXT("시험이 아닌데 TrialCircle"), Q.Replace(TEXT("Main,False,0,False,A"), TEXT("Main,False,3,False,A")), S, O);
	ExpectProblem(TEXT("모르는 줄 것"), Q, S.Replace(TEXT("Item:Meat"), TEXT("Sword:Big")), O);
	ExpectProblem(TEXT("빈 @태그"), Q, S.Replace(TEXT("Bedroll@Camp"), TEXT("Bedroll@")), O);

	// 줄 것 읽기
	const FGrimQuestDef* A = Database.Find(TEXT("A"));
	if (TestTrue(TEXT("A 의 줄 것"), A && A->Steps.Num() == 2 && A->Steps[0].Gives.Num() == 1 && A->Steps[1].Gives.Num() == 2))
	{
		TestTrue(TEXT("Skillbook:Ember"), A->Steps[0].Gives[0].Kind == FGrimQuestGive::EKind::Skillbook && A->Steps[0].Gives[0].Id == TEXT("Ember") && A->Steps[0].Gives[0].AtTag.IsNone());
		TestTrue(TEXT("Bedroll@Camp"), A->Steps[1].Gives[0].Kind == FGrimQuestGive::EKind::Bedroll && A->Steps[1].Gives[0].AtTag == TEXT("Camp"));
		TestTrue(TEXT("Item:Meat"), A->Steps[1].Gives[1].Kind == FGrimQuestGive::EKind::Item && A->Steps[1].Gives[1].Id == TEXT("Meat"));
	}
	FGrimQuestGive Give;
	TestFalse(TEXT("Skillbook 은 ID 가 있어야"), FGrimQuestGive::Parse(TEXT("Skillbook"), Give));
	TestFalse(TEXT("Bedroll 은 ID 가 없어야"), FGrimQuestGive::Parse(TEXT("Bedroll:Big"), Give));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimQuestTimeOfDayTest, "Grimoire.Quest.TimeOfDay", QuestTestFlags)
bool FGrimQuestTimeOfDayTest::RunTest(const FString& Parameters)
{
	const float DayRatio = 0.625f;
	float Time = 0.f;
	bool bValid = true;
	TestFalse(TEXT("비면 옮기지 않음"), FGrimQuestDatabase::ParseTimeOfDay(TEXT(""), DayRatio, Time, bValid));
	TestTrue(TEXT("빈 것은 문제 아님"), bValid);
	TestTrue(TEXT("Dusk"), FGrimQuestDatabase::ParseTimeOfDay(TEXT("Dusk"), DayRatio, Time, bValid));
	TestNearlyEqual(TEXT("Dusk = 해넘이"), Time, FGrimTimeOfDayMath::GetDusk(DayRatio), 1e-4f);
	FGrimQuestDatabase::ParseTimeOfDay(TEXT("Dusk+0.2"), DayRatio, Time, bValid);
	TestNearlyEqual(TEXT("Dusk+0.2 = 해넘이 + 0.2시간"), Time, FGrimTimeOfDayMath::GetDusk(DayRatio) + 0.2f / 24.f, 1e-4f);
	TestTrue(TEXT("해넘이 뒤면 밤"), FGrimTimeOfDayMath::IsNight(Time, DayRatio));
	FGrimQuestDatabase::ParseTimeOfDay(TEXT("Dawn - 1"), DayRatio, Time, bValid);
	TestNearlyEqual(TEXT("Dawn-1"), Time, FGrimTimeOfDayMath::GetDawn(DayRatio) - 1.f / 24.f, 1e-4f);
	FGrimQuestDatabase::ParseTimeOfDay(TEXT("Midnight-1"), DayRatio, Time, bValid);
	TestNearlyEqual(TEXT("자정 1시간 전은 23시"), Time, 23.f / 24.f, 1e-4f);
	FGrimQuestDatabase::ParseTimeOfDay(TEXT("21.5"), DayRatio, Time, bValid);
	TestNearlyEqual(TEXT("숫자는 시"), Time, 21.5f / 24.f, 1e-4f);
	TestFalse(TEXT("24시는 안 됨"), FGrimQuestDatabase::ParseTimeOfDay(TEXT("24"), DayRatio, Time, bValid) || bValid);
	TestFalse(TEXT("모르는 말"), FGrimQuestDatabase::ParseTimeOfDay(TEXT("Sunset"), DayRatio, Time, bValid) || bValid);
	TestFalse(TEXT("오프셋 형식"), FGrimQuestDatabase::ParseTimeOfDay(TEXT("Dusk*2"), DayRatio, Time, bValid) || bValid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimQuestModelTest, "Grimoire.Quest.Model", QuestTestFlags)
bool FGrimQuestModelTest::RunTest(const FString& Parameters)
{
	FGrimQuestDatabase Database;
	TArray<FString> Problems;
	if (!FGrimQuestDatabase::BuildFromCSV(MiniQuests, MiniSteps, MiniObjectives, Database, Problems))
	{
		AddError(TEXT("작은 표를 못 읽었다"));
		return false;
	}

	int32 LitCandles = 0;
	FFakeFacts Facts;
	Facts.Query = [&LitCandles](const FGrimQuestObjectiveDef& Objective)
	{
		return Objective.Type == EGrimQuestObjectiveType::ElementalState && Objective.Target == TEXT("Candle") ? LitCandles : 0;
	};

	FGrimQuestLog Log;
	TArray<FGrimQuestChange> Changes;
	TestTrue(TEXT("B 는 A 를 끝내기 전엔 못 시작"), FGrimQuestModel::CanStart(Database, Log, TEXT("B")) == EGrimQuestStartResult::PrerequisiteMissing);
	TestTrue(TEXT("모르는 퀘스트"), FGrimQuestModel::CanStart(Database, Log, TEXT("Nope")) == EGrimQuestStartResult::UnknownQuest);
	TestTrue(TEXT("A 시작"), FGrimQuestModel::StartQuest(Database, Log, TEXT("A"), 0.f, Changes) == EGrimQuestStartResult::Started);
	TestTrue(TEXT("시작 · 단계 시작 알림"), HasChange(Changes, EGrimQuestChangeType::QuestStarted, TEXT("A")) && HasChange(Changes, EGrimQuestChangeType::StepStarted, TEXT("A")));
	TestTrue(TEXT("두 번 시작 안 됨"), FGrimQuestModel::CanStart(Database, Log, TEXT("A")) == EGrimQuestStartResult::AlreadyActive);

	// 신호: 다른 NPC 는 세지 않는다
	TestFalse(TEXT("다른 NPC 와 대화"), FGrimQuestModel::ApplySignal(Database, Log, { EGrimQuestObjectiveType::TalkToNpc, TEXT("Other"), 1 }));
	TestTrue(TEXT("스승과 대화"), FGrimQuestModel::ApplySignal(Database, Log, { EGrimQuestObjectiveType::TalkToNpc, TEXT("Master"), 1 }));
	Changes.Reset();
	FGrimQuestModel::Advance(Database, Log, &Facts, 1.f, Changes);
	TestEqual(TEXT("A 단계 1"), StepOf(Log, TEXT("A")), 1);
	const FGrimQuestProgress* A = Log.FindActive(TEXT("A"));
	TestTrue(TEXT("시간 제한 = 지금 + 10"), A && FMath::IsNearlyEqual(A->Deadline, 11.f));

	// 조회: 하나만 켜면 모자라고, 둘이면 채운다. 채운 값은 남는다(꺼져도).
	LitCandles = 1;
	FGrimQuestModel::Advance(Database, Log, &Facts, 2.f, Changes);
	TestEqual(TEXT("양초 1/2"), Log.FindActive(TEXT("A"))->Counts[0], 1);
	LitCandles = 0;
	FGrimQuestModel::Advance(Database, Log, &Facts, 3.f, Changes);
	TestEqual(TEXT("꺼져도 1 은 남는다"), Log.FindActive(TEXT("A"))->Counts[0], 1);
	LitCandles = 2;
	Changes.Reset();
	FGrimQuestModel::Advance(Database, Log, &Facts, 4.f, Changes);
	TestTrue(TEXT("A 끝"), Log.IsCompleted(TEXT("A")) && !Log.IsActive(TEXT("A")));
	TestTrue(TEXT("A 완료 알림 · 단계 끝"), HasChange(Changes, EGrimQuestChangeType::QuestCompleted, TEXT("A")) && HasChange(Changes, EGrimQuestChangeType::StepEnded, TEXT("A")));
	TestTrue(TEXT("NextQuest B 가 이어서 시작"), Log.IsActive(TEXT("B")) && HasChange(Changes, EGrimQuestChangeType::QuestStarted, TEXT("B")));
	TestTrue(TEXT("끝낸 퀘스트는 다시 못 함"), FGrimQuestModel::CanStart(Database, Log, TEXT("A")) == EGrimQuestStartResult::AlreadyCompleted);

	// 시험 T: B 의 단계 1 부터 → 아직(단계 0) 못 함
	TestTrue(TEXT("T 는 B 단계 1 부터"), FGrimQuestModel::CanStart(Database, Log, TEXT("T")) == EGrimQuestStartResult::PrerequisiteMissing);
	// B0 = CompleteQuest T — T 를 못 끝내니 막힌다. 개발용으로 B 를 단계 1 에 놓고 T 를 확인.
	Log.Active.FindByPredicate([](const FGrimQuestProgress& P) { return P.QuestId == TEXT("B"); })->Step = 1;
	Log.Active.FindByPredicate([](const FGrimQuestProgress& P) { return P.QuestId == TEXT("B"); })->Counts.Init(0, 1);
	TestTrue(TEXT("이제 T 시작"), FGrimQuestModel::StartQuest(Database, Log, TEXT("T"), 5.f, Changes) == EGrimQuestStartResult::Started);
	FGrimQuestModel::ApplySignal(Database, Log, { EGrimQuestObjectiveType::HitTarget, TEXT("Dummy"), 1 });
	FGrimQuestModel::ApplySignal(Database, Log, { EGrimQuestObjectiveType::HitTarget, TEXT("Dummy"), 5 });
	TestEqual(TEXT("명중은 Count 에서 멈춘다"), Log.FindActive(TEXT("T"))->Counts[0], 3);
	FGrimQuestModel::Advance(Database, Log, &Facts, 6.f, Changes);
	TestTrue(TEXT("T 끝"), Log.IsCompleted(TEXT("T")));

	// 밤 넘김 신호는 Target 과 상관없이
	TestTrue(TEXT("밤 넘김"), FGrimQuestModel::ApplySignal(Database, Log, { EGrimQuestObjectiveType::SleepThroughNight, NAME_None, 1 }));
	FGrimQuestModel::Advance(Database, Log, &Facts, 7.f, Changes);
	TestTrue(TEXT("B 끝"), Log.IsCompleted(TEXT("B")) && Log.Active.Num() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimQuestTimeoutTest, "Grimoire.Quest.Timeout", QuestTestFlags)
bool FGrimQuestTimeoutTest::RunTest(const FString& Parameters)
{
	FGrimQuestDatabase Database;
	TArray<FString> Problems;
	FGrimQuestDatabase::BuildFromCSV(MiniQuests, MiniSteps, MiniObjectives, Database, Problems);
	FFakeFacts Facts;
	FGrimQuestLog Log;
	TArray<FGrimQuestChange> Changes;
	FGrimQuestModel::StartQuest(Database, Log, TEXT("A"), 0.f, Changes);
	FGrimQuestModel::ApplySignal(Database, Log, { EGrimQuestObjectiveType::TalkToNpc, TEXT("Master"), 1 });
	FGrimQuestModel::Advance(Database, Log, &Facts, 100.f, Changes);
	TestEqual(TEXT("시간 제한 단계"), StepOf(Log, TEXT("A")), 1);
	Changes.Reset();
	FGrimQuestModel::Advance(Database, Log, &Facts, 109.f, Changes);
	TestTrue(TEXT("제한 안"), Log.IsActive(TEXT("A")));
	FGrimQuestModel::Advance(Database, Log, &Facts, 110.5f, Changes);
	TestFalse(TEXT("제한을 넘기면 진행에서 빠진다"), Log.IsActive(TEXT("A")));
	TestFalse(TEXT("끝낸 것은 아니다"), Log.IsCompleted(TEXT("A")));
	TestTrue(TEXT("실패 · 단계 끝 알림"), HasChange(Changes, EGrimQuestChangeType::QuestFailed, TEXT("A")) && HasChange(Changes, EGrimQuestChangeType::StepEnded, TEXT("A")));
	TestTrue(TEXT("다시 시작할 수 있다"), FGrimQuestModel::CanStart(Database, Log, TEXT("A")) == EGrimQuestStartResult::Started);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimQuestDialogueTest, "Grimoire.Quest.Dialogue", QuestTestFlags)
bool FGrimQuestDialogueTest::RunTest(const FString& Parameters)
{
	FGrimQuestDatabase Quests;
	TArray<FString> Problems;
	FGrimQuestDatabase::BuildFromCSV(MiniQuests, MiniSteps, MiniObjectives, Quests, Problems);

	const TCHAR* Npcs = TEXT("Name,DisplayName,Title,Services,ServiceLabels\nMaster,\"스승\",\"\",\"\",\"\"\nOther,\"다른 이\",\"\",\"\",\"\"\n");
	const TCHAR* Lines =
		TEXT("Name,Npc,Context,Quest,Step,Priority,Lines\n")
		TEXT("Default,Master,Talk,None,-1,0,\"기본\"\n")
		TEXT("AnyNpc,None,Talk,None,-1,0,\"아무나\"\n")
		TEXT("A_Active,Master,Talk,A,-1,0,\"A 진행 중\"\n")
		TEXT("A_Step1,Master,Talk,A,1,0,\"A 단계 1|둘째 줄\"\n")
		TEXT("A_Done,Master,Talk,A,-2,0,\"A 끝\"\n")
		TEXT("Hi,Master,Talk,A,1,5,\"A 단계 1 (우선)\"\n");
	FGrimDialogueDatabase Dialogue;
	TestTrue(TEXT("대사 표 읽기"), FGrimDialogueDatabase::BuildFromCSV(Npcs, Lines, &Quests, Dialogue, Problems));
	for (const FString& Problem : Problems)
	{
		AddError(Problem);
	}

	FGrimQuestLog Log;
	auto Pick = [&](FName Npc) { const FGrimDialogueEntry* Entry = Dialogue.Select(Npc, TEXT("Talk"), Log); return Entry ? Entry->RowName : NAME_None; };
	TestEqual(TEXT("진행 없으면 기본"), Pick(TEXT("Master")), FName(TEXT("Default")));
	TestEqual(TEXT("다른 NPC 는 모두의 행"), Pick(TEXT("Other")), FName(TEXT("AnyNpc")));

	FGrimQuestProgress& Progress = Log.Active.AddDefaulted_GetRef();
	Progress.QuestId = TEXT("A");
	Progress.Step = 0;
	TestEqual(TEXT("진행 중(단계 0 행 없음)"), Pick(TEXT("Master")), FName(TEXT("A_Active")));
	Log.Active[0].Step = 1;
	TestEqual(TEXT("단계가 맞으면 그 행, 같은 조건은 Priority"), Pick(TEXT("Master")), FName(TEXT("Hi")));
	Log.Active.Reset();
	Log.Completed.Add(TEXT("A"));
	TestEqual(TEXT("끝냈으면 -2 행"), Pick(TEXT("Master")), FName(TEXT("A_Done")));

	// 모르는 퀘스트 · 단계를 가리키면 문제
	FGrimDialogueDatabase Bad;
	TArray<FString> BadProblems;
	FGrimDialogueDatabase::BuildFromCSV(Npcs, FString(Lines) + TEXT("Bad,Master,Talk,Nope,0,0,\"x\"\nBad2,Master,Talk,A,9,0,\"x\"\n"), &Quests, Bad, BadProblems);
	TestTrue(TEXT("모르는 퀘스트 · 단계"), BadProblems.Num() >= 2);
	// 서비스가 있으면 결과 대사가 있어야 한다
	BadProblems.Reset();
	FGrimDialogueDatabase::BuildFromCSV(TEXT("Name,DisplayName,Title,Services,ServiceLabels\nMaster,\"스승\",\"\",\"Trial\",\"시험\"\n"), Lines, &Quests, Bad, BadProblems);
	TestTrue(TEXT("서비스 결과 대사 빠짐"), BadProblems.Num() > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimQuestSaveTest, "Grimoire.Quest.Save", QuestTestFlags)
bool FGrimQuestSaveTest::RunTest(const FString& Parameters)
{
	// 세이브 형식: 끝낸 목록 + 진행(ID · 단계 · 센 값). 읽기 → 쓰기 → 읽기가 같고, 시험(시간 제한)은 쓰지 않는다.
	if (!UGrimQuestSettings::GetQuestDatabase().Find(TEXT("Tutorial")) || !UGrimQuestSettings::GetQuestDatabase().Find(TEXT("Trial_Circle3")))
	{
		AddError(TEXT("저장소 퀘스트 표가 필요하다"));
		return false;
	}

	TArray<uint8> Bytes;
	{
		FMemoryWriter Writer(Bytes);
		int32 NumCompleted = 1;
		FString Done = TEXT("Trial_Circle2");
		Writer << NumCompleted << Done;
		int32 NumActive = 3;
		Writer << NumActive;
		auto WriteActive = [&Writer](const TCHAR* Id, int32 Step, TArray<int32> Counts)
		{
			FString IdString = Id;
			Writer << IdString << Step << Counts;
		};
		WriteActive(TEXT("Tutorial"), 9, { 0 });
		WriteActive(TEXT("NoSuchQuest"), 0, {});        // 표에서 사라진 퀘스트는 버린다
		WriteActive(TEXT("Trial_Circle3"), 1, { 2, 0 }); // 옛 세이브에 시험이 있어도 읽기는 한다
	}

	UGrimQuestComponent* First = NewObject<UGrimQuestComponent>();
	{
		FMemoryReader Reader(Bytes);
		First->ReadSaveData(Reader, 1);
	}
	const FGrimQuestLog& Loaded = First->GetLog();
	TestTrue(TEXT("끝낸 목록"), Loaded.IsCompleted(TEXT("Trial_Circle2")));
	TestEqual(TEXT("모르는 퀘스트는 버림"), Loaded.Active.Num(), 2);
	TestEqual(TEXT("Tutorial 단계 9"), StepOf(Loaded, TEXT("Tutorial")), 9);
	const FGrimQuestProgress* Trial = Loaded.FindActive(TEXT("Trial_Circle3"));
	TestTrue(TEXT("센 값 그대로"), Trial && Trial->Counts.Num() == 2 && Trial->Counts[0] == 2);

	TArray<uint8> Rewritten;
	{
		FMemoryWriter Writer(Rewritten);
		First->WriteSaveData(Writer);
	}
	UGrimQuestComponent* Second = NewObject<UGrimQuestComponent>();
	{
		FMemoryReader Reader(Rewritten);
		Second->ReadSaveData(Reader, Second->GetSaveVersion());
	}
	TestTrue(TEXT("다시 읽어도 끝낸 목록"), Second->GetLog().IsCompleted(TEXT("Trial_Circle2")));
	TestEqual(TEXT("다시 읽어도 Tutorial 9"), StepOf(Second->GetLog(), TEXT("Tutorial")), 9);
	TestFalse(TEXT("시험은 쓰지 않는다"), Second->GetLog().IsActive(TEXT("Trial_Circle3")));
	TestTrue(TEXT("세이브 범위 = 플레이어"), Second->GetSaveScope() == EGrimSaveScope::Player);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimQuestTutorialFlowTest, "Grimoire.Quest.TutorialFlow", QuestTestFlags)
bool FGrimQuestTutorialFlowTest::RunTest(const FString& Parameters)
{
	// 저장소 표로 튜토리얼을 처음부터 끝까지(가짜 세계): 완드 · 기본 공격 → 플라미스 · 아쿠엘 · 벤투스 → 양초 → 작물 → 앞마당 사냥
	// → 파크툼(길들이기) → 주방 요리 → 모닥불 · 침낭 · 첫 밤 → 거점. 그 뒤 2서클 · 3서클 시험, 사냥꾼 첫 등장.
	FGrimQuestDatabase Database;
	if (!LoadRepoQuests(*this, Database))
	{
		return false;
	}

	struct FWorld
	{
		TMap<FName, TMap<FName, float>> States; // 태그 → 상태 → 세기
		TMap<FName, int32> Counts;              // 여러 사물 태그 → 조건을 채운 수
		TSet<FName> NearMarkers;
		TSet<FName> KnownSpells;                // 시작할 때 아는 마법이 없다
		float Fatigue = 60.f;
		int32 Circle = 1;
	} W;

	FFakeFacts Facts;
	Facts.Query = [&W](const FGrimQuestObjectiveDef& Objective) -> int32
	{
		switch (Objective.Type)
		{
		case EGrimQuestObjectiveType::ElementalState:
		{
			if (const int32* Count = W.Counts.Find(Objective.Target))
			{
				return *Count;
			}
			const float Value = W.States.FindRef(Objective.Target).FindRef(Objective.State);
			return FGrimQuestModel::Compare(Objective.Compare, Value, Objective.Value) ? 1 : 0;
		}
		case EGrimQuestObjectiveType::ReachLocation: return W.NearMarkers.Contains(Objective.Target) ? 1 : 0;
		case EGrimQuestObjectiveType::KnowSpell: return W.KnownSpells.Contains(Objective.Target) ? 1 : 0;
		case EGrimQuestObjectiveType::ReachCircle: return W.Circle >= FMath::RoundToInt(Objective.Value) ? 1 : 0;
		case EGrimQuestObjectiveType::Attribute:
			return Objective.Target == TEXT("Fatigue") && FGrimQuestModel::Compare(Objective.Compare, W.Fatigue, Objective.Value) ? 1 : 0;
		default: return 0;
		}
	};

	FGrimQuestLog Log;
	TArray<FGrimQuestChange> Changes;
	float Now = 0.f;
	// 서버 한 주기(조회 + 단계 넘김). 새 단계는 다음 주기부터 본다 — 그래서 매 행동 뒤 두 번 돈다.
	auto Tick = [&]()
	{
		for (int32 Pass = 0; Pass < 2; ++Pass)
		{
			Now += 0.25f;
			Changes.Reset();
			FGrimQuestModel::Advance(Database, Log, &Facts, Now, Changes);
			const TArray<FGrimQuestChange> Seen = Changes;
			for (const FGrimQuestChange& Change : Seen)
			{
				const FGrimQuestDef* Def = Database.Find(Change.QuestId);
				if (Change.Type == EGrimQuestChangeType::QuestCompleted && Def && Def->IsTrial())
				{
					W.Circle = Def->TrialCircle; // 시험 통과 보상 = 승급(컴포넌트가 하는 일)
				}
			}
		}
	};
	auto Signal = [&](EGrimQuestObjectiveType Type, const TCHAR* Target) { FGrimQuestModel::ApplySignal(Database, Log, { Type, Target, 1 }); Tick(); };
	auto Talk = [&]() { Signal(EGrimQuestObjectiveType::TalkToNpc, TEXT("Master")); };
	auto Event = [&](const TCHAR* Id) { Signal(EGrimQuestObjectiveType::Event, Id); };
	auto Learn = [&](const TCHAR* Spell) { W.KnownSpells.Add(Spell); Tick(); };
	auto SetState = [&](const TCHAR* Tag, const TCHAR* State, float Value) { W.States.FindOrAdd(Tag).Add(State, Value); Tick(); };
	auto Arrive = [&](const TCHAR* Marker) { W.NearMarkers.Add(Marker); Tick(); };
	auto Step = [&]() { return StepOf(Log, TEXT("Tutorial")); };

	FGrimQuestModel::StartQuest(Database, Log, TEXT("Tutorial"), Now, Changes);
	Tick();
	TestEqual(TEXT("0 스승에게 말 걸기"), Step(), 0);
	TestTrue(TEXT("대화 뒤 기본 공격 스킬북을 준다"), Database.Find(TEXT("Tutorial"))->Steps[1].Gives.Num() == 1);
	Talk();
	TestEqual(TEXT("1 기본 공격 스킬북"), Step(), 1);
	Learn(TEXT("BasicAttack"));
	TestEqual(TEXT("2 과녁 부수기"), Step(), 2);
	Signal(EGrimQuestObjectiveType::HitTarget, TEXT("Tut_TrainingTarget"));
	TestEqual(TEXT("맞히기만으로는 모자람(부숴야)"), Step(), 2);
	Signal(EGrimQuestObjectiveType::BreakTarget, TEXT("Tut_TrainingTarget"));
	TestEqual(TEXT("3 스승에게"), Step(), 3);
	Talk();
	TestEqual(TEXT("4 세 마법 스킬북"), Step(), 4);
	Learn(TEXT("Ember"));
	Learn(TEXT("WaterJet"));
	TestEqual(TEXT("둘만 익히면 모자람"), Step(), 4);
	Learn(TEXT("Breeze"));
	TestEqual(TEXT("5 양초 켜기"), Step(), 5);
	SetState(TEXT("Tut_Candle"), TEXT("State.Burning"), 1.f);
	TestEqual(TEXT("6 불꽃 키우기"), Step(), 6);
	SetState(TEXT("Tut_Candle"), TEXT("State.Burning"), 2.f);
	TestEqual(TEXT("7 끄기"), Step(), 7);
	SetState(TEXT("Tut_Candle"), TEXT("State.Burning"), 0.f);
	TestEqual(TEXT("꺼지기만 해서는 모자람(젖어야)"), Step(), 7);
	SetState(TEXT("Tut_Candle"), TEXT("State.Wet"), 1.f);
	TestEqual(TEXT("8 작물에 물"), Step(), 8);
	W.Counts.Add(TEXT("Tut_Crop"), 2);
	Tick();
	TestEqual(TEXT("작물 둘로는 모자람"), Step(), 8);
	W.Counts.Add(TEXT("Tut_Crop"), 3);
	Tick();
	TestEqual(TEXT("9 앞마당"), Step(), 9);
	Arrive(TEXT("Tut_Yard"));
	TestEqual(TEXT("10 사냥"), Step(), 10);
	Event(TEXT("Creature.Killed.Small"));
	TestEqual(TEXT("11 고기"), Step(), 11);
	Event(TEXT("Item.Obtained.Meat"));
	TestEqual(TEXT("12 파크툼 스킬북"), Step(), 12);
	Learn(TEXT("Pactum"));
	TestEqual(TEXT("13 체력 30%"), Step(), 13);
	Event(TEXT("Creature.Tamed.Small"));
	TestEqual(TEXT("순서가 다른 이벤트는 세지 않는다"), Step(), 13);
	Event(TEXT("Creature.Weakened.Small"));
	TestEqual(TEXT("14 길들이기"), Step(), 14);
	Event(TEXT("Creature.Tamed.Small"));
	TestEqual(TEXT("15 주방"), Step(), 15);
	Arrive(TEXT("Tut_Kitchen"));
	TestEqual(TEXT("16 아궁이"), Step(), 16);
	SetState(TEXT("Tut_Stove"), TEXT("State.Burning"), 1.f);
	TestEqual(TEXT("17 요리"), Step(), 17);
	Event(TEXT("Cooking.IngredientsAdded"));
	Event(TEXT("Cooking.Completed"));
	TestEqual(TEXT("18 먹기"), Step(), 18);
	Event(TEXT("Food.Eaten"));
	TestEqual(TEXT("19 스승에게"), Step(), 19);
	Talk();
	TestEqual(TEXT("20 모닥불(해넘이)"), Step(), 20);
	TestFalse(TEXT("모닥불 단계는 해넘이로 시각을 옮긴다"), Database.Find(TEXT("Tutorial"))->Steps[20].TimeOfDayOnStart.IsEmpty());
	TestTrue(TEXT("첫 밤 전엔 승급 시험 못 함"), FGrimQuestModel::CanStart(Database, Log, TEXT("Trial_Circle2")) == EGrimQuestStartResult::PrerequisiteMissing);
	SetState(TEXT("Tut_Campfire"), TEXT("State.Burning"), 1.f);
	TestEqual(TEXT("21 침낭 · 첫 밤"), Step(), 21);
	Signal(EGrimQuestObjectiveType::SleepThroughNight, TEXT(""));
	TestEqual(TEXT("피로가 남아 있으면 모자람"), Step(), 21);
	W.Fatigue = 0.f;
	Tick();
	TestEqual(TEXT("22 아침 — 스승에게"), Step(), 22);
	Talk();
	TestEqual(TEXT("23 거점 세울 곳"), Step(), 23);
	TestTrue(TEXT("첫 밤 뒤엔 승급 시험을 청할 수 있다"), FGrimQuestModel::CanStart(Database, Log, TEXT("Trial_Circle2")) == EGrimQuestStartResult::Started);
	Arrive(TEXT("Tut_BaseSite"));
	TestEqual(TEXT("24 첫 거점"), Step(), 24);
	TestTrue(TEXT("튜토리얼 중엔 사냥꾼 퀘스트 없음"), FGrimQuestModel::CanStart(Database, Log, TEXT("Hunter_FirstEncounter")) == EGrimQuestStartResult::PrerequisiteMissing);
	Event(TEXT("Base.Founded"));
	TestTrue(TEXT("튜토리얼 끝"), Log.IsCompleted(TEXT("Tutorial")) && Log.Active.Num() == 0);

	// 승급 시험(튜토리얼 밖, 스승 서비스)
	TestTrue(TEXT("2서클 시험 시작"), FGrimQuestModel::StartQuest(Database, Log, TEXT("Trial_Circle2"), Now, Changes) == EGrimQuestStartResult::Started);
	Arrive(TEXT("Trial_Room"));
	TestEqual(TEXT("시험 단계 1(시간 제한)"), StepOf(Log, TEXT("Trial_Circle2")), 1);
	W.Counts.Add(TEXT("Trial2_Candle"), 3);
	Tick();
	TestTrue(TEXT("2서클 시험 통과"), Log.IsCompleted(TEXT("Trial_Circle2")));
	TestEqual(TEXT("서클 2"), W.Circle, 2);
	TestTrue(TEXT("3서클 시험 시작"), FGrimQuestModel::StartQuest(Database, Log, TEXT("Trial_Circle3"), Now, Changes) == EGrimQuestStartResult::Started);
	Tick();
	TestEqual(TEXT("시험의 방에 이미 있다 → 단계 1"), StepOf(Log, TEXT("Trial_Circle3")), 1);
	for (int32 Hit = 0; Hit < 3; ++Hit)
	{
		FGrimQuestModel::ApplySignal(Database, Log, { EGrimQuestObjectiveType::HitTarget, TEXT("Trial3_Target"), 1 });
	}
	Tick();
	TestTrue(TEXT("표적만으로는 모자람(젖은 양초)"), Log.IsActive(TEXT("Trial_Circle3")));
	SetState(TEXT("Trial3_WetCandle"), TEXT("State.Burning"), 1.f);
	TestTrue(TEXT("3서클 시험 통과"), Log.IsCompleted(TEXT("Trial_Circle3")));
	TestEqual(TEXT("서클 3"), W.Circle, 3);

	// 사냥꾼 첫 등장(발동 볼륨이 시작)
	TestTrue(TEXT("사냥꾼 퀘스트 시작"), FGrimQuestModel::StartQuest(Database, Log, TEXT("Hunter_FirstEncounter"), Now, Changes) == EGrimQuestStartResult::Started);
	Arrive(TEXT("Hunter_Camp"));
	TestEqual(TEXT("사냥꾼 단계 1 — 스승에게"), StepOf(Log, TEXT("Hunter_FirstEncounter")), 1);
	Talk();
	TestTrue(TEXT("사냥꾼 퀘스트 끝"), Log.IsCompleted(TEXT("Hunter_FirstEncounter")));
	return true;
}

#endif
