#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UI/GrimUIData.h"
#include "UI/GrimUIMath.h"
#include "UI/GrimUISettings.h"
#include "Core/GrimGameplayTags.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "Survival/GrimVitalAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

namespace GrimUITestsLocal // 유니티 빌드에서 다른 테스트 파일과 이름이 부딪히지 않게
{
	constexpr EAutomationTestFlags UITestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimUIBarFractionTest, "Grimoire.UI.BarFraction", GrimUITestsLocal::UITestFlags)
bool FGrimUIBarFractionTest::RunTest(const FString& Parameters)
{
	using namespace GrimUITestsLocal;
	TestEqual(TEXT("절반"), FGrimUIMath::BarFraction(50.f, 0.f, 100.f), 0.5f);
	TestEqual(TEXT("범위 밖 위"), FGrimUIMath::BarFraction(150.f, 0.f, 100.f), 1.f);
	TestEqual(TEXT("범위 밖 아래"), FGrimUIMath::BarFraction(-5.f, 0.f, 100.f), 0.f);
	TestEqual(TEXT("체온 37 (30~44)"), FGrimUIMath::BarFraction(37.f, 30.f, 44.f), 0.5f, 0.001f);
	TestEqual(TEXT("범위 없음"), FGrimUIMath::BarFraction(5.f, 10.f, 10.f), 0.f);
	TestEqual(TEXT("쿨다운 절반"), FGrimUIMath::CooldownFraction(0.3f, 0.6f), 0.5f, 0.001f);
	TestEqual(TEXT("쿨다운 없음"), FGrimUIMath::CooldownFraction(0.f, 0.f), 0.f);
	TestEqual(TEXT("쿨다운 1초 미만 글"), FGrimUIMath::FormatCooldown(0.46f).ToString(), FString(TEXT("0.5")));
	TestEqual(TEXT("쿨다운 초 글"), FGrimUIMath::FormatCooldown(2.2f).ToString(), FString(TEXT("3")));
	TestTrue(TEXT("쿨다운 0 = 빈 글"), FGrimUIMath::FormatCooldown(0.f).IsEmpty());
	return true;
}

// 어트리뷰트를 이름으로: "Health" · "GrimVitalAttributeSet.Health" · 없는 이름은 숨김.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimUIAttributeNameTest, "Grimoire.UI.AttributeName", GrimUITestsLocal::UITestFlags)
bool FGrimUIAttributeNameTest::RunTest(const FString& Parameters)
{
	using namespace GrimUITestsLocal;
	FString Set, Property;
	TestTrue(TEXT("셋.속성"), FGrimUIMath::SplitAttributeName(TEXT(" GrimVitalAttributeSet.Health "), Set, Property));
	TestEqual(TEXT("셋"), Set, FString(TEXT("GrimVitalAttributeSet")));
	TestEqual(TEXT("속성"), Property, FString(TEXT("Health")));
	TestFalse(TEXT("속성만"), FGrimUIMath::SplitAttributeName(TEXT("Mana"), Set, Property));
	TestTrue(TEXT("셋 비움"), Set.IsEmpty());
	TestTrue(TEXT("U 접두 무시"), FGrimUIMath::AttributeSetNameMatches(TEXT("GrimVitalAttributeSet"), TEXT("UGrimVitalAttributeSet")));
	TestTrue(TEXT("빈 셋 = 아무 셋"), FGrimUIMath::AttributeSetNameMatches(TEXT("GrimMagicAttributeSet"), FString()));
	TestFalse(TEXT("다른 셋"), FGrimUIMath::AttributeSetNameMatches(TEXT("GrimMagicAttributeSet"), TEXT("GrimVitalAttributeSet")));

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	{
		AActor* Actor = World->SpawnActor<AActor>();
		UAbilitySystemComponent* ASC = NewObject<UAbilitySystemComponent>(Actor, TEXT("ASC"));
		ASC->RegisterComponent();
		ASC->AddAttributeSetSubobject(NewObject<UGrimMagicAttributeSet>(Actor));
		ASC->AddAttributeSetSubobject(NewObject<UGrimVitalAttributeSet>(Actor));
		ASC->InitAbilityActorInfo(Actor, Actor);

		FGameplayAttribute Attribute;
		TestTrue(TEXT("Health 찾음"), GrimUIData::FindAttribute(ASC, TEXT("Health"), Attribute));
		TestTrue(TEXT("Health = Vital"), Attribute == UGrimVitalAttributeSet::GetHealthAttribute());
		TestTrue(TEXT("셋 이름으로 Mana"), GrimUIData::FindAttribute(ASC, TEXT("GrimMagicAttributeSet.Mana"), Attribute));
		TestTrue(TEXT("Mana = Magic"), Attribute == UGrimMagicAttributeSet::GetManaAttribute());
		TestFalse(TEXT("셋이 틀리면 없음"), GrimUIData::FindAttribute(ASC, TEXT("GrimMagicAttributeSet.Health"), Attribute));
		TestFalse(TEXT("없는 어트리뷰트(피곤함 — 생존 갈래 전)"), GrimUIData::FindAttribute(ASC, TEXT("NoSuchAttributeForTest"), Attribute));
		TestFalse(TEXT("ASC 없음"), GrimUIData::FindAttribute(nullptr, TEXT("Health"), Attribute));
	}
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimUIElementFilterTest, "Grimoire.UI.ElementFilter", GrimUITestsLocal::UITestFlags)
bool FGrimUIElementFilterTest::RunTest(const FString& Parameters)
{
	using namespace GrimUITestsLocal;
	using namespace GrimTags;
	TestTrue(TEXT("필터 없음 = 모두"), FGrimUIMath::MatchesElementFilter(Element_Fire, FGameplayTag()));
	TestTrue(TEXT("필터 없음 = 무속성도"), FGrimUIMath::MatchesElementFilter(FGameplayTag(), FGameplayTag()));
	TestTrue(TEXT("불 = 불"), FGrimUIMath::MatchesElementFilter(Element_Fire, Element_Fire));
	TestFalse(TEXT("물 ≠ 불"), FGrimUIMath::MatchesElementFilter(Element_Water, Element_Fire));
	TestFalse(TEXT("무속성은 속성 필터에 안 걸림"), FGrimUIMath::MatchesElementFilter(FGameplayTag(), Element_Fire));

	const UGrimUISettings* Settings = UGrimUISettings::Get();
	const FGrimElementDisplay* Fire = Settings->FindElementDisplay(Element_Fire);
	if (TestNotNull(TEXT("ini: 불 표시"), Fire))
	{
		TestEqual(TEXT("불 이름"), Fire->Name.ToString(), FString(TEXT("불")));
	}
	TestNull(TEXT("무속성 표시 없음"), Settings->FindElementDisplay(FGameplayTag()));
	return true;
}

