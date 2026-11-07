#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/GrimCreature.h"
#include "Creatures/GrimCreatureBox.h"
#include "Creatures/GrimCreatureMath.h"
#include "Creatures/GrimCreatureSettings.h"
#include "Creatures/GrimCreatureSpecies.h"
#include "Creatures/GrimCreatureStorageComponent.h"
#include "Creatures/GrimCreatureStorageMath.h"
#include "Creatures/GrimCreatureStorageSettings.h"
#include "Creatures/GrimCreatureStorageWidget.h"
#include "Creatures/GrimCreatureTamingComponent.h"
#include "Creatures/GrimTamerComponent.h"
#include "Creatures/GrimTamingMath.h"
#include "Creatures/GrimTamingTags.h"
#include "Core/GrimPlayerState.h"
#include "Factions/GrimFactionTypes.h"
#include "Factions/GrimShopSettings.h"
#include "Items/GrimInventoryComponent.h"
#include "Items/GrimItemSettings.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

namespace GrimCreatureStorageTest // 유니티 빌드 이름 충돌 방지
{
	constexpr EAutomationTestFlags StoragePureFlags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;
	constexpr EAutomationTestFlags StorageWorldFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	struct FStorageScopedWorld
	{
		UWorld* World = nullptr;

		FStorageScopedWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("GrimCreatureStorageTestWorld"));
			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
			World->BeginPlay();
			World->GetWorldSettings()->NotifyBeginPlay();
		}

		~FStorageScopedWorld()
		{
			World->EndPlay(EEndPlayReason::Quit);
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}
	};

	/** 길들이기 · 보관함 · 가방 컴포넌트가 붙은 플레이어(서클 Circle) */
	AGrimPlayerState* SpawnPlayer(UWorld* World, int32 Circle)
	{
		// 플레이어마다 다른 PlayerId(세이브 id 가 없으면 "PS-<번호>") — 시작(BeginPlay)에서 id 를 정하므로 그 전에 번호를 준다.
		static int32 NextPlayerNumber = 4100;
		AGrimPlayerState* PlayerState = World->SpawnActorDeferred<AGrimPlayerState>(AGrimPlayerState::StaticClass(), FTransform::Identity);
		if (!PlayerState)
		{
			return nullptr;
		}
		PlayerState->SetPlayerId(++NextPlayerNumber);
		PlayerState->FinishSpawning(FTransform::Identity);
		UAbilitySystemComponent* ASC = PlayerState->GetAbilitySystemComponent();
		ASC->InitAbilityActorInfo(PlayerState, PlayerState);
		if (!ASC->GetSet<UGrimMagicAttributeSet>())
		{
			ASC->AddSpawnedAttribute(PlayerState->GetMagicSet());
		}
		ASC->SetNumericAttributeBase(UGrimMagicAttributeSet::GetCircleAttribute(), static_cast<float>(Circle));
		return PlayerState;
	}

	FGrimCreatureInstance RollJackalope(int32 Seed)
	{
		const UGrimCreatureSpecies* Species = UGrimCreatureSettings::Get()->FindSpecies(TEXT("Jackalope"));
		return Species ? GrimCreatureMath::RollInstance(*Species, Seed, UGrimCreatureSettings::Get()->Roll) : FGrimCreatureInstance();
	}

	/** 길들인 재칼로프를 세계에 놓는다(주인 = 그 플레이어) */
	AGrimCreature* SpawnOwned(UWorld* World, AGrimPlayerState* PlayerState, int32 Seed)
	{
		UGrimCreatureSpecies* Species = UGrimCreatureSettings::Get()->FindSpecies(TEXT("Jackalope"));
		UGrimTamerComponent* Tamer = UGrimTamerComponent::FindForActor(PlayerState);
		AGrimCreature* Creature = Species && Tamer ? AGrimCreature::SpawnCreature(World, Species, RollJackalope(Seed), FVector(Seed % 7 * 200.f, 0.f, 0.f), 0.f) : nullptr;
		if (Creature && Creature->GetTaming())
		{
			Creature->GetTaming()->Claim(PlayerState, Tamer->GetPlayerId());
			Tamer->AddOrUpdateRecord(Creature->GetTaming()->MakeRecord());
		}
		return Creature;
	}

	/** 보관함 기록(세계에 없는 개체) */
	FGrimOwnedCreatureRecord MakeStoredRecord(const FString& OwnerId, int32 Seed)
	{
		FGrimOwnedCreatureRecord Record;
		Record.Instance = RollJackalope(Seed);
		Record.Instance.OwnerId = OwnerId;
		Record.Instance.Loyalty = 60.f;
		Record.HealthRatio = 0.5f;
		return Record;
	}

	bool SameInstance(const FGrimCreatureInstance& A, const FGrimCreatureInstance& B)
	{
		if (A.Colors.Num() != B.Colors.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < A.Colors.Num(); ++Index)
		{
			if (A.Colors[Index].PaletteIndex != B.Colors[Index].PaletteIndex || A.Colors[Index].bMutant != B.Colors[Index].bMutant || !A.Colors[Index].Color.Equals(B.Colors[Index].Color, 1e-5f))
			{
				return false;
			}
		}
		return A.SpeciesId == B.SpeciesId && A.Seed == B.Seed && A.Sex == B.Sex && FMath::IsNearlyEqual(A.SizeScale, B.SizeScale) && A.bGiant == B.bGiant
			&& A.bMutant == B.bMutant && A.Personality == B.Personality && FMath::IsNearlyEqual(A.StatScale, B.StatScale) && A.OwnerId == B.OwnerId
			&& FMath::IsNearlyEqual(A.Loyalty, B.Loyalty, 1e-3f) && A.Nickname == B.Nickname;
	}

	AGrimCreature* FindPartyBySeed(const UGrimCreatureStorageComponent* Storage, int32 Seed)
	{
		for (AGrimCreature* Creature : Storage->GetPartyCreatures())
		{
			if (Creature->GetInstance().Seed == Seed)
			{
				return Creature;
			}
		}
		return nullptr;
	}
}

using namespace GrimCreatureStorageTest;

