#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Magic/GrimDamageExecution.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "Magic/GrimMagicEffects.h"
#include "Magic/GrimMagicMath.h"
#include "Magic/GrimMagicSettings.h"
#include "Magic/GrimMagicTags.h"
#include "Magic/GrimSpellAbility.h"
#include "Magic/GrimSpellDefinition.h"
#include "Core/GrimGameplayTags.h"
#include "Survival/GrimVitalAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

namespace
{
	constexpr EAutomationTestFlags GrimMagicTestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	/** 테스트용 빈 월드 하나. 소멸자에서 정리한다. */
	struct FGrimMagicTestWorld
	{
		UWorld* World = nullptr;

		FGrimMagicTestWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false);
			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
		}

		~FGrimMagicTestWorld()
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}

		/** ASC + 마법 · 생존 어트리뷰트를 가진 액터. */
		UAbilitySystemComponent* SpawnCaster()
		{
			AActor* Actor = World->SpawnActor<AActor>();
			UAbilitySystemComponent* ASC = NewObject<UAbilitySystemComponent>(Actor, TEXT("ASC"));
			ASC->RegisterComponent();
			ASC->AddAttributeSetSubobject(NewObject<UGrimMagicAttributeSet>(Actor));
			ASC->AddAttributeSetSubobject(NewObject<UGrimVitalAttributeSet>(Actor));
			ASC->InitAbilityActorInfo(Actor, Actor);
			return ASC;
		}
	};
}

// 상성 표: 스펙 「제안」값(물 → 불 → 바람 → 땅 → 물 ×1.5 / 역방향 ×0.75, 빛 ↔ 어둠 ×1.5, 그 외 ×1).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimMagicAffinityTest, "Grimoire.Magic.Affinity", GrimMagicTestFlags)
bool FGrimMagicAffinityTest::RunTest(const FString& Parameters)
{
	const UGrimMagicSettings* Settings = UGrimMagicSettings::Get();
	using namespace GrimTags;

	struct FCase { FGameplayTag Attacker; FGameplayTag Defender; float Expected; };
	const FCase Cases[] = {
		{ Element_Water, Element_Fire, 1.5f },
		{ Element_Fire, Element_Wind, 1.5f },
		{ Element_Wind, Element_Earth, 1.5f },
		{ Element_Earth, Element_Water, 1.5f },
		{ Element_Fire, Element_Water, 0.75f },
		{ Element_Wind, Element_Fire, 0.75f },
		{ Element_Earth, Element_Wind, 0.75f },
		{ Element_Water, Element_Earth, 0.75f },
		{ Element_Light, Element_Dark, 1.5f },
		{ Element_Dark, Element_Light, 1.5f },
		{ Element_Fire, Element_Fire, 1.f },
		{ Element_Water, Element_Wind, 1.f },
		{ Element_Fire, Element_Light, 1.f },
		{ Element_Dark, Element_Earth, 1.f },
		{ Element_Fire, FGameplayTag(), 1.f },
		{ FGameplayTag(), Element_Fire, 1.f },
	};

	for (const FCase& Case : Cases)
	{
		TestEqual(FString::Printf(TEXT("%s → %s"), *Case.Attacker.ToString(), *Case.Defender.ToString()),
			Settings->GetAffinity(Case.Attacker, Case.Defender), Case.Expected);
	}

	// 순수 함수: 표에 없으면 기본 배율.
	TArray<FGrimElementAffinity> Table;
	FGrimElementAffinity& Row = Table.AddDefaulted_GetRef();
	Row.Attacker = Element_Water;
	Row.Defender = Element_Fire;
	Row.Multiplier = 2.f;
	TestEqual(TEXT("표 값"), UGrimMagicSettings::FindAffinity(Table, 0.5f, Element_Water, Element_Fire), 2.f);
	TestEqual(TEXT("표에 없음 → 기본"), UGrimMagicSettings::FindAffinity(Table, 0.5f, Element_Fire, Element_Water), 0.5f);

	// 태그 묶음에서 속성 찾기.
	FGameplayTagContainer Tags;
	Tags.AddTag(GrimMagicTags::Spell_Ember);
	Tags.AddTag(Element_Fire);
	TestTrue(TEXT("속성 태그 찾기"), UGrimDamageExecution::FindElementTag(Tags) == Element_Fire.GetTag());
	return true;
}

