#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/GrimCreature.h"
#include "Creatures/GrimCreatureMath.h"
#include "Creatures/GrimCreatureSettings.h"
#include "Creatures/GrimCreatureSpecies.h"
#include "Creatures/GrimCreatureTags.h"
#include "Creatures/GrimEcosystemSubsystem.h"
#include "Creatures/GrimHabitatVolume.h"
#include "Core/GrimGameplayTags.h"
#include "Interaction/GrimElementalStateComponent.h"
#include "Interaction/GrimInteractionTags.h"
#include "Magic/GrimDamageExecution.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "Magic/GrimMagicEffects.h"
#include "Magic/GrimMagicSettings.h"
#include "Magic/GrimMagicTags.h"
#include "Survival/GrimVitalAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/Paths.h"

namespace GrimCreatureTest // 유니티 빌드 이름 충돌 방지
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	/** 시험용 종(에셋 없이) — 색 영역 3 · 변이 팔레트 · 수컷 뿔 */
	UGrimCreatureSpecies* MakeTestSpecies()
	{
		UGrimCreatureSpecies* Species = NewObject<UGrimCreatureSpecies>(GetTransientPackage());
		Species->SpeciesId = TEXT("TestHare");
		Species->PrimaryElement = GrimTags::Element_Wind;
		Species->SecondaryElement = GrimCreatureTags::Element_Earth_Plant;
		Species->MaxHealth = 40.f;

		FGrimCreatureColorRegion Back;
		Back.RegionName = TEXT("Back");
		Back.BasePalette = { FLinearColor(0.5f, 0.3f, 0.1f), FLinearColor(0.4f, 0.4f, 0.4f), FLinearColor(0.7f, 0.6f, 0.4f), FLinearColor(0.2f, 0.15f, 0.1f) };
		FGrimCreatureColorRegion Belly;
		Belly.RegionName = TEXT("Belly");
		Belly.BasePalette = { FLinearColor(0.9f, 0.9f, 0.85f), FLinearColor(0.8f, 0.75f, 0.7f) };
		Belly.MalePalette = { FLinearColor(1.f, 0.f, 0.f) };
		FGrimCreatureColorRegion Eye;
		Eye.RegionName = TEXT("Eye");
		Eye.BasePalette = { FLinearColor(0.f, 0.f, 0.f) };
		Eye.bJitter = false;
		Species->ColorRegions = { Back, Belly, Eye };
		Species->MutantPalette = { FLinearColor(0.f, 1.f, 1.f), FLinearColor(1.f, 0.f, 1.f), FLinearColor(1.f, 1.f, 0.f) };

		FGrimCreatureBodyPart Body;
		Body.PartName = TEXT("Body");
		FGrimCreatureBodyPart Horn;
		Horn.PartName = TEXT("Horn");
		Horn.Sex = EGrimCreatureSexFilter::MaleOnly;
		Species->BodyParts = { Body, Horn };
		return Species;
	}

	FGrimCreatureRollTuning DefaultTuning()
	{
		return FGrimCreatureRollTuning();
	}

	/** 자동화 테스트용 게임 월드(BeginPlay 까지) */
	struct FScopedWorld
	{
		UWorld* World = nullptr;

		FScopedWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("GrimCreatureTestWorld"));
			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
			World->BeginPlay();
			World->GetWorldSettings()->NotifyBeginPlay();
		}

		~FScopedWorld()
		{
			World->EndPlay(EEndPlayReason::Quit);
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}
	};
}

using namespace GrimCreatureTest;

