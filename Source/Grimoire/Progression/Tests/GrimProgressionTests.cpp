#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Progression/GrimLevelComponent.h"
#include "Progression/GrimProgressionEffects.h"
#include "Progression/GrimProgressionSettings.h"
#include "Progression/GrimProgressionTags.h"
#include "Progression/GrimProgressionTypes.h"
#include "Progression/GrimSkillTreeComponent.h"
#include "Core/GrimPlayerState.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "Survival/GrimVitalAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

namespace GrimProgressionTestsLocal // 유니티 빌드에서 다른 테스트 파일과 이름이 부딪히지 않게
{
	constexpr EAutomationTestFlags ProgressionTestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	/** 레벨 4 개짜리 작은 곡선: 0 · 100 · 300 · 600, HP 5 · 5 · 10, 마나 4 · 4 · 8, 포인트 1 · 1 · 2 */
	const TCHAR* SmallLevelCSV = TEXT("Level,TotalXP,MaxHealth,MaxMana,SkillPoints\n")
		TEXT("1,0,0,0,0\n2,100,5,4,1\n3,300,5,4,1\n4,600,10,8,2\n");

	/** 트리 2 개 · 노드 5 개: A(1) → B(1) → C(2, 서클 3) / A → D(1), 다른 트리 E(1) */
	const TCHAR* SmallTreeCSV = TEXT("Name,Tree,Branch,Kind,DisplayName,Description,GridX,GridY,Prerequisites,Cost,RequiredCircle,Effects\n")
		TEXT("T.A,One,Fire,Power,가,설명,0,0,,1,1,Fire.Power=+5;MaxHealth=+10\n")
		TEXT("T.B,One,Fire,Efficiency,나,설명,0,1,T.A,1,1,ManaCostScale=*0.9;Fire.Power=+5\n")
		TEXT("T.C,One,Fire,Trait,다,설명,0,2,T.B,2,3,Fire.Trait.Wildfire=+1\n")
		TEXT("T.D,One,Fire,Survival,라,설명,1,1,T.A,1,1,ColdResist=+1\n")
		TEXT("T.E,Two,Holy,Power,마,설명,0,0,,1,1,MaxMana=+15;Unknown.Key=+3\n");

	bool LoadCurve(FAutomationTestBase& Test, const TCHAR* CSV, FGrimLevelCurve& OutCurve, TArray<FString>& OutProblems)
	{
		const UDataTable* Table = UGrimLevelSettings::CreateLevelTableFromCSV(CSV, OutProblems);
		return Test.TestNotNull(TEXT("레벨 표"), Table) && FGrimLevelMath::ReadCurve(Table, OutCurve, OutProblems);
	}

	bool LoadTree(FAutomationTestBase& Test, const TCHAR* CSV, FGrimSkillTreeDefinition& OutDefinition, TArray<FString>& OutProblems)
	{
		const UDataTable* Table = UGrimSkillTreeSettings::CreateNodeTableFromCSV(CSV, OutProblems);
		return Test.TestNotNull(TEXT("트리 표"), Table) && FGrimSkillTreeMath::BuildDefinition(Table, OutDefinition, OutProblems);
	}

	struct FTestWorld
	{
		UWorld* World = nullptr;

		FTestWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false);
			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
		}

		~FTestWorld()
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}
	};
}