// 비용 · 쿨다운 · 마나 계산.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimMagicCostTest, "Grimoire.Magic.Cost", GrimMagicTestFlags)
bool FGrimMagicCostTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("효율 0"), GrimMagicMath::CalcManaCost(10.f, 0.f), 10.f);
	TestEqual(TEXT("효율 20%"), GrimMagicMath::CalcManaCost(10.f, 0.2f), 8.f);
	TestEqual(TEXT("효율 상한"), GrimMagicMath::CalcManaCost(10.f, 1.5f), 0.f);
	TestEqual(TEXT("음수 효율은 0 으로"), GrimMagicMath::CalcManaCost(10.f, -1.f), 10.f);

	TestEqual(TEXT("시전 속도 1"), GrimMagicMath::CalcCooldown(1.f, 1.f), 1.f);
	TestEqual(TEXT("시전 속도 2"), GrimMagicMath::CalcCooldown(1.f, 2.f), 0.5f);
	TestEqual(TEXT("음수 쿨다운은 0"), GrimMagicMath::CalcCooldown(-1.f, 1.f), 0.f);

	TestTrue(TEXT("딱 맞는 마나"), GrimMagicMath::CanAfford(10.f, 10.f));
	TestFalse(TEXT("모자란 마나"), GrimMagicMath::CanAfford(9.9f, 10.f));
	TestTrue(TEXT("공짜 마법"), GrimMagicMath::CanAfford(0.f, 0.f));

	TestEqual(TEXT("피해 = 기본 × 완드 × 상성"), GrimMagicMath::CalcDamage(10.f, 1.2f, 1.5f), 18.f, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("정의 없음 → 비용 0"), UGrimSpellAbility::GetEffectiveManaCost(nullptr, nullptr), 0.f);

	const UGrimMagicSettings* Settings = UGrimMagicSettings::Get();
	TestTrue(TEXT("서클 1 최대 마나 > 0"), Settings->GetMaxManaForCircle(1) > 0.f);
	TestTrue(TEXT("서클이 오르면 최대 마나가 줄지 않는다"), Settings->GetMaxManaForCircle(2) >= Settings->GetMaxManaForCircle(1));
	TestEqual(TEXT("표 밖 서클은 마지막 값"), Settings->GetMaxManaForCircle(99), Settings->GetMaxManaForCircle(10));
	TestEqual(TEXT("시작 서클 1"), Settings->StartingCircle, 1);
	return true;
}

// GE 실제 적용: 비용 GE 가 마나를 깎고, 피해 GE 가 상성대로 Health 를 깎고, 마나가 상한에서 잘린다.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimMagicEffectsTest, "Grimoire.Magic.Effects", GrimMagicTestFlags)
bool FGrimMagicEffectsTest::RunTest(const FString& Parameters)
{
	FGrimMagicTestWorld TestWorld;
	UAbilitySystemComponent* Source = TestWorld.SpawnCaster();
	UAbilitySystemComponent* Target = TestWorld.SpawnCaster();

	Source->SetNumericAttributeBase(UGrimMagicAttributeSet::GetMaxManaAttribute(), 100.f);
	Source->SetNumericAttributeBase(UGrimMagicAttributeSet::GetManaAttribute(), 50.f);

	// 비용
	{
		FGameplayEffectSpecHandle Cost = Source->MakeOutgoingSpec(UGrimGE_SpellCost::StaticClass(), 1.f, Source->MakeEffectContext());
		Cost.Data->SetSetByCallerMagnitude(GrimMagicTags::SetByCaller_ManaCost, -10.f);
		Source->ApplyGameplayEffectSpecToSelf(*Cost.Data.Get());
		TestEqual(TEXT("마나 50 - 10"), Source->GetNumericAttribute(UGrimMagicAttributeSet::GetManaAttribute()), 40.f);
	}

	// 마나 상한 · 하한
	Source->SetNumericAttributeBase(UGrimMagicAttributeSet::GetManaAttribute(), 500.f);
	TestEqual(TEXT("마나는 최대치에서 잘린다"), Source->GetNumericAttribute(UGrimMagicAttributeSet::GetManaAttribute()), 100.f);
	Source->SetNumericAttributeBase(UGrimMagicAttributeSet::GetManaAttribute(), -5.f);
	TestEqual(TEXT("마나는 0 아래로 가지 않는다"), Source->GetNumericAttribute(UGrimMagicAttributeSet::GetManaAttribute()), 0.f);

	// 피해: 물 마법 → 불 속성 대상 ×1.5
	auto ApplyDamage = [Source, Target](const FGameplayTag& Element, float Base)
	{
		FGameplayEffectSpecHandle Damage = Source->MakeOutgoingSpec(UGrimGE_SpellDamage::StaticClass(), 1.f, Source->MakeEffectContext());
		Damage.Data->SetSetByCallerMagnitude(GrimMagicTags::SetByCaller_Damage, Base);
		Damage.Data->AddDynamicAssetTag(Element);
		Source->ApplyGameplayEffectSpecToTarget(*Damage.Data.Get(), Target);
	};

	Target->SetNumericAttributeBase(UGrimVitalAttributeSet::GetHealthAttribute(), 100.f);
	Target->AddLooseGameplayTag(GrimTags::Element_Fire);

	ApplyDamage(GrimTags::Element_Water, 10.f);
	TestEqual(TEXT("물 → 불 ×1.5"), Target->GetNumericAttribute(UGrimVitalAttributeSet::GetHealthAttribute()), 85.f, KINDA_SMALL_NUMBER);

	ApplyDamage(GrimTags::Element_Wind, 10.f);
	TestEqual(TEXT("바람 → 불 ×0.75"), Target->GetNumericAttribute(UGrimVitalAttributeSet::GetHealthAttribute()), 77.5f, KINDA_SMALL_NUMBER);

	Target->RemoveLooseGameplayTag(GrimTags::Element_Fire);
	ApplyDamage(GrimTags::Element_Fire, 10.f);
	TestEqual(TEXT("무속성 대상 ×1"), Target->GetNumericAttribute(UGrimVitalAttributeSet::GetHealthAttribute()), 67.5f, KINDA_SMALL_NUMBER);
	return true;
}

