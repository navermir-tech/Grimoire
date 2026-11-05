#include "Misc/AutomationTest.h"
#include "Items/GrimCookingPot.h"
#include "Items/GrimInventoryComponent.h"
#include "Items/GrimInventoryWidget.h"
#include "Items/GrimItemPickup.h"
#include "Items/GrimItemSettings.h"
#include "Items/GrimItemStatics.h"
#include "Items/GrimItemTags.h"
#include "Items/GrimItemTypes.h"
#include "Core/GrimElementalTypes.h"
#include "Core/GrimGameplayTags.h"
#include "Creatures/GrimCreatureStorageMath.h"
#include "Interaction/GrimElementalObject.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "Quest/GrimQuestSettings.h"
#include "Quest/GrimQuestTypes.h"
#include "Survival/GrimSurvivalSettings.h"
#include "Survival/GrimVitalAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/WorldSettings.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

#if WITH_DEV_AUTOMATION_TESTS

// 유니티 빌드에서 다른 테스트 파일의 도우미와 부딪히지 않게 이름을 붙인다.
namespace GrimItemTestsLocal
{
	constexpr EAutomationTestFlags PureFlags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;
	constexpr EAutomationTestFlags WorldFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	const TCHAR* ItemsHeader = TEXT("Name,DisplayName,Description,Kind,MaxStack,Weight,Groups,HungerRestore,bRaw,DiseaseChance,BuffSeconds,BuffColdResist,BuffHeatResist,BuffDiseaseResist,CookResult,CookSeconds,TeachesSpell,Shape,Size,Color\n");

	/** 시험용 작은 표: 날고기(10 쌓임, 익으면 구운 고기) · 구운 고기 · 허브 · 소금 · 칼(쌓이지 않음) */
	FGrimItemDatabase MakeTestDatabase(FAutomationTestBase& Test)
	{
		const FString Items = FString(ItemsHeader)
			+ TEXT("Raw,날고기,,Food,10,0,\"Meat|Raw\",8,True,-1,0,0,0,0,Cooked,10,,Cube,20,\"(R=1,G=0,B=0,A=1)\"\n")
			+ TEXT("Cooked,구운 고기,,Food,10,0,\"Meat|Cooked\",30,False,0,0,0,0,0,,0,,Cube,20,\"(R=1,G=0,B=0,A=1)\"\n")
			+ TEXT("Herb,허브,,Material,20,0,Ingredient,0,False,0,0,0,0,0,,0,,Sphere,10,\"(R=0,G=1,B=0,A=1)\"\n")
			+ TEXT("Salt,소금,,Material,20,0,Ingredient,0,False,0,0,0,0,0,,0,,Cube,10,\"(R=1,G=1,B=1,A=1)\"\n")
			+ TEXT("Knife,칼,,Tool,1,0,,0,False,0,0,0,0,0,,0,,Cube,10,\"(R=1,G=1,B=1,A=1)\"\n");
		const FString Recipes = TEXT("Name,DisplayName,Station,Inputs,Output,OutputCount,CookSeconds\n")
			TEXT("Soup,수프,CookingPot,\"#Meat:1|Herb:2|Salt:1\",Cooked,1,5\n");
		FGrimItemDatabase Database;
		TArray<FString> Problems;
		FGrimItemDatabase::BuildFromCSV(Items, Recipes, Database, Problems);
		for (const FString& Problem : Problems)
		{
			Test.AddError(FString::Printf(TEXT("시험 표 문제: %s"), *Problem));
		}
		return Database;
	}

	TArray<FGrimItemInstance> EmptySlots(int32 Num)
	{
		TArray<FGrimItemInstance> Slots;
		Slots.SetNum(Num);
		return Slots;
	}

	/** 자동화 테스트용 게임 월드(BeginPlay 까지). 소멸자가 정리한다 */
	struct FItemTestWorld
	{
		UWorld* World = nullptr;

		FItemTestWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("GrimItemTestWorld"));
			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
			World->BeginPlay();
			World->GetWorldSettings()->NotifyBeginPlay();
		}

		~FItemTestWorld()
		{
			World->EndPlay(EEndPlayReason::Quit);
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}

		template <typename T>
		T* Spawn(const FVector& Location)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			return World->SpawnActor<T>(Location, FRotator::ZeroRotator, Params);
		}

		UGrimInventoryComponent* MakeInventory()
		{
			AActor* Actor = World->SpawnActor<AActor>();
			UGrimInventoryComponent* Inventory = NewObject<UGrimInventoryComponent>(Actor, UGrimInventoryComponent::ComponentName);
			Actor->AddInstanceComponent(Inventory);
			Inventory->RegisterComponent();
			return Inventory;
		}

		AGrimCampfire* SpawnLitCampfire(const FVector& Location)
		{
			AGrimCampfire* Campfire = Spawn<AGrimCampfire>(Location);
			FGrimElementalStimulus Fire;
			Fire.Element = GrimTags::Element_Fire;
			Fire.Intensity = 1.f;
			UGrimElementalStatics::ApplyStimulusToActor(Campfire, Fire, nullptr);
			return Campfire;
		}

		int32 CountPickups(FName ItemId) const
		{
			int32 Count = 0;
			for (TActorIterator<AGrimItemPickup> It(World); It; ++It)
			{
				if (!It->IsActorBeingDestroyed() && It->GetItem().ItemId == ItemId)
				{
					Count += It->GetItem().Count;
				}
			}
			return Count;
		}
	};
}