// 개체 굴림: 같은 씨앗 = 같은 개체, 크기 · 색 흔들림 범위, 거대 · 돌연변이 · 성별 팔레트 · 성별 부위.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimCreatureRollTest, "Grimoire.Creatures.Roll", Flags)
bool FGrimCreatureRollTest::RunTest(const FString& Parameters)
{
	UGrimCreatureSpecies* Species = MakeTestSpecies();
	FGrimCreatureRollTuning Tuning = DefaultTuning();
	Tuning.WildMutantChance = 0.f;
	Species->GiantChance = 0.f;

	const FGrimCreatureInstance A = GrimCreatureMath::RollInstance(*Species, 1234, Tuning);
	const FGrimCreatureInstance B = GrimCreatureMath::RollInstance(*Species, 1234, Tuning);
	TestEqual(TEXT("같은 씨앗 → 같은 성별"), A.Sex, B.Sex);
	TestEqual(TEXT("같은 씨앗 → 같은 크기"), A.SizeScale, B.SizeScale);
	TestEqual(TEXT("색 영역 수"), A.Colors.Num(), 3);
	for (int32 i = 0; i < A.Colors.Num(); ++i)
	{
		TestTrue(FString::Printf(TEXT("같은 씨앗 → 같은 색 %d"), i), A.Colors[i].Color.Equals(B.Colors[i].Color));
	}
	TestEqual(TEXT("종 id"), A.SpeciesId, FName(TEXT("TestHare")));
	TestFalse(TEXT("야생 개체는 길들지 않았다(주인 없음)"), A.IsTamed());

	int32 Males = 0;
	bool bDifferentSeedDiffers = false;
	for (int32 Seed = 1; Seed <= 400; ++Seed)
	{
		const FGrimCreatureInstance I = GrimCreatureMath::RollInstance(*Species, Seed, Tuning);
		TestTrue(TEXT("보통 크기 0.9~1.1"), I.SizeScale >= Tuning.SizeMin - KINDA_SMALL_NUMBER && I.SizeScale <= Tuning.SizeMax + KINDA_SMALL_NUMBER);
		TestTrue(TEXT("능력치 ±5%"), FMath::Abs(I.StatScale - 1.f) <= 0.05f + KINDA_SMALL_NUMBER);
		TestFalse(TEXT("거대 확률 0 → 거대 없음"), I.bGiant);
		TestFalse(TEXT("돌연변이 확률 0"), I.bMutant);
		TestNotEqual(TEXT("성격이 굴려진다"), I.Personality, EGrimCreaturePersonality::None);
		Males += I.Sex == EGrimCreatureSex::Male ? 1 : 0;
		if (I.Sex == EGrimCreatureSex::Male)
		{
			TestTrue(TEXT("수컷 배 = 수컷 팔레트(빨강 근처)"), I.Colors[1].Color.R > 0.8f && I.Colors[1].Color.G < 0.2f);
		}
		TestTrue(TEXT("흔들림 없는 영역(눈)은 팔레트 그대로"), I.Colors[2].Color.Equals(FLinearColor(0.f, 0.f, 0.f)));
		// 등 색: 팔레트 색의 HSV 에서 ±4° · ±8% 안.
		const FLinearColor Base = Species->ColorRegions[0].BasePalette[I.Colors[0].PaletteIndex].LinearRGBToHSV();
		const FLinearColor Got = I.Colors[0].Color.LinearRGBToHSV();
		const float HueDiff = FMath::Abs(FMath::FindDeltaAngleDegrees(Base.R, Got.R));
		TestTrue(TEXT("색조 흔들림 ±4°"), HueDiff <= Tuning.HueJitterDegrees + 0.01f);
		TestTrue(TEXT("밝기 흔들림 ±8%"), FMath::Abs(Got.B - Base.B) <= Base.B * Tuning.ValueJitter + 0.001f);
		if (I.Colors[0].PaletteIndex != A.Colors[0].PaletteIndex || I.Sex != A.Sex)
		{
			bDifferentSeedDiffers = true;
		}
	}
	TestTrue(TEXT("암수 반반 근처(400 중 140~260)"), Males >= 140 && Males <= 260);
	TestTrue(TEXT("씨앗이 다르면 개체가 달라진다"), bDifferentSeedDiffers);

	// 거대
	Species->GiantChance = 1.f;
	const FGrimCreatureInstance Giant = GrimCreatureMath::RollInstance(*Species, 99, Tuning);
	TestTrue(TEXT("거대 개체"), Giant.bGiant);
	TestTrue(TEXT("거대 크기 1.3~1.5"), Giant.SizeScale >= 1.3f - KINDA_SMALL_NUMBER && Giant.SizeScale <= 1.5f + KINDA_SMALL_NUMBER);
	TestTrue(TEXT("거대 능력치가 크게 높다"), Giant.StatScale >= 1.5f);
	Species->GiantChance = 0.f;

	// 돌연변이: 영역 하나가 변이 팔레트에서
	Tuning.WildMutantChance = 1.f;
	const FGrimCreatureInstance Mutant = GrimCreatureMath::RollInstance(*Species, 7, Tuning);
	TestTrue(TEXT("돌연변이"), Mutant.bMutant);
	int32 MutantRegions = 0;
	for (const FGrimCreatureRegionColor& Color : Mutant.Colors)
	{
		MutantRegions += Color.bMutant ? 1 : 0;
	}
	TestEqual(TEXT("변이 영역 하나"), MutantRegions, 1);

	// 성별 부위
	TestTrue(TEXT("Any 는 둘 다"), GrimCreatureMath::IsPartForSex(EGrimCreatureSexFilter::Any, EGrimCreatureSex::Female));
	TestTrue(TEXT("수컷 뿔은 수컷에"), GrimCreatureMath::IsPartForSex(EGrimCreatureSexFilter::MaleOnly, EGrimCreatureSex::Male));
	TestFalse(TEXT("수컷 뿔은 암컷에 없다"), GrimCreatureMath::IsPartForSex(EGrimCreatureSexFilter::MaleOnly, EGrimCreatureSex::Female));
	TestFalse(TEXT("암컷 부위는 수컷에 없다"), GrimCreatureMath::IsPartForSex(EGrimCreatureSexFilter::FemaleOnly, EGrimCreatureSex::Male));
	return true;
}

