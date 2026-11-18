#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Dressing/GrimDressingSettings.h"
#include "Dressing/GrimDressingTags.h"
#include "Dressing/GrimDressingTypes.h"
#include "Dressing/GrimHarvestCell.h"
#include "Dressing/GrimReactiveProp.h"
#include "Core/GrimElementalTypes.h"
#include "Core/GrimGameplayTags.h"
#include "Interaction/GrimElementalReactionLibrary.h"
#include "Interaction/GrimElementalStateComponent.h"
#include "Interaction/GrimInteractionSettings.h"
#include "Interaction/GrimInteractionTags.h"
#include "Items/GrimItemPickup.h"
#include "Items/GrimItemStatics.h"
#include "Magic/GrimMagicTags.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/WorldSettings.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "NiagaraSystem.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

namespace GrimDressingTest
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	FGrimElementalStimulus Stim(const FGameplayTag& Element, float Intensity)
	{
		FGrimElementalStimulus Stimulus;
		Stimulus.Element = Element;
		Stimulus.Intensity = Intensity;
		return Stimulus;
	}

	/** 규칙표 + 재질 + 상태만으로 반응을 본다(게임과 같은 CSV) */
	struct FRuleObject
	{
		const UDataTable* Table = nullptr;
		FGameplayTagContainer Material;
		FGrimElementalStateArray States;

		void Hit(const FGameplayTag& Element, float Intensity)
		{
			TArray<FGrimElementalStateChange> Changes;
			FGrimElementalReactionLibrary::ApplyStimulus(Table, States, Material, Stim(Element, Intensity), 1000.f, Changes);
		}
		void Wait(float Seconds)
		{
			TArray<FGrimElementalStateChange> Changes;
			FGrimElementalReactionLibrary::AdvanceTime(Table, States, Material, Seconds, 1000.f, Changes);
		}
		bool Has(const FGameplayTag& Tag) const { return States.Has(Tag); }
	};

	struct FScopedGameWorld
	{
		UWorld* World = nullptr;

		FScopedGameWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("GrimDressingTestWorld"));
			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
			World->BeginPlay();
			World->GetWorldSettings()->NotifyBeginPlay();
		}

		~FScopedGameWorld()
		{
			World->EndPlay(EEndPlayReason::Quit);
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}

		AGrimReactiveProp* SpawnProp(FName PropId, const FVector& Location)
		{
			const FTransform Transform(Location);
			AGrimReactiveProp* Prop = World->SpawnActorDeferred<AGrimReactiveProp>(AGrimReactiveProp::StaticClass(), Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			Prop->SetPropId(PropId);
			Prop->FinishSpawning(Transform);
			return Prop;
		}

		int32 CountPickups(FName ItemId = NAME_None) const
		{
			int32 Count = 0;
			for (TActorIterator<AGrimItemPickup> It(World); It; ++It)
			{
				Count += (ItemId.IsNone() || It->GetItem().ItemId == ItemId) ? 1 : 0;
			}
			return Count;
		}
	};

	bool HasReplicatedProperty(UClass* Class, const TCHAR* Name)
	{
		const FProperty* Property = Class->FindPropertyByName(Name);
		return Property && Property->HasAnyPropertyFlags(CPF_Net);
	}
}