using namespace GrimItemTestsLocal;

// ───── 표 ─────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimItemsTableTest, "Grimoire.Items.Table", PureFlags)
bool FGrimItemsTableTest::RunTest(const FString& Parameters)
{
	// 저장소의 표: 문제 없이 읽힌다.
	const UGrimItemSettings* Settings = UGrimItemSettings::Get();
	FString Items, Recipes;
	if (!TestTrue(TEXT("Items.csv 를 연다"), UGrimItemSettings::LoadContentFile(Settings->ItemsCSV, Items))
		|| !TestTrue(TEXT("Recipes.csv 를 연다"), UGrimItemSettings::LoadContentFile(Settings->RecipesCSV, Recipes)))
	{
		return false;
	}
	FGrimItemDatabase Database;
	TArray<FString> Problems;
	TestTrue(TEXT("저장소 표에 문제 없음"), FGrimItemDatabase::BuildFromCSV(Items, Recipes, Database, Problems));
	for (const FString& Problem : Problems)
	{
		AddError(Problem);
	}

	// M3 생물 전리품(갈래 C 가 SpawnPickup 에 넘길 ID) · 튜토리얼 지급
	for (const TCHAR* Id : { TEXT("JackalopeMeat"), TEXT("JackalopeFur"), TEXT("JackalopeAntler"), TEXT("KelpieMane"), TEXT("KelpieWaterEssence"), TEXT("BlackDogHide"), TEXT("BlackDogFang"),
		TEXT("Herb"), TEXT("Salt"), TEXT("Grain"), TEXT("Stew"), TEXT("RoastedJackalopeMeat") })
	{
		TestNotNull(*FString::Printf(TEXT("아이템 %s"), Id), Database.Find(Id));
	}
	const FGrimItemDefinition* Meat = Database.Find(TEXT("JackalopeMeat"));
	if (TestNotNull(TEXT("날고기"), Meat))
	{
		TestTrue(TEXT("날고기는 음식 · 날것"), Meat->IsFood() && Meat->Row.bRaw);
		TestTrue(TEXT("날고기는 익는다 → 구운 고기"), Meat->IsCookable() && Meat->Row.CookResult == FName(TEXT("RoastedJackalopeMeat")));
		TestTrue(TEXT("고기 묶음(튜토리얼 Item.Obtained.Meat)"), Meat->IsInGroup(TEXT("Meat")));
		TestEqual(TEXT("묶음 이벤트 이름"), GrimItemEvents::Obtained(TEXT("Meat")), FName(TEXT("Item.Obtained.Meat")));
	}
	const FGrimItemDefinition* Roasted = Database.Find(TEXT("RoastedJackalopeMeat"));
	if (Meat && TestNotNull(TEXT("구운 고기"), Roasted))
	{
		TestTrue(TEXT("익힌 것이 배를 더 채운다"), Roasted->Row.HungerRestore > Meat->Row.HungerRestore);
		TestFalse(TEXT("익힌 것은 날것이 아니다"), Roasted->Row.bRaw);
	}
	const FGrimRecipeDefinition* Stew = Database.Recipes.FindByPredicate([](const FGrimRecipeDefinition& Recipe) { return Recipe.Row.Output == FName(TEXT("Stew")); });
	if (TestNotNull(TEXT("스튜 레시피"), Stew))
	{
		TestEqual(TEXT("스튜는 냄비에서"), Stew->Row.Station, FName(TEXT("CookingPot")));
		TArray<FGrimItemInstance> Contents = { FGrimItemInstance(TEXT("JackalopeMeat"), 1), FGrimItemInstance(TEXT("Herb"), 1), FGrimItemInstance(TEXT("Salt"), 1), FGrimItemInstance(TEXT("Grain"), 1) };
		TestTrue(TEXT("튜토리얼 재료(고기 + 허브 · 소금 · 곡물)로 스튜가 된다"), Database.FindCompleteRecipe(TEXT("CookingPot"), Contents) == Stew);
	}

	// 스킬북 아이템의 마법 정의가 있다(에디터에서). 노말 스킬 책(Skill:<ID> — 생물 보관함 R38)은 마법이 아니다.
	for (const TPair<FName, FGrimItemDefinition>& Pair : Database.Items)
	{
		FName NormalSkill;
		if (Pair.Value.Row.Kind == EGrimItemKind::Skillbook && GIsEditor && !GrimCreatureStorageMath::ParseNormalSkill(Pair.Value.Row.TeachesSpell, NormalSkill))
		{
			TestNotNull(*FString::Printf(TEXT("스킬북 %s 의 마법 %s"), *Pair.Key.ToString(), *Pair.Value.Row.TeachesSpell.ToString()),
				UGrimQuestSettings::LoadSpell(Pair.Value.Row.TeachesSpell));
		}
	}

	// 튜토리얼 단계의 Item:<ID> 는 거점 핵(M4) 말고 모두 표에 있다.
	const FGrimQuestDatabase& Quests = UGrimQuestSettings::GetQuestDatabase();
	for (const FGrimQuestDef& Quest : Quests.Quests)
	{
		for (const FGrimQuestStepDef& Step : Quest.Steps)
		{
			for (const FGrimQuestGive& Give : Step.Gives)
			{
				if (Give.Kind == FGrimQuestGive::EKind::Item && Give.Id != FName(TEXT("BaseCore")))
				{
					TestNotNull(*FString::Printf(TEXT("퀘스트 %s 의 지급 아이템 %s"), *Quest.Id.ToString(), *Give.Id.ToString()), Database.Find(Give.Id));
				}
			}
		}
	}

	// 잘못된 표는 문제를 낸다.
	const FString BadItems = FString(ItemsHeader)
		+ TEXT("A,가,,Food,0,0,,0,True,2,0,0,0,0,Nope,0,,Cube,10,\"(R=1,G=1,B=1,A=1)\"\n")
		+ TEXT("B,나,,Skillbook,1,0,,0,False,0,0,0,0,0,,0,,Cube,10,\"(R=1,G=1,B=1,A=1)\"\n");
	const FString BadRecipes = TEXT("Name,DisplayName,Station,Inputs,Output,OutputCount,CookSeconds\n")
		TEXT("R1,,CookingPot,\"#Nothing:1|Ghost:2|A:x\",Missing,0,0\n");
	FGrimItemDatabase Bad;
	TArray<FString> BadProblems;
	TestFalse(TEXT("잘못된 표는 실패"), FGrimItemDatabase::BuildFromCSV(BadItems, BadRecipes, Bad, BadProblems));
	auto Expect = [this, &BadProblems](const TCHAR* What, const TCHAR* Needle)
	{
		TestTrue(What, BadProblems.ContainsByPredicate([Needle](const FString& Problem) { return Problem.Contains(Needle); }));
	};
	Expect(TEXT("MaxStack 0"), TEXT("MaxStack"));
	Expect(TEXT("음식인데 배고픔 0"), TEXT("HungerRestore"));
	Expect(TEXT("확률 > 1"), TEXT("DiseaseChance"));
	Expect(TEXT("모르는 익은 결과"), TEXT("Nope"));
	Expect(TEXT("스킬북 마법 없음"), TEXT("TeachesSpell"));
	Expect(TEXT("빈 묶음"), TEXT("#Nothing"));
	Expect(TEXT("모르는 재료"), TEXT("Ghost"));
	Expect(TEXT("개수를 못 읽음"), TEXT("A:x"));
	Expect(TEXT("모르는 결과"), TEXT("Missing"));
	Expect(TEXT("OutputCount 0"), TEXT("OutputCount"));
	return true;
}