// 레벨 곡선(순수): 경계 · 상한 · 경험 멈춤 · 포인트 · HP · 마나 합 · 막대
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimProgressionLevelCurveTest, "Grimoire.Progression.LevelCurve", GrimProgressionTestsLocal::ProgressionTestFlags)
bool FGrimProgressionLevelCurveTest::RunTest(const FString& Parameters)
{
	using namespace GrimProgressionTestsLocal;
	FGrimLevelCurve Curve;
	TArray<FString> Problems;
	if (!LoadCurve(*this, SmallLevelCSV, Curve, Problems))
	{
		AddError(FString::Join(Problems, TEXT(" / ")));
		return false;
	}
	TestEqual(TEXT("레벨 4 개"), Curve.Steps.Num(), 4);
	TestEqual(TEXT("상한 0 = 표 끝"), FGrimLevelMath::GetMaxLevel(Curve, 0), 4);
	TestEqual(TEXT("상한 3"), FGrimLevelMath::GetMaxLevel(Curve, 3), 3);
	TestEqual(TEXT("상한이 표보다 크면 표 끝"), FGrimLevelMath::GetMaxLevel(Curve, 50), 4);

	TestEqual(TEXT("0 → 1"), FGrimLevelMath::GetLevelForXP(Curve, 0.f, 0), 1);
	TestEqual(TEXT("99 → 1"), FGrimLevelMath::GetLevelForXP(Curve, 99.f, 0), 1);
	TestEqual(TEXT("100 → 2"), FGrimLevelMath::GetLevelForXP(Curve, 100.f, 0), 2);
	TestEqual(TEXT("599 → 3"), FGrimLevelMath::GetLevelForXP(Curve, 599.f, 0), 3);
	TestEqual(TEXT("9999 → 4"), FGrimLevelMath::GetLevelForXP(Curve, 9999.f, 0), 4);
	TestEqual(TEXT("상한 3: 9999 → 3"), FGrimLevelMath::GetLevelForXP(Curve, 9999.f, 3), 3);
	TestEqual(TEXT("상한 3 에서 경험이 멈춘다"), FGrimLevelMath::ClampXP(Curve, 9999.f, 3), 300.f);

	TestEqual(TEXT("레벨 1 포인트 0"), FGrimLevelMath::GetSkillPointsAtLevel(Curve, 1), 0);
	TestEqual(TEXT("레벨 3 포인트 2"), FGrimLevelMath::GetSkillPointsAtLevel(Curve, 3), 2);
	TestEqual(TEXT("레벨 4 포인트 4"), FGrimLevelMath::GetSkillPointsAtLevel(Curve, 4), 4);
	TestEqual(TEXT("레벨 4 HP +20"), FGrimLevelMath::GetMaxHealthBonusAtLevel(Curve, 4), 20.f);
	TestEqual(TEXT("레벨 3 마나 +8"), FGrimLevelMath::GetMaxManaBonusAtLevel(Curve, 3), 8.f);

	const FGrimLevelProgress Mid = FGrimLevelMath::GetProgress(Curve, 200.f, 0);
	TestEqual(TEXT("200 → 레벨 2"), Mid.Level, 2);
	TestEqual(TEXT("막대 절반"), Mid.Fraction, 0.5f, 0.001f);
	TestEqual(TEXT("다음 문턱 300"), Mid.NextLevelXP, 300.f);
	const FGrimLevelProgress Top = FGrimLevelMath::GetProgress(Curve, 600.f, 0);
	TestTrue(TEXT("상한"), Top.bAtCap);
	TestEqual(TEXT("상한 막대 1"), Top.Fraction, 1.f);

	// 잘못된 표: 빈틈 · 줄어드는 경험
	FGrimLevelCurve Bad;
	TArray<FString> BadProblems;
	const UDataTable* Gap = UGrimLevelSettings::CreateLevelTableFromCSV(TEXT("Level,TotalXP,MaxHealth,MaxMana,SkillPoints\n1,0,0,0,0\n3,100,0,0,1\n"), BadProblems);
	TestFalse(TEXT("빈틈 → 문제"), FGrimLevelMath::ReadCurve(Gap, Bad, BadProblems));
	BadProblems.Reset();
	const UDataTable* Down = UGrimLevelSettings::CreateLevelTableFromCSV(TEXT("Level,TotalXP,MaxHealth,MaxMana,SkillPoints\n1,0,0,0,0\n2,100,0,0,1\n3,50,0,0,1\n"), BadProblems);
	TestFalse(TEXT("줄어드는 경험 → 문제"), FGrimLevelMath::ReadCurve(Down, Bad, BadProblems));
	return true;
}

// 같은 출처 반복 감쇠 · 최소 배율 · 창 · 출처 규칙(ini)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimProgressionRepeatDecayTest, "Grimoire.Progression.RepeatDecay", GrimProgressionTestsLocal::ProgressionTestFlags)
bool FGrimProgressionRepeatDecayTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("처음 그대로"), FGrimLevelMath::ComputeXPGain(100.f, 0, 0.5f, 0.1f, 1.f), 100.f);
	TestEqual(TEXT("두 번째 절반"), FGrimLevelMath::ComputeXPGain(100.f, 1, 0.5f, 0.1f, 1.f), 50.f);
	TestEqual(TEXT("많이 되풀이해도 최소 배율"), FGrimLevelMath::ComputeXPGain(100.f, 20, 0.5f, 0.1f, 1.f), 10.f, 0.001f);
	TestEqual(TEXT("감쇠 1 = 그대로"), FGrimLevelMath::ComputeXPGain(100.f, 9, 1.f, 0.1f, 1.f), 100.f);
	TestEqual(TEXT("전역 배율"), FGrimLevelMath::ComputeXPGain(100.f, 0, 0.5f, 0.1f, 2.f), 200.f);
	TestEqual(TEXT("음수는 0"), FGrimLevelMath::ComputeXPGain(-5.f, 0, 0.5f, 0.1f, 1.f), 0.f);

	FGrimXPRepeatTracker Tracker;
	const FGameplayTag Hunt = GrimProgressionTags::XPSource_Hunt;
	const FGameplayTag Quest = GrimProgressionTags::XPSource_Quest;
	TestEqual(TEXT("사냥 처음"), Tracker.Register(Hunt, 0.0, 60.0), 0);
	TestEqual(TEXT("사냥 두 번째"), Tracker.Register(Hunt, 10.0, 60.0), 1);
	TestEqual(TEXT("다른 출처는 따로"), Tracker.Register(Quest, 10.0, 60.0), 0);
	TestEqual(TEXT("창이 지나면 다시 0"), Tracker.Register(Hunt, 100.0, 60.0), 0);

	const UGrimLevelSettings* Settings = UGrimLevelSettings::Get();
	TestEqual(TEXT("ini: 상한 30(오픈 베타)"), Settings->LevelCap, 30);
	TestEqual(TEXT("ini: 퀘스트는 감쇠 없음"), Settings->GetSourceRule(Quest).RepeatDecay, 1.f);
	TestTrue(TEXT("ini: 사냥은 감쇠"), Settings->GetSourceRule(Hunt).RepeatDecay < 1.f);
	TestTrue(TEXT("ini: 스킬북 경험 > 0"), Settings->GetSourceRule(GrimProgressionTags::XPSource_Skillbook).DefaultXP > 0.f);
	TestTrue(TEXT("ini: 지역 발견 경험 > 0"), Settings->GetSourceRule(GrimProgressionTags::XPSource_RegionDiscovery).DefaultXP > 0.f);
	const FGrimXPSourceRule Fallback = Settings->GetSourceRule(GrimProgressionTags::XPSource);
	TestEqual(TEXT("규칙 없는 출처 = 기본 감쇠"), Fallback.RepeatDecay, Settings->DefaultRepeatDecay);
	return true;
}