// 유니티 빌드에서 다른 테스트 파일과 이름이 겹치지 않게 using 없이 GrimDressingTest:: 로 쓴다

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimDressingTablesTest, "Grimoire.Dressing.Tables", GrimDressingTest::Flags)
bool FGrimDressingTablesTest::RunTest(const FString& Parameters)
{
	FGrimDressingData::Reset();
	const UDataTable* Props = FGrimDressingData::GetPropTable();
	const UDataTable* Harvest = FGrimDressingData::GetHarvestTable();
	const UDataTable* Journal = FGrimDressingData::GetJournalTable();
	if (!TestNotNull(TEXT("ReactiveProps.csv"), Props) || !TestNotNull(TEXT("HarvestNodes.csv"), Harvest) || !TestNotNull(TEXT("Journal.csv"), Journal))
	{
		return false;
	}
	// 1차 묶음 중 액터 소품(D01 · D02 · D03 · D04 · D16 · D20 · D21 · D29) — D10 은 채집점, D26 · D27 은 폴리지
	for (const TCHAR* Id : { TEXT("D01_WindCotton"), TEXT("D02_DryGrass"), TEXT("D03_ThirstyFlower"), TEXT("D04_StoneLamp"), TEXT("D16_Butterflies"),
		TEXT("D20_Campfire"), TEXT("D21_Page"), TEXT("D29_FloatStone") })
	{
		TestNotNull(FString::Printf(TEXT("소품 %s"), Id), FGrimDressingData::FindProp(FName(Id)));
	}
	for (const TPair<FName, uint8*>& Pair : Props->GetRowMap())
	{
		const FGrimReactivePropRow& Row = *reinterpret_cast<const FGrimReactivePropRow*>(Pair.Value);
		const FString Id = Pair.Key.ToString();
		for (const TSoftObjectPtr<UStaticMesh>* Mesh : { &Row.Mesh, &Row.AltMesh, &Row.AccentMesh })
		{
			if (!Mesh->IsNull())
			{
				TestNotNull(FString::Printf(TEXT("%s 메시 %s"), *Id, *Mesh->ToString()), Mesh->LoadSynchronous());
			}
		}
		for (const TSoftObjectPtr<UMaterialInterface>* Mat : { &Row.MeshMaterial, &Row.AltMaterial, &Row.AccentMaterial })
		{
			if (!Mat->IsNull())
			{
				TestNotNull(FString::Printf(TEXT("%s 재질 %s"), *Id, *Mat->ToString()), Mat->LoadSynchronous());
			}
		}
		for (const TSoftObjectPtr<UNiagaraSystem>* FX : { &Row.LoopFX, &Row.BurstFX })
		{
			if (!FX->IsNull())
			{
				const UNiagaraSystem* System = FX->LoadSynchronous();
				TestNotNull(FString::Printf(TEXT("%s 이펙트 %s"), *Id, *FX->ToString()), System);
				// 성능 규칙 24 · BUG-006: 환경 이펙트는 EFT_Grim_Ambient
				if (System)
				{
					TestTrue(FString::Printf(TEXT("%s 이펙트에 Effect Type"), *Id), System->GetEffectType() != nullptr && System->GetEffectType()->GetName() == TEXT("EFT_Grim_Ambient"));
				}
			}
		}
		TestTrue(FString::Printf(TEXT("%s 재질 태그가 하나 이상"), *Id), Row.Materials.Num() > 0);
		TArray<FString> Problems;
		for (const FGrimRewardEntry& Entry : FGrimDressingData::ParseRewards(Row.Rewards, &Problems))
		{
			TestTrue(FString::Printf(TEXT("%s 보상 %s 이 Items.csv 에"), *Id, *Entry.Item.ToString()), UGrimItemStatics::IsKnownItem(Entry.Item));
		}
		TestEqual(FString::Printf(TEXT("%s 보상 문법"), *Id), Problems.Num(), 0);
		if (!Row.JournalId.IsNone())
		{
			TestNotNull(FString::Printf(TEXT("%s 도감 줄"), *Id), FGrimDressingData::FindJournal(Row.JournalId));
		}
		TestTrue(FString::Printf(TEXT("%s 빛 그림자 거리(성능 규칙 23): 끄는 거리 30 m 이하"), *Id), Row.LightCandela <= 0.f || Row.LightMaxDrawDistance <= 3000.f);
	}
	for (const TPair<FName, uint8*>& Pair : Harvest->GetRowMap())
	{
		const FGrimHarvestNodeRow& Row = *reinterpret_cast<const FGrimHarvestNodeRow*>(Pair.Value);
		TestNotNull(FString::Printf(TEXT("채집점 %s 메시"), *Pair.Key.ToString()), Row.Mesh.LoadSynchronous());
		TestTrue(FString::Printf(TEXT("채집점 %s 아이템 %s"), *Pair.Key.ToString(), *Row.Item.ToString()), UGrimItemStatics::IsKnownItem(Row.Item));
		TestTrue(FString::Printf(TEXT("채집점 %s 개수"), *Pair.Key.ToString()), Row.CountMin >= 1 && Row.CountMax >= Row.CountMin);
	}
	TestTrue(TEXT("책장 20 쪽"), Journal->GetRowMap().Num() >= 20);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimDressingRewardsTest, "Grimoire.Dressing.Rewards", GrimDressingTest::Flags)
