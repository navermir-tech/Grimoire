#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Magic/GrimMagicAttributeSet.h"
#include "Magic/GrimProficiencySettings.h"
#include "Magic/GrimProficiencyTypes.h"
#include "Magic/GrimProgressionComponent.h"
#include "Magic/GrimSpellAbility.h"
#include "Magic/GrimSpellbookComponent.h"
#include "Magic/GrimSpellDefinition.h"
#include "Core/GrimPlayerState.h"
#include "AbilitySystemComponent.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

namespace GrimProficiencyTestsLocal // 유니티 빌드에서 다른 테스트 파일과 이름이 부딪히지 않게
{
	constexpr EAutomationTestFlags ProficiencyTestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	/** 스펙 예시 표(D 0 · C 100 · B 300 · A 700 · S 1500) */
	TArray<FGrimProficiencyGrade> MakeSpecGrades()
	{
		auto Make = [](const TCHAR* Name, float XP, float Mana, float Cooldown, float Extra)
		{
			FGrimProficiencyGrade Grade;
			Grade.Grade = Name;
			Grade.XPToReach = XP;
			Grade.ManaCostMultiplier = Mana;
			Grade.CooldownMultiplier = Cooldown;
			Grade.ExtraEffectScale = Extra;
			return Grade;
		};
		return { Make(TEXT("D"), 0.f, 1.f, 1.f, 0.f), Make(TEXT("C"), 100.f, 0.95f, 0.95f, 0.25f), Make(TEXT("B"), 300.f, 0.9f, 0.9f, 0.5f),
			Make(TEXT("A"), 700.f, 0.85f, 0.85f, 0.75f), Make(TEXT("S"), 1500.f, 0.75f, 0.8f, 1.f) };
	}

	const TCHAR* SpecCSV = TEXT("Grade,DisplayName,XPToReach,ManaCostMultiplier,CooldownMultiplier,ExtraEffectScale\n")
		TEXT("D,입문,0,1.00,1.00,0.0\nC,익숙,100,0.95,0.95,0.25\nB,숙달,300,0.90,0.90,0.5\nA,능숙,700,0.85,0.85,0.75\nS,마스터,1500,0.75,0.80,1.0\n");

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

	UGrimSpellDefinition* MakeSpell(const TCHAR* Name, int32 Circle, bool bTracks = true)
	{
		UGrimSpellDefinition* Spell = NewObject<UGrimSpellDefinition>(GetTransientPackage(), Name);
		Spell->Circle = Circle;
		Spell->bTracksProficiency = bTracks;
		Spell->ManaCost = 10.f;
		Spell->Cooldown = 1.f;
		return Spell;
	}
}

