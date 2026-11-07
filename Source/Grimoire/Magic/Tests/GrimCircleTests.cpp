#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Magic/GrimCastFailure.h"
#include "Magic/GrimCircleTypes.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "Magic/GrimProficiencySettings.h"
#include "Magic/GrimProgressionComponent.h"
#include "Magic/GrimSpellAbility.h"
#include "Magic/GrimSpellbookComponent.h"
#include "Magic/GrimSpellDefinition.h"
#include "Core/GrimPlayerState.h"
#include "AbilitySystemComponent.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

// 서클 보상 · 승급 자격 (Q122 — docs/spec/progression 「세 축의 역할」). 표 수치는 제안이라 순수 계산은 테스트 표로 본다.
namespace GrimCircleTestsLocal // 유니티 빌드에서 다른 테스트 파일과 이름이 부딪히지 않게
{
	constexpr EAutomationTestFlags CircleTestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	const TCHAR* TestCSV =
		TEXT("Name,GapPowerPerCircle,GapPowerCap,GapManaPerCircle,GapManaCap,MaxEnchantTier,SlotSets,MaxSustainedSpells,EquipTier,Unlocks,UnlockText,PromotionGrades,PromotionCurrentCircleSpells,PromotionTask\n")
		TEXT("1,0.03,0.15,0.03,0.15,1,1,1,1,Access.A,\"A\",,0,\"\"\n")
		TEXT("2,0.03,0.15,0.03,0.15,1,2,1,2,,\"\",C:2,1,\"과제2\"\n")
		TEXT("3,0.03,0.15,0.03,0.15,1,3,2,2,Access.B|Access.C,\"B C\",B:2,1,\"과제3\"\n")
		TEXT("4,0.03,0.15,0.03,0.15,2,4,2,3,,\"\",B:3|A:1,1,\"과제4\"\n")
		TEXT("5,0.03,0.15,0.03,0.15,2,4,3,3,,\"\",B:3|A:1,1,\"과제5\"\n")
		TEXT("6,0.03,0.15,0.03,0.15,2,4,3,4,,\"\",B:4|A:2,1,\"과제6\"\n")
		TEXT("7,0.03,0.15,0.03,0.15,3,4,4,4,,\"\",A:3,1,\"과제7\"\n");

	const TArray<FName> GradeOrder = { TEXT("D"), TEXT("C"), TEXT("B"), TEXT("A"), TEXT("S") };