// 상한: 서클 표 · 성장 노드 태그 · 상한 자르기, ini 의 제안값(3 · 4 · 5 / 8 · 12 · 16).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimCreatureStorageLimitsTest, "Grimoire.Creatures.Storage.Limits", StoragePureFlags)
bool FGrimCreatureStorageLimitsTest::RunTest(const FString& Parameters)
{
	auto Step = [](int32 MinCircle, int32 Value)
	{
		FGrimCircleLimitStep Row;
		Row.MinCircle = MinCircle;
		Row.Value = Value;
		return Row;
	};
	TArray<FGrimCircleLimitStep> Steps = { Step(5, 9), Step(1, 2), Step(3, 6) };
	TestEqual(TEXT("1서클 → 2"), GrimCreatureStorageMath::GetCircleValue(Steps, 1, -1), 2);
	TestEqual(TEXT("4서클 → 3서클 값 6(순서 무관)"), GrimCreatureStorageMath::GetCircleValue(Steps, 4, -1), 6);
	TestEqual(TEXT("10서클 → 9"), GrimCreatureStorageMath::GetCircleValue(Steps, 10, -1), 9);
	TestEqual(TEXT("맞는 구간 없음 → 기본"), GrimCreatureStorageMath::GetCircleValue(Steps, 0, -1), -1);

	TArray<FGrimTagLimitBonus> Bonuses;
	FGrimTagLimitBonus& Bonus = Bonuses.AddDefaulted_GetRef();
	Bonus.Tag = GrimTamingTags::Progression_Node_CreatureParty;
	Bonus.Bonus = 1;
	FGameplayTagContainer NoTags;
	FGameplayTagContainer WithTag;
	WithTag.AddTag(GrimTamingTags::Progression_Node_CreatureParty);
	TestEqual(TEXT("태그 없음"), GrimCreatureStorageMath::ComputeLimit(Steps, 3, Bonuses, NoTags, 99, 1), 6);
	TestEqual(TEXT("성장 노드 태그 +1(진행이 올리는 자리)"), GrimCreatureStorageMath::ComputeLimit(Steps, 3, Bonuses, WithTag, 99, 1), 7);
	TestEqual(TEXT("상한으로 자른다"), GrimCreatureStorageMath::ComputeLimit(Steps, 5, Bonuses, WithTag, 8, 1), 8);

	// ini 제안값(Q60 · Q61)
	const UGrimCreatureStorageSettings* Settings = UGrimCreatureStorageSettings::Get();
	auto Party = [Settings, &NoTags](int32 Circle) { return GrimCreatureStorageMath::ComputeLimit(Settings->PartyLimitByCircle, Circle, Settings->PartyLimitTagBonuses, NoTags, Settings->PartyLimitMax, 1); };
	auto Capacity = [Settings, &NoTags](int32 Circle, bool bLearned) { return GrimCreatureStorageMath::ComputeStorageCapacity(Settings->StorageBaseCapacity, bLearned, Settings->StorageCapacityByCircle, Circle, Settings->StorageCapacityTagBonuses, NoTags, Settings->StorageCapacityMax); };
	TestEqual(TEXT("ini: 1서클 데리고 다님 3"), Party(1), 3);
	TestEqual(TEXT("ini: 3서클 데리고 다님 4"), Party(3), 4);
	TestEqual(TEXT("ini: 5서클 데리고 다님 5(팰월드 파티)"), Party(5), 5);
	// Q42 답(2026-11-07): 쿠스토디아 없이 처음부터 10칸, 배우면 기존 설계 수치(8 · 12 · 16 + 노드 4)만큼 늘어난다.
	TestEqual(TEXT("ini: 안 배움 보관함 10(처음부터)"), Capacity(1, false), 10);
	TestEqual(TEXT("ini: 안 배움은 서클이 올라도 10"), Capacity(5, false), 10);
	TestEqual(TEXT("ini: 1서클 쿠스토디아 10 + 8"), Capacity(1, true), 18);
	TestEqual(TEXT("ini: 3서클 쿠스토디아 10 + 12"), Capacity(3, true), 22);
	TestEqual(TEXT("ini: 5서클 쿠스토디아 10 + 16"), Capacity(5, true), 26);
	FGameplayTagContainer StorageTag;
	StorageTag.AddTag(GrimTamingTags::Progression_Node_CreatureStorage);
	TestEqual(TEXT("ini: 보관함 성장 노드 +4(배웠을 때)"), GrimCreatureStorageMath::ComputeStorageCapacity(Settings->StorageBaseCapacity, true, Settings->StorageCapacityByCircle, 1, Settings->StorageCapacityTagBonuses, StorageTag, Settings->StorageCapacityMax), 22);
	TestEqual(TEXT("ini: 노드도 배워야 붙는다"), GrimCreatureStorageMath::ComputeStorageCapacity(Settings->StorageBaseCapacity, false, Settings->StorageCapacityByCircle, 1, Settings->StorageCapacityTagBonuses, StorageTag, Settings->StorageCapacityMax), 10);
	TestEqual(TEXT("상한으로 자른다(기본 칸은 넘김)"), GrimCreatureStorageMath::ComputeStorageCapacity(10, true, Settings->StorageCapacityByCircle, 5, Settings->StorageCapacityTagBonuses, StorageTag, 20), 20);
	TestEqual(TEXT("상한이 기본 칸보다 작아도 기본 칸"), GrimCreatureStorageMath::ComputeStorageCapacity(10, false, Settings->StorageCapacityByCircle, 1, Settings->StorageCapacityTagBonuses, NoTags, 5), 10);
	TestEqual(TEXT("ini: 상자 60칸"), Settings->BoxCapacity, 60);
	TestEqual(TEXT("ini: 풀어놓기 10마리"), Settings->BoxMaxReleased, 10);
	TestEqual(TEXT("ini: 노말 스킬 ID"), Settings->StorageSkillId, FName(TEXT("CreatureStorage")));
	return true;
}