// 그리모어 목록: 숙련도 없는 항목(기본 공격) → 서클 → 배운 순서, 필터.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimUIGrimoireOrderTest, "Grimoire.UI.GrimoireOrder", GrimUITestsLocal::UITestFlags)
bool FGrimUIGrimoireOrderTest::RunTest(const FString& Parameters)
{
	using namespace GrimUITestsLocal;
	using namespace GrimTags;
	TArray<FGrimGrimoireEntryKey> Keys;
	auto Add = [&Keys](int32 Circle, bool bTracked, const FGameplayTag& Element)
	{
		FGrimGrimoireEntryKey& Key = Keys.AddDefaulted_GetRef();
		Key.KnownIndex = Keys.Num() - 1;
		Key.Circle = Circle;
		Key.bTracked = bTracked;
		Key.Element = Element;
	};
	Add(2, true, Element_Fire);   // 0 화염구
	Add(1, true, Element_Fire);   // 1 불씨
	Add(1, true, Element_Water);  // 2 물줄기
	Add(1, false, FGameplayTag()); // 3 기본 공격

	const TArray<int32> All = FGrimUIMath::BuildGrimoireOrder(Keys, FGameplayTag());
	TestEqual(TEXT("전체 4"), All.Num(), 4);
	if (All.Num() == 4)
	{
		TestEqual(TEXT("기본 공격이 맨 앞"), All[0], 3);
		TestEqual(TEXT("1서클 배운 순: 불씨"), All[1], 1);
		TestEqual(TEXT("1서클 배운 순: 물줄기"), All[2], 2);
		TestEqual(TEXT("2서클 화염구"), All[3], 0);
	}
	const TArray<int32> Fire = FGrimUIMath::BuildGrimoireOrder(Keys, Element_Fire);
	TestEqual(TEXT("불 필터 2"), Fire.Num(), 2);
	if (Fire.Num() == 2)
	{
		TestEqual(TEXT("불씨 먼저"), Fire[0], 1);
	}
	TestEqual(TEXT("등급 글자"), FGrimUIMath::GradeLabel(true, TEXT("C")).ToString(), FString(TEXT("C")));
	TestEqual(TEXT("숙련도 없음 = —"), FGrimUIMath::GradeLabel(false, NAME_None).ToString(), FString(TEXT("—")));
	return true;
}