// ───── 쌓기 ─────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimItemsStackTest, "Grimoire.Items.Stack", PureFlags)
bool FGrimItemsStackTest::RunTest(const FString& Parameters)
{
	const FGrimItemDatabase Database = MakeTestDatabase(*this);
	TArray<FGrimItemInstance> Slots = EmptySlots(3);

	TestEqual(TEXT("빈 3칸에 날고기 30 까지"), FGrimInventoryModel::GetAddableCount(Slots, Database, TEXT("Raw")), 30);
	TestEqual(TEXT("25 를 넣는다"), FGrimInventoryModel::Add(Slots, Database, FGrimItemInstance(TEXT("Raw"), 25)), 25);
	TestEqual(TEXT("칸 0 = 10"), Slots[0].Count, 10);
	TestEqual(TEXT("칸 1 = 10"), Slots[1].Count, 10);
	TestEqual(TEXT("칸 2 = 5"), Slots[2].Count, 5);
	TestEqual(TEXT("더 넣으면 5 만 들어간다"), FGrimInventoryModel::Add(Slots, Database, FGrimItemInstance(TEXT("Raw"), 8)), 5);
	TestEqual(TEXT("가득 — 허브는 못 넣는다"), FGrimInventoryModel::Add(Slots, Database, FGrimItemInstance(TEXT("Herb"), 1)), 0);
	TArray<FGrimItemInstance> Single = EmptySlots(1);
	TestEqual(TEXT("모르는 아이템은 못 넣는다"), FGrimInventoryModel::Add(Single, Database, FGrimItemInstance(TEXT("Nope"), 1)), 0);
	TestEqual(TEXT("모두 30"), FGrimInventoryModel::CountOf(Slots, TEXT("Raw")), 30);

	TestEqual(TEXT("12 를 뺀다(뒤 칸부터)"), FGrimInventoryModel::Remove(Slots, TEXT("Raw"), 12), 12);
	TestEqual(TEXT("칸 2 비었다"), Slots[2].IsEmpty(), true);
	TestEqual(TEXT("칸 1 = 8"), Slots[1].Count, 8);
	TestEqual(TEXT("있는 것보다 많이 빼면 있는 만큼"), FGrimInventoryModel::Remove(Slots, TEXT("Raw"), 100), 18);

	// 앞의 빈 칸보다 쌓을 수 있는 칸이 먼저
	TArray<FGrimItemInstance> Mixed = EmptySlots(3);
	Mixed[2] = FGrimItemInstance(TEXT("Herb"), 15);
	FGrimInventoryModel::Add(Mixed, Database, FGrimItemInstance(TEXT("Herb"), 8));
	TestEqual(TEXT("허브 칸 2 를 먼저 채운다(20)"), Mixed[2].Count, 20);
	TestEqual(TEXT("남은 3 은 첫 빈 칸"), Mixed[0].Count, 3);

	// 쌓이지 않는 것 · 내구도가 있는 것
	TArray<FGrimItemInstance> Tools = EmptySlots(3);
	TestEqual(TEXT("칼 2 개는 두 칸"), FGrimInventoryModel::Add(Tools, Database, FGrimItemInstance(TEXT("Knife"), 2)), 2);
	TestTrue(TEXT("칼 칸 0 · 1"), Tools[0].Count == 1 && Tools[1].Count == 1);
	TArray<FGrimItemInstance> Worn = EmptySlots(2);
	FGrimItemInstance WornMeat(TEXT("Raw"), 1);
	WornMeat.Durability = 0.5f;
	FGrimInventoryModel::Add(Worn, Database, WornMeat);
	FGrimInventoryModel::Add(Worn, Database, FGrimItemInstance(TEXT("Raw"), 1));
	TestTrue(TEXT("내구도가 있으면 따로 쌓인다(M4 자리)"), Worn[0].Count == 1 && Worn[1].Count == 1 && Worn[0].Durability == 0.5f);

	// 칸에서 빼기
	TArray<FGrimItemInstance> One = EmptySlots(1);
	One[0] = FGrimItemInstance(TEXT("Salt"), 4);
	const FGrimItemInstance Taken = FGrimInventoryModel::RemoveAt(One, 0, 3);
	TestTrue(TEXT("칸에서 3 을 뺀다"), Taken.ItemId == FName(TEXT("Salt")) && Taken.Count == 3 && One[0].Count == 1);
	TArray<FGrimItemInstance> Empty = EmptySlots(1);
	TestTrue(TEXT("빈 칸에서 빼면 빈 것"), FGrimInventoryModel::RemoveAt(Empty, 0, 1).IsEmpty());
	return true;
}