// 트리 순수 계산: 효과 읽기 · 선행 · 서클 · 포인트 · 비용 합 · 트리 비우기 · 효과 합 → GE 값 · 순환 검사
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimProgressionTreeMathTest, "Grimoire.Progression.TreeMath", GrimProgressionTestsLocal::ProgressionTestFlags)
bool FGrimProgressionTreeMathTest::RunTest(const FString& Parameters)
{
	using namespace GrimProgressionTestsLocal;

	TArray<FGrimSkillEffect> Effects;
	TestTrue(TEXT("효과 읽기"), FGrimSkillTreeMath::ParseEffects(TEXT("MaxMana=+10; ManaCostScale=*0.97 ;Wand.Durability=-10"), Effects));
	TestEqual(TEXT("효과 3 개"), Effects.Num(), 3);
	if (Effects.Num() == 3)
	{
		TestEqual(TEXT("더하기 10"), Effects[0].Value, 10.f);
		TestFalse(TEXT("더하기"), Effects[0].bMultiply);
		TestTrue(TEXT("곱하기"), Effects[1].bMultiply);
		TestEqual(TEXT("곱하기 0.97"), Effects[1].Value, 0.97f, 0.0001f);
		TestEqual(TEXT("빼기 -10"), Effects[2].Value, -10.f);
	}
	TestFalse(TEXT("= 없음 → 문제"), FGrimSkillTreeMath::ParseEffects(TEXT("MaxMana10"), Effects));
	TestFalse(TEXT("숫자 아님 → 문제"), FGrimSkillTreeMath::ParseEffects(TEXT("MaxMana=+ten"), Effects));
	TestEqual(TEXT("선행 둘"), FGrimSkillTreeMath::ParsePrerequisites(TEXT("A | B|")).Num(), 2);

	FGrimSkillTreeDefinition Definition;
	TArray<FString> Problems;
	if (!LoadTree(*this, SmallTreeCSV, Definition, Problems))
	{
		AddError(FString::Join(Problems, TEXT(" / ")));
		return false;
	}
	TestEqual(TEXT("노드 5"), Definition.Nodes.Num(), 5);
	TestEqual(TEXT("트리 2"), Definition.GetTreeIds().Num(), 2);
	TestEqual(TEXT("트리 One 비용 5"), FGrimSkillTreeMath::GetTotalCost(Definition, TEXT("One")), 5);
	TestEqual(TEXT("전체 노드 5"), FGrimSkillTreeMath::GetNodeCount(Definition), 5);

	const FName A(TEXT("T.A")), B(TEXT("T.B")), C(TEXT("T.C")), D(TEXT("T.D")), E(TEXT("T.E"));
	TArray<FName> Unlocked;
	TestEqual(TEXT("A 찍을 수 있음"), FGrimSkillTreeMath::CanUnlock(Definition, Unlocked, A, 1, 1), EGrimSkillUnlockResult::Unlocked);
	TestEqual(TEXT("B 앞 노드 없음"), FGrimSkillTreeMath::CanUnlock(Definition, Unlocked, B, 9, 9), EGrimSkillUnlockResult::MissingPrerequisite);
	TestEqual(TEXT("포인트 0"), FGrimSkillTreeMath::CanUnlock(Definition, Unlocked, A, 0, 1), EGrimSkillUnlockResult::NotEnoughPoints);
	TestEqual(TEXT("없는 노드"), FGrimSkillTreeMath::CanUnlock(Definition, Unlocked, TEXT("T.Z"), 9, 9), EGrimSkillUnlockResult::UnknownNode);
	Unlocked = { A, B };
	TestEqual(TEXT("이미 찍음"), FGrimSkillTreeMath::CanUnlock(Definition, Unlocked, A, 9, 9), EGrimSkillUnlockResult::AlreadyUnlocked);
	TestEqual(TEXT("C 서클 모자람"), FGrimSkillTreeMath::CanUnlock(Definition, Unlocked, C, 9, 2), EGrimSkillUnlockResult::CircleTooLow);
	TestEqual(TEXT("C 포인트 1 로 모자람(비용 2)"), FGrimSkillTreeMath::CanUnlock(Definition, Unlocked, C, 1, 3), EGrimSkillUnlockResult::NotEnoughPoints);
	TestEqual(TEXT("C 열림"), FGrimSkillTreeMath::CanUnlock(Definition, Unlocked, C, 2, 3), EGrimSkillUnlockResult::Unlocked);

	Unlocked = { A, B, D, E };
	TestEqual(TEXT("쓴 포인트 4"), FGrimSkillTreeMath::GetSpentPoints(Definition, Unlocked), 4);
	TestEqual(TEXT("트리 One 3"), FGrimSkillTreeMath::GetSpentPoints(Definition, Unlocked, TEXT("One")), 3);
	const TArray<FName> AfterReset = FGrimSkillTreeMath::RemoveTree(Definition, Unlocked, TEXT("One"));
	TestEqual(TEXT("트리 One 비우면 E 만"), AfterReset.Num(), 1);
	TestTrue(TEXT("E 남음"), AfterReset.Contains(E));

	// 효과 합 → GE 값
	TMap<FName, FGrimSkillEffectTotal> Totals;
	FGrimSkillTreeMath::SumEffects(Definition, Unlocked, Totals);
	TestEqual(TEXT("불 위력 +10"), Totals.FindRef(TEXT("Fire.Power")).Add, 10.f);
	TestEqual(TEXT("마나 소모 x0.9"), Totals.FindRef(TEXT("ManaCostScale")).Mul, 0.9f, 0.0001f);
	TArray<FName> Unknown;
	const FGrimProgressionBonus Bonus = FGrimProgressionEffects::MakeBonus(Totals, &Unknown);
	TestEqual(TEXT("GE 최대 HP +10"), Bonus.MaxHealth, 10.f);
	TestEqual(TEXT("GE 최대 마나 +15"), Bonus.MaxMana, 15.f);
	TestEqual(TEXT("GE 추위저항 +1"), Bonus.ColdResist, 1.f);
	TestEqual(TEXT("GE 마나 소모 x0.9"), Bonus.ManaCostScale, 0.9f, 0.0001f);
	TestEqual(TEXT("GE 이동 배율 그대로"), Bonus.MoveSpeedScale, 1.f);
	TestTrue(TEXT("불 위력은 태그로"), Bonus.GrantedTags.HasTagExact(GrimProgressionTags::FindNodeEffectTag(TEXT("Fire.Power"))));
	TestTrue(TEXT("모르는 키는 따로 알린다"), Unknown.Contains(FName(TEXT("Unknown.Key"))));
	TestTrue(TEXT("빈 합 = 중립"), FGrimProgressionEffects::MakeBonus(TMap<FName, FGrimSkillEffectTotal>()).IsNeutral());
	TestTrue(TEXT("어트리뷰트 키"), FGrimProgressionEffects::IsAttributeKey(TEXT("ColdResist")));
	TestFalse(TEXT("어트리뷰트 아님"), FGrimProgressionEffects::IsAttributeKey(TEXT("Fire.Power")));

	// 상한 추정: 가장 싼 것부터
	int32 Spent = 0;
	TestEqual(TEXT("포인트 3 → 노드 3(A · B · D 또는 E)"), FGrimSkillTreeMath::GetMaxNodesForPoints(Definition, 3, &Spent), 3);
	TestEqual(TEXT("포인트 3 다 씀"), Spent, 3);

	// 잘못된 표: 없는 선행 · 다른 트리 선행 · 순환
	FGrimSkillTreeDefinition Bad;
	TArray<FString> BadProblems;
	const TCHAR* BadCSV = TEXT("Name,Tree,Branch,Kind,DisplayName,Description,GridX,GridY,Prerequisites,Cost,RequiredCircle,Effects\n")
		TEXT("X.A,One,Fire,Power,가,,0,0,X.B,1,1,\nX.B,One,Fire,Power,나,,0,1,X.A,1,1,\nX.C,Two,Fire,Power,다,,0,0,X.A|X.Q,1,1,\n");
	TestFalse(TEXT("잘못된 표 → 문제"), LoadTree(*this, BadCSV, Bad, BadProblems));
	TestTrue(TEXT("문제 3 개 이상(없는 선행 · 다른 트리 · 순환)"), BadProblems.Num() >= 3);
	return true;
}