// HUD 슬롯: 4칸 세트 여러 개, 보이는 세트, 기본 공격 자리.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimUISlotLayoutTest, "Grimoire.UI.SlotLayout", GrimUITestsLocal::UITestFlags)
bool FGrimUISlotLayoutTest::RunTest(const FString& Parameters)
{
	using namespace GrimUITestsLocal;
	// 배운 마법: 0 불씨 · 1 물줄기 · 2 기본 공격(숙련도 없음) · 3 미풍 · 4 화염구
	const TArray<bool> Tracked = { true, true, false, true, true };
	// 슬롯(세트 2 × 4칸): 세트 0 = 불씨 · 물줄기 · 빈 칸 · 미풍, 세트 1 = 화염구
	const TArray<int32> SlotKnown = { 0, 1, INDEX_NONE, 3, 4, INDEX_NONE, INDEX_NONE, INDEX_NONE };

	FGrimSlotLayout Layout = FGrimUIMath::ComputeSlotLayout(SlotKnown, Tracked, 0, 4, 2, true);
	TestEqual(TEXT("기본 공격 자리"), Layout.BasicIndex, 2);
	TestEqual(TEXT("칸 4"), Layout.Slots.Num(), 4);
	TestEqual(TEXT("세트 0 칸 0"), Layout.Slots[0], 0);
	TestEqual(TEXT("세트 0 빈 칸"), Layout.Slots[2], (int32)INDEX_NONE);
	TestEqual(TEXT("세트 0 칸 3"), Layout.Slots[3], 3);

	Layout = FGrimUIMath::ComputeSlotLayout(SlotKnown, Tracked, 1, 4, 2, true);
	TestEqual(TEXT("세트 1"), Layout.SetIndex, 1);
	TestEqual(TEXT("세트 1 칸 0 = 화염구"), Layout.Slots[0], 4);

	Layout = FGrimUIMath::ComputeSlotLayout(SlotKnown, Tracked, 7, 4, 2, true);
	TestEqual(TEXT("세트 번호 자름"), Layout.SetIndex, 1);

	// 슬롯에 기본 공격이 들어 있어도 따로 두면 빈 칸으로, 배운 범위 밖 번호도 빈 칸
	const TArray<int32> Odd = { 2, 9 };
	Layout = FGrimUIMath::ComputeSlotLayout(Odd, Tracked, 0, 4, 1, true);
	TestEqual(TEXT("기본 공격은 슬롯에서 뺌"), Layout.Slots[0], (int32)INDEX_NONE);
	TestEqual(TEXT("범위 밖 = 빈 칸"), Layout.Slots[1], (int32)INDEX_NONE);
	Layout = FGrimUIMath::ComputeSlotLayout(Odd, Tracked, 0, 4, 1, false);
	TestEqual(TEXT("따로 안 두면 슬롯에"), Layout.Slots[0], 2);
	TestEqual(TEXT("따로 안 두면 기본 자리 없음"), Layout.BasicIndex, (int32)INDEX_NONE);
	return true;
}

// ini 의 HUD 설정: 막대 다섯(HP · 마나 · 배고픔 · 체온 · 피곤함), 서클 호칭.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimUISettingsTest, "Grimoire.UI.Settings", GrimUITestsLocal::UITestFlags)
bool FGrimUISettingsTest::RunTest(const FString& Parameters)
{
	using namespace GrimUITestsLocal;
	const UGrimUISettings* Settings = UGrimUISettings::Get();
	TestEqual(TEXT("막대 5"), Settings->VitalBars.Num(), 5);
	if (Settings->VitalBars.Num() >= 2)
	{
		TestEqual(TEXT("첫 막대 Health"), Settings->VitalBars[0].Attribute, FString(TEXT("Health")));
		TestEqual(TEXT("둘째 막대 Mana"), Settings->VitalBars[1].Attribute, FString(TEXT("Mana")));
	}
	TestEqual(TEXT("1서클 견습"), Settings->GetCircleTitle(1).ToString(), FString(TEXT("견습")));
	TestEqual(TEXT("3서클 수습 마법사"), Settings->GetCircleTitle(3).ToString(), FString(TEXT("수습 마법사")));
	TestTrue(TEXT("없는 서클 = 빈 글"), Settings->GetCircleTitle(11).IsEmpty());
	TestFalse(TEXT("그리모어 입력 지정"), Settings->ToggleGrimoireAction.IsNull());
	return true;
}

#endif