bool FGrimDressingRewardsTest::RunTest(const FString& Parameters)
{
	TArray<FString> Problems;
	const TArray<FGrimRewardEntry> Entries = FGrimDressingData::ParseRewards(TEXT("Stone:1:4|Ash:1-2:4|Bad:0:1|:1"), &Problems);
	TestEqual(TEXT("맞는 조각 둘"), Entries.Num(), 2);
	TestEqual(TEXT("틀린 조각 둘"), Problems.Num(), 2);
	TestEqual(TEXT("Ash 최대 2"), Entries.Num() > 1 ? Entries[1].Max : 0, 2);
	// 무게 4 · 4 · 빈손 2 → 0~0.4 Stone, 0.4~0.8 Ash, 0.8~1 빈손
	TestEqual(TEXT("굴림 0.1 → Stone"), FGrimDressingData::PickReward(Entries, 2.f, 0.1f).Item, FName(TEXT("Stone")));
	TestEqual(TEXT("굴림 0.5 → Ash"), FGrimDressingData::PickReward(Entries, 2.f, 0.5f).Item, FName(TEXT("Ash")));
	TestTrue(TEXT("굴림 0.9 → 빈손"), FGrimDressingData::PickReward(Entries, 2.f, 0.9f).Item.IsNone());
	TestTrue(TEXT("빈 목록 → 빈손"), FGrimDressingData::PickReward({}, 0.f, 0.5f).Item.IsNone());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimDressingRulesTest, "Grimoire.Dressing.ReactionRules", GrimDressingTest::Flags)