// 옮기기 판정 · 새로 길들일 때 갈 곳 · 보관 중 회복 · 노말 스킬 · 상점 서비스 이름.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimCreatureStorageRulesTest, "Grimoire.Creatures.Storage.Rules", StoragePureFlags)
bool FGrimCreatureStorageRulesTest::RunTest(const FString& Parameters)
{
	using EP = EGrimCreaturePlace;
	using ER = EGrimCreatureStorageResult;
	FGrimCreatureStorageCounts C;
	C.PartyCount = 2;
	C.PartyLimit = 3;
	C.bStorageLearned = true;
	C.StorageCount = 7;
	C.StorageCapacity = 8;
	C.bHasBox = true;
	C.BoxCount = 59;
	C.BoxCapacity = 60;
	C.ReleasedCount = 9;
	C.ReleasedMax = 10;
	TestEqual(TEXT("파티 자리 있음 → 꺼내기"), GrimCreatureStorageMath::CheckTransfer(EP::Storage, EP::Party, C), ER::Ok);
	TestEqual(TEXT("보관함 자리 있음 → 넣기"), GrimCreatureStorageMath::CheckTransfer(EP::Party, EP::Storage, C), ER::Ok);
	TestEqual(TEXT("같은 곳"), GrimCreatureStorageMath::CheckTransfer(EP::Party, EP::Party, C), ER::Invalid);

	FGrimCreatureStorageCounts Full = C;
	Full.PartyCount = 3;
	Full.StorageCount = 8;
	Full.BoxCount = 60;
	Full.ReleasedCount = 10;
	TestEqual(TEXT("데리고 다니는 수 가득"), GrimCreatureStorageMath::CheckTransfer(EP::Storage, EP::Party, Full), ER::PartyFull);
	TestEqual(TEXT("보관함 가득"), GrimCreatureStorageMath::CheckTransfer(EP::Party, EP::Storage, Full), ER::StorageFull);
	TestEqual(TEXT("상자 가득"), GrimCreatureStorageMath::CheckTransfer(EP::Party, EP::Box, Full), ER::BoxFull);
	TestEqual(TEXT("풀어놓은 것을 거두는 건 칸이 그대로라 된다"), GrimCreatureStorageMath::CheckTransfer(EP::Released, EP::Box, Full), ER::Ok);
	TestEqual(TEXT("상자 안 → 풀어놓기 상한"), GrimCreatureStorageMath::CheckTransfer(EP::Box, EP::Released, Full), ER::ReleasedFull);
	FGrimCreatureStorageCounts NotLearned = C;
	NotLearned.bStorageLearned = false;
	NotLearned.StorageCapacity = 10;
	TestEqual(TEXT("안 배워도 기본 칸에 넣는다(Q42)"), GrimCreatureStorageMath::CheckTransfer(EP::Party, EP::Storage, NotLearned), ER::Ok);
	NotLearned.StorageCount = 10;
	TestEqual(TEXT("기본 칸 가득 → 쿠스토디아 안내"), GrimCreatureStorageMath::CheckTransfer(EP::Party, EP::Storage, NotLearned), ER::StorageFullUnlearned);
	FGrimCreatureStorageCounts NoBox = C;
	NoBox.bHasBox = false;
	TestEqual(TEXT("상자 곁이 아님"), GrimCreatureStorageMath::CheckTransfer(EP::Storage, EP::Box, NoBox), ER::NoBox);
	TestEqual(TEXT("상자 곁이 아님(풀어놓은 것)"), GrimCreatureStorageMath::CheckTransfer(EP::Released, EP::Party, NoBox), ER::NoBox);

	// 새로 길들임: 파티 → 보관함 → 상자 → 거절
	TestEqual(TEXT("파티 자리 → 곁에"), GrimCreatureStorageMath::DecideNewTame(C, true), EGrimNewTameDestination::Party);
	FGrimCreatureStorageCounts PartyFull = C;
	PartyFull.PartyCount = 3;
	TestEqual(TEXT("파티 가득 → 보관함"), GrimCreatureStorageMath::DecideNewTame(PartyFull, true), EGrimNewTameDestination::Storage);
	PartyFull.StorageCount = 8;
	TestEqual(TEXT("보관함도 가득 → 상자(팰월드처럼)"), GrimCreatureStorageMath::DecideNewTame(PartyFull, true), EGrimNewTameDestination::Box);
	TestEqual(TEXT("상자로 보내기 끔 → 거절"), GrimCreatureStorageMath::DecideNewTame(PartyFull, false), EGrimNewTameDestination::Refused);
	PartyFull.BoxCount = 60;
	TestEqual(TEXT("모두 가득 → 거절"), GrimCreatureStorageMath::DecideNewTame(PartyFull, true), EGrimNewTameDestination::Refused);
	FGrimCreatureStorageCounts PartyFullNotLearned = PartyFull;
	PartyFullNotLearned.bStorageLearned = false;
	PartyFullNotLearned.StorageCount = 0;
	PartyFullNotLearned.StorageCapacity = 10;
	PartyFullNotLearned.BoxCount = 0;
	TestEqual(TEXT("안 배워도 기본 칸 → 보관함(Q42)"), GrimCreatureStorageMath::DecideNewTame(PartyFullNotLearned, true), EGrimNewTameDestination::Storage);
	PartyFullNotLearned.StorageCount = 10;
	TestEqual(TEXT("기본 칸 가득 → 상자(Q64 순서)"), GrimCreatureStorageMath::DecideNewTame(PartyFullNotLearned, true), EGrimNewTameDestination::Box);
	PartyFullNotLearned.BoxCount = 60;
	TestEqual(TEXT("기본 칸 · 상자 가득 → 거절"), GrimCreatureStorageMath::DecideNewTame(PartyFullNotLearned, true), EGrimNewTameDestination::Refused);

	// 보관 중 회복
	TestEqual(TEXT("2분 × 분당 0.2 = +0.4"), GrimCreatureStorageMath::GetHealedRatio(0.4f, 1000.0, 1120.0, 0.2f), 0.8f, 1e-4f);
	TestEqual(TEXT("최대 1"), GrimCreatureStorageMath::GetHealedRatio(0.9f, 1000.0, 9000.0, 0.2f), 1.f, 1e-4f);
	TestEqual(TEXT("넣은 때 없음 → 그대로"), GrimCreatureStorageMath::GetHealedRatio(0.4f, 0.0, 9000.0, 0.2f), 0.4f, 1e-4f);

	// 노말 스킬 · 상점
	FName Skill;
	TestTrue(TEXT("Skill:CreatureStorage → 노말 스킬"), GrimCreatureStorageMath::ParseNormalSkill(TEXT("Skill:CreatureStorage"), Skill) && Skill == FName(TEXT("CreatureStorage")));
	TestFalse(TEXT("마법 ID 는 노말 스킬이 아니다"), GrimCreatureStorageMath::ParseNormalSkill(TEXT("Pactum"), Skill));
	TestFalse(TEXT("빈 ID"), GrimCreatureStorageMath::ParseNormalSkill(TEXT("Skill:"), Skill));
	FName Item;
	TestTrue(TEXT("Buy:<아이템>"), GrimTowerServices::ParseBuy(TEXT("Buy:Skillbook_CreatureStorage"), Item) && Item == FName(TEXT("Skillbook_CreatureStorage")));
	TestTrue(TEXT("상점 서비스는 아는 서비스"), GrimTowerServices::IsKnown(TEXT("Buy:Skillbook_CreatureStorage")));
	EGrimCreaturePlace Place;
	TestTrue(TEXT("콘솔 자리 이름"), GrimCreatureStorageMath::ParsePlace(TEXT("released"), Place) && Place == EP::Released);

	// 상점이 있는 NPC 는 Buy 대사도 있어야 한다.
	FGrimDialogueDatabase Dialogue;
	TArray<FString> Problems;
	FGrimDialogueDatabase::BuildFromCSV(TEXT("Name,DisplayName,Title,Services,ServiceLabels\nShop,\"상인\",\"\",\"Buy:Solidus\",\"솔\"\n"),
		TEXT("Name,Npc,Context,Quest,Step,Priority,Lines\nShop_Talk,Shop,Talk,None,-1,0,\"안녕\"\n"), nullptr, Dialogue, Problems);
	TestTrue(TEXT("Buy.NoMoney 대사 없음을 잡는다"), Problems.ContainsByPredicate([](const FString& P) { return P.Contains(TEXT("Buy.NoMoney")); }));

	// 위젯 단추
	TestEqual(TEXT("가방 탭: 데리고 다님 → 보관함만"), UGrimCreatureStorageWidget::GetTargets(EP::Party, false).Num(), 1);
	TestEqual(TEXT("상자 화면: 데리고 다님 → 보관함 · 상자 · 풀어놓기"), UGrimCreatureStorageWidget::GetTargets(EP::Party, true).Num(), 3);
	return true;
}