// ───── 옮기기 ─────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimItemsMoveTest, "Grimoire.Items.Move", PureFlags)
bool FGrimItemsMoveTest::RunTest(const FString& Parameters)
{
	const FGrimItemDatabase Database = MakeTestDatabase(*this);
	TArray<FGrimItemInstance> Slots = EmptySlots(4);
	Slots[0] = FGrimItemInstance(TEXT("Raw"), 7);
	Slots[1] = FGrimItemInstance(TEXT("Raw"), 6);
	Slots[2] = FGrimItemInstance(TEXT("Herb"), 3);

	TestTrue(TEXT("같은 것 → 합친다(쌓을 수 있는 만큼)"), FGrimInventoryModel::Move(Slots, Database, 0, 1));
	TestEqual(TEXT("칸 1 = 10"), Slots[1].Count, 10);
	TestEqual(TEXT("칸 0 에 3 이 남는다"), Slots[0].Count, 3);
	TestFalse(TEXT("가득 찬 칸으로는 못 합친다"), FGrimInventoryModel::Move(Slots, Database, 0, 1));

	TestTrue(TEXT("다른 것 → 바꾼다"), FGrimInventoryModel::Move(Slots, Database, 0, 2));
	TestTrue(TEXT("칸 0 = 허브 3 · 칸 2 = 날고기 3"), Slots[0].ItemId == FName(TEXT("Herb")) && Slots[0].Count == 3 && Slots[2].ItemId == FName(TEXT("Raw")) && Slots[2].Count == 3);

	TestTrue(TEXT("빈 칸으로 옮긴다"), FGrimInventoryModel::Move(Slots, Database, 2, 3));
	TestTrue(TEXT("칸 2 비고 칸 3 = 날고기 3"), Slots[2].IsEmpty() && Slots[3].Count == 3);

	TestFalse(TEXT("같은 칸"), FGrimInventoryModel::Move(Slots, Database, 3, 3));
	TestFalse(TEXT("빈 칸에서"), FGrimInventoryModel::Move(Slots, Database, 2, 0));
	TestFalse(TEXT("범위 밖"), FGrimInventoryModel::Move(Slots, Database, 0, 9));

	TArray<FGrimItemInstance> Merge = EmptySlots(2);
	Merge[0] = FGrimItemInstance(TEXT("Raw"), 2);
	Merge[1] = FGrimItemInstance(TEXT("Raw"), 2);
	TestTrue(TEXT("다 합쳐지면"), FGrimInventoryModel::Move(Merge, Database, 0, 1));
	TestTrue(TEXT("옮긴 칸은 빈다"), Merge[0].IsEmpty() && Merge[1].Count == 4);
	return true;
}