// 두 속성 상성(Q35): 식 · 주 · 부 가리기 · 실제 GE.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimCreatureDualAffinityTest, "Grimoire.Creatures.DualAffinity", Flags)
bool FGrimCreatureDualAffinityTest::RunTest(const FString& Parameters)
{
	// 스펙 표(공격 = 물, 강함 1.5 · 막힘 0.75 · 보통 1, 부속성 절반)
	struct FCase { float Primary; float Secondary; float Expected; const TCHAR* Name; };
	const FCase Cases[] = {
		{ 1.5f, 1.5f, 1.875f, TEXT("강함 · 강함") },
		{ 1.5f, 1.0f, 1.5f, TEXT("강함 · 보통") },
		{ 1.5f, 0.75f, 1.3125f, TEXT("강함 · 막힘") },
		{ 1.0f, 1.5f, 1.25f, TEXT("보통 · 강함") },
		{ 1.0f, 0.75f, 0.875f, TEXT("보통 · 막힘") },
		{ 0.75f, 1.5f, 0.9375f, TEXT("막힘 · 강함") },
		{ 0.75f, 0.75f, 0.65625f, TEXT("막힘 · 막힘") },
	};
	for (const FCase& Case : Cases)
	{
		TestEqual(Case.Name, UGrimDamageExecution::CombineDualAffinity(Case.Primary, Case.Secondary, true, 0.5f), Case.Expected, KINDA_SMALL_NUMBER);
	}
	TestEqual(TEXT("부속성 없음 → 주 그대로"), UGrimDamageExecution::CombineDualAffinity(1.5f, 0.75f, false, 0.5f), 1.5f);
	TestEqual(TEXT("비율 0 → 주만"), UGrimDamageExecution::CombineDualAffinity(1.5f, 0.75f, true, 0.f), 1.5f);
	TestEqual(TEXT("비율 1 → 곱"), UGrimDamageExecution::CombineDualAffinity(1.5f, 1.5f, true, 1.f), 2.25f, KINDA_SMALL_NUMBER);
	TestEqual(TEXT("설정 기본 0.5"), UGrimDamageSettings::Get()->SecondaryElementWeight, 0.5f);

	// 주 · 부 가리기
	FGameplayTag Primary, Secondary;
	{
		FGameplayTagContainer Tags;
		Tags.AddTag(GrimTags::Element_Earth);
		Tags.AddTag(GrimTags::Element_Fire);
		Tags.AddTag(UGrimDamageExecution::GetSecondaryMarkerTag(GrimTags::Element_Earth));
		UGrimDamageExecution::ResolveDefenderElements(Tags, Primary, Secondary);
		TestTrue(TEXT("표지 없는 쪽이 주(불)"), Primary == GrimTags::Element_Fire.GetTag());
		TestTrue(TEXT("표지 있는 쪽이 부(땅)"), Secondary == GrimTags::Element_Earth.GetTag());
	}
	{
		FGameplayTagContainer Tags;
		Tags.AddTag(GrimTags::Element_Water);
		UGrimDamageExecution::ResolveDefenderElements(Tags, Primary, Secondary);
		TestTrue(TEXT("하나면 주만"), Primary == GrimTags::Element_Water.GetTag() && !Secondary.IsValid());
	}
	{
		FGameplayTagContainer Tags;
		Tags.AddTag(GrimMagicTags::Element_None);
		UGrimDamageExecution::ResolveDefenderElements(Tags, Primary, Secondary);
		TestFalse(TEXT("Element.None 은 속성이 아니다"), Primary.IsValid());
	}
	{
		FGameplayTagContainer Tags;
		Tags.AddTag(GrimCreatureTags::Element_Dark_Shadow);
		Tags.AddTag(GrimTags::Element_Fire);
		Tags.AddTag(UGrimDamageExecution::GetSecondaryMarkerTag(GrimTags::Element_Fire));
		UGrimDamageExecution::ResolveDefenderElements(Tags, Primary, Secondary);
		TestTrue(TEXT("하위 태그(그림자술) 주"), Primary == GrimCreatureTags::Element_Dark_Shadow.GetTag());
		TestTrue(TEXT("부 불"), Secondary == GrimTags::Element_Fire.GetTag());
	}
	TestTrue(TEXT("표지 태그 이름"), UGrimDamageExecution::GetSecondaryMarkerTag(GrimCreatureTags::Element_Earth_Plant).ToString() == TEXT("SubElement.Earth.Plant"));

	// 실제 GE: 물 → 불(주) · 땅(부). 상성 표 ini(물 → 불 1.5, 물 → 땅 0.75) 로 1.5 × 0.875 = 1.3125.
	const float WaterFire = UGrimMagicSettings::Get()->GetAffinity(GrimTags::Element_Water, GrimTags::Element_Fire);
	const float WaterEarth = UGrimMagicSettings::Get()->GetAffinity(GrimTags::Element_Water, GrimTags::Element_Earth);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	{
		auto SpawnASC = [World]()
		{
			AActor* Actor = World->SpawnActor<AActor>();
			UAbilitySystemComponent* ASC = NewObject<UAbilitySystemComponent>(Actor, TEXT("ASC"));
			ASC->RegisterComponent();
			ASC->AddAttributeSetSubobject(NewObject<UGrimMagicAttributeSet>(Actor));
			ASC->AddAttributeSetSubobject(NewObject<UGrimVitalAttributeSet>(Actor));
			ASC->InitAbilityActorInfo(Actor, Actor);
			return ASC;
		};
		UAbilitySystemComponent* Source = SpawnASC();
		UAbilitySystemComponent* Target = SpawnASC();
		Target->SetNumericAttributeBase(UGrimVitalAttributeSet::GetMaxHealthAttribute(), 100.f);
		Target->SetNumericAttributeBase(UGrimVitalAttributeSet::GetHealthAttribute(), 100.f);
		Target->AddLooseGameplayTag(GrimTags::Element_Fire);
		Target->AddLooseGameplayTag(GrimTags::Element_Earth);
		Target->AddLooseGameplayTag(UGrimDamageExecution::GetSecondaryMarkerTag(GrimTags::Element_Earth));

		auto Hit = [Source, Target](const FGameplayTag& Element, float Base)
		{
			FGameplayEffectSpecHandle Damage = Source->MakeOutgoingSpec(UGrimGE_SpellDamage::StaticClass(), 1.f, Source->MakeEffectContext());
			Damage.Data->SetSetByCallerMagnitude(GrimMagicTags::SetByCaller_Damage, Base);
			Damage.Data->AddDynamicAssetTag(Element);
			Source->ApplyGameplayEffectSpecToTarget(*Damage.Data.Get(), Target);
		};
		auto Health = [Target]() { return Target->GetNumericAttribute(UGrimVitalAttributeSet::GetHealthAttribute()); };

		const float Expected = 10.f * WaterFire * (1.f + 0.5f * (WaterEarth - 1.f));
		Hit(GrimTags::Element_Water, 10.f);
		TestEqual(TEXT("물 → 불 · 땅(부) = 기본 × 주 × (1 + 0.5 × (부 − 1))"), 100.f - Health(), Expected, 0.001f);

		// 주 · 부를 바꾼다: 땅(주) · 불(부)
		Target->RemoveLooseGameplayTag(UGrimDamageExecution::GetSecondaryMarkerTag(GrimTags::Element_Earth));
		Target->AddLooseGameplayTag(UGrimDamageExecution::GetSecondaryMarkerTag(GrimTags::Element_Fire));
		const float Before = Health();
		Hit(GrimTags::Element_Water, 10.f);
		TestEqual(TEXT("주 · 부를 바꾸면 배율이 바뀐다"), Before - Health(), 10.f * WaterEarth * (1.f + 0.5f * (WaterFire - 1.f)), 0.001f);

		// 무속성 공격은 ×1
		const float Before2 = Health();
		Hit(GrimMagicTags::Element_None, 10.f);
		TestEqual(TEXT("무속성 공격 ×1"), Before2 - Health(), 10.f, 0.001f);
	}
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

// 전리품: 확률 · 개수 · 거대 배율 · 같은 아이템 합치기.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimCreatureLootTest, "Grimoire.Creatures.Loot", Flags)
bool FGrimCreatureLootTest::RunTest(const FString& Parameters)
{
	TArray<FGrimCreatureLootEntry> Table;
	Table.Add({ TEXT("Meat"), 1, 2, 1.f });
	Table.Add({ TEXT("Fur"), 1, 1, 0.f });
	Table.Add({ TEXT("Meat"), 1, 1, 1.f });
	FRandomStream Rng(42);
	const TArray<FGrimCreatureLootDrop> Drops = GrimCreatureMath::RollLoot(Table, false, 2.f, Rng);
	TestEqual(TEXT("확률 0 은 안 떨어진다 · 같은 아이템은 합친다"), Drops.Num(), 1);
	if (Drops.Num() == 1)
	{
		TestEqual(TEXT("고기"), Drops[0].ItemId, FName(TEXT("Meat")));
		TestTrue(TEXT("고기 2~3"), Drops[0].Count >= 2 && Drops[0].Count <= 3);
	}
	TArray<FGrimCreatureLootEntry> One;
	One.Add({ TEXT("Horn"), 1, 1, 1.f });
	const TArray<FGrimCreatureLootDrop> Giant = GrimCreatureMath::RollLoot(One, true, 2.f, Rng);
	TestTrue(TEXT("거대 개체는 두 배"), Giant.Num() == 1 && Giant[0].Count == 2);
	return true;
}

// 활동 시간 · AI 판단(순수).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimCreatureAIDecideTest, "Grimoire.Creatures.AIDecide", Flags)
bool FGrimCreatureAIDecideTest::RunTest(const FString& Parameters)
{
	using namespace GrimCreatureMath;
	TestTrue(TEXT("주행성 낮"), IsActiveTime(EGrimCreatureActivity::Diurnal, false, 12.f, 17.f, 21.f));
	TestFalse(TEXT("주행성 밤"), IsActiveTime(EGrimCreatureActivity::Diurnal, true, 23.f, 17.f, 21.f));
	TestTrue(TEXT("야행성 밤"), IsActiveTime(EGrimCreatureActivity::Nocturnal, true, 1.f, 17.f, 21.f));
	TestFalse(TEXT("야행성 낮"), IsActiveTime(EGrimCreatureActivity::Nocturnal, false, 12.f, 17.f, 21.f));
	TestTrue(TEXT("낮 + 해 질 녘: 밤이어도 20시면"), IsActiveTime(EGrimCreatureActivity::DayAndDusk, true, 20.f, 17.f, 21.f));
	TestFalse(TEXT("낮 + 해 질 녘: 새벽 2시는 아님"), IsActiveTime(EGrimCreatureActivity::DayAndDusk, true, 2.f, 17.f, 21.f));

	FGrimCreatureTemperamentInfo Timid;
	Timid.Temperament = EGrimCreatureTemperament::Timid;
	FGrimCreatureTemperamentInfo Docile;
	Docile.Temperament = EGrimCreatureTemperament::Docile;
	FGrimCreatureTemperamentInfo Dog;
	Dog.Temperament = EGrimCreatureTemperament::Aggressive;
	Dog.Diet = EGrimCreatureDiet::Carnivore;
	Dog.bAvoidsLight = true;

	FGrimCreatureSenses Calm;
	TestEqual(TEXT("한가함 → Idle"), Decide(Calm, Timid), EGrimCreatureAIState::Idle);

	FGrimCreatureSenses Threat;
	Threat.bThreatNear = true;
	TestEqual(TEXT("겁 많음 + 위협 → 도망"), Decide(Threat, Timid), EGrimCreatureAIState::Flee);
	TestEqual(TEXT("온순 + 위협만 → 한가함"), Decide(Threat, Docile), EGrimCreatureAIState::Idle);

	FGrimCreatureSenses Hit;
	Hit.bProvoked = true;
	TestEqual(TEXT("온순 + 맞음 → 싸움"), Decide(Hit, Docile), EGrimCreatureAIState::Fight);
	Hit.HealthRatio = 0.2f;
	TestEqual(TEXT("온순 + 맞음 + 체력 낮음 → 도망"), Decide(Hit, Docile), EGrimCreatureAIState::Flee);

	FGrimCreatureSenses Burning;
	Burning.bBurning = true;
	Burning.bProvoked = true;
	TestEqual(TEXT("불붙음 → 도망(맞았어도)"), Decide(Burning, Dog), EGrimCreatureAIState::Flee);

	FGrimCreatureSenses NightPlayer;
	NightPlayer.bHostileInAggroRange = true;
	NightPlayer.bThreatNear = true;
	TestEqual(TEXT("검은 개 + 밤 + 가까운 플레이어 → 싸움"), Decide(NightPlayer, Dog), EGrimCreatureAIState::Fight);
	NightPlayer.bLightNear = true;
	TestEqual(TEXT("검은 개 + 모닥불 → 물러남"), Decide(NightPlayer, Dog), EGrimCreatureAIState::Flee);

	FGrimCreatureSenses Prey;
	Prey.bPreyNear = true;
	TestEqual(TEXT("육식 + 먹이 → 사냥"), Decide(Prey, Dog), EGrimCreatureAIState::Hunt);
	TestEqual(TEXT("초식은 먹이를 쫓지 않는다"), Decide(Prey, Timid), EGrimCreatureAIState::Idle);

	FGrimCreatureSenses Day;
	Day.bActiveTime = false;
	TestEqual(TEXT("활동 시간 밖 → 잠"), Decide(Day, Timid), EGrimCreatureAIState::Sleep);
	Day.bHostileInAggroRange = true;
	TestEqual(TEXT("잠든 공격적 종은 덤비지 않는다"), Decide(Day, Dog), EGrimCreatureAIState::Sleep);
	Day.bProvoked = true;
	TestEqual(TEXT("자다가 맞으면 싸운다"), Decide(Day, Dog), EGrimCreatureAIState::Fight);

	FGrimCreatureTemperamentInfo Harmless = Docile;
	Harmless.bCanAttack = false;
	Hit.HealthRatio = 1.f;
	TestEqual(TEXT("공격이 없으면 맞아도 도망"), Decide(Hit, Harmless), EGrimCreatureAIState::Flee);

	const FVector Weights(1.f, 1.f, 1.f);
	TestEqual(TEXT("한가함 0 → 가만히"), PickCalmActivity(0.f, EGrimCreatureDiet::Herbivore, Weights), EGrimCreatureAIState::Idle);
	TestEqual(TEXT("한가함 0.5 → 배회"), PickCalmActivity(0.5f, EGrimCreatureDiet::Herbivore, Weights), EGrimCreatureAIState::Wander);
	TestEqual(TEXT("한가함 0.9 → 풀 뜯기"), PickCalmActivity(0.9f, EGrimCreatureDiet::Herbivore, Weights), EGrimCreatureAIState::Graze);
	TestEqual(TEXT("육식은 풀을 뜯지 않는다"), PickCalmActivity(0.9f, EGrimCreatureDiet::Carnivore, Weights), EGrimCreatureAIState::Wander);
	return true;
}

// 개체군: 회복 · 접기 · 펼치기 · 무리 크기.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimCreaturePopulationTest, "Grimoire.Creatures.Population", Flags)
bool FGrimCreaturePopulationTest::RunTest(const FString& Parameters)
{
	using namespace GrimCreatureMath;
	float Timer = 0.f;
	TestEqual(TEXT("상한이면 회복 없음"), StepRegrow(6, 6, Timer, 100.f, 10.f), 0);
	TestEqual(TEXT("상한이면 타이머 0"), Timer, 0.f);
	TestEqual(TEXT("9초는 아직"), StepRegrow(4, 6, Timer, 9.f, 10.f), 0);
	TestEqual(TEXT("10초째 하나"), StepRegrow(4, 6, Timer, 1.f, 10.f), 1);
	TestEqual(TEXT("한 번에 여럿(상한까지만)"), StepRegrow(3, 6, Timer, 1000.f, 10.f), 3);
	TestEqual(TEXT("상한에 닿으면 타이머 0"), Timer, 0.f);
	TestEqual(TEXT("회복 0초 = 바로 채움"), StepRegrow(0, 5, Timer, 0.f, 0.f), 5);

	TestTrue(TEXT("가까우면 펼침"), ShouldUnfold(1000.f, 6000.f, true, false, 0, 40));
	TestFalse(TEXT("멀면 안 펼침"), ShouldUnfold(7000.f, 6000.f, true, false, 0, 40));
	TestFalse(TEXT("전역 상한(서버 성능)"), ShouldUnfold(1000.f, 6000.f, true, false, 40, 40));
	TestFalse(TEXT("숨는 종은 활동 시간 밖에 안 펼침"), ShouldUnfold(1000.f, 6000.f, false, true, 0, 40));
	TestTrue(TEXT("숨지 않는 종은 시간 밖에도 펼침(잔다)"), ShouldUnfold(1000.f, 6000.f, false, false, 0, 40));
	TestTrue(TEXT("멀어지면 접음"), ShouldFold(9000.f, 8000.f, true, false));
	TestFalse(TEXT("펼침 ~ 접음 사이는 그대로(되풀이 방지)"), ShouldFold(7000.f, 8000.f, true, false));
	TestTrue(TEXT("숨는 종은 시간 밖이면 접음"), ShouldFold(100.f, 8000.f, false, true));

	FRandomStream Rng(5);
	for (int32 i = 0; i < 50; ++i)
	{
		const int32 Size = RollHerdSize(3, 6, 100, Rng);
		TestTrue(TEXT("무리 3~6"), Size >= 3 && Size <= 6);
	}
	TestEqual(TEXT("남은 수를 넘지 않는다"), RollHerdSize(3, 6, 2, Rng), 2);
	return true;
}

// 종 에셋 3종(make_creature_species.py) · 먹이 관계 표 · 설정.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimCreatureSpeciesAssetsTest, "Grimoire.Creatures.SpeciesAssets", Flags)
bool FGrimCreatureSpeciesAssetsTest::RunTest(const FString& Parameters)
{
	const UGrimCreatureSettings* Settings = UGrimCreatureSettings::Get();
	TestEqual(TEXT("설정에 종 3"), Settings->Species.Num(), 3);

	const UGrimCreatureSpecies* Jackalope = Settings->FindSpecies(TEXT("Jackalope"));
	const UGrimCreatureSpecies* Kelpie = Settings->FindSpecies(TEXT("Kelpie"));
	const UGrimCreatureSpecies* BlackDog = Settings->FindSpecies(TEXT("BlackDog"));
	if (!TestNotNull(TEXT("재칼로프"), Jackalope) || !TestNotNull(TEXT("켈피"), Kelpie) || !TestNotNull(TEXT("검은 개"), BlackDog))
	{
		return false;
	}

	for (const UGrimCreatureSpecies* Species : { Jackalope, Kelpie, BlackDog })
	{
		const FString Name = Species->SpeciesId.ToString();
		TestTrue(Name + TEXT(": 주속성(무속성 생물 없음)"), Species->PrimaryElement.IsValid());
		TestTrue(Name + TEXT(": 부속성"), Species->SecondaryElement.IsValid() && Species->SecondaryElement != Species->PrimaryElement);
		TestTrue(Name + TEXT(": 색 영역 2~6"), Species->ColorRegions.Num() >= 2 && Species->ColorRegions.Num() <= 6);
		TestTrue(Name + TEXT(": 변이 팔레트 3~5"), Species->MutantPalette.Num() >= 3 && Species->MutantPalette.Num() <= 5);
		TestTrue(Name + TEXT(": 부위가 있다"), Species->BodyParts.Num() >= 5);
		TestEqual(Name + TEXT(": 고유 능력 하나"), Species->AbilityTags.Num(), 1);
		TestTrue(Name + TEXT(": 공격 = 주속성 하나 · 부속성 하나"), Species->FindAttack(Species->PrimaryElement) && Species->FindAttack(Species->SecondaryElement));
		TestTrue(Name + TEXT(": 전리품이 있다"), !Species->Loot.IsEmpty());
		TestTrue(Name + TEXT(": 길들이기 서클 자리"), Species->TamingCircle >= 1 && Species->PreferredPactElement.IsValid());
		bool bHasSexPart = false;
		for (const FGrimCreatureBodyPart& Part : Species->BodyParts)
		{
			TestTrue(Name + TEXT(": 부위 색 영역"), Species->ColorRegions.IsValidIndex(Part.ColorRegion));
			bHasSexPart |= Part.Sex != EGrimCreatureSexFilter::Any;
		}
		TestTrue(Name + TEXT(": 성별 부위"), bHasSexPart);
		for (const FGrimCreatureColorRegion& Region : Species->ColorRegions)
		{
			TestTrue(Name + TEXT(": 팔레트 1색 이상"), !Region.BasePalette.IsEmpty());
		}
	}

	TestEqual(TEXT("재칼로프 소형"), Jackalope->SizeClass, EGrimCreatureSizeClass::Small);
	TestTrue(TEXT("재칼로프 바람 · 풀"), Jackalope->PrimaryElement == GrimTags::Element_Wind.GetTag() && Jackalope->SecondaryElement == GrimCreatureTags::Element_Earth_Plant.GetTag());
	TestTrue(TEXT("재칼로프 씨앗 퍼뜨리기"), Jackalope->AbilityTags.HasTagExact(GrimCreatureTags::Creature_Ability_SeedSpread));
	TestEqual(TEXT("재칼로프 겁 많음"), Jackalope->Temperament, EGrimCreatureTemperament::Timid);
	TestTrue(TEXT("재칼로프 무리 3~6"), Jackalope->HerdSizeMin == 3 && Jackalope->HerdSizeMax == 6);

	TestTrue(TEXT("켈피 물 · 바람"), Kelpie->PrimaryElement == GrimTags::Element_Water.GetTag() && Kelpie->SecondaryElement == GrimTags::Element_Wind.GetTag());
	TestTrue(TEXT("켈피 물 위 달리기"), Kelpie->AbilityTags.HasTagExact(GrimCreatureTags::Creature_Ability_WaterRun) && Kelpie->WaterMovement == EGrimCreatureWaterMovement::RunOnSurface);
	TestTrue(TEXT("켈피 탈것(다음 묶음)"), Kelpie->bMountable);
	TestEqual(TEXT("켈피 낮 · 해 질 녘"), Kelpie->Activity, EGrimCreatureActivity::DayAndDusk);

	TestTrue(TEXT("검은 개 그림자술 · 불"), BlackDog->PrimaryElement == GrimCreatureTags::Element_Dark_Shadow.GetTag() && BlackDog->SecondaryElement == GrimTags::Element_Fire.GetTag());
	TestTrue(TEXT("검은 개 밤에만"), BlackDog->Activity == EGrimCreatureActivity::Nocturnal && BlackDog->bHideOutsideActiveTime);
	TestTrue(TEXT("검은 개 빛을 피함"), BlackDog->bAvoidsLight);
	TestTrue(TEXT("검은 개 그림자 걸음"), BlackDog->AbilityTags.HasTagExact(GrimCreatureTags::Creature_Ability_ShadowStep));
	TestEqual(TEXT("검은 개 공격적"), BlackDog->Temperament, EGrimCreatureTemperament::Aggressive);
	TestEqual(TEXT("검은 개 육식"), BlackDog->Diet, EGrimCreatureDiet::Carnivore);

	// 먹이 관계 표
	TArray<FGrimCreatureEcologyRow> Rows;
	TArray<FString> Problems;
	const FString Path = FPaths::Combine(FPaths::ProjectContentDir(), Settings->EcologyCSV);
	TestTrue(TEXT("먹이 관계 표를 읽는다"), UGrimEcosystemSubsystem::LoadEcologyFromCSV(Path, Rows, Problems));
	TestTrue(TEXT("검은 개 → 재칼로프"), Rows.ContainsByPredicate([](const FGrimCreatureEcologyRow& Row) { return Row.Predator == TEXT("BlackDog") && Row.Prey == TEXT("Jackalope"); }));

	// 재료 태그: 풀 몫(불붙음 · 물 머금음)
	const FGameplayTagContainer Materials = Settings->GetMaterialsForElements(Jackalope->PrimaryElement, Jackalope->SecondaryElement);
	TestTrue(TEXT("재칼로프(풀)는 탄다"), Materials.HasTag(GrimInteractionTags::Material_Flammable));
	TestTrue(TEXT("퀘스트 이벤트 이름"), UGrimCreatureSettings::GetQuestEventName(Settings->KilledEventPrefix, EGrimCreatureSizeClass::Small) == FName(TEXT("Creature.Killed.Small")));
	TestTrue(TEXT("약해짐 이벤트 이름"), UGrimCreatureSettings::GetQuestEventName(Settings->WeakenedEventPrefix, EGrimCreatureSizeClass::Small) == FName(TEXT("Creature.Weakened.Small")));
	TestTrue(TEXT("펼침 < 접음 반경"), Settings->Ecosystem.SpawnRadius < Settings->Ecosystem.DespawnRadius);
	return true;
}

// 생물 액터: 스폰 → 태그 · 체력 · 재질 → 피해 → 약해짐 → 죽음 · 전리품 이벤트.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimCreatureActorTest, "Grimoire.Creatures.Actor", Flags)
bool FGrimCreatureActorTest::RunTest(const FString& Parameters)
{
	UGrimCreatureSpecies* Species = UGrimCreatureSettings::Get()->FindSpecies(TEXT("Jackalope"));
	if (!TestNotNull(TEXT("재칼로프 에셋"), Species))
	{
		return false;
	}
	FScopedWorld TestWorld;
	FGrimCreatureInstance Instance = GrimCreatureMath::RollInstance(*Species, 777, UGrimCreatureSettings::Get()->Roll);
	AGrimCreature* Creature = AGrimCreature::SpawnCreature(TestWorld.World, Species, Instance, FVector::ZeroVector, 0.f);
	if (!TestNotNull(TEXT("스폰"), Creature))
	{
		return false;
	}
	UAbilitySystemComponent* ASC = Creature->GetAbilitySystemComponent();
	TestTrue(TEXT("주속성 태그"), ASC->HasMatchingGameplayTag(Species->PrimaryElement));
	TestTrue(TEXT("부속성 태그"), ASC->HasMatchingGameplayTag(Species->SecondaryElement));
	TestTrue(TEXT("부속성 표지"), ASC->HasMatchingGameplayTag(UGrimDamageExecution::GetSecondaryMarkerTag(Species->SecondaryElement)));
	TestEqual(TEXT("체력 = 종 × 개체 배율"), Creature->GetMaxHealth(), Species->MaxHealth * Instance.StatScale, 0.01f);
	TestEqual(TEXT("가득"), Creature->GetHealthRatio(), 1.f, 0.001f);
	TestTrue(TEXT("재질 = 두 속성 몫"), Creature->GetElementalState()->Material.HasTag(GrimInteractionTags::Material_Flammable));
	TestEqual(TEXT("조준 종류 = 생물"), Creature->GetTargetKind(), EGrimTargetKind::Creature);
	TestTrue(TEXT("조준 가능"), Creature->CanBeTargetedBy(nullptr));
	TestNotNull(TEXT("AI 컨트롤러"), Creature->GetController());

	int32 Deaths = 0;
	int32 LootEvents = 0;
	Creature->OnCreatureDiedNative.AddLambda([&Deaths](AGrimCreature*, AActor*) { ++Deaths; });
	Creature->OnLootDroppedNative.AddLambda([&LootEvents](AGrimCreature*, FName, int32 Count, FVector) { LootEvents += Count > 0 ? 1 : 0; });

	const float Max = Creature->GetMaxHealth();
	Creature->ApplyDebugDamage(Max * 0.75f, FGameplayTag(), nullptr);
	TestTrue(TEXT("약해짐(30% 이하) 태그"), ASC->HasMatchingGameplayTag(GrimCreatureTags::Creature_State_Weakened));
	TestFalse(TEXT("아직 살아 있다"), Creature->IsDead());

	Creature->ApplyDebugDamage(Max, FGameplayTag(), nullptr);
	TestTrue(TEXT("죽음"), Creature->IsDead());
	TestEqual(TEXT("죽음 이벤트 한 번"), Deaths, 1);
	TestEqual(TEXT("전리품 이벤트 = 떨군 줄 수"), LootEvents, Creature->GetDeath().Drops.Num());
	TestFalse(TEXT("죽으면 조준 안 됨"), Creature->CanBeTargetedBy(nullptr));
	TestEqual(TEXT("AI 상태 Dead"), Creature->GetAIState(), EGrimCreatureAIState::Dead);

	// 두 번 죽지 않는다
	Creature->ApplyDebugDamage(10.f, FGameplayTag(), nullptr);
	TestEqual(TEXT("죽음은 한 번"), Deaths, 1);
	return true;
}

// 서식 구역 → 개체군 기록 → 플레이어가 없으면 펼치지 않는다(접힌 기록만).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimCreatureEcosystemTest, "Grimoire.Creatures.Ecosystem", Flags)
bool FGrimCreatureEcosystemTest::RunTest(const FString& Parameters)
{
	UGrimCreatureSpecies* Species = UGrimCreatureSettings::Get()->FindSpecies(TEXT("Jackalope"));
	if (!TestNotNull(TEXT("재칼로프 에셋"), Species))
	{
		return false;
	}
	FScopedWorld TestWorld;
	UGrimEcosystemSubsystem* Ecosystem = TestWorld.World->GetSubsystem<UGrimEcosystemSubsystem>();
	if (!TestNotNull(TEXT("생태계 서브시스템"), Ecosystem))
	{
		return false;
	}
	TestTrue(TEXT("검은 개는 재칼로프를 쫓는다"), Ecosystem->IsPreyOf(TEXT("BlackDog"), TEXT("Jackalope")));
	TestFalse(TEXT("재칼로프는 검은 개를 쫓지 않는다"), Ecosystem->IsPreyOf(TEXT("Jackalope"), TEXT("BlackDog")));

	const FTransform Transform(FVector(0.f, 0.f, 0.f));
	AGrimHabitatVolume* Volume = TestWorld.World->SpawnActorDeferred<AGrimHabitatVolume>(AGrimHabitatVolume::StaticClass(), Transform);
	FGrimHabitatSpeciesEntry& Entry = Volume->Species.AddDefaulted_GetRef();
	Entry.Species = Species;
	Entry.MaxCount = 5;
	Entry.RegrowSeconds = 10.f;
	Volume->RandomSeed = 11;
	Volume->FinishSpawning(Transform);

	if (!TestEqual(TEXT("구역 하나 등록"), Ecosystem->GetHabitats().Num(), 1))
	{
		return false;
	}
	const FGrimHabitatPopulation& Population = Ecosystem->GetHabitats()[0].Populations[0];
	TestEqual(TEXT("처음부터 상한만큼(접힌 기록)"), Population.GetCurrent(), 5);
	TestEqual(TEXT("플레이어가 없으면 펼치지 않는다"), Population.Active.Num(), 0);
	for (const FGrimFoldedCreature& Record : Population.Folded)
	{
		TestTrue(TEXT("기록은 구역 안(무리 반경 여유)"), Volume->GetDistanceTo(Record.Location) <= UGrimCreatureSettings::Get()->Ecosystem.HerdRadius * 1.5f);
		TestTrue(TEXT("기록은 굴린 개체"), Record.Instance.IsValid());
	}
	Ecosystem->UpdateNow(100.f);
	TestEqual(TEXT("갱신해도 플레이어 없으면 그대로"), Ecosystem->GetHabitats()[0].Populations[0].Active.Num(), 0);
	return true;
}

#endif