// 상자 칸 세이브 형식 왕복 · 깨진 데이터.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimCreatureStorageSerializeTest, "Grimoire.Creatures.Storage.Serialize", StoragePureFlags)
bool FGrimCreatureStorageSerializeTest::RunTest(const FString& Parameters)
{
	TArray<FGrimCreatureBoxEntry> Entries;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		FGrimCreatureBoxEntry& Entry = Entries.AddDefaulted_GetRef();
		Entry.Record.Instance.SpeciesId = TEXT("Kelpie");
		Entry.Record.Instance.Seed = 100 + Index;
		Entry.Record.Instance.OwnerId = TEXT("host-abc");
		Entry.Record.Instance.Nickname = FString::Printf(TEXT("물결%d"), Index);
		Entry.Record.Instance.Loyalty = 40.f + Index;
		Entry.Record.HealthRatio = 0.3f;
		Entry.Record.Command = Index == 1 ? EGrimTamedCommand::Roam : EGrimTamedCommand::Follow;
		Entry.Record.SavedUtcSeconds = 1.7e9 + Index;
		Entry.bReleased = Index == 1;
	}
	TArray<uint8> Bytes;
	FMemoryWriter Writer(Bytes, true);
	GrimCreatureStorageMath::SerializeBoxEntries(Writer, Entries);
	TArray<FGrimCreatureBoxEntry> Loaded;
	FMemoryReader Reader(Bytes, true);
	TestTrue(TEXT("읽기"), GrimCreatureStorageMath::SerializeBoxEntries(Reader, Loaded));
	if (TestEqual(TEXT("칸 수"), Loaded.Num(), 3))
	{
		TestTrue(TEXT("풀어놓음"), Loaded[1].bReleased && !Loaded[0].bReleased);
		TestEqual(TEXT("명령 Roam(풀어놓음)이 남는다"), Loaded[1].Record.Command, EGrimTamedCommand::Roam);
		TestEqual(TEXT("이름"), Loaded[2].Record.Instance.Nickname, FString(TEXT("물결2")));
		TestEqual(TEXT("넣은 때"), Loaded[2].Record.SavedUtcSeconds, 1.7e9 + 2, 1e-3);
	}
	TArray<uint8> Broken;
	FMemoryWriter BrokenWriter(Broken, true);
	int32 Huge = 99999;
	BrokenWriter << Huge;
	TArray<FGrimCreatureBoxEntry> BrokenLoaded;
	FMemoryReader BrokenReader(Broken, true);
	TestFalse(TEXT("너무 큰 수는 거부"), GrimCreatureStorageMath::SerializeBoxEntries(BrokenReader, BrokenLoaded));
	return true;
}