// ───── 레시피 ─────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimItemsRecipeTest, "Grimoire.Items.Recipe", PureFlags)
bool FGrimItemsRecipeTest::RunTest(const FString& Parameters)
{
	const FGrimItemDatabase Database = MakeTestDatabase(*this);
	if (!TestEqual(TEXT("레시피 하나"), Database.Recipes.Num(), 1))
	{
		return false;
	}
	const FGrimRecipeDefinition& Soup = Database.Recipes[0];
	TestEqual(TEXT("재료 셋"), Soup.Inputs.Num(), 3);
	TestTrue(TEXT("#Meat 는 묶음"), Soup.Inputs[0].bGroup && Soup.Inputs[0].Id == FName(TEXT("Meat")));

	TArray<FGrimItemInstance> Contents = { FGrimItemInstance(TEXT("Cooked"), 1), FGrimItemInstance(TEXT("Herb"), 1) };
	FGrimRecipeProgress Progress = Database.EvaluateRecipe(Soup, Contents);
	TestTrue(TEXT("고기 1/1(구운 것도 고기 묶음) · 허브 1/2 · 소금 0/1"), Progress.Filled == TArray<int32>({ 1, 1, 0 }));
	TestFalse(TEXT("아직 덜 찼다"), Progress.IsComplete());
	TestNull(TEXT("완성 레시피 없음"), Database.FindCompleteRecipe(TEXT("CookingPot"), Contents));

	Contents = { FGrimItemInstance(TEXT("Raw"), 1), FGrimItemInstance(TEXT("Herb"), 2), FGrimItemInstance(TEXT("Salt"), 1) };
	TestTrue(TEXT("다 찼다"), Database.FindCompleteRecipe(TEXT("CookingPot"), Contents) == &Soup);
	TestNull(TEXT("다른 시설에서는 아니다"), Database.FindCompleteRecipe(TEXT("Oven"), Contents));

	Contents.Add(FGrimItemInstance(TEXT("Knife"), 1));
	Progress = Database.EvaluateRecipe(Soup, Contents);
	TestTrue(TEXT("남는 것(칼)이 있으면 완성이 아니다"), Progress.IsComplete() && Progress.bHasExtra);
	TestNull(TEXT("남는 것이 있으면 끓지 않는다"), Database.FindCompleteRecipe(TEXT("CookingPot"), Contents));

	TArray<FGrimRecipeInput> Inputs;
	TArray<FString> Problems;
	TestTrue(TEXT("\"A|#B:2\" 를 읽는다"), FGrimItemDatabase::ParseInputs(TEXT("A|#B:2"), Inputs, Problems));
	TestTrue(TEXT("A:1, #B:2"), Inputs.Num() == 2 && Inputs[0].Count == 1 && !Inputs[0].bGroup && Inputs[1].bGroup && Inputs[1].Count == 2);
	Inputs.Reset();
	TestFalse(TEXT("빈 재료는 실패"), FGrimItemDatabase::ParseInputs(TEXT(""), Inputs, Problems));
	TestFalse(TEXT("개수 0 은 실패"), FGrimItemDatabase::ParseInputs(TEXT("A:0"), Inputs, Problems));
	return true;
}

// ───── 먹기 ─────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimItemsEatTest, "Grimoire.Items.Eat", WorldFlags)
bool FGrimItemsEatTest::RunTest(const FString& Parameters)
{
	const FGrimItemDatabase& Database = UGrimItemSettings::GetDatabase();
	const FGrimItemDefinition* Raw = Database.Find(TEXT("JackalopeMeat"));
	const FGrimItemDefinition* Roasted = Database.Find(TEXT("RoastedJackalopeMeat"));
	const FGrimItemDefinition* Stew = Database.Find(TEXT("Stew"));
	if (!TestNotNull(TEXT("날고기"), Raw) || !TestNotNull(TEXT("구운 고기"), Roasted) || !TestNotNull(TEXT("스튜"), Stew))
	{
		return false;
	}

	// 배탈 확률: 날것 -1 → 생존 기본, 익힌 것 0
	const float DefaultChance = GetDefault<UGrimSurvivalSettings>()->Disease.RawFoodStomachChance;
	TestEqual(TEXT("날고기는 생존 기본 확률"), FGrimFoodModel::GetBaseDiseaseChance(*Raw, DefaultChance), DefaultChance);
	TestEqual(TEXT("구운 고기는 배탈 없음"), FGrimFoodModel::GetBaseDiseaseChance(*Roasted, DefaultChance), 0.f);
	if (const FGrimItemDefinition* Weed = Database.Find(TEXT("Waterweed")))
	{
		TestEqual(TEXT("수초는 표의 확률"), FGrimFoodModel::GetBaseDiseaseChance(*Weed, DefaultChance), Weed->Row.DiseaseChance);
	}

	FItemTestWorld TestWorld;
	AActor* Actor = TestWorld.World->SpawnActor<AActor>();
	UAbilitySystemComponent* ASC = NewObject<UAbilitySystemComponent>(Actor, TEXT("ASC"));
	ASC->RegisterComponent();
	ASC->AddAttributeSetSubobject(NewObject<UGrimMagicAttributeSet>(Actor));
	ASC->AddAttributeSetSubobject(NewObject<UGrimVitalAttributeSet>(Actor));
	ASC->InitAbilityActorInfo(Actor, Actor);

	const FGameplayAttribute Hunger = UGrimVitalAttributeSet::GetHungerAttribute();
	ASC->SetNumericAttributeBase(Hunger, 20.f);
	UGrimItemStatics::ApplyFoodEffects(ASC, *Roasted);
	TestEqual(TEXT("구운 고기: 배고픔 20 + 회복"), ASC->GetNumericAttribute(Hunger), 20.f + Roasted->Row.HungerRestore);
	UGrimItemStatics::ApplyFoodEffects(ASC, *Raw);
	TestTrue(TEXT("익힌 것이 날것보다 효율이 높다(표)"), Roasted->Row.HungerRestore > Raw->Row.HungerRestore);

	const float MaxHunger = GetDefault<UGrimSurvivalSettings>()->Tuning.MaxHunger;
	ASC->SetNumericAttributeBase(Hunger, MaxHunger - 5.f);
	UGrimItemStatics::ApplyFoodEffects(ASC, *Stew);
	TestEqual(TEXT("배고픔은 최대를 넘지 않는다"), ASC->GetNumericAttribute(Hunger), MaxHunger);

	TestTrue(TEXT("스튜 버프 태그"), ASC->HasMatchingGameplayTag(GrimItemTags::Status_Buff_Food));
	const float ColdResist = ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetColdResistAttribute());
	TestEqual(TEXT("스튜: 추위 저항 + 표의 값"), ColdResist, Stew->Row.BuffColdResist);
	UGrimItemStatics::ApplyFoodEffects(ASC, *Stew);
	TestEqual(TEXT("다시 먹어도 겹치지 않는다"), ASC->GetNumericAttribute(UGrimVitalAttributeSet::GetColdResistAttribute()), ColdResist);
	return true;
}