// 실제 트리 표: 트리 4 개(노말 · 4원소 · 빛 · 어둠), 갈래 · 트리 표시가 설정에 있음, 효과 키가 모두 어트리뷰트 또는 등록된 태그, 큰 노드는 서클 조건
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimProgressionTreeTableTest, "Grimoire.Progression.TreeTable", GrimProgressionTestsLocal::ProgressionTestFlags)
bool FGrimProgressionTreeTableTest::RunTest(const FString& Parameters)
{
	UGrimSkillTreeSettings::ResetDefinition();
	const FGrimSkillTreeDefinition& Definition = UGrimSkillTreeSettings::GetDefinition();
	const UGrimSkillTreeSettings* Settings = UGrimSkillTreeSettings::Get();
	if (!TestTrue(TEXT("트리 표를 읽었다"), Definition.Nodes.Num() > 0))
	{
		return false;
	}
	const TArray<FName> Trees = Settings->GetTreeOrder(Definition);
	TestEqual(TEXT("트리 4 개"), Trees.Num(), 4);
	for (const TCHAR* Expected : { TEXT("Normal"), TEXT("Elemental"), TEXT("Light"), TEXT("Dark") })
	{
		TestTrue(FString::Printf(TEXT("트리 %s"), Expected), Trees.Contains(FName(Expected)));
	}
	for (const TCHAR* Branch : { TEXT("Water"), TEXT("Ice"), TEXT("Fire"), TEXT("Wind"), TEXT("Earth"), TEXT("Plant") })
	{
		const FGrimSkillNode* Power = Definition.Find(FName(*FString::Printf(TEXT("%s.Power"), Branch)));
		TestTrue(FString::Printf(TEXT("4원소 갈래 %s"), Branch), Power && Power->Tree == FName(TEXT("Elemental")));
	}

	TSet<FName> TraitTrees;
	for (const FGrimSkillNode& Node : Definition.Nodes)
	{
		TestNotNull(FString::Printf(TEXT("%s 트리 표시"), *Node.Id.ToString()), Settings->FindTree(Node.Tree));
		TestNotNull(FString::Printf(TEXT("%s 갈래 표시"), *Node.Id.ToString()), Settings->FindBranch(Node.Branch));
		TestTrue(FString::Printf(TEXT("%s 비용 > 0"), *Node.Id.ToString()), Node.Cost > 0);
		TestFalse(FString::Printf(TEXT("%s 효과 있음"), *Node.Id.ToString()), Node.Effects.IsEmpty());
		for (const FGrimSkillEffect& Effect : Node.Effects)
		{
			const bool bKnown = FGrimProgressionEffects::IsAttributeKey(Effect.Key) || GrimProgressionTags::FindNodeEffectTag(Effect.Key).IsValid();
			TestTrue(FString::Printf(TEXT("%s 효과 키 %s 는 어트리뷰트 또는 등록된 태그(GrimProgressionTags.cpp)"), *Node.Id.ToString(), *Effect.Key.ToString()), bKnown);
		}
		if (Node.Kind == EGrimSkillNodeKind::Trait)
		{
			TraitTrees.Add(Node.Tree);
			TestTrue(FString::Printf(TEXT("큰 노드 %s 는 서클 조건"), *Node.Id.ToString()), Node.RequiredCircle > 1);
		}
		if (Node.Prerequisites.IsEmpty())
		{
			TestEqual(FString::Printf(TEXT("뿌리 노드 %s 는 맨 위 줄"), *Node.Id.ToString()), Node.Grid.Y, 0.0);
		}
	}
	TestEqual(TEXT("트리마다 큰 노드"), TraitTrees.Num(), Trees.Num());
	return true;
}