// 실제 액터: 스킬북으로 배움 → 데리고 다니는 수 → 넣기 · 꺼내기(개체 기록 그대로) → 가득 → 거점 상자 · 풀어놓기 · 거두기 → 세이브 왕복.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimCreatureStorageActorTest, "Grimoire.Creatures.Storage.Actor", StorageWorldFlags)
bool FGrimCreatureStorageActorTest::RunTest(const FString& Parameters)
{
	using EP = EGrimCreaturePlace;
	using ER = EGrimCreatureStorageResult;
	if (!TestNotNull(TEXT("재칼로프 에셋"), UGrimCreatureSettings::Get()->FindSpecies(TEXT("Jackalope"))))
	{
		return false;
	}
	FStorageScopedWorld TestWorld;
	UWorld* World = TestWorld.World;
	AGrimPlayerState* Player = SpawnPlayer(World, 1);
	UGrimCreatureStorageComponent* Storage = UGrimCreatureStorageComponent::FindForActor(Player);
	UGrimTamerComponent* Tamer = UGrimTamerComponent::FindForActor(Player);
	UGrimInventoryComponent* Inventory = UGrimInventoryComponent::FindForActor(Player);
	if (!TestNotNull(TEXT("보관함 컴포넌트(서브시스템이 붙임)"), Storage) || !TestNotNull(TEXT("길들이기 컴포넌트"), Tamer) || !TestNotNull(TEXT("가방"), Inventory))
	{
		return false;
	}
	const FString OwnerId = Tamer->GetPlayerId();
	Storage->RefreshLimits();
	const int32 Limit = Storage->GetPartyLimit();
	TestEqual(TEXT("1서클 데리고 다니는 수(ini)"), Limit, 3);

	// ── 스킬북으로 배운다(노말 스킬) ──
	TestFalse(TEXT("처음엔 쿠스토디아를 모름"), Storage->IsStorageLearned());
	TestEqual(TEXT("안 배워도 보관함 10칸(Q42)"), Storage->GetStorageCapacity(), 10);
	const FName Book(TEXT("Skillbook_CreatureStorage"));
	TestEqual(TEXT("스킬북을 가방에"), Inventory->AddItem(FGrimItemInstance(Book, 2), false), 2);
	const int32 BookSlot = Inventory->GetSlots().IndexOfByPredicate([Book](const FGrimItemInstance& Slot) { return Slot.ItemId == Book; });
	TestEqual(TEXT("읽으면 배운다"), Inventory->UseItem(BookSlot), EGrimItemUseResult::Learned);
	TestTrue(TEXT("배웠다"), Storage->IsStorageLearned());
	TestEqual(TEXT("책 하나 소모"), Inventory->CountOf(Book), 1);
	const int32 BookSlot2 = Inventory->GetSlots().IndexOfByPredicate([Book](const FGrimItemInstance& Slot) { return Slot.ItemId == Book; });
	TestEqual(TEXT("이미 배움 → 못 씀"), Inventory->UseItem(BookSlot2), EGrimItemUseResult::CannotUse);
	TestEqual(TEXT("못 쓴 책은 남는다"), Inventory->CountOf(Book), 1);
	const int32 Capacity = Storage->GetStorageCapacity();
	TestEqual(TEXT("1서클 쿠스토디아 보관함 칸 10 + 8(ini)"), Capacity, 18);

	// ── 데리고 다니는 수 ──
	TArray<AGrimCreature*> Party;
	for (int32 Index = 0; Index < Limit; ++Index)
	{
		Party.Add(SpawnOwned(World, Player, 7001 + Index));
	}
	TestEqual(TEXT("데리고 다님 = 상한"), Storage->GetPartyCount(), Limit);
	TestEqual(TEXT("가득 → 새로 길들이면 보관함"), Storage->DecideNewTame(), EGrimNewTameDestination::Storage);
	AGrimCreature* Extra = SpawnOwned(World, Player, 7100);
	Storage->PlaceNewlyTamed(Extra, EGrimNewTameDestination::Storage, false);
	TestTrue(TEXT("넘친 생물은 세계에서 사라진다"), !IsValid(Extra) || Extra->IsActorBeingDestroyed());
	TestEqual(TEXT("데리고 다님은 상한 그대로"), Storage->GetPartyCount(), Limit);
	TestEqual(TEXT("보관함 1"), Storage->GetStored().Num(), 1);
	TestFalse(TEXT("주인 기록(Creatures.Owned)에서 뺐다"), Tamer->GetRecords().ContainsByPredicate([](const FGrimOwnedCreatureRecord& R) { return R.Instance.Seed == 7100; }));

	// ── 넣기 · 꺼내기: 개체 기록 그대로 ──
	AGrimCreature* Star = Party[0];
	Star->GetTaming()->SetNickname(TEXT("보름이"));
	Star->GetTaming()->DebugSetLoyalty(77.f);
	Star->SetHealthRatioServer(0.4f);
	const FGrimCreatureInstance Before = Star->GetInstance();
	TestEqual(TEXT("넣기"), Storage->Transfer(nullptr, Before.SpeciesId, Before.Seed, EP::Party, EP::Storage, false), ER::Ok);
	TestTrue(TEXT("넣은 생물은 세계에서 사라진다"), !IsValid(Star) || Star->IsActorBeingDestroyed());
	const int32 StoredIndex = GrimCreatureStorageMath::FindRecord(Storage->GetStored(), Before.SpeciesId, Before.Seed);
	if (TestTrue(TEXT("보관함에 있다"), StoredIndex != INDEX_NONE))
	{
		const FGrimOwnedCreatureRecord& Stored = Storage->GetStored()[StoredIndex];
		TestTrue(TEXT("개체 기록 그대로(성별 · 색 · 크기 · 성격 · 이름 · 충성도 · 주인)"), SameInstance(Stored.Instance, Before));
		TestEqual(TEXT("체력 비율"), Stored.HealthRatio, 0.4f, 0.02f);
		TestTrue(TEXT("넣은 때를 적었다"), Stored.SavedUtcSeconds > 0.0);
	}
	TestEqual(TEXT("데리고 다님 −1"), Storage->GetPartyCount(), Limit - 1);
	TestFalse(TEXT("주인 기록에서 뺐다"), Tamer->GetRecords().ContainsByPredicate([&Before](const FGrimOwnedCreatureRecord& R) { return R.IsSameCreature(Before); }));

	TestEqual(TEXT("꺼내기"), Storage->Transfer(nullptr, Before.SpeciesId, Before.Seed, EP::Storage, EP::Party, false), ER::Ok);
	AGrimCreature* Back = FindPartyBySeed(Storage, Before.Seed);
	if (TestNotNull(TEXT("다시 세계에"), Back))
	{
		TestTrue(TEXT("꺼낸 개체 기록 그대로"), SameInstance(Back->GetInstance(), Before));
		TestTrue(TEXT("따라오기"), Back->GetTaming()->GetCommand() == EGrimTamedCommand::Follow && !Back->GetTaming()->IsAtBase());
		TestTrue(TEXT("주인 PlayerState"), Back->GetTaming()->GetOwnerPlayerState() == Player);
		TestTrue(TEXT("체력은 줄지 않는다(보관 중 회복)"), Back->GetHealthRatio() >= 0.39f);
	}
	TestTrue(TEXT("주인 기록에 돌아왔다"), Tamer->GetRecords().ContainsByPredicate([&Before](const FGrimOwnedCreatureRecord& R) { return R.IsSameCreature(Before); }));

	// ── 가득 ──
	int32 FakeSeed = 9000;
	while (Storage->GetStored().Num() < Capacity && Storage->AcceptOverflowRecord(MakeStoredRecord(OwnerId, FakeSeed)))
	{
		++FakeSeed;
	}
	TestEqual(TEXT("보관함을 칸까지 채움"), Storage->GetStored().Num(), Capacity);
	TestFalse(TEXT("가득이면 넘침을 받지 않는다"), Storage->AcceptOverflowRecord(MakeStoredRecord(OwnerId, 9999)));
	TestEqual(TEXT("보관함 가득 → 넣기 거절"), Storage->Transfer(nullptr, Party[1]->GetInstance().SpeciesId, Party[1]->GetInstance().Seed, EP::Party, EP::Storage, false), ER::StorageFull);
	TestEqual(TEXT("거절되면 세계에 그대로"), Storage->GetPartyCount(), Limit);
	TestEqual(TEXT("데리고 다니는 수 가득 → 꺼내기 거절"), Storage->Transfer(nullptr, TEXT("Jackalope"), 9000, EP::Storage, EP::Party, false), ER::PartyFull);
	TestEqual(TEXT("상자 없음 · 모두 가득 → 계약 거절"), Storage->DecideNewTame(), EGrimNewTameDestination::Refused);
	TestEqual(TEXT("상자 없이 상자로 → NoBox"), Storage->Transfer(nullptr, TEXT("Jackalope"), 9000, EP::Storage, EP::Box, false), ER::NoBox);

	// ── 거점 생물 상자 ──
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGrimCreatureBox* Box = World->SpawnActor<AGrimCreatureBox>(AGrimCreatureBox::StaticClass(), FVector(3000.f, 0.f, 0.f), FRotator::ZeroRotator, Params);
	if (!TestNotNull(TEXT("상자"), Box))
	{
		return false;
	}
	TestEqual(TEXT("상자가 있으면 넘친 계약은 상자로"), Storage->DecideNewTame(), EGrimNewTameDestination::Box);
	TestEqual(TEXT("보관함 → 상자"), Storage->Transfer(Box, TEXT("Jackalope"), 9000, EP::Storage, EP::Box, false), ER::Ok);
	TestEqual(TEXT("보관함 −1"), Storage->GetStored().Num(), Capacity - 1);
	TestEqual(TEXT("상자 1"), Box->GetEntries().Num(), 1);
	const FGrimCreatureInstance Roamer = Party[1]->GetInstance();
	TestEqual(TEXT("데리고 다님 → 상자"), Storage->Transfer(Box, Roamer.SpeciesId, Roamer.Seed, EP::Party, EP::Box, false), ER::Ok);
	TestEqual(TEXT("데리고 다님 −1"), Storage->GetPartyCount(), Limit - 1);
	TestEqual(TEXT("풀어놓기"), Storage->Transfer(Box, Roamer.SpeciesId, Roamer.Seed, EP::Box, EP::Released, false), ER::Ok);
	AGrimCreature* Released = Box->FindReleasedActor(Roamer.SpeciesId, Roamer.Seed);
	if (TestNotNull(TEXT("풀어놓은 생물이 세계에"), Released))
	{
		TestTrue(TEXT("집 = 상자"), Released->GetTaming()->GetHomeBox() == Box && Released->GetTaming()->IsAtBase());
		TestEqual(TEXT("명령 Roam"), Released->GetTaming()->GetCommand(), EGrimTamedCommand::Roam);
		TestTrue(TEXT("개체 기록 그대로"), SameInstance(Released->GetInstance(), Roamer));
		TestTrue(TEXT("풀어놓은 곳은 상자 반경 안"), FVector::Dist2D(Released->GetActorLocation(), Box->GetActorLocation()) <= Box->GetRoamRadius() + 1.f);
	}
	TestEqual(TEXT("풀어놓은 생물은 데리고 다니는 수에 들지 않는다"), Storage->GetPartyCount(), Limit - 1);
	TestEqual(TEXT("상자 풀어놓음 1"), Box->GetReleasedCount(), 1);
	TestEqual(TEXT("상자 → 데리고 다님(자리 있음)"), Storage->Transfer(Box, TEXT("Jackalope"), 9000, EP::Box, EP::Party, false), ER::Ok);
	TestEqual(TEXT("데리고 다님 가득"), Storage->GetPartyCount(), Limit);
	TestEqual(TEXT("풀어놓은 것 → 데리고 다님: 가득이라 거절"), Storage->Transfer(Box, Roamer.SpeciesId, Roamer.Seed, EP::Released, EP::Party, false), ER::PartyFull);
	TestEqual(TEXT("거두기"), Storage->Transfer(Box, Roamer.SpeciesId, Roamer.Seed, EP::Released, EP::Box, false), ER::Ok);
	TestNull(TEXT("거둔 생물은 세계에 없다"), Box->FindReleasedActor(Roamer.SpeciesId, Roamer.Seed));
	TestEqual(TEXT("상자 풀어놓음 0"), Box->GetReleasedCount(), 0);
	TestEqual(TEXT("남의 칸은 못 옮긴다"), UGrimCreatureStorageComponent::FindForActor(SpawnPlayer(World, 1))->Transfer(Box, Roamer.SpeciesId, Roamer.Seed, EP::Box, EP::Storage, false), ER::NotFound);

	// 상자 가득
	int32 BoxSeed = 20000;
	while (Box->HasRoom() && Box->AddEntry(MakeStoredRecord(OwnerId, BoxSeed), false))
	{
		++BoxSeed;
	}
	TestEqual(TEXT("상자 칸까지"), Box->GetEntries().Num(), Box->GetCapacity());
	TestEqual(TEXT("상자 가득 → 거절"), Storage->Transfer(Box, Party[2]->GetInstance().SpeciesId, Party[2]->GetInstance().Seed, EP::Party, EP::Box, false), ER::BoxFull);

	// ── 세이브 왕복 ──
	TArray<uint8> StorageBytes;
	FMemoryWriter StorageWriter(StorageBytes, true);
	Storage->WriteSaveData(StorageWriter);
	AGrimPlayerState* Reloaded = SpawnPlayer(World, 1);
	UGrimCreatureStorageComponent* ReloadedStorage = UGrimCreatureStorageComponent::FindForActor(Reloaded);
	FMemoryReader StorageReader(StorageBytes, true);
	ReloadedStorage->ReadSaveData(StorageReader, ReloadedStorage->GetSaveVersion());
	TestTrue(TEXT("세이브: 배움"), ReloadedStorage->IsStorageLearned());
	if (TestEqual(TEXT("세이브: 보관함 기록 수"), ReloadedStorage->GetStored().Num(), Storage->GetStored().Num()))
	{
		TestTrue(TEXT("세이브: 개체 기록 그대로"), SameInstance(ReloadedStorage->GetStored()[0].Instance, Storage->GetStored()[0].Instance));
	}

	TArray<uint8> BoxBytes;
	FMemoryWriter BoxWriter(BoxBytes, true);
	Box->WriteSaveData(BoxWriter);
	AGrimCreatureBox* Box2 = World->SpawnActor<AGrimCreatureBox>(AGrimCreatureBox::StaticClass(), FVector(-3000.f, 0.f, 0.f), FRotator::ZeroRotator, Params);
	FMemoryReader BoxReader(BoxBytes, true);
	Box2->ReadSaveData(BoxReader, Box2->GetSaveVersion());
	TestEqual(TEXT("세이브: 상자 칸 수"), Box2->GetEntries().Num(), Box->GetEntries().Num());
	const int32 RoamerIndex = GrimCreatureStorageMath::FindBoxEntry(Box2->GetEntries(), Roamer.SpeciesId, Roamer.Seed);
	if (TestTrue(TEXT("세이브: 상자에 그 개체"), RoamerIndex != INDEX_NONE))
	{
		TestTrue(TEXT("세이브: 상자 개체 기록 그대로"), SameInstance(Box2->GetEntries()[RoamerIndex].Record.Instance, Roamer));
	}
	return true;
}