// 시작 마법 에셋: Scripts/Magic/make_spells.py 가 만든 DA_Spell_* 가 설정에 걸려 있고 값이 맞는다.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimMagicStartingSpellsTest, "Grimoire.Magic.StartingSpells", GrimMagicTestFlags)
bool FGrimMagicStartingSpellsTest::RunTest(const FString& Parameters)
{
	const UGrimMagicSettings* Settings = UGrimMagicSettings::Get();
	TestEqual(TEXT("시작 마법 3개"), Settings->StartingSpells.Num(), 3);

	TSet<FGameplayTag> Elements;
	TSet<FGameplayTag> CooldownTags;
	for (const TSoftObjectPtr<UGrimSpellDefinition>& Ref : Settings->StartingSpells)
	{
		const UGrimSpellDefinition* Spell = Ref.LoadSynchronous();
		if (!TestNotNull(*FString::Printf(TEXT("에셋 로드 %s"), *Ref.ToString()), Spell))
		{
			continue;
		}
		TestEqual(*FString::Printf(TEXT("%s 서클 1"), *Spell->GetName()), Spell->Circle, 1);
		TestTrue(*FString::Printf(TEXT("%s 자극 세기 > 0"), *Spell->GetName()), Spell->Stimulus.Intensity > 0.f);
		TestTrue(*FString::Printf(TEXT("%s 자극 속성 = 마법 속성"), *Spell->GetName()), Spell->Stimulus.Element == Spell->Element);
		TestTrue(*FString::Printf(TEXT("%s 마나 비용 > 0"), *Spell->GetName()), Spell->ManaCost > 0.f);
		TestTrue(*FString::Printf(TEXT("%s 쿨다운 태그"), *Spell->GetName()), Spell->CooldownTag.IsValid());
		TestTrue(*FString::Printf(TEXT("%s 사거리 > 0"), *Spell->GetName()), Spell->Range > 0.f);
		Elements.Add(Spell->Element);
		CooldownTags.Add(Spell->CooldownTag);
	}

	TestTrue(TEXT("불씨(불)"), Elements.Contains(GrimTags::Element_Fire));
	TestTrue(TEXT("물줄기(물)"), Elements.Contains(GrimTags::Element_Water));
	TestTrue(TEXT("미풍(바람)"), Elements.Contains(GrimTags::Element_Wind));
	TestEqual(TEXT("쿨다운 태그는 마법마다 다르다"), CooldownTags.Num(), 3);
	return true;
}

#endif