// ───── 요리 ─────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimItemsCookTest, "Grimoire.Items.Cook", WorldFlags)
bool FGrimItemsCookTest::RunTest(const FString& Parameters)
{
	// 모델: 세기 1 = 초당 1, 바람으로 키운 불(세기 3)은 1 + 2 × 0.5 = 2
	TestEqual(TEXT("불 없음"), FGrimCookingModel::GetCookRate(0.f, 0.5f), 0.f);
	TestEqual(TEXT("세기 1"), FGrimCookingModel::GetCookRate(1.f, 0.5f), 1.f);
	TestEqual(TEXT("세기 3"), FGrimCookingModel::GetCookRate(3.f, 0.5f), 2.f);
	float Progress = 0.f;
	TestFalse(TEXT("5초 — 덜 익음"), FGrimCookingModel::Advance(Progress, 1.f, 5.f, 10.f, 0.5f));
	TestTrue(TEXT("세기 3 으로 2.5초 더 — 다 익음"), FGrimCookingModel::Advance(Progress, 3.f, 2.5f, 10.f, 0.5f));

	const FGrimItemDefinition* Raw = UGrimItemSettings::GetDatabase().Find(TEXT("JackalopeMeat"));
	if (!TestNotNull(TEXT("날고기"), Raw))
	{
		return false;
	}
	const float CookSeconds = Raw->Row.CookSeconds;
	const float Step = UGrimItemSettings::Get()->CookCheckInterval;
	const int32 StepsToCook = FMath::CeilToInt(CookSeconds / Step) + 1;

	FItemTestWorld TestWorld;

	// 1) 불붙은 모닥불 곁의 날고기 → 익는다
	AGrimCampfire* Campfire = TestWorld.SpawnLitCampfire(FVector::ZeroVector);
	TestTrue(TEXT("모닥불에 불이 붙었다"), UGrimElementalStatics::GetStateIntensity(Campfire, GrimTags::State_Burning) > 0.f);
	AGrimItemPickup* NearFire = UGrimItemStatics::SpawnPickup(TestWorld.World, TEXT("JackalopeMeat"), 2, FVector(60.f, 0.f, 20.f));
	if (!TestNotNull(TEXT("모닥불 곁 픽업"), NearFire))
	{
		return false;
	}
	NearFire->UpdateCooking(Step);
	TestTrue(TEXT("모닥불 곁은 달궈진다"), NearFire->IsHeated());
	TestTrue(TEXT("진행이 오른다"), NearFire->GetCookProgress() > 0.f);
	for (int32 Index = 0; Index < StepsToCook; ++Index)
	{
		NearFire->UpdateCooking(Step);
	}
	TestEqual(TEXT("구운 고기가 됐다"), NearFire->GetItem().ItemId, FName(TEXT("RoastedJackalopeMeat")));
	TestEqual(TEXT("개수는 그대로"), NearFire->GetItem().Count, 2);
	TestEqual(TEXT("State.Cooked"), NearFire->GetElementalStateIntensity(GrimItemTags::State_Cooked), 1.f);
	TestEqual(TEXT("구운 고기는 더 익지 않는다"), NearFire->GetCookSeconds(), 0.f);

	// 2) 불에서 먼 날고기 → 그대로, 불 마법이 직접 닿으면 그동안 익는다
	AGrimItemPickup* Far = UGrimItemStatics::SpawnPickup(TestWorld.World, TEXT("JackalopeMeat"), 1, FVector(2000.f, 0.f, 20.f));
	if (!TestNotNull(TEXT("먼 픽업"), Far))
	{
		return false;
	}
	Far->UpdateCooking(Step);
	TestFalse(TEXT("불이 없으면 안 달궈진다"), Far->IsHeated());
	TestEqual(TEXT("진행 없음"), Far->GetCookProgress(), 0.f);
	FGrimElementalStimulus Fire;
	Fire.Element = GrimTags::Element_Fire;
	Fire.Intensity = 1.f;
	TestTrue(TEXT("픽업은 속성 자극을 받는다"), UGrimElementalStatics::ApplyStimulusToActor(Far, Fire, nullptr));
	const float DirectSeconds = UGrimItemSettings::Get()->DirectFireHeatSeconds;
	for (float Time = 0.f; Time < DirectSeconds + 2.f * Step; Time += Step)
	{
		Far->UpdateCooking(Step);
	}
	TestFalse(TEXT("직접 불이 끝나면 식는다"), Far->IsHeated());
	TestTrue(TEXT("직접 불만큼 익었다"), FMath::IsNearlyEqual(Far->GetCookProgress(), DirectSeconds, Step + KINDA_SMALL_NUMBER));
	TestEqual(TEXT("아직 날고기"), Far->GetItem().ItemId, FName(TEXT("JackalopeMeat")));

	// 3) 냄비: 재료가 다 들어가고 아래 불이 있으면 끓어 스튜가 냄비 앞에 놓인다
	AGrimCookingPot* Pot = TestWorld.Spawn<AGrimCookingPot>(FVector(0.f, 0.f, 55.f));
	UGrimInventoryComponent* Inventory = TestWorld.MakeInventory();
	Inventory->AddItem(FGrimItemInstance(TEXT("JackalopeMeat"), 2), false);
	Inventory->AddItem(FGrimItemInstance(TEXT("Herb"), 1), false);
	Inventory->AddItem(FGrimItemInstance(TEXT("Salt"), 1), false);
	Inventory->AddItem(FGrimItemInstance(TEXT("Grain"), 3), false);
	Inventory->AddItem(FGrimItemInstance(TEXT("JackalopeFur"), 1), false);
	TestEqual(TEXT("가방 칸 수 = 설정"), Inventory->GetSlotCount(), UGrimItemSettings::Get()->PlayerSlotCount);

	TestTrue(TEXT("가방에서 재료를 넣는다"), Inventory->InteractWithPot(Pot, false));
	TestEqual(TEXT("고기 하나 남음"), Inventory->CountOf(TEXT("JackalopeMeat")), 1);
	TestEqual(TEXT("곡물 둘 남음"), Inventory->CountOf(TEXT("Grain")), 2);
	TestEqual(TEXT("털은 그대로(레시피에 없음)"), Inventory->CountOf(TEXT("JackalopeFur")), 1);
	TestEqual(TEXT("허브는 들어갔다"), Inventory->CountOf(TEXT("Herb")), 0);
	const FGrimRecipeDefinition* Recipe = Pot->GetMatchingRecipe();
	if (!TestNotNull(TEXT("냄비 내용물이 스튜 레시피와 맞는다"), Recipe))
	{
		return false;
	}
	TestEqual(TEXT("더 넣을 것이 없다 — 끓기 전이면 꺼낸다"), Pot->AddIngredientsFrom(Inventory), 0);

	const int32 PotSteps = FMath::CeilToInt(Recipe->Row.CookSeconds / Step) + 1;
	Pot->UpdateCooking(Step);
	TestTrue(TEXT("아래 모닥불로 달궈진다"), Pot->IsHeated());
	TestEqual(TEXT("끓는 중엔 꺼내지 못한다"), Pot->TakeBackInto(Inventory), 0);
	for (int32 Index = 0; Index < PotSteps; ++Index)
	{
		Pot->UpdateCooking(Step);
	}
	TestEqual(TEXT("다 끓으면 내용물이 사라진다"), Pot->GetContents().Num(), 0);
	TestEqual(TEXT("스튜 하나가 냄비 앞에"), TestWorld.CountPickups(TEXT("Stew")), 1);

	// 4) 불이 없는 냄비는 끓지 않고, 꺼낼 수 있다
	AGrimCookingPot* ColdPot = TestWorld.Spawn<AGrimCookingPot>(FVector(5000.f, 0.f, 0.f));
	Inventory->AddItem(FGrimItemInstance(TEXT("Herb"), 1), false);
	Inventory->AddItem(FGrimItemInstance(TEXT("Salt"), 1), false);
	TestTrue(TEXT("찬 냄비에도 넣는다"), ColdPot->AddIngredientsFrom(Inventory) > 0);
	ColdPot->UpdateCooking(Step);
	TestFalse(TEXT("불이 없다"), ColdPot->IsHeated());
	TestEqual(TEXT("진행 없음"), ColdPot->GetProgress(), 0.f);
	TestTrue(TEXT("꺼낸다"), ColdPot->TakeBackInto(Inventory) > 0);
	TestEqual(TEXT("냄비가 비었다"), ColdPot->GetContents().Num(), 0);

	// 안내 글
	TestTrue(TEXT("냄비 안내에 레시피 이름"), UGrimInventoryWidget::DescribePot(Pot).Contains(TEXT("고기 스튜")));
	TestTrue(TEXT("픽업 안내에 아이템 이름"), UGrimInventoryWidget::DescribePickup(Far).Contains(TEXT("재칼로프 고기")));
	return true;
}