// 「포인트는 모자라게」: 레벨 상한까지 올려도 전체의 30~40% 만 찍힌다 — 비용으로도, 노드 수(가장 싼 것부터)로도.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimProgressionTreeBudgetTest, "Grimoire.Progression.TreeBudget", GrimProgressionTestsLocal::ProgressionTestFlags)
bool FGrimProgressionTreeBudgetTest::RunTest(const FString& Parameters)
{
	UGrimLevelSettings::ResetCurve();
	UGrimSkillTreeSettings::ResetDefinition();
	const FGrimLevelCurve& Curve = UGrimLevelSettings::GetCurve();
	const FGrimSkillTreeDefinition& Definition = UGrimSkillTreeSettings::GetDefinition();
	const int32 Cap = UGrimLevelSettings::Get()->LevelCap;
	if (!TestTrue(TEXT("레벨 표"), Curve.Steps.Num() > 1) || !TestTrue(TEXT("트리 표"), Definition.Nodes.Num() > 0))
	{
		return false;
	}
	TestTrue(TEXT("표가 상한까지 있다"), Curve.Steps.Num() >= Cap);

	const int32 MaxLevel = FGrimLevelMath::GetMaxLevel(Curve, Cap);
	const int32 Points = FGrimLevelMath::GetSkillPointsAtLevel(Curve, MaxLevel);
	const int32 TotalCost = FGrimSkillTreeMath::GetTotalCost(Definition);
	const int32 NodeCount = FGrimSkillTreeMath::GetNodeCount(Definition);
	int32 Spent = 0;
	const int32 MaxNodes = FGrimSkillTreeMath::GetMaxNodesForPoints(Definition, Points, &Spent);
	const float CostRatio = TotalCost > 0 ? static_cast<float>(Points) / TotalCost : 1.f;
	const float NodeRatio = NodeCount > 0 ? static_cast<float>(MaxNodes) / NodeCount : 1.f;

	AddInfo(FString::Printf(TEXT("레벨 %d: 포인트 %d / 비용 합 %d = %.1f%%, 노드 최대 %d / %d = %.1f%%"), MaxLevel, Points, TotalCost, CostRatio * 100.f, MaxNodes, NodeCount, NodeRatio * 100.f));
	for (const FName Tree : Definition.GetTreeIds())
	{
		AddInfo(FString::Printf(TEXT("  트리 %s: 노드 %d · 비용 %d"), *Tree.ToString(), FGrimSkillTreeMath::GetNodeCount(Definition, Tree), FGrimSkillTreeMath::GetTotalCost(Definition, Tree)));
	}
	TestTrue(TEXT("비용으로 30~40%"), CostRatio >= 0.30f && CostRatio <= 0.40f);
	TestTrue(TEXT("노드 수로 30~40%"), NodeRatio >= 0.30f && NodeRatio <= 0.40f);
	TestEqual(TEXT("상한 포인트를 다 쓸 수 있다"), Spent, Points);

	// 레벨이 오를 때마다 포인트 · 최대 HP · 마나가 줄지 않는다
	for (int32 Level = 2; Level <= MaxLevel; ++Level)
	{
		TestTrue(FString::Printf(TEXT("레벨 %d 포인트"), Level), FGrimLevelMath::GetSkillPointsAtLevel(Curve, Level) >= FGrimLevelMath::GetSkillPointsAtLevel(Curve, Level - 1));
		TestTrue(FString::Printf(TEXT("레벨 %d HP"), Level), FGrimLevelMath::GetMaxHealthBonusAtLevel(Curve, Level) > FGrimLevelMath::GetMaxHealthBonusAtLevel(Curve, Level - 1));
		TestTrue(FString::Printf(TEXT("레벨 %d 마나"), Level), FGrimLevelMath::GetMaxManaBonusAtLevel(Curve, Level) > FGrimLevelMath::GetMaxManaBonusAtLevel(Curve, Level - 1));
	}
	return true;
}