// 풀어놓은 생물 세이브: 상자를 다시 불러오면 풀어놓은 생물이 다시 나온다.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimCreatureStorageReleasedSaveTest, "Grimoire.Creatures.Storage.ReleasedSave", StorageWorldFlags)
bool FGrimCreatureStorageReleasedSaveTest::RunTest(const FString& Parameters)
{
	using EP = EGrimCreaturePlace;
	FStorageScopedWorld TestWorld;
	UWorld* World = TestWorld.World;
	AGrimPlayerState* Player = SpawnPlayer(World, 1);
	UGrimCreatureStorageComponent* Storage = UGrimCreatureStorageComponent::FindForActor(Player);
	if (!TestNotNull(TEXT("보관함"), Storage))
	{
		return false;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGrimCreatureBox* Box = World->SpawnActor<AGrimCreatureBox>(AGrimCreatureBox::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
	AGrimCreature* Creature = SpawnOwned(World, Player, 5150);
	const FGrimCreatureInstance Instance = Creature->GetInstance();
	TestEqual(TEXT("데리고 다님 → 풀어놓기 한 번에"), Storage->Transfer(Box, Instance.SpeciesId, Instance.Seed, EP::Party, EP::Released, false), EGrimCreatureStorageResult::Ok);
	AGrimCreature* Released = Box->FindReleasedActor(Instance.SpeciesId, Instance.Seed);
	if (!TestNotNull(TEXT("풀어놓음"), Released))
	{
		return false;
	}
	Released->GetTaming()->DebugSetLoyalty(66.f);
	TArray<uint8> Bytes;
	FMemoryWriter Writer(Bytes, true);
	Box->WriteSaveData(Writer);
	// 세계가 다시 열린 셈: 액터를 거두고 같은 상자에 다시 읽힌다.
	Released->Destroy();
	FMemoryReader Reader(Bytes, true);
	Box->ReadSaveData(Reader, Box->GetSaveVersion());
	AGrimCreature* Again = Box->FindReleasedActor(Instance.SpeciesId, Instance.Seed);
	if (TestNotNull(TEXT("불러오면 다시 풀어놓는다"), Again))
	{
		TestEqual(TEXT("풀어놓은 동안의 충성도가 남는다"), Again->GetTaming()->GetLoyalty(), 66.f, 1e-3f);
		TestTrue(TEXT("집 = 상자"), Again->GetTaming()->GetHomeBox() == Box);
	}
	// 놓아주면(야생으로) 상자 칸에서도 지운다.
	if (Again)
	{
		Again->GetTaming()->ReleaseToWild(false);
		TestEqual(TEXT("놓아주면 상자 칸에서 지운다"), Box->GetEntries().Num(), 0);
	}
	return true;
}

// 상점: 솔이 모자람 → 산다(솔 −값, 책 +1) → 팔지 않는 물건.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimCreatureStorageShopTest, "Grimoire.Creatures.Storage.Shop", StorageWorldFlags)
bool FGrimCreatureStorageShopTest::RunTest(const FString& Parameters)
{
	FStorageScopedWorld TestWorld;
	AGrimPlayerState* Player = SpawnPlayer(TestWorld.World, 1);
	UGrimInventoryComponent* Inventory = UGrimInventoryComponent::FindForActor(Player);
	if (!TestNotNull(TEXT("가방"), Inventory))
	{
		return false;
	}
	const UGrimShopSettings* Shop = UGrimShopSettings::Get();
	const FName Book(TEXT("Skillbook_CreatureStorage"));
	const int32 Price = Shop->FindPrice(Book);
	TestEqual(TEXT("ini: 쿠스토디아 스킬북 120 솔(제안 — Q42)"), Price, 120);
	TestEqual(TEXT("ini: 화폐 = 솔리두스"), Shop->CurrencyItem, FName(TEXT("Solidus")));
	TestNotNull(TEXT("아이템 표: 솔리두스"), UGrimItemSettings::GetDatabase().Find(Shop->CurrencyItem));
	TestEqual(TEXT("솔 없음"), GrimTowerShop::TryBuy(Inventory, Book), GrimDialogueContexts::BuyNoMoney);
	Inventory->AddItem(FGrimItemInstance(Shop->CurrencyItem, Price + 50), false);
	TestEqual(TEXT("산다"), GrimTowerShop::TryBuy(Inventory, Book), GrimDialogueContexts::Buy);
	TestEqual(TEXT("솔 − 값"), Inventory->CountOf(Shop->CurrencyItem), 50);
	TestEqual(TEXT("책이 가방에"), Inventory->CountOf(Book), 1);
	TestEqual(TEXT("팔지 않는 물건"), GrimTowerShop::TryBuy(Inventory, TEXT("Herb")), GrimDialogueContexts::Refused);

	// Q71: 기본 완드는 아주 싼 값에 늘 판다(여러 번 사도 된다).
	const FName BasicWand(TEXT("Wand_Basic"));
	TestEqual(TEXT("ini: 기본 완드 10 솔(제안 — Q71)"), Shop->FindPrice(BasicWand), 10);
	TestEqual(TEXT("기본 완드 산다"), GrimTowerShop::TryBuy(Inventory, BasicWand), GrimDialogueContexts::Buy);
	TestEqual(TEXT("또 산다(늘 판다)"), GrimTowerShop::TryBuy(Inventory, BasicWand), GrimDialogueContexts::Buy);
	TestEqual(TEXT("완드 둘"), Inventory->CountOf(BasicWand), 2);
	TestEqual(TEXT("솔 50 − 20"), Inventory->CountOf(Shop->CurrencyItem), 30);
	return true;
}

// Q42 답: 쿠스토디아 없이 처음부터 10칸 — 가득이면 보관함 → 거점 상자 → 거절(Q64), 배우면 칸이 늘어난다.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimCreatureStorageBaseCapacityTest, "Grimoire.Creatures.Storage.BaseCapacity", StorageWorldFlags)
bool FGrimCreatureStorageBaseCapacityTest::RunTest(const FString& Parameters)
{
	using EP = EGrimCreaturePlace;
	using ER = EGrimCreatureStorageResult;
	if (!TestNotNull(TEXT("재칼로프 에셋"), UGrimCreatureSettings::Get()->FindSpecies(TEXT("Jackalope"))))
	{
		return false;
	}
	FStorageScopedWorld TestWorld;
	UWorld* World = TestWorld.World;
	AGrimPlayerState* Player = SpawnPlayer(World, 1);
	UGrimCreatureStorageComponent* Storage = UGrimCreatureStorageComponent::FindForActor(Player);
	UGrimTamerComponent* Tamer = UGrimTamerComponent::FindForActor(Player);
	if (!TestNotNull(TEXT("보관함 컴포넌트"), Storage) || !TestNotNull(TEXT("길들이기 컴포넌트"), Tamer))
	{
		return false;
	}
	const FString OwnerId = Tamer->GetPlayerId();
	Storage->RefreshLimits();
	TestFalse(TEXT("안 배움"), Storage->IsStorageLearned());
	TestEqual(TEXT("기본 10칸"), Storage->GetStorageCapacity(), 10);

	TArray<AGrimCreature*> Party;
	for (int32 Index = 0; Index < Storage->GetPartyLimit(); ++Index)
	{
		Party.Add(SpawnOwned(World, Player, 8001 + Index));
	}
	TestEqual(TEXT("데리고 다님 가득 → 안 배워도 보관함"), Storage->DecideNewTame(), EGrimNewTameDestination::Storage);
	const FGrimCreatureInstance First = Party[0]->GetInstance();
	TestEqual(TEXT("안 배워도 넣기(인벤토리처럼)"), Storage->Transfer(nullptr, First.SpeciesId, First.Seed, EP::Party, EP::Storage, false), ER::Ok);
	TestEqual(TEXT("꺼내기"), Storage->Transfer(nullptr, First.SpeciesId, First.Seed, EP::Storage, EP::Party, false), ER::Ok);

	int32 Seed = 8100;
	while (Storage->GetStored().Num() < 10 && Storage->AcceptOverflowRecord(MakeStoredRecord(OwnerId, Seed)))
	{
		++Seed;
	}
	TestEqual(TEXT("10마리까지"), Storage->GetStored().Num(), 10);
	TestFalse(TEXT("11번째는 받지 않는다"), Storage->AcceptOverflowRecord(MakeStoredRecord(OwnerId, 8199)));
	const FGrimCreatureInstance Second = Party[1]->GetInstance();
	TestEqual(TEXT("기본 칸 가득 → 쿠스토디아 안내"), Storage->Transfer(nullptr, Second.SpeciesId, Second.Seed, EP::Party, EP::Storage, false), ER::StorageFullUnlearned);
	TestEqual(TEXT("상자 없음 → 계약 거절(Q64)"), Storage->DecideNewTame(), EGrimNewTameDestination::Refused);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGrimCreatureBox* Box = World->SpawnActor<AGrimCreatureBox>(AGrimCreatureBox::StaticClass(), FVector(3000.f, 0.f, 0.f), FRotator::ZeroRotator, Params);
	TestEqual(TEXT("세계에 상자 → 상자로(Q64 순서)"), Box ? Storage->DecideNewTame() : EGrimNewTameDestination::Refused, EGrimNewTameDestination::Box);

	Storage->DebugSetLearned(true);
	TestEqual(TEXT("쿠스토디아 → 10 + 8"), Storage->GetStorageCapacity(), 18);
	TestEqual(TEXT("배우면 다시 보관함으로"), Storage->DecideNewTame(), EGrimNewTameDestination::Storage);
	TestTrue(TEXT("11번째를 받는다"), Storage->AcceptOverflowRecord(MakeStoredRecord(OwnerId, 8199)));
	return true;
}

#endif