	bool MakeTable(FAutomationTestBase& Test, FGrimCircleTable& OutTable)
	{
		TArray<FString> Problems;
		UDataTable* Table = UGrimProgressionSettings::CreateCircleTableFromCSV(TestCSV, Problems);
		const bool bRead = FGrimCircleMath::ReadTable(Table, OutTable, GradeOrder, Problems);
		for (const FString& Problem : Problems)
		{
			Test.AddError(Problem);
		}
		return bRead && OutTable.GetMaxCircle() == 7;
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

	UGrimSpellDefinition* MakeSpell(const TCHAR* Name, int32 Circle)
	{
		UGrimSpellDefinition* Spell = NewObject<UGrimSpellDefinition>(GetTransientPackage(), Name);
		Spell->Circle = Circle;
		Spell->bTracksProficiency = true;
		Spell->ManaCost = 10.f;
		Spell->Cooldown = 1.f;
		return Spell;
	}
}

// 표 읽기 — 테스트 표 · 실제 표(Circles.csv)가 문제없이 읽히는지, 잘못된 표를 알리는지.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimCircleTableTest, "Grimoire.Magic.Circle.Table", GrimCircleTestsLocal::CircleTestFlags)
bool FGrimCircleTableTest::RunTest(const FString& Parameters)
{
	using namespace GrimCircleTestsLocal;
	FGrimCircleTable Table;
	if (!TestTrue(TEXT("테스트 표 읽힘"), MakeTable(*this, Table)))
	{
		return false;
	}
	TestEqual(TEXT("3서클 해금 ID 둘"), Table.Find(3)->Unlocks.Num(), 2);
	TestEqual(TEXT("4서클 자격 둘(B:3 · A:1)"), Table.Find(4)->PromotionGrades.Num(), 2);
	TestEqual(TEXT("4서클 자격 A 1"), Table.Find(4)->PromotionGrades[1].Count, 1);
	TestEqual(TEXT("표보다 높으면 마지막 줄"), Table.FindClamped(99)->Circle, 7);
	TestEqual(TEXT("0 이하면 첫 줄"), Table.FindClamped(0)->Circle, 1);

	// 잘못된 표: 빈틈 · 없는 등급 · 꼴이 틀린 자격
	{
		TArray<FString> Problems;
		UDataTable* Bad = UGrimProgressionSettings::CreateCircleTableFromCSV(
			TEXT("Name,SlotSets,PromotionGrades\n1,1,\n2,2,Q:2\n4,4,B3\n"), Problems);
		FGrimCircleTable BadTable;
		Problems.Reset();
		TestFalse(TEXT("잘못된 표는 문제를 알린다"), FGrimCircleMath::ReadTable(Bad, BadTable, GradeOrder, Problems));
		TestEqual(TEXT("빈틈(3 없음)에서 멈춘다"), BadTable.GetMaxCircle(), 2);
		TestTrue(TEXT("없는 등급 Q"), Problems.ContainsByPredicate([](const FString& P) { return P.Contains(TEXT("Q")); }));
	}

	// 실제 표: 10서클, 문제 없음, 해금은 서클이 오를수록 줄지 않는다
	TArray<FString> Problems;
	TArray<FName> RealOrder;
	UGrimProgressionSettings::GetGradeOrder(RealOrder);
	TestEqual(TEXT("숙련도 등급 다섯(D~S)"), RealOrder.Num(), 5);
	const FGrimCircleTable& Real = UGrimProgressionSettings::GetCircleTable();
	TestEqual(TEXT("Circles.csv = 10서클"), Real.GetMaxCircle(), 10);
	for (int32 Circle = 2; Circle <= Real.GetMaxCircle(); ++Circle)
	{
		const FGrimCircleStep& Prev = *Real.Find(Circle - 1);
		const FGrimCircleStep& Step = *Real.Find(Circle);
		TestTrue(FString::Printf(TEXT("%d서클: 슬롯 세트 · 동시 유지 · 장비 · 각인이 줄지 않음"), Circle),
			Step.SlotSets >= Prev.SlotSets && Step.MaxSustainedSpells >= Prev.MaxSustainedSpells && Step.EquipTier >= Prev.EquipTier && Step.MaxEnchantTier >= Prev.MaxEnchantTier);
		TestTrue(FString::Printf(TEXT("%d서클: 승급 자격이 있다"), Circle), Step.PromotionGrades.Num() > 0);
		for (const FGrimCircleGradeRequirement& Grade : Step.PromotionGrades)
		{
			TestTrue(FString::Printf(TEXT("%d서클: 등급 %s 가 등급표에 있다"), Circle, *Grade.Grade.ToString()), RealOrder.Contains(Grade.Grade));
		}
	}
	TestTrue(TEXT("마법서 설정 SpellSlotSets 이상으로 열지 않는다"), Real.FindClamped(10)->SlotSets <= UGrimSpellbookSettings::Get()->SpellSlotSets);
	return true;
}

// ① 수치 보정 — 격 차이 위력 · 마나 효율(상한), 숙련도와 곱.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimCircleGapTest, "Grimoire.Magic.Circle.GapBonus", GrimCircleTestsLocal::CircleTestFlags)
bool FGrimCircleGapTest::RunTest(const FString& Parameters)
{
	using namespace GrimCircleTestsLocal;
	FGrimCircleTable Table;
	if (!MakeTable(*this, Table))
	{
		return false;
	}
	TestEqual(TEXT("같은 서클: 보정 없음"), FGrimCircleMath::GetGapPowerBonus(Table, 3, 3), 0.f);
	TestEqual(TEXT("높은 서클 마법: 음수 없음"), FGrimCircleMath::GetGapPowerBonus(Table, 2, 5), 0.f);
	TestEqual(TEXT("3서클이 1서클 마법: +6%"), FGrimCircleMath::GetGapPowerBonus(Table, 3, 1), 0.06f, 0.0001f);
	TestEqual(TEXT("6서클이 1서클 마법: +15%(상한 닿음)"), FGrimCircleMath::GetGapPowerBonus(Table, 6, 1), 0.15f, 0.0001f);
	TestEqual(TEXT("7서클(표 끝 넘어 99 도): 상한"), FGrimCircleMath::GetGapPowerBonus(Table, 99, 1), 0.15f, 0.0001f);
	TestEqual(TEXT("위력 배율 1.09(4서클 · 1서클 마법)"), FGrimCircleMath::GetPowerMultiplier(Table, 4, 1), 1.09f, 0.0001f);
	TestEqual(TEXT("마나 배율 0.94(3서클 · 1서클 마법)"), FGrimCircleMath::GetManaMultiplier(Table, 3, 1), 0.94f, 0.0001f);
	TestEqual(TEXT("마나 배율 상한 0.85"), FGrimCircleMath::GetManaMultiplier(Table, 7, 1), 0.85f, 0.0001f);
	// 스펙 예: 숙련도 S(0.75) × 격 차이 상한(0.85) ≈ 0.64
	TestEqual(TEXT("숙련도 S × 격 차이 상한 ≈ 0.64"), 0.75f * FGrimCircleMath::GetManaMultiplier(Table, 7, 1), 0.6375f, 0.0001f);
	TestEqual(TEXT("빈 표: 보정 없음"), FGrimCircleMath::GetPowerMultiplier(FGrimCircleTable(), 5, 1), 1.f);
	return true;
}

// ② 해금 조회 — 슬롯 세트 · 동시 유지 · 장비 등급 · 각인 단계 · 출입 ID.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimCircleUnlockTest, "Grimoire.Magic.Circle.Unlocks", GrimCircleTestsLocal::CircleTestFlags)
bool FGrimCircleUnlockTest::RunTest(const FString& Parameters)
{
	using namespace GrimCircleTestsLocal;
	FGrimCircleTable Table;
	if (!MakeTable(*this, Table))
	{
		return false;
	}
	TestEqual(TEXT("1서클 슬롯 세트 1"), FGrimCircleMath::GetSlotSets(Table, 1, 4), 1);
	TestEqual(TEXT("3서클 슬롯 세트 3"), FGrimCircleMath::GetSlotSets(Table, 3, 4), 3);
	TestEqual(TEXT("빈 표면 Fallback"), FGrimCircleMath::GetSlotSets(FGrimCircleTable(), 3, 4), 4);
	TestEqual(TEXT("5서클 동시 유지 3"), FGrimCircleMath::GetMaxSustainedSpells(Table, 5, 1), 3);
	TestEqual(TEXT("4서클 각인 Ⅱ"), FGrimCircleMath::GetMaxEnchantTier(Table, 4, 1), 2);
	TestEqual(TEXT("7서클 각인 Ⅲ"), FGrimCircleMath::GetMaxEnchantTier(Table, 7, 1), 3);

	int32 Required = 0;
	TestTrue(TEXT("1서클은 1등급 장비"), FGrimCircleMath::CanWearTier(Table, 1, 1, Required));
	TestFalse(TEXT("1서클은 2등급 못 입음"), FGrimCircleMath::CanWearTier(Table, 1, 2, Required));
	TestEqual(TEXT("2등급은 2서클부터"), Required, 2);
	TestFalse(TEXT("2서클은 3등급 못 입음"), FGrimCircleMath::CanWearTier(Table, 2, 3, Required));
	TestEqual(TEXT("3등급은 4서클부터"), Required, 4);
	TestFalse(TEXT("표에 없는 등급(9)"), FGrimCircleMath::CanWearTier(Table, 7, 9, Required));
	TestEqual(TEXT("표에 없으면 끝 + 1"), Required, 8);
	TestTrue(TEXT("빈 표면 다 입음"), FGrimCircleMath::CanWearTier(FGrimCircleTable(), 1, 5, Required));

	TestEqual(TEXT("Access.B 는 3서클"), FGrimCircleMath::GetUnlockCircle(Table, TEXT("Access.B")), 3);
	TestFalse(TEXT("2서클엔 Access.C 닫힘"), FGrimCircleMath::IsUnlocked(Table, 2, TEXT("Access.C")));
	TestTrue(TEXT("5서클엔 Access.C 열림(누적)"), FGrimCircleMath::IsUnlocked(Table, 5, TEXT("Access.C")));
	TestFalse(TEXT("없는 ID 는 닫힘"), FGrimCircleMath::IsUnlocked(Table, 7, TEXT("Access.None")));
	return true;
}

// 승급 자격 — 등급별 마법 수(누적) + 지금 서클 마법 하나 이상, 실패 까닭 Param.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimCirclePromotionTest, "Grimoire.Magic.Circle.Promotion", GrimCircleTestsLocal::CircleTestFlags)
bool FGrimCirclePromotionTest::RunTest(const FString& Parameters)
{
	using namespace GrimCircleTestsLocal;
	FGrimCircleTable Table;
	if (!MakeTable(*this, Table))
	{
		return false;
	}
	// 등급 번호: D 0 · C 1 · B 2 · A 3 · S 4
	{
		const FGrimSpellGradeEntry Spells[] = { { 1, 1 }, { 1, 0 } };
		const FGrimCircleCheck Check = FGrimCircleMath::EvaluatePromotion(Table, 1, Spells, GradeOrder);
		TestEqual(TEXT("다음 2"), Check.NextCircle, 2);
		TestFalse(TEXT("C 이상 하나 — 모자람"), Check.bMet);
		TestEqual(TEXT("C 이상 1/2"), Check.Grades[0].Have, 1);
		TestEqual(TEXT("과제 글"), Check.Task.ToString(), FString(TEXT("과제2")));
	}
	{
		const FGrimSpellGradeEntry Spells[] = { { 1, 1 }, { 1, 4 } };
		TestTrue(TEXT("C · S 둘 — 2서클 자격(높은 등급도 센다)"), FGrimCircleMath::EvaluatePromotion(Table, 1, Spells, GradeOrder).bMet);
	}
	{
		// 2 → 3: B 이상 둘이지만 둘 다 1서클 마법 → 지금 서클(2) 마법이 없다
		const FGrimSpellGradeEntry Spells[] = { { 1, 2 }, { 1, 3 }, { 2, 1 } };
		const FGrimCircleCheck Check = FGrimCircleMath::EvaluatePromotion(Table, 2, Spells, GradeOrder);
		TestNull(TEXT("등급 조건은 채움"), Check.FirstUnmetGrade());
		TestEqual(TEXT("지금 서클 마법 0(2서클 마법은 C 뿐)"), Check.CurrentCircleSpells, 0);
		TestFalse(TEXT("자격 없음 — 낮은 마법만 갈고닦음"), Check.bMet);
	}
	{
		const FGrimSpellGradeEntry Spells[] = { { 1, 2 }, { 2, 2 } };
		TestTrue(TEXT("2서클 마법 B 하나 끼면 3서클 자격"), FGrimCircleMath::EvaluatePromotion(Table, 2, Spells, GradeOrder).bMet);
	}
	{
		// 3 → 4: B:3 | A:1 — A 는 B 이상에도 든다(누적)
		const FGrimSpellGradeEntry Spells[] = { { 1, 3 }, { 2, 2 }, { 3, 2 } };
		const FGrimCircleCheck Check = FGrimCircleMath::EvaluatePromotion(Table, 3, Spells, GradeOrder);
		TestEqual(TEXT("B 이상 3"), Check.Grades[0].Have, 3);
		TestEqual(TEXT("A 이상 1"), Check.Grades[1].Have, 1);
		TestTrue(TEXT("4서클 자격(누적으로 셈)"), Check.bMet);
	}
	{
		const FGrimSpellGradeEntry Spells[] = { { 1, 2 }, { 2, 2 }, { 3, 2 } };
		const FGrimCircleCheck Check = FGrimCircleMath::EvaluatePromotion(Table, 3, Spells, GradeOrder);
		TestFalse(TEXT("A 가 없으면 4서클 못 감"), Check.bMet);
		const FGrimCircleGradeCheck* Unmet = Check.FirstUnmetGrade();
		TestTrue(TEXT("모자란 것 = A"), Unmet && Unmet->Grade == FName(TEXT("A")));
		// 실패 알림 Param 왕복
		int32 GradeIndex = 0;
		int32 Required = 0;
		int32 Have = 0;
		FGrimCircleMath::UnpackGradeParam(FGrimCircleMath::PackGradeParam(Unmet ? Unmet->GradeIndex : 0, 1, 0), GradeIndex, Required, Have);
		TestEqual(TEXT("Param 등급 A = 3"), GradeIndex, 3);
		TestEqual(TEXT("Param 필요 1"), Required, 1);
		TestEqual(TEXT("Param 지금 0"), Have, 0);
		const FString Message = GrimCastFailure::GetMessage(EGrimCastFailReason::PromotionGrade, FGrimCircleMath::PackGradeParam(3, 1, 0)).ToString();
		TestTrue(TEXT("알림 글에 등급 A · 0/1"), Message.Contains(TEXT("A")) && Message.Contains(TEXT("0/1")));
		TestFalse(TEXT("승급 까닭은 피식 연출 없음"), GrimCastFailure::ShouldFizzle(EGrimCastFailReason::PromotionGrade));
		TestFalse(TEXT("장비 등급 까닭도 연출 없음"), GrimCastFailure::ShouldFizzle(EGrimCastFailReason::EquipTierTooLow));
	}
	{
		const FGrimSpellGradeEntry Spells[] = { { 7, 4 } };
		TestTrue(TEXT("표 끝(7)이면 더 없음"), FGrimCircleMath::EvaluatePromotion(Table, 7, Spells, GradeOrder).bAtMax);
	}
	return true;
}

// 마법서 · 진행 — 슬롯 세트 잠김, 봉인된 책, 승급(자격 · 봉인 풀림 · 새 세트 채움), 격 차이 마나 · 위력(실제 Circles.csv).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimCircleSpellbookTest, "Grimoire.Magic.Circle.Spellbook", GrimCircleTestsLocal::CircleTestFlags)
bool FGrimCircleSpellbookTest::RunTest(const FString& Parameters)
{
	using namespace GrimCircleTestsLocal;
	const FGrimCircleTable& Real = UGrimProgressionSettings::GetCircleTable();
	if (!TestTrue(TEXT("실제 표 있음"), Real.GetMaxCircle() >= 3))
	{
		return false;
	}
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
	UGrimSpellbookComponent* Spellbook = PlayerState->GetSpellbook();
	UGrimProgressionComponent* Progression = PlayerState->GetProgression();
	const int32 PerSet = UGrimSpellbookSettings::Get()->SpellSlotsPerSet;
	const int32 Sets1 = FGrimCircleMath::GetSlotSets(Real, 1, 1);
	const int32 Sets2 = FMath::Min(FGrimCircleMath::GetSlotSets(Real, 2, 1), UGrimSpellbookSettings::Get()->SpellSlotSets);

	// 1서클: 열린 세트만큼만 자동 슬롯, 잠긴 세트엔 놓지 못함
	TestEqual(TEXT("1서클 열린 세트 = 표"), Spellbook->GetUnlockedSlotSets(), Sets1);
	TArray<UGrimSpellDefinition*> Spells;
	for (int32 Index = 0; Index < Sets1 * PerSet + 1; ++Index)
	{
		UGrimSpellDefinition* Spell = MakeSpell(*FString::Printf(TEXT("CircleTest_Spell%d"), Index), 1);
		Spells.Add(Spell);
		TestTrue(TEXT("1서클 마법 배움"), Spellbook->LearnSpell(Spell));
	}
	UGrimSpellDefinition* Overflow = Spells.Last();
	TestFalse(TEXT("열린 칸이 다 차면 넘친 마법은 슬롯 없음"), Spellbook->GetSlotSpells().Contains(Overflow));
	Spellbook->AssignSlot(Sets1 * PerSet, Spells[0]);
	TestEqual(TEXT("잠긴 칸에 놓지 못함 — 원래 칸 그대로"), Spellbook->GetSlotSpells()[0].Get(), Spells[0]);
	Spellbook->SetActiveSlotSet(3);
	TestEqual(TEXT("잠긴 세트로 못 바꿈"), Spellbook->GetActiveSlotSet(), Sets1 - 1);

	// 봉인된 책: 2 · 3서클 책을 주우면 기록, 같은 책은 다시 기록하지 않음
	UGrimSpellDefinition* Circle2Book = MakeSpell(TEXT("CircleTest_Circle2"), 2);
	UGrimSpellDefinition* Circle3Book = MakeSpell(TEXT("CircleTest_Circle3"), 3);
	TestEqual(TEXT("봉인 없이 쓰면 서클 모자람"), Spellbook->UseSkillbook(Circle2Book), EGrimLearnResult::CircleTooLow);
	TestEqual(TEXT("주우면 봉인된 책"), Spellbook->UseSkillbook(Circle2Book, true), EGrimLearnResult::Sealed);
	TestEqual(TEXT("같은 책 또 → 서클 모자람(책 남음)"), Spellbook->UseSkillbook(Circle2Book, true), EGrimLearnResult::CircleTooLow);
	TestEqual(TEXT("3서클 책도 봉인"), Spellbook->UseSkillbook(Circle3Book, true), EGrimLearnResult::Sealed);
	TestEqual(TEXT("봉인 둘"), Spellbook->GetSealedSpells().Num(), 2);
	TestFalse(TEXT("봉인된 책은 아직 모름"), Spellbook->KnowsSpell(Circle2Book));

	// 승급 자격: C 이상 둘이 없으면 실패(까닭 알림), 채우면 오른다
	TestFalse(TEXT("자격 없음 → 승급 실패"), Progression->TryCircleUp(false));
	TestEqual(TEXT("1서클 그대로"), Progression->GetCircle(), 1);
	Spellbook->AddProficiencyXP(Spells[0], 100000.f);
	Spellbook->AddProficiencyXP(Spells[1], 100000.f);
	const FGrimCircleCheck Check = Progression->EvaluateNextCircle();
	TestTrue(TEXT("숙련도를 채우면 2서클 자격"), Check.bMet);
	TestTrue(TEXT("승급"), Progression->TryCircleUp(false));
	TestEqual(TEXT("2서클"), Progression->GetCircle(), 2);

	// 서클이 오르면: 2서클 책 봉인 풀려 배움, 3서클 책은 그대로, 새 세트에 넘친 마법이 들어감
	TestTrue(TEXT("2서클 책 저절로 배움"), Spellbook->KnowsSpell(Circle2Book));
	TestTrue(TEXT("3서클 책은 아직 봉인"), Spellbook->IsSealed(Circle3Book) && !Spellbook->KnowsSpell(Circle3Book));
	TestEqual(TEXT("열린 세트 = 2서클 표"), Spellbook->GetUnlockedSlotSets(), Sets2);
	if (Sets2 > Sets1)
	{
		const int32 OverflowSlot = Spellbook->GetSlotSpells().IndexOfByKey(Overflow);
		TestTrue(TEXT("넘친 마법이 새 세트에 들어감"), OverflowSlot >= Sets1 * PerSet && OverflowSlot < Sets2 * PerSet);
	}

	// 격 차이: 2서클이 1서클 마법을 쓰면 위력 ↑ · 마나 ↓(실제 표 값), 2서클 마법은 그대로
	const float PowerGap = FGrimCircleMath::GetPowerMultiplier(Real, 2, 1);
	TestEqual(TEXT("위력 배율(1서클 마법)"), UGrimSpellAbility::GetCirclePowerMultiplier(Spells[2], ASC), PowerGap, 0.0001f);
	TestEqual(TEXT("위력 배율(2서클 마법) 1"), UGrimSpellAbility::GetCirclePowerMultiplier(Circle2Book, ASC), 1.f, 0.0001f);
	const float ManaGap = FGrimCircleMath::GetManaMultiplier(Real, 2, 1);
	TestEqual(TEXT("시전자 마나 배율 = 숙련도(D 1) × 격 차이"), UGrimSpellAbility::GetCasterMultipliers(Spells[2], ASC).ManaCost, ManaGap, 0.0001f);
	TestTrue(TEXT("2서클이면 1서클 마법이 정말 세짐(표가 비지 않으면)"), PowerGap > 1.f && ManaGap < 1.f);

	// 세이브 v2: 봉인된 책이 남는다
	TArray<uint8> Bytes;
	{
		FMemoryWriter Writer(Bytes);
		Spellbook->WriteSaveData(Writer);
	}
	Spellbook->HandleCircleRaised(2); // 아무 일 없음(3서클 책은 그대로)
	{
		FMemoryReader Reader(Bytes);
		Spellbook->ReadSaveData(Reader, Spellbook->GetSaveVersion());
	}
	TestTrue(TEXT("세이브를 읽어도 3서클 책 봉인 그대로"), Spellbook->IsSealed(Circle3Book));

	PlayerState->Destroy();
	return true;
}

#endif