// ───── 가방 컴포넌트: 줍기 · 버리기 · 세이브 왕복 ─────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimItemsInventoryTest, "Grimoire.Items.Inventory", WorldFlags)
bool FGrimItemsInventoryTest::RunTest(const FString& Parameters)
{
	FItemTestWorld TestWorld;
	UGrimInventoryComponent* Inventory = TestWorld.MakeInventory();

	TestEqual(TEXT("모르는 아이템은 못 넣는다"), Inventory->AddItem(FGrimItemInstance(TEXT("NoSuchItem"), 1)), 0);
	TestEqual(TEXT("털 25(쌓기 20)"), Inventory->AddItem(FGrimItemInstance(TEXT("JackalopeFur"), 25)), 25);
	TestEqual(TEXT("두 칸에"), Inventory->GetSlots()[0].Count + Inventory->GetSlots()[1].Count, 25);
	TestTrue(TEXT("칸 옮기기"), Inventory->MoveItem(1, 5));
	TestEqual(TEXT("칸 5 = 5"), Inventory->GetSlots()[5].Count, 5);

	// 픽업을 줍는다(거리 검사 없이 — 폰이 없다)
	AGrimItemPickup* Pickup = UGrimItemStatics::SpawnPickup(TestWorld.World, TEXT("Herb"), 3, FVector(100.f, 0.f, 0.f));
	if (!TestNotNull(TEXT("픽업"), Pickup))
	{
		return false;
	}
	TestEqual(TEXT("셋을 주웠다"), Inventory->PickUp(Pickup, false), 3);
	TestTrue(TEXT("다 주우면 픽업이 사라진다"), !IsValid(Pickup) || Pickup->IsActorBeingDestroyed());
	TestEqual(TEXT("허브 3"), Inventory->CountOf(TEXT("Herb")), 3);
	TestEqual(TEXT("모르는 아이템은 픽업이 안 생긴다"), UGrimItemStatics::SpawnPickup(TestWorld.World, TEXT("NoSuchItem"), 1, FVector::ZeroVector), static_cast<AGrimItemPickup*>(nullptr));

	// 세이브 왕복
	FGrimItemInstance Worn(TEXT("Salt"), 2);
	Worn.Durability = 0.75f;
	Worn.Enchantments = { TEXT("Ench.Test") };
	Inventory->AddItem(Worn, false);
	TArray<uint8> Bytes;
	{
		FMemoryWriter Writer(Bytes, true);
		Inventory->WriteSaveData(Writer);
	}
	UGrimInventoryComponent* Loaded = TestWorld.MakeInventory();
	Loaded->AddItem(FGrimItemInstance(TEXT("Stew"), 1), false); // 읽으면 덮어쓴다
	{
		FMemoryReader Reader(Bytes, true);
		Loaded->ReadSaveData(Reader, Loaded->GetSaveVersion());
		TestFalse(TEXT("다 읽음"), Reader.IsError());
	}
	TestTrue(TEXT("칸 그대로"), Loaded->GetSlots() == Inventory->GetSlots());
	TestEqual(TEXT("스튜는 없다(덮어씀)"), Loaded->CountOf(TEXT("Stew")), 0);
	const FGrimItemInstance* LoadedWorn = Loaded->GetSlots().FindByPredicate([](const FGrimItemInstance& Slot) { return Slot.ItemId == FName(TEXT("Salt")); });
	TestTrue(TEXT("내구도 · 인챈트 자리도 저장된다(M4)"), LoadedWorn && LoadedWorn->Durability == 0.75f && LoadedWorn->Enchantments.Num() == 1);
	TestEqual(TEXT("SaveId"), Inventory->GetSaveId(), FName(TEXT("Items.Inventory")));
	TestTrue(TEXT("플레이어 몫"), Inventory->GetSaveScope() == EGrimSaveScope::Player);

	// 표에 없는 아이템 · 칸 수보다 많은 세이브
	TArray<uint8> Odd;
	{
		FMemoryWriter Writer(Odd, true);
		int32 Num = UGrimItemSettings::Get()->PlayerSlotCount + 1;
		Writer << Num;
		for (int32 Index = 0; Index < Num; ++Index)
		{
			FGrimItemInstance Slot;
			if (Index == 0)
			{
				Slot = FGrimItemInstance(TEXT("RemovedItem"), 1);
			}
			else if (Index == Num - 1)
			{
				Slot = FGrimItemInstance(TEXT("Grain"), 4); // 넘친 칸 — 빈 칸(0)에 들어간다
			}
			Slot.Serialize(Writer);
		}
	}
	UGrimInventoryComponent* OddInventory = TestWorld.MakeInventory();
	{
		FMemoryReader Reader(Odd, true);
		OddInventory->ReadSaveData(Reader, 1);
	}
	TestEqual(TEXT("표에 없는 것은 버린다"), OddInventory->CountOf(TEXT("RemovedItem")), 0);
	TestEqual(TEXT("넘친 칸은 빈 칸으로"), OddInventory->CountOf(TEXT("Grain")), 4);

	// 픽업 세이브
	AGrimItemPickup* Meat = UGrimItemStatics::SpawnPickup(TestWorld.World, TEXT("JackalopeMeat"), 2, FVector(4000.f, 0.f, 0.f));
	if (TestNotNull(TEXT("고기 픽업"), Meat))
	{
		FGrimElementalStimulus Fire;
		Fire.Element = GrimTags::Element_Fire;
		Fire.Intensity = 1.f;
		Meat->ApplyElementalStimulus(Fire, nullptr);
		Meat->UpdateCooking(1.f);
		TArray<uint8> PickupBytes;
		{
			FMemoryWriter Writer(PickupBytes, true);
			Meat->WriteSaveData(Writer);
		}
		AGrimItemPickup* Copy = UGrimItemStatics::SpawnPickup(TestWorld.World, TEXT("Herb"), 1, FVector(4500.f, 0.f, 0.f));
		FMemoryReader Reader(PickupBytes, true);
		Copy->ReadSaveData(Reader, 1);
		TestTrue(TEXT("픽업 아이템 · 요리 진행이 이어진다"), Copy->GetItem() == Meat->GetItem() && FMath::IsNearlyEqual(Copy->GetCookProgress(), Meat->GetCookProgress()));
	}
	return true;
}

#endif