bool FGrimDressingRulesTest::RunTest(const FString& Parameters)
{
	TArray<FString> Problems;
	TStrongObjectPtr<UDataTable> Table(FGrimElementalReactionLibrary::LoadTableFromCSVFile(
		FPaths::Combine(FPaths::ProjectContentDir(), GetDefault<UGrimInteractionSettings>()->ReactionTableCSV), Problems));
	if (!TestNotNull(TEXT("규칙표"), Table.Get()))
	{
		return false;
	}
	TestEqual(TEXT("규칙표 문제 없음"), Problems.Num(), 0);

	// D01 바람솜꽃: 바람 → 비었다(한 번만). 젖었으면 먼저 마른다
	{
		GrimDressingTest::FRuleObject Cotton{ Table.Get() };
		Cotton.Material.AddTag(GrimDressingTags::Material_Seedhead);
		Cotton.Hit(GrimTags::Element_Wind, 0.3f);
		TestFalse(TEXT("약한 바람(0.3)엔 안 날린다"), Cotton.Has(GrimDressingTags::State_Spent));
		Cotton.Hit(GrimTags::Element_Water, 0.5f);
		Cotton.Hit(GrimTags::Element_Wind, 1.f);
		TestFalse(TEXT("젖은 솜은 바람에 먼저 마른다"), Cotton.Has(GrimDressingTags::State_Spent));
		Cotton.Wait(25.f);
		Cotton.Hit(GrimTags::Element_Wind, 1.f);
		TestTrue(TEXT("마른 솜은 바람에 날린다 → State.Spent"), Cotton.Has(GrimDressingTags::State_Spent));
		TestFalse(TEXT("불에 안 탄다(재질에 Flammable 없음)"), (Cotton.Hit(GrimTags::Element_Fire, 1.f), Cotton.Has(GrimTags::State_Burning)));
	}
	// D03 목마른 들꽃: 비(물 0.5) → 핀다
	{
		GrimDressingTest::FRuleObject Flower{ Table.Get() };
		Flower.Material.AddTag(GrimDressingTags::Material_Thirsty);
		TArray<FGrimElementalStateChange> Changes;
		Flower.States.SetIntensity(GrimDressingTags::State_Wilted, 1.f, Changes);
		Flower.Hit(GrimTags::Element_Water, 0.5f);
		TestTrue(TEXT("비를 맞고 핀다 → State.Bloomed"), Flower.Has(GrimDressingTags::State_Bloomed));
		TestFalse(TEXT("시듦이 지워진다"), Flower.Has(GrimDressingTags::State_Wilted));
		TestFalse(TEXT("처음 물은 젖음이 아니라 피기"), Flower.Has(GrimTags::State_Wet));
	}
	// D02 마른 풀: 불 → 8초 탄다 → 다 탄다
	{
		GrimDressingTest::FRuleObject Grass{ Table.Get() };
		Grass.Material.AddTag(GrimInteractionTags::Material_Flammable);
		Grass.Material.AddTag(GrimInteractionTags::Material_Consumable);
		Grass.Hit(GrimTags::Element_Fire, 1.f);
		TestTrue(TEXT("마른 풀에 불이 붙는다"), Grass.Has(GrimTags::State_Burning));
		Grass.Wait(8.5f);
		TestTrue(TEXT("8초 뒤 다 탔다 → State.Charred"), Grass.Has(GrimInteractionTags::State_Charred));
		Grass.Hit(GrimTags::Element_Fire, 1.f);
		TestFalse(TEXT("탄 풀엔 다시 안 붙는다"), Grass.Has(GrimTags::State_Burning));
	}
	// D04 등잔: 불 → 켜짐, 돌풍 → 꺼짐, 비 → 꺼짐
	{
		GrimDressingTest::FRuleObject Lamp{ Table.Get() };
		Lamp.Material.AddTag(GrimInteractionTags::Material_Flammable);
		Lamp.Hit(GrimTags::Element_Fire, 1.f);
		TestTrue(TEXT("등잔이 켜진다"), Lamp.Has(GrimTags::State_Burning));
		Lamp.Hit(GrimTags::Element_Water, 0.5f);
		TestFalse(TEXT("비에 꺼진다"), Lamp.Has(GrimTags::State_Burning));
	}
	// D21 책장 · D16 나비 · D29 떠 있는 돌
	{
		GrimDressingTest::FRuleObject Page{ Table.Get() };
		Page.Material.AddTag(GrimDressingTags::Material_Paper);
		Page.Hit(GrimTags::Element_Wind, 0.6f);
		TestFalse(TEXT("약한 바람엔 책장이 안 날린다"), Page.Has(GrimDressingTags::State_Blown));
		Page.Hit(GrimTags::Element_Wind, 1.2f);
		TestTrue(TEXT("센 바람에 날린다 → State.Blown"), Page.Has(GrimDressingTags::State_Blown));
		Page.Wait(2.f);
		TestFalse(TEXT("날림은 잠깐"), Page.Has(GrimDressingTags::State_Blown));

		GrimDressingTest::FRuleObject Swarm{ Table.Get() };
		Swarm.Material.AddTag(GrimDressingTags::Material_Swarm);
		Swarm.Hit(GrimTags::Element_Wind, 0.5f);
		TestTrue(TEXT("나비가 흩어진다"), Swarm.Has(GrimDressingTags::State_Scattered));
		Swarm.Wait(21.f);
		TestFalse(TEXT("20초 뒤 돌아온다"), Swarm.Has(GrimDressingTags::State_Scattered));

		GrimDressingTest::FRuleObject Stone{ Table.Get() };
		Stone.Material.AddTag(GrimDressingTags::Material_Floating);
		Stone.Hit(GrimMagicTags::Element_None, 1.f);
		TestTrue(TEXT("충격에 튕긴다 → State.Struck"), Stone.Has(GrimDressingTags::State_Struck));
		Stone.Hit(GrimTags::Element_Fire, 1.f);
		TestFalse(TEXT("돌은 불에 안 붙는다"), Stone.Has(GrimTags::State_Burning));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimDressingPropReactTest, "Grimoire.Dressing.PropReact", GrimDressingTest::Flags)
bool FGrimDressingPropReactTest::RunTest(const FString& Parameters)
{
	FGrimDressingData::Reset();
	GrimDressingTest::FScopedGameWorld TestWorld;

	// D01: 바람 → 솜이 빠지고 보상, 아침 → 다시
	AGrimReactiveProp* Cotton = TestWorld.SpawnProp(TEXT("D01_WindCotton"), FVector(0.f, 0.f, 0.f));
	TestTrue(TEXT("D01 처음 상태"), Cotton->IsAtInitialState());
	UGrimElementalStatics::ApplyStimulusToActor(Cotton, GrimDressingTest::Stim(GrimTags::Element_Wind, 1.f), nullptr);
	TestTrue(TEXT("D01 바람 → State.Spent"), Cotton->GetElementalState()->HasState(GrimDressingTags::State_Spent));
	TestTrue(TEXT("D01 바뀐 모습(솜 없음)"), Cotton->IsShowingAlt());
	TestEqual(TEXT("D01 솜씨앗 픽업 하나"), TestWorld.CountPickups(TEXT("WindCotton")), 1);
	UGrimElementalStatics::ApplyStimulusToActor(Cotton, GrimDressingTest::Stim(GrimTags::Element_Wind, 1.f), nullptr);
	TestEqual(TEXT("D01 다시 쏴도 보상은 한 번"), TestWorld.CountPickups(TEXT("WindCotton")), 1);
	Cotton->HandleDawn();
	TestTrue(TEXT("D01 아침에 다시 난다"), Cotton->IsAtInitialState());
	TestFalse(TEXT("D01 솜이 다시 보인다"), Cotton->IsShowingAlt());

	// D03: 처음 시듦, 물 → 핌 + 꽃잎, 아침 → 다시 시듦
	AGrimReactiveProp* Flower = TestWorld.SpawnProp(TEXT("D03_ThirstyFlower"), FVector(500.f, 0.f, 0.f));
	TestTrue(TEXT("D03 처음엔 시들었다"), Flower->GetElementalState()->HasState(GrimDressingTags::State_Wilted));
	UGrimElementalStatics::ApplyStimulusToActor(Flower, GrimDressingTest::Stim(GrimTags::Element_Water, 0.5f), nullptr);
	TestTrue(TEXT("D03 물 → 핌"), Flower->GetElementalState()->HasState(GrimDressingTags::State_Bloomed));
	TestEqual(TEXT("D03 들꽃잎"), TestWorld.CountPickups(TEXT("WildPetal")), 1);
	Flower->HandleDawn();
	TestTrue(TEXT("D03 아침에 다시 시든다"), Flower->GetElementalState()->HasState(GrimDressingTags::State_Wilted) && Flower->IsAtInitialState());

	// D02: 불 → 탄다 → 다 타면 몸이 사라지고 재 자국 · 보상 굴림(빈손일 수 있다)
	AGrimReactiveProp* Grass = TestWorld.SpawnProp(TEXT("D02_DryGrass"), FVector(1000.f, 0.f, 0.f));
	Grass->SetRewardSeed(7);
	UGrimElementalStatics::ApplyStimulusToActor(Grass, GrimDressingTest::Stim(GrimTags::Element_Fire, 1.f), nullptr);
	TestTrue(TEXT("D02 불 → 탄다"), Grass->GetElementalState()->HasState(GrimTags::State_Burning));
	Grass->GetElementalState()->AdvanceTime(9.f);
	TestTrue(TEXT("D02 다 탔다"), Grass->GetElementalState()->HasState(GrimInteractionTags::State_Charred));
	TestTrue(TEXT("D02 탄 모습"), Grass->IsShowingAlt());
	Grass->HandleDawn();
	TestTrue(TEXT("D02 아침에 다시 자란다"), Grass->IsAtInitialState());

	// D02 → 옆 등잔 불 번짐은 규칙표 2.5 m(전파 서브시스템) — 여기서는 등잔 자체만
	AGrimReactiveProp* Lamp = TestWorld.SpawnProp(TEXT("D04_StoneLamp"), FVector(1500.f, 0.f, 0.f));
	UGrimElementalStatics::ApplyStimulusToActor(Lamp, GrimDressingTest::Stim(GrimTags::Element_Fire, 1.f), nullptr);
	TestTrue(TEXT("D04 켜진다"), Lamp->GetElementalState()->HasState(GrimTags::State_Burning));
	Lamp->HandleDawn();
	TestFalse(TEXT("D04 아침이면 꺼진다"), Lamp->GetElementalState()->HasState(GrimTags::State_Burning));

	// D20: 켜면 아침에도 그대로(되돌리지 않는다)
	AGrimReactiveProp* Campfire = TestWorld.SpawnProp(TEXT("D20_Campfire"), FVector(2000.f, 0.f, 0.f));
	UGrimElementalStatics::ApplyStimulusToActor(Campfire, GrimDressingTest::Stim(GrimTags::Element_Fire, 1.f), nullptr);
	Campfire->HandleDawn();
	TestTrue(TEXT("D20 모닥불은 아침에도 탄다"), Campfire->GetElementalState()->HasState(GrimTags::State_Burning));

	// D29: 충격 → 튕긴다(연출 틱이 켜졌다 꺼진다)
	AGrimReactiveProp* Stone = TestWorld.SpawnProp(TEXT("D29_FloatStone"), FVector(2500.f, 0.f, 50.f));
	UGrimElementalStatics::ApplyStimulusToActor(Stone, GrimDressingTest::Stim(GrimMagicTags::Element_None, 1.f), nullptr);
	TestTrue(TEXT("D29 State.Struck"), Stone->GetElementalState()->HasState(GrimDressingTags::State_Struck));

	// D21: 센 바람 → 옮겨 간다
	AGrimReactiveProp* Page = TestWorld.SpawnProp(TEXT("D21_Page"), FVector(3000.f, 0.f, 0.f));
	const FVector Before = Page->GetActorLocation();
	UGrimElementalStatics::ApplyStimulusToActor(Page, GrimDressingTest::Stim(GrimTags::Element_Wind, 1.5f), nullptr);
	TestTrue(TEXT("D21 바람에 2.5 m 넘게 날려 갔다"), FVector::Dist2D(Before, Page->GetActorLocation()) >= 249.f);
	TestEqual(TEXT("D21 도감 줄"), Page->GetJournalId(), FName(TEXT("Page_01")));
	Page->SetJournalOverride(TEXT("Page_07"));
	TestEqual(TEXT("D21 쪽마다 덮어쓰기"), Page->GetJournalId(), FName(TEXT("Page_07")));

	// D16: 바람 → 흩어짐
	AGrimReactiveProp* Swarm = TestWorld.SpawnProp(TEXT("D16_Butterflies"), FVector(3500.f, 0.f, 0.f));
	UGrimElementalStatics::ApplyStimulusToActor(Swarm, GrimDressingTest::Stim(GrimTags::Element_Wind, 0.5f), nullptr);
	TestTrue(TEXT("D16 흩어졌다"), Swarm->GetElementalState()->HasState(GrimDressingTags::State_Scattered));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimDressingHarvestTest, "Grimoire.Dressing.HarvestRespawn", GrimDressingTest::Flags)
bool FGrimDressingHarvestTest::RunTest(const FString& Parameters)
{
	FGrimDressingData::Reset();
	// 시간 재생 · 사람별 · 두 번 쳐야 하는 행을 시험 표로 더한다(나중의 나무 · 풀 채집 모양)
	TArray<FString> Problems;
	UDataTable* Harvest = FGrimDressingData::LoadCSV(GetDefault<UGrimDressingSettings>()->HarvestNodesCSV, FGrimHarvestNodeRow::StaticStruct(), Problems);
	if (!TestNotNull(TEXT("채집점 표"), Harvest))
	{
		return false;
	}
	FGrimHarvestNodeRow TreeLike = *Harvest->FindRow<FGrimHarvestNodeRow>(TEXT("D10_Branch1"), TEXT("test"));
	TreeLike.Ownership = EGrimHarvestOwnership::PerPlayer;
	TreeLike.HitsToHarvest = 2;
	TreeLike.ResetRule = EGrimPropReset::Seconds;
	TreeLike.ResetSeconds = 5.f;
	Harvest->AddRow(TEXT("Test_TreeLike"), TreeLike);
	FGrimDressingData::SetTablesForTest(nullptr, Harvest, nullptr);

	GrimDressingTest::FScopedGameWorld TestWorld;
	AGrimHarvestCell* Cell = TestWorld.World->SpawnActorDeferred<AGrimHarvestCell>(AGrimHarvestCell::StaticClass(), FTransform::Identity);
	Cell->AddPoint(TEXT("D10_Branch1"), FTransform(FVector(0.f, 0.f, 0.f)));
	Cell->AddPoint(TEXT("D10_Pebble1"), FTransform(FRotator::ZeroRotator, FVector(300.f, 0.f, 0.f), FVector(0.5f)));
	Cell->AddPoint(TEXT("Test_TreeLike"), FTransform(FVector(600.f, 0.f, 0.f)));
	Cell->FinishSpawning(FTransform::Identity);

	TestTrue(TEXT("점 0 보인다"), Cell->IsPointVisible(0));
	Cell->HarvestPoint(0, TEXT(""), nullptr);
	TestTrue(TEXT("공유 점은 누구에게나 거둔 것"), Cell->IsHarvestedFor(0, TEXT("A")) && Cell->IsHarvestedFor(0, TEXT("B")));
	TestFalse(TEXT("거둔 점은 숨는다(ISM 크기 0)"), Cell->IsPointVisible(0));
	TestEqual(TEXT("복제 기록 1"), Cell->GetHarvestedEntryCount(), 1);
	TestTrue(TEXT("기록이 복제 대상으로 표시됐다"), Cell->GetHarvested().Items[0].ReplicationID != INDEX_NONE);

	// 사람별 · 두 번
	Cell->HarvestPoint(2, TEXT("A"), nullptr);
	TestFalse(TEXT("한 번 친 것은 아직"), Cell->IsHarvestedFor(2, TEXT("A")));
	Cell->HarvestPoint(2, TEXT("A"), nullptr);
	TestTrue(TEXT("두 번 치면 A 몫은 거뒀다"), Cell->IsHarvestedFor(2, TEXT("A")));
	TestFalse(TEXT("B 몫은 그대로"), Cell->IsHarvestedFor(2, TEXT("B")));

	// 세이브 왕복
	TArray<uint8> Bytes;
	{
		FMemoryWriter Writer(Bytes);
		Cell->WriteSaveData(Writer);
	}
	AGrimHarvestCell* Loaded = TestWorld.World->SpawnActorDeferred<AGrimHarvestCell>(AGrimHarvestCell::StaticClass(), FTransform::Identity);
	for (const FGrimHarvestPoint& Point : Cell->GetPoints())
	{
		Loaded->AddPoint(Point.Kind, Point.Transform);
	}
	Loaded->FinishSpawning(FTransform::Identity);
	{
		FMemoryReader Reader(Bytes);
		Loaded->ReadSaveData(Reader, 1);
	}
	TestTrue(TEXT("세이브 왕복: 점 0 거둠"), Loaded->IsHarvestedFor(0, TEXT("")));
	TestTrue(TEXT("세이브 왕복: 점 2 는 A 몫만"), Loaded->IsHarvestedFor(2, TEXT("A")) && !Loaded->IsHarvestedFor(2, TEXT("B")));

	// 시간 재생: 5초 뒤
	const double Now = TestWorld.World->GetTimeSeconds();
	Cell->RespawnDue(Now + 2.0);
	TestTrue(TEXT("2초 뒤엔 아직"), Cell->IsHarvestedFor(2, TEXT("A")));
	Cell->RespawnDue(Now + 6.0);
	TestFalse(TEXT("5초 지나면 다시 났다"), Cell->IsHarvestedFor(2, TEXT("A")));
	TestTrue(TEXT("아침 재생 점은 시간으로는 안 난다"), Cell->IsHarvestedFor(0, TEXT("")));

	// 아침
	Cell->HandleDawn();
	TestFalse(TEXT("아침에 곁가지가 다시 났다"), Cell->IsHarvestedFor(0, TEXT("")));
	TestTrue(TEXT("다시 보인다"), Cell->IsPointVisible(0));

	FGrimDressingData::Reset();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimDressingReplicationTest, "Grimoire.Dressing.Replication", GrimDressingTest::Flags)
bool FGrimDressingReplicationTest::RunTest(const FString& Parameters)
{
	FGrimDressingData::Reset();
	// 설정: 복제 · 휴면 · 복제 속성(성능 규칙 18 — 상태가 바뀔 때만 깨어난다)
	const AGrimReactiveProp* PropCDO = GetDefault<AGrimReactiveProp>();
	TestTrue(TEXT("소품은 복제된다"), PropCDO->GetIsReplicated());
	TestTrue(TEXT("소품은 휴면(DormantAll)"), PropCDO->NetDormancy == DORM_DormantAll);
	TestTrue(TEXT("소품은 날려 간 자리를 복제한다"), PropCDO->IsReplicatingMovement());
	TestFalse(TEXT("소품은 틱으로 시작하지 않는다"), PropCDO->PrimaryActorTick.bStartWithTickEnabled);
	TestTrue(TEXT("PropId 복제"), GrimDressingTest::HasReplicatedProperty(AGrimReactiveProp::StaticClass(), TEXT("PropId")));
	TestTrue(TEXT("주운 사람 복제"), GrimDressingTest::HasReplicatedProperty(AGrimReactiveProp::StaticClass(), TEXT("CollectedBy")));
	TestTrue(TEXT("상태 컴포넌트는 복제된다"), PropCDO->GetElementalState() && PropCDO->GetElementalState()->GetIsReplicated());

	const AGrimHarvestCell* CellCDO = GetDefault<AGrimHarvestCell>();
	TestTrue(TEXT("셀은 복제된다"), CellCDO->GetIsReplicated());
	TestTrue(TEXT("셀은 휴면"), CellCDO->NetDormancy == DORM_DormantAll);
	TestTrue(TEXT("거둔 점 FastArray 복제"), GrimDressingTest::HasReplicatedProperty(AGrimHarvestCell::StaticClass(), TEXT("Harvested")));
	TestFalse(TEXT("셀은 틱이 없다"), CellCDO->PrimaryActorTick.bCanEverTick);

	// 서버 셀 → 클라이언트 셀: 복제 콜백(PostReplicatedAdd)으로 클라이언트 모습이 따라온다
	GrimDressingTest::FScopedGameWorld TestWorld;
	auto MakeCell = [&]()
	{
		AGrimHarvestCell* Cell = TestWorld.World->SpawnActorDeferred<AGrimHarvestCell>(AGrimHarvestCell::StaticClass(), FTransform::Identity);
		Cell->AddPoint(TEXT("D10_Branch2"), FTransform(FVector(0.f, 0.f, 0.f)));
		Cell->AddPoint(TEXT("D10_Pebble2"), FTransform(FVector(200.f, 0.f, 0.f)));
		Cell->FinishSpawning(FTransform::Identity);
		return Cell;
	};
	AGrimHarvestCell* Server = MakeCell();
	AGrimHarvestCell* Client = MakeCell();
	Server->HarvestPoint(1, TEXT(""), nullptr);
	Client->ApplyReplicatedForTest(Server->GetHarvested());
	TestFalse(TEXT("클라이언트: 거둔 점이 숨었다"), Client->IsPointVisible(1));
	TestTrue(TEXT("클라이언트: 다른 점은 그대로"), Client->IsPointVisible(0));
	Server->HandleDawn();
	Client->ApplyReplicatedForTest(Server->GetHarvested());
	TestTrue(TEXT("클라이언트: 아침에 다시 보인다"), Client->IsPointVisible(1));

	// 소품 상태 → 클라이언트 모습: 상태 컴포넌트의 복제 콜백(HandleReplicatedStateChange)이 같은 델리게이트를 부른다
	AGrimReactiveProp* Cotton = TestWorld.SpawnProp(TEXT("D01_WindCotton"), FVector::ZeroVector);
	Cotton->GetElementalState()->HandleReplicatedStateChange(GrimDressingTags::State_Spent, 0.f, 1.f);
	TestFalse(TEXT("복제 콜백만으로는 서버 상태를 바꾸지 않는다"), Cotton->GetElementalState()->HasState(GrimDressingTags::State_Spent));
	return true;
}

#endif