// 등급 경계: 문턱 바로 아래 · 위, XPScale 로 문턱이 늘어나는지.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimProficiencyGradeTest, "Grimoire.Magic.Proficiency.Grade", GrimProficiencyTestsLocal::ProficiencyTestFlags)
bool FGrimProficiencyGradeTest::RunTest(const FString& Parameters)
{
	using namespace GrimProficiencyTestsLocal;
	const TArray<FGrimProficiencyGrade> Grades = MakeSpecGrades();
	TestEqual(TEXT("빈 표"), FGrimProficiencyMath::GetGradeIndex(50.f, TArray<FGrimProficiencyGrade>()), INDEX_NONE);
	TestEqual(TEXT("0 → D"), FGrimProficiencyMath::GetGradeIndex(0.f, Grades), 0);
	TestEqual(TEXT("99.9 → D"), FGrimProficiencyMath::GetGradeIndex(99.9f, Grades), 0);
	TestEqual(TEXT("100 → C"), FGrimProficiencyMath::GetGradeIndex(100.f, Grades), 1);
	TestEqual(TEXT("699 → B"), FGrimProficiencyMath::GetGradeIndex(699.f, Grades), 2);
	TestEqual(TEXT("1500 → S"), FGrimProficiencyMath::GetGradeIndex(1500.f, Grades), 4);
	TestEqual(TEXT("99999 → S"), FGrimProficiencyMath::GetGradeIndex(99999.f, Grades), 4);
	TestEqual(TEXT("XPScale 2: 199 → D"), FGrimProficiencyMath::GetGradeIndex(199.f, Grades, 2.f), 0);
	TestEqual(TEXT("XPScale 2: 200 → C"), FGrimProficiencyMath::GetGradeIndex(200.f, Grades, 2.f), 1);

	const FGrimProficiencyView Mid = FGrimProficiencyMath::MakeView(200.f, Grades);
	TestTrue(TEXT("숙련도 있음"), Mid.bTracked);
	TestEqual(TEXT("200 → C"), Mid.Grade, FName(TEXT("C")));
	TestEqual(TEXT("C 문턱"), Mid.CurrentGradeXP, 100.f);
	TestEqual(TEXT("B 문턱"), Mid.NextGradeXP, 300.f);
	TestEqual(TEXT("막대 절반"), Mid.Progress, 0.5f, 0.001f);
	TestFalse(TEXT("최고 아님"), Mid.bMaxGrade);

	const FGrimProficiencyView Max = FGrimProficiencyMath::MakeView(2000.f, Grades);
	TestTrue(TEXT("최고 등급"), Max.bMaxGrade);
	TestEqual(TEXT("최고 막대 1"), Max.Progress, 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimProficiencyMultipliersTest, "Grimoire.Magic.Proficiency.Multipliers", GrimProficiencyTestsLocal::ProficiencyTestFlags)
bool FGrimProficiencyMultipliersTest::RunTest(const FString& Parameters)
{
	using namespace GrimProficiencyTestsLocal;
	const TArray<FGrimProficiencyGrade> Grades = MakeSpecGrades();
	const FGrimProficiencyMultipliers D = FGrimProficiencyMath::GetMultipliers(Grades, 0);
	const FGrimProficiencyMultipliers S = FGrimProficiencyMath::GetMultipliers(Grades, 4);
	const FGrimProficiencyMultipliers Out = FGrimProficiencyMath::GetMultipliers(Grades, 9);
	TestEqual(TEXT("D 마나"), D.ManaCost, 1.f);
	TestEqual(TEXT("S 마나"), S.ManaCost, 0.75f);
	TestEqual(TEXT("S 쿨"), S.Cooldown, 0.8f);
	TestEqual(TEXT("범위 밖 = 기본"), Out.ManaCost, 1.f);
	TestEqual(TEXT("추가 효과 = 최대 × 배율"), FGrimProficiencyMath::GetExtraEffectValue(0.2f, FGrimProficiencyMath::GetMultipliers(Grades, 2)), 0.1f, 0.0001f);

	// 어빌리티 비용 · 쿨다운 식에 곱해진다(완드 없음).
	UGrimSpellDefinition* Spell = MakeSpell(TEXT("ProficiencyMultSpell"), 1);
	TestEqual(TEXT("S 비용 10 → 7.5"), UGrimSpellAbility::GetEffectiveManaCost(Spell, nullptr, S.ManaCost), 7.5f, 0.001f);
	TestEqual(TEXT("S 쿨 1 → 0.8"), UGrimSpellAbility::GetEffectiveCooldown(Spell, nullptr, S.Cooldown), 0.8f, 0.001f);
	TestEqual(TEXT("배율 없이 = 그대로"), UGrimSpellAbility::GetEffectiveManaCost(Spell, nullptr), 10.f, 0.001f);
	return true;
}

// 같은 대상 반복 감쇠: 1, 0.5, 0.25 … · 창이 지나면 다시 1.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimProficiencyRepeatDecayTest, "Grimoire.Magic.Proficiency.RepeatDecay", GrimProficiencyTestsLocal::ProficiencyTestFlags)
bool FGrimProficiencyRepeatDecayTest::RunTest(const FString& Parameters)
{
	using namespace GrimProficiencyTestsLocal;
	TestEqual(TEXT("처음"), FGrimProficiencyMath::ComputeXPGain(4.f, 0, 0.5f, 1.f), 4.f);
	TestEqual(TEXT("두 번째"), FGrimProficiencyMath::ComputeXPGain(4.f, 1, 0.5f, 1.f), 2.f);
	TestEqual(TEXT("세 번째"), FGrimProficiencyMath::ComputeXPGain(4.f, 2, 0.5f, 1.f), 1.f);
	TestEqual(TEXT("전역 배율"), FGrimProficiencyMath::ComputeXPGain(4.f, 0, 0.5f, 2.f), 8.f);
	TestEqual(TEXT("감쇠 1 = 그대로"), FGrimProficiencyMath::ComputeXPGain(4.f, 5, 1.f, 1.f), 4.f);

	UObject* SpellA = NewObject<UGrimSpellDefinition>(GetTransientPackage(), TEXT("RepeatSpellA"));
	UObject* SpellB = NewObject<UGrimSpellDefinition>(GetTransientPackage(), TEXT("RepeatSpellB"));
	UObject* Candle = NewObject<UGrimSpellDefinition>(GetTransientPackage(), TEXT("RepeatTarget"));
	FGrimRepeatTracker Tracker;
	FGrimRepeatKey KeyA{ SpellA, Candle };
	FGrimRepeatKey KeyB{ SpellB, Candle };
	TestEqual(TEXT("A 처음"), Tracker.Register(KeyA, 0.0, 30.0), 0);
	TestEqual(TEXT("A 두 번째"), Tracker.Register(KeyA, 1.0, 30.0), 1);
	TestEqual(TEXT("다른 마법은 따로"), Tracker.Register(KeyB, 2.0, 30.0), 0);
	FGrimRepeatKey KeyAHit{ SpellA, Candle, 1 };
	TestEqual(TEXT("시전과 명중은 따로"), Tracker.Register(KeyAHit, 2.0, 30.0), 0);
	TestEqual(TEXT("A 세 번째"), Tracker.Register(KeyA, 20.0, 30.0), 2);
	TestEqual(TEXT("창이 지나면 다시 0"), Tracker.Register(KeyA, 60.0, 30.0), 0);
	Tracker.Prune(100.0, 30.0);
	TestEqual(TEXT("오래된 기록 버림"), Tracker.Num(), 0);
	return true;
}

// 표를 바꾸면 결과가 따라 바뀐다(CSV 로 읽은 표 · 행 고치기).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimProficiencyTableEditTest, "Grimoire.Magic.Proficiency.TableEdit", GrimProficiencyTestsLocal::ProficiencyTestFlags)
bool FGrimProficiencyTableEditTest::RunTest(const FString& Parameters)
{
	using namespace GrimProficiencyTestsLocal;
	TArray<FString> Problems;
	UDataTable* Table = UGrimProficiencySettings::CreateGradeTableFromCSV(SpecCSV, Problems);
	if (!TestNotNull(TEXT("CSV 읽기"), Table))
	{
		return false;
	}
	TestEqual(TEXT("CSV 문제 없음"), Problems.Num(), 0);

	TArray<FGrimProficiencyGrade> Grades;
	FGrimProficiencyMath::ReadGrades(Table, Grades);
	TestEqual(TEXT("등급 5개"), Grades.Num(), 5);
	TestEqual(TEXT("행 순서 = 등급 순서"), Grades.Last().Grade, FName(TEXT("S")));
	TestEqual(TEXT("이름"), Grades[4].DisplayName.ToString(), FString(TEXT("마스터")));
	TestEqual(TEXT("150 → C"), FGrimProficiencyMath::GetGradeIndex(150.f, Grades), 1);

	// C 문턱을 200 으로 올리면 150 은 D.
	FGrimProficiencyGradeRow* RowC = Table->FindRow<FGrimProficiencyGradeRow>(TEXT("C"), TEXT("test"));
	if (TestNotNull(TEXT("C 행"), RowC))
	{
		RowC->XPToReach = 200.f;
		RowC->ManaCostMultiplier = 0.5f;
	}
	FGrimProficiencyMath::ReadGrades(Table, Grades);
	TestEqual(TEXT("고친 뒤 150 → D"), FGrimProficiencyMath::GetGradeIndex(150.f, Grades), 0);
	TestEqual(TEXT("고친 뒤 C 마나"), FGrimProficiencyMath::GetMultipliers(Grades, 1).ManaCost, 0.5f);

	// 등급을 하나 더(SS)해도 코드는 그대로.
	FGrimProficiencyGradeRow RowSS;
	RowSS.DisplayName = FText::FromString(TEXT("전설"));
	RowSS.XPToReach = 3000.f;
	RowSS.ManaCostMultiplier = 0.6f;
	RowSS.CooldownMultiplier = 0.6f;
	RowSS.ExtraEffectScale = 1.f;
	Table->AddRow(TEXT("SS"), RowSS);
	FGrimProficiencyMath::ReadGrades(Table, Grades);
	TestEqual(TEXT("등급 6개"), Grades.Num(), 6);
	TestEqual(TEXT("3000 → SS"), FGrimProficiencyMath::GetGradeIndex(3000.f, Grades), 5);

	// 실제 설정의 표(CSV 원본)도 읽힌다.
	UGrimProficiencySettings::ResetDefaultGradeTable();
	TArray<FGrimProficiencyGrade> Default;
	UGrimProficiencySettings::GetGradesFor(nullptr, Default);
	TestEqual(TEXT("ProficiencyGrades.csv 5등급"), Default.Num(), 5);
	return true;
}

// 이미 아는 마법의 스킬북: 지금 구간의 Fraction + Flat, S 면 0.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimProficiencySkillbookXPTest, "Grimoire.Magic.Proficiency.SkillbookXP", GrimProficiencyTestsLocal::ProficiencyTestFlags)
bool FGrimProficiencySkillbookXPTest::RunTest(const FString& Parameters)
{
	using namespace GrimProficiencyTestsLocal;
	const TArray<FGrimProficiencyGrade> Grades = MakeSpecGrades();
	TestEqual(TEXT("D 구간 100 × 0.4"), FGrimProficiencyMath::ComputeSkillbookXP(FGrimProficiencyMath::MakeView(10.f, Grades), 0.4f, 0.f), 40.f, 0.001f);
	TestEqual(TEXT("C 구간 200 × 0.4 + 5"), FGrimProficiencyMath::ComputeSkillbookXP(FGrimProficiencyMath::MakeView(150.f, Grades), 0.4f, 5.f), 85.f, 0.001f);
	TestEqual(TEXT("S 는 0"), FGrimProficiencyMath::ComputeSkillbookXP(FGrimProficiencyMath::MakeView(1500.f, Grades), 0.4f, 5.f), 0.f);
	TestEqual(TEXT("숙련도 없음 0"), FGrimProficiencyMath::ComputeSkillbookXP(FGrimProficiencyView(), 0.4f, 5.f), 0.f);
	return true;
}

// 서클 승급 조건(순수)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimProficiencyCircleUpTest, "Grimoire.Magic.Proficiency.CircleUp", GrimProficiencyTestsLocal::ProficiencyTestFlags)
bool FGrimProficiencyCircleUpTest::RunTest(const FString& Parameters)
{
	using namespace GrimProficiencyTestsLocal;
	TArray<FGrimCircleRequirement> Requirements;
	Requirements.Add({ 2, 3, 2 });
	Requirements.Add({ 3, 4, 5 });

	FGrimCircleCheck Check = UGrimProgressionSettings::Evaluate(Requirements, 1, 3, 1);
	TestFalse(TEXT("등급 합 모자람"), Check.bMet);
	TestEqual(TEXT("다음 = 2"), Check.NextCircle, 2);
	Check = UGrimProgressionSettings::Evaluate(Requirements, 1, 3, 2);
	TestTrue(TEXT("1 → 2 충족"), Check.bMet);
	Check = UGrimProgressionSettings::Evaluate(Requirements, 2, 3, 9);
	TestFalse(TEXT("2 → 3 마법 수 모자람"), Check.bMet);
	Check = UGrimProgressionSettings::Evaluate(Requirements, 3, 9, 99);
	TestTrue(TEXT("3 이 끝"), Check.bAtMax);
	TestFalse(TEXT("끝이면 충족 아님"), Check.bMet);

	// 설정 값: M2 는 3서클까지
	const FGrimCircleCheck Configured = UGrimProgressionSettings::Evaluate(UGrimProgressionSettings::Get()->CircleRequirements, 3, 99, 99);
	TestTrue(TEXT("ini: 3서클이 끝"), Configured.bAtMax);
	return true;
}

// 마법서에서: 배움 · 경험 · 배율 · 스킬북(배움 / 경험 / S 막힘 / 서클 모자람) · 슬롯 · 승급(최대 마나는 그대로).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimProficiencySpellbookTest, "Grimoire.Magic.Proficiency.Spellbook", GrimProficiencyTestsLocal::ProficiencyTestFlags)
bool FGrimProficiencySpellbookTest::RunTest(const FString& Parameters)
{
	using namespace GrimProficiencyTestsLocal;
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
	ASC->SetNumericAttributeBase(UGrimMagicAttributeSet::GetCircleAttribute(), 1.f);
	ASC->SetNumericAttributeBase(UGrimMagicAttributeSet::GetMaxManaAttribute(), 100.f);

	UGrimSpellbookComponent* Spellbook = PlayerState->GetSpellbook();
	UGrimProgressionComponent* Progression = PlayerState->GetProgression();
	TestTrue(TEXT("ASC → 마법서"), UGrimSpellbookComponent::FindForASC(ASC) == Spellbook);

	UGrimSpellDefinition* Ember = MakeSpell(TEXT("TestSpell_Ember"), 1);
	UGrimSpellDefinition* Fireball = MakeSpell(TEXT("TestSpell_Fireball"), 2);
	UGrimSpellDefinition* Basic = MakeSpell(TEXT("TestSpell_Basic"), 1, false);

	TestEqual(TEXT("모르는 1서클 → 배움"), Spellbook->UseSkillbook(Ember), EGrimLearnResult::Learned);
	TestTrue(TEXT("배웠다"), Spellbook->KnowsSpell(Ember));
	TestEqual(TEXT("자동 슬롯 0"), Spellbook->GetSlotSpells().Num() > 0 ? Spellbook->GetSlotSpells()[0].Get() : nullptr, Ember);
	TestEqual(TEXT("2서클 → 서클 모자람"), Spellbook->UseSkillbook(Fireball), EGrimLearnResult::CircleTooLow);
	TestTrue(TEXT("기본 공격(숙련도 없음) 배움"), Spellbook->LearnSpell(Basic));
	TestFalse(TEXT("기본 공격은 슬롯에 안 들어감"), Spellbook->GetSlotSpells().Contains(Basic));
	TestEqual(TEXT("기본 공격 스킬북 = 이미 앎"), Spellbook->PreviewSkillbook(Basic), EGrimLearnResult::AlreadyKnown);
	TestFalse(TEXT("기본 공격 숙련도 없음"), Spellbook->GetProficiency(Basic).bTracked);

	// 경험 · 등급 · 배율(표 = ProficiencyGrades.csv: C 100)
	Spellbook->AddProficiencyXP(Ember, 100.f);
	TestEqual(TEXT("100 → C"), Spellbook->GetProficiency(Ember).GradeIndex, 1);
	TestEqual(TEXT("C 마나 배율"), Spellbook->GetProficiencyMultipliers(Ember).ManaCost, 0.95f, 0.001f);
	// 생존 배율 어트리뷰트가 없으면 시전자 배율 = 숙련도 배율, 차단 태그 없음
	TestEqual(TEXT("시전자 마나 배율"), UGrimSpellAbility::GetCasterMultipliers(Ember, ASC).ManaCost, 0.95f, 0.001f);
	TestEqual(TEXT("시전자 쿨 배율"), UGrimSpellAbility::GetCasterMultipliers(Ember, ASC).Cooldown, 0.95f, 0.001f);
	TestFalse(TEXT("차단 태그 없음"), UGrimSpellAbility::HasCastBlockingTag(ASC));
	Spellbook->AddProficiencyXP(Basic, 100.f);
	TestEqual(TEXT("숙련도 없는 항목은 안 쌓임"), Spellbook->GetProficiencyXP(Basic), 0.f);

	// 같은 대상 반복 감쇠(1서클 곡선 XPPerMeaningfulHit 4 × 0.5^n)
	const float Before = Spellbook->GetProficiencyXP(Ember);
	Spellbook->AwardHitXP(Ember, PlayerState);
	Spellbook->AwardHitXP(Ember, PlayerState);
	const float Gain = Spellbook->GetProficiencyXP(Ember) - Before;
	const FGrimProficiencyCurve Curve = UGrimProficiencySettings::GetCurveFor(Ember);
	TestEqual(TEXT("명중 두 번 = 기본 + 절반"), Gain, Curve.XPPerMeaningfulHit * (1.f + UGrimProficiencySettings::Get()->RepeatTargetDecay) * UGrimProficiencySettings::Get()->GlobalXPRate, 0.01f);

	// 아는 마법 스킬북 → 지금 구간의 40%
	const FGrimProficiencyView ViewBefore = Spellbook->GetProficiency(Ember);
	TestEqual(TEXT("아는 마법 → 숙련도"), Spellbook->UseSkillbook(Ember), EGrimLearnResult::ProficiencyGained);
	const float Expected = FGrimProficiencyMath::ComputeSkillbookXP(ViewBefore, UGrimSpellbookSettings::Get()->SkillbookXPFractionOfGrade, UGrimSpellbookSettings::Get()->SkillbookFlatXP);
	TestEqual(TEXT("스킬북 경험"), Spellbook->GetProficiencyXP(Ember) - ViewBefore.XP, Expected, 0.01f);

	// S 면 못 쓴다
	Spellbook->AddProficiencyXP(Ember, 100000.f);
	TestTrue(TEXT("S"), Spellbook->GetProficiency(Ember).bMaxGrade);
	TestEqual(TEXT("S 스킬북 → 못 씀"), Spellbook->UseSkillbook(Ember), EGrimLearnResult::AlreadyMastered);

	// 승급: 조건 미달 → 실패, 강제 → 2서클, 최대 마나는 그대로(레벨 몫)
	const FGrimCircleCheck Check = Progression->EvaluateNextCircle();
	TestEqual(TEXT("숙련도 마법 수 1"), Check.KnownSpells, 1);
	TestFalse(TEXT("1 → 2 조건 미달"), Progression->TryCircleUp(false));
	TestTrue(TEXT("강제 승급"), Progression->TryCircleUp(true));
	TestEqual(TEXT("2서클"), Progression->GetCircle(), 2);
	TestEqual(TEXT("최대 마나 그대로"), ASC->GetNumericAttributeBase(UGrimMagicAttributeSet::GetMaxManaAttribute()), 100.f);
	TestEqual(TEXT("이제 화염구 배움"), Spellbook->UseSkillbook(Fireball), EGrimLearnResult::Learned);
	TestEqual(TEXT("화염구는 슬롯 1"), Spellbook->GetSlotSpells()[1].Get(), Fireball);

	// 슬롯 옮기기: 같은 마법은 한 칸에만
	Spellbook->AssignSlot(5, Ember);
	TestNull(TEXT("옛 칸 비움"), Spellbook->GetSlotSpells()[0].Get());
	TestEqual(TEXT("새 칸"), Spellbook->GetSlotSpells()[5].Get(), Ember);
	Spellbook->SetActiveSlotSet(99);
	TestEqual(TEXT("세트 번호는 범위로 자름"), Spellbook->GetActiveSlotSet(), UGrimSpellbookSettings::Get()->SpellSlotSets - 1);

	PlayerState->Destroy();
	return true;
}

#endif