// PlayerState 에서: 경험 → 레벨 → 최대 HP · 마나 GE, 반복 감쇠, 첫 발견, 찍기 → 어트리뷰트 · 태그, 정화 의식(비용 거절 · 통과), 세이브 왕복.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimProgressionComponentTest, "Grimoire.Progression.Component", GrimProgressionTestsLocal::ProgressionTestFlags)
bool FGrimProgressionComponentTest::RunTest(const FString& Parameters)
{
	using namespace GrimProgressionTestsLocal;
	UGrimLevelSettings::ResetCurve();
	UGrimSkillTreeSettings::ResetDefinition();
	const FGrimLevelCurve& Curve = UGrimLevelSettings::GetCurve();

	FTestWorld TestWorld;
	AGrimPlayerState* PlayerState = TestWorld.World->SpawnActor<AGrimPlayerState>();
	if (!TestNotNull(TEXT("PlayerState"), PlayerState))
	{
		return false;
	}
	UAbilitySystemComponent* ASC = PlayerState->GetAbilitySystemComponent();
	ASC->InitAbilityActorInfo(PlayerState, PlayerState);
	if (!ASC->GetSet<UGrimMagicAttributeSet>())
	{
		ASC->AddSpawnedAttribute(PlayerState->GetMagicSet());
	}
	if (!ASC->GetSet<UGrimVitalAttributeSet>())
	{
		ASC->AddSpawnedAttribute(PlayerState->GetVitalSet());
	}
	ASC->SetNumericAttributeBase(UGrimVitalAttributeSet::GetMaxHealthAttribute(), 100.f);
	ASC->SetNumericAttributeBase(UGrimMagicAttributeSet::GetMaxManaAttribute(), 100.f);
	ASC->SetNumericAttributeBase(UGrimMagicAttributeSet::GetCircleAttribute(), 1.f);
	ASC->SetNumericAttributeBase(UGrimVitalAttributeSet::GetColdResistAttribute(), 0.f);

	UGrimLevelComponent* Level = PlayerState->GetCharacterLevel();
	UGrimSkillTreeComponent* Tree = PlayerState->GetSkillTree();
	if (!TestNotNull(TEXT("레벨 컴포넌트"), Level) || !TestNotNull(TEXT("트리 컴포넌트"), Tree))
	{
		return false;
	}
	TestEqual(TEXT("FindFor"), UGrimLevelComponent::FindFor(PlayerState), Level);
	TestEqual(TEXT("처음 레벨 1"), Level->GetLevel(), 1);
	TestEqual(TEXT("처음 포인트 0"), Tree->GetAvailablePoints(), 0);

	auto Attr = [ASC](const FGameplayAttribute& Attribute) { return ASC->GetNumericAttribute(Attribute); };

	// 경험 → 레벨 2: 최대 HP · 마나가 GE 로 오른다
	const float Need2 = FGrimLevelMath::GetXPForLevel(Curve, 2);
	TestEqual(TEXT("퀘스트 경험 그대로"), Level->AwardExperience(Need2, GrimProgressionTags::XPSource_Quest), Need2, 0.01f);
	TestEqual(TEXT("레벨 2"), Level->GetLevel(), 2);
	TestEqual(TEXT("최대 HP = 100 + 레벨 보너스"), Attr(UGrimVitalAttributeSet::GetMaxHealthAttribute()), 100.f + FGrimLevelMath::GetMaxHealthBonusAtLevel(Curve, 2), 0.01f);
	TestEqual(TEXT("최대 마나 = 100 + 레벨 보너스"), Attr(UGrimMagicAttributeSet::GetMaxManaAttribute()), 100.f + FGrimLevelMath::GetMaxManaBonusAtLevel(Curve, 2), 0.01f);
	TestEqual(TEXT("서클은 그대로(레벨과 무관)"), Attr(UGrimMagicAttributeSet::GetCircleAttribute()), 1.f);

	// 반복 감쇠: 사냥 두 번째는 덜
	const float FirstHunt = Level->AwardExperience(30.f, GrimProgressionTags::XPSource_Hunt);
	const float SecondHunt = Level->AwardExperience(30.f, GrimProgressionTags::XPSource_Hunt);
	TestEqual(TEXT("사냥 처음 30"), FirstHunt, 30.f, 0.01f);
	TestTrue(TEXT("사냥 두 번째는 덜"), SecondHunt < FirstHunt);

	// 첫 발견은 한 번만
	TestTrue(TEXT("새 지역 처음"), Level->AwardDiscovery(GrimProgressionTags::XPSource_RegionDiscovery, TEXT("SnowPeak")));
	TestFalse(TEXT("같은 지역 두 번째는 없음"), Level->AwardDiscovery(GrimProgressionTags::XPSource_RegionDiscovery, TEXT("SnowPeak")));
	TestTrue(TEXT("규칙 발견 처음"), Level->NotifyRuleDiscovered(TEXT("FireIgnitesGrass")));

	// 레벨 10: 포인트
	Level->SetLevel(10);
	TestEqual(TEXT("레벨 10"), Level->GetLevel(), 10);
	const int32 Points10 = FGrimLevelMath::GetSkillPointsAtLevel(Curve, 10);
	TestEqual(TEXT("레벨 10 포인트"), Tree->GetAvailablePoints(), Points10);
	TestEqual(TEXT("레벨 10 최대 HP"), Attr(UGrimVitalAttributeSet::GetMaxHealthAttribute()), 100.f + FGrimLevelMath::GetMaxHealthBonusAtLevel(Curve, 10), 0.01f);

	// 찍기: 불 위력 → 불 상태 → (서클 2) 불씨 품기 = 추위저항 +1
	TestEqual(TEXT("선행 없이 불씨 품기"), Tree->TryUnlock(TEXT("Fire.Survival")), EGrimSkillUnlockResult::MissingPrerequisite);
	TestEqual(TEXT("불 위력"), Tree->TryUnlock(TEXT("Fire.Power")), EGrimSkillUnlockResult::Unlocked);
	TestEqual(TEXT("불 상태"), Tree->TryUnlock(TEXT("Fire.Status")), EGrimSkillUnlockResult::Unlocked);
	TestEqual(TEXT("서클 1 이면 불씨 품기 막힘"), Tree->TryUnlock(TEXT("Fire.Survival")), EGrimSkillUnlockResult::CircleTooLow);
	ASC->SetNumericAttributeBase(UGrimMagicAttributeSet::GetCircleAttribute(), 2.f);
	TestEqual(TEXT("서클 2 → 불씨 품기"), Tree->TryUnlock(TEXT("Fire.Survival")), EGrimSkillUnlockResult::Unlocked);
	TestEqual(TEXT("추위저항 +1"), Attr(UGrimVitalAttributeSet::GetColdResistAttribute()), 1.f, 0.001f);
	TestTrue(TEXT("불 위력 태그"), ASC->HasMatchingGameplayTag(GrimProgressionTags::FindNodeEffectTag(TEXT("Fire.Power"))));
	TestEqual(TEXT("불 위력 합 +5"), Tree->GetEffectValue(TEXT("Fire.Power")), 5.f);
	TestEqual(TEXT("마나 그릇"), Tree->TryUnlock(TEXT("Mana.Power")), EGrimSkillUnlockResult::Unlocked);
	TestEqual(TEXT("최대 마나 = 100 + 레벨 + 노드"), Attr(UGrimMagicAttributeSet::GetMaxManaAttribute()),
		100.f + FGrimLevelMath::GetMaxManaBonusAtLevel(Curve, 10) + 10.f, 0.01f);
	TestEqual(TEXT("쓴 포인트 4"), Tree->GetSpentPoints(), 4);
	TestEqual(TEXT("남은 포인트"), Tree->GetAvailablePoints(), Points10 - 4);

	// 세이브 왕복
	TArray<uint8> LevelBytes;
	TArray<uint8> TreeBytes;
	{
		FMemoryWriter LevelWriter(LevelBytes);
		Level->WriteSaveData(LevelWriter);
		FMemoryWriter TreeWriter(TreeBytes);
		Tree->WriteSaveData(TreeWriter);
	}

	// 정화 의식: 비용을 못 내면 그대로, 내면 4원소만 비운다
	FGrimTreeResetCostCheck Refuse;
	Refuse.BindLambda([](FName, int32) { return false; });
	TestFalse(TEXT("비용 거절 → 그대로"), Tree->ResetTree(TEXT("Elemental"), Refuse));
	TestTrue(TEXT("불 위력 남음"), Tree->IsUnlocked(TEXT("Fire.Power")));
	int32 Refunded = 0;
	FGrimTreeResetCostCheck Accept;
	Accept.BindLambda([&Refunded](FName, int32 RefundPoints) { Refunded = RefundPoints; return true; });
	TestTrue(TEXT("정화 의식"), Tree->ResetTree(TEXT("Elemental"), Accept));
	TestEqual(TEXT("돌려받은 포인트 3"), Refunded, 3);
	TestFalse(TEXT("불 위력 지워짐"), Tree->IsUnlocked(TEXT("Fire.Power")));
	TestTrue(TEXT("다른 트리(마나 그릇)는 남음"), Tree->IsUnlocked(TEXT("Mana.Power")));
	TestEqual(TEXT("추위저항 0 으로"), Attr(UGrimVitalAttributeSet::GetColdResistAttribute()), 0.f, 0.001f);
	TestFalse(TEXT("불 위력 태그 빠짐"), ASC->HasMatchingGameplayTag(GrimProgressionTags::FindNodeEffectTag(TEXT("Fire.Power"))));
	TestFalse(TEXT("빈 트리는 비울 것이 없다"), Tree->ResetTree(TEXT("Elemental")));

	// 세이브 되돌리기: 레벨 1 로 내린 뒤 읽으면 레벨 10 · 노드 4 개 · 발견 기록
	Level->SetLevel(1);
	TestEqual(TEXT("레벨 1 로 내림"), Level->GetLevel(), 1);
	TestEqual(TEXT("레벨 1 최대 HP 100"), Attr(UGrimVitalAttributeSet::GetMaxHealthAttribute()), 100.f, 0.01f);
	{
		FMemoryReader LevelReader(LevelBytes);
		Level->ReadSaveData(LevelReader, Level->GetSaveVersion());
		FMemoryReader TreeReader(TreeBytes);
		Tree->ReadSaveData(TreeReader, Tree->GetSaveVersion());
	}
	TestEqual(TEXT("세이브: 레벨 10"), Level->GetLevel(), 10);
	TestEqual(TEXT("세이브: 노드 4 개"), Tree->GetUnlockedNodes().Num(), 4);
	TestEqual(TEXT("세이브: 추위저항 +1"), Attr(UGrimVitalAttributeSet::GetColdResistAttribute()), 1.f, 0.001f);
	TestTrue(TEXT("세이브: 발견 기록"), Level->HasDiscovered(GrimProgressionTags::XPSource_RegionDiscovery, TEXT("SnowPeak")));
	TestEqual(TEXT("세이브 범위 = Player"), Level->GetSaveScope(), EGrimSaveScope::Player);
	TestEqual(TEXT("세이브 id"), Tree->GetSaveId(), FName(TEXT("Progression.SkillTree")));

	// 상한: 아무리 받아도 상한 레벨
	Level->AwardExperience(1.0e9f, GrimProgressionTags::XPSource_Quest);
	TestEqual(TEXT("상한 레벨"), Level->GetLevel(), Level->GetMaxLevel());
	TestEqual(TEXT("상한에서 경험 멈춤"), Level->GetTotalXP(), FGrimLevelMath::GetXPForLevel(Curve, Level->GetMaxLevel()), 0.01f);
	return true;
}

#endif
