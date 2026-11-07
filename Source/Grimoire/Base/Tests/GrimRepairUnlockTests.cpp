#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Base/GrimBaseCore.h"
#include "Base/GrimBaseSettings.h"
#include "Base/GrimBaseStations.h"
#include "Base/GrimBuilderComponent.h"
#include "Base/GrimEquipmentComponent.h"
#include "Core/GrimAbilitySystemComponent.h"
#include "Core/GrimPlayerState.h"
#include "Factions/GrimFactionTypes.h"
#include "Factions/GrimShopSettings.h"
#include "Items/GrimInventoryComponent.h"
#include "Items/GrimItemSettings.h"
#include "Magic/GrimCastFailure.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "Magic/GrimSpellbookComponent.h"
#include "Magic/GrimSpellDefinition.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

namespace GrimRepairUnlockTestsLocal // 유니티 빌드에서 다른 테스트 파일과 이름이 부딪히지 않게
{
	constexpr EAutomationTestFlags WorldFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	struct FRepairTestWorld
	{
		UWorld* World = nullptr;

		FRepairTestWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("GrimRepairUnlockTestWorld"));
			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
			World->BeginPlay();
			World->GetWorldSettings()->NotifyBeginPlay();
		}

		~FRepairTestWorld()
		{
			World->EndPlay(EEndPlayReason::Quit);
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}
	};

	/** 건축 · 장비 · 가방 컴포넌트가 붙은 플레이어(서브시스템이 붙인다), 서클 · 마나 */
	AGrimPlayerState* SpawnPlayer(UWorld* World, int32 Circle)
	{
		AGrimPlayerState* PlayerState = World->SpawnActor<AGrimPlayerState>();
		if (!PlayerState)
		{
			return nullptr;
		}
		UAbilitySystemComponent* ASC = PlayerState->GetAbilitySystemComponent();
		ASC->InitAbilityActorInfo(PlayerState, PlayerState);
		if (!ASC->GetSet<UGrimMagicAttributeSet>())
		{
			ASC->AddSpawnedAttribute(PlayerState->GetMagicSet());
		}
		ASC->SetNumericAttributeBase(UGrimMagicAttributeSet::GetCircleAttribute(), static_cast<float>(Circle));
		ASC->SetNumericAttributeBase(UGrimMagicAttributeSet::GetMaxManaAttribute(), 500.f);
		ASC->SetNumericAttributeBase(UGrimMagicAttributeSet::GetManaAttribute(), 500.f);
		return PlayerState;
	}

	/** 소유 클라이언트에 간 마법 실패 · 잠김 알림(단독 — 바로 돈다)을 센다 */
	struct FNoticeCounter
	{
		TArray<EGrimCastFailReason> Reasons;
		FDelegateHandle Handle;
		TWeakObjectPtr<UGrimAbilitySystemComponent> ASC;

		explicit FNoticeCounter(UAbilitySystemComponent* InASC)
		{
			ASC = Cast<UGrimAbilitySystemComponent>(InASC);
			if (ASC.IsValid())
			{
				Handle = ASC->OnCastFailedNotice.AddLambda([this](uint8 Reason, int32 Param) { Reasons.Add(static_cast<EGrimCastFailReason>(Reason)); });
			}
		}

		~FNoticeCounter()
		{
			if (ASC.IsValid())
			{
				ASC->OnCastFailedNotice.Remove(Handle);
			}
		}

		int32 Count(EGrimCastFailReason Reason) const
		{
			return Reasons.FilterByPredicate([Reason](EGrimCastFailReason R) { return R == Reason; }).Num();
		}
	};
}

// Q121: 작업대 「수리」 탭은 레스티투오를 배워야 열리고(잠김은 Q64 방식으로 알림), 거점 핵 「전체 수리」는 레스티투오 막시마(노말 스킬)를 배워야 열린다.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimRepairUnlockTest, "Grimoire.Base.Repair.Unlock", GrimRepairUnlockTestsLocal::WorldFlags)
bool FGrimRepairUnlockTest::RunTest(const FString& Parameters)
{
	using namespace GrimRepairUnlockTestsLocal;
	const UGrimBaseSettings* Settings = UGrimBaseSettings::Get();
	TestTrue(TEXT("ini: 수리는 배워야"), Settings->bRequireRepairSpell);
	TestEqual(TEXT("ini: 막시마 ID"), Settings->BaseRepairSkillId, FName(TEXT("RestituoMaxima")));
	TestEqual(TEXT("ini: 막시마 4서클(제안)"), Settings->BaseRepairRequiredCircle, 4);
	const FGrimItemDefinition* MaximaBook = UGrimItemSettings::GetDatabase().Find(TEXT("Skillbook_Restituo_Maxima"));
	if (TestNotNull(TEXT("아이템 표: 스킬북 레스티투오 막시마"), MaximaBook))
	{
		TestEqual(TEXT("노말 스킬 책"), MaximaBook->Row.TeachesSpell, FName(TEXT("Skill:RestituoMaxima")));
	}

	FRepairTestWorld TestWorld;
	UWorld* World = TestWorld.World;
	AGrimPlayerState* Player = SpawnPlayer(World, 1);
	UGrimBuilderComponent* Builder = UGrimBuilderComponent::FindForActor(Player);
	UGrimEquipmentComponent* Equipment = UGrimEquipmentComponent::FindForActor(Player);
	UGrimInventoryComponent* Bag = UGrimInventoryComponent::FindForActor(Player);
	UAbilitySystemComponent* ASC = Player ? Player->GetAbilitySystemComponent() : nullptr;
	if (!TestNotNull(TEXT("건축 컴포넌트"), Builder) || !TestNotNull(TEXT("장비"), Equipment) || !TestNotNull(TEXT("가방"), Bag) || !TestNotNull(TEXT("ASC"), ASC))
	{
		return false;
	}
	FNoticeCounter Notices(ASC);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGrimWorkbench* Workbench = World->SpawnActor<AGrimWorkbench>(AGrimWorkbench::StaticClass(), FVector(200.f, 0.f, 0.f), FRotator::ZeroRotator, Params);
	AGrimBaseCore* Core = World->SpawnActor<AGrimBaseCore>(AGrimBaseCore::StaticClass(), FVector(-400.f, 0.f, 0.f), FRotator::ZeroRotator, Params);
	if (!TestNotNull(TEXT("작업대"), Workbench) || !TestNotNull(TEXT("거점 핵"), Core))
	{
		return false;
	}

	// ── 레스티투오 전: 작업대 「수리」 잠김 ──
	Equipment->SetSlotItem(EGrimEquipSlot::Wand, FGrimItemInstance(TEXT("Wand_Basic"), 1));
	Equipment->Wear(EGrimEquipSlot::Wand, 100.f);
	Bag->AddItem(FGrimItemInstance(TEXT("Wood"), 10), false);
	TestFalse(TEXT("처음엔 레스티투오 모름"), Builder->KnowsRepairSpell());
	TestEqual(TEXT("잠긴 작업대 수리 → 0"), Builder->PerformRepair(Workbench, false, false), 0);
	TestEqual(TEXT("잠김 까닭을 알림(Q64 방식)"), Notices.Count(EGrimCastFailReason::RepairLocked), 1);
	TestEqual(TEXT("나무 그대로"), Bag->CountOf(TEXT("Wood")), 10);
	TestFalse(TEXT("잠김은 마법 실패 연출 없음"), GrimCastFailure::ShouldFizzle(EGrimCastFailReason::RepairLocked));

	// ── 레스티투오(2서클)를 배우면 열린다 — 재료 · 최대치 −10% 그대로 ──
	UGrimSpellDefinition* Restituo = Settings->RepairSpell.LoadSynchronous();
	if (!TestNotNull(TEXT("레스티투오 정의 DA_Spell_Repair"), Restituo))
	{
		return false;
	}
	TestEqual(TEXT("레스티투오 = 2서클(Q121)"), Restituo->Circle, 2);
	TestFalse(TEXT("1서클은 못 배움"), Player->GetSpellbook()->LearnSpell(Restituo));
	ASC->SetNumericAttributeBase(UGrimMagicAttributeSet::GetCircleAttribute(), 2.f);
	TestTrue(TEXT("2서클 — 배움"), Player->GetSpellbook()->LearnSpell(Restituo));
	TestTrue(TEXT("레스티투오 앎"), Builder->KnowsRepairSpell());
	const int32 Used = Builder->PerformRepair(Workbench, false, false);
	TestTrue(TEXT("작업대 수리 — 재료를 쓴다"), Used > 0);
	TestEqual(TEXT("가방 나무 − 쓴 수"), Bag->CountOf(TEXT("Wood")), 10 - Used);
	const FGrimItemInstance& Wand = Equipment->GetSlot(EGrimEquipSlot::Wand);
	TestEqual(TEXT("수리 횟수 1"), Wand.RepairCount, 1);
	TestEqual(TEXT("최대치 −10%(200 → 180)"), UGrimEquipmentComponent::GetMaxDurability(Wand), 180.f, 0.01f);

	// ── 거점 핵 「전체 수리」: 막시마 전엔 잠김 ──
	TestFalse(TEXT("막시마 모름"), Builder->KnowsBaseRepairSkill());
	TestEqual(TEXT("잠긴 전체 수리 → 0"), Builder->PerformRepair(Core, false, false), 0);
	TestEqual(TEXT("전체 수리 잠김 알림"), Notices.Count(EGrimCastFailReason::BaseRepairLocked), 1);

	// 스킬북(가방)으로 배운다 — 서클이 모자라면 못 읽고 까닭 알림(Q64)
	const FName MaximaId(TEXT("Skillbook_Restituo_Maxima"));
	Bag->AddItem(FGrimItemInstance(MaximaId, 1), false);
	auto BookSlot = [Bag, MaximaId]() { return Bag->GetSlots().IndexOfByPredicate([MaximaId](const FGrimItemInstance& Slot) { return Slot.ItemId == MaximaId; }); };
	TestEqual(TEXT("2서클 — 막시마 못 읽음"), Bag->UseItem(BookSlot()), EGrimItemUseResult::CannotUse);
	TestEqual(TEXT("책 남음"), Bag->CountOf(MaximaId), 1);
	ASC->SetNumericAttributeBase(UGrimMagicAttributeSet::GetCircleAttribute(), 4.f);
	TestEqual(TEXT("4서클 — 배움"), Bag->UseItem(BookSlot()), EGrimItemUseResult::Learned);
	TestTrue(TEXT("막시마 앎"), Builder->KnowsBaseRepairSkill());
	TestEqual(TEXT("책 소모"), Bag->CountOf(MaximaId), 0);
	TestEqual(TEXT("이미 앎"), Builder->LearnNormalSkill(Settings->BaseRepairSkillId), EGrimLearnResult::AlreadyKnown);
	Builder->PerformRepair(Core, false, false);
	TestEqual(TEXT("배운 뒤엔 잠김 알림 없음"), Notices.Count(EGrimCastFailReason::BaseRepairLocked), 1);

	// 세이브 Base.Skills 왕복
	TArray<uint8> Bytes;
	FMemoryWriter Writer(Bytes, true);
	Builder->WriteSaveData(Writer);
	AGrimPlayerState* Reloaded = SpawnPlayer(World, 1);
	UGrimBuilderComponent* ReloadedBuilder = UGrimBuilderComponent::FindForActor(Reloaded);
	if (TestNotNull(TEXT("새 플레이어 건축"), ReloadedBuilder))
	{
		TestFalse(TEXT("새 플레이어는 모름"), ReloadedBuilder->KnowsBaseRepairSkill());
		FMemoryReader Reader(Bytes, true);
		ReloadedBuilder->ReadSaveData(Reader, ReloadedBuilder->GetSaveVersion());
		TestTrue(TEXT("세이브: 막시마 앎"), ReloadedBuilder->KnowsBaseRepairSkill());
		Reloaded->Destroy();
	}
	Player->Destroy();
	return true;
}

// Q121: 레스티투오를 배우지 않은 사람은 마법탑 스승에게 솔을 내고 입은 장비를 고친다(값 = 재료 몫 × RepairPricePerMaterial).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimRepairNpcServiceTest, "Grimoire.Base.Repair.NpcService", GrimRepairUnlockTestsLocal::WorldFlags)
bool FGrimRepairNpcServiceTest::RunTest(const FString& Parameters)
{
	using namespace GrimRepairUnlockTestsLocal;
	const UGrimShopSettings* Shop = UGrimShopSettings::Get();
	TestEqual(TEXT("ini: 재료 1개 몫 5 솔(제안)"), Shop->RepairPricePerMaterial, 5);
	TestTrue(TEXT("대리 수리 맥락은 표가 요구"), GrimDialogueContexts::RequiredForServices().Contains(GrimDialogueContexts::RepairNoMoney)
		&& GrimDialogueContexts::RequiredForServices().Contains(GrimDialogueContexts::RepairNothing));

	FRepairTestWorld TestWorld;
	AGrimPlayerState* Player = SpawnPlayer(TestWorld.World, 1);
	UGrimEquipmentComponent* Equipment = UGrimEquipmentComponent::FindForActor(Player);
	UGrimInventoryComponent* Bag = UGrimInventoryComponent::FindForActor(Player);
	if (!TestNotNull(TEXT("장비"), Equipment) || !TestNotNull(TEXT("가방"), Bag))
	{
		return false;
	}
	Equipment->SetSlotItem(EGrimEquipSlot::Wand, FGrimItemInstance(TEXT("Wand_Basic"), 1));
	int32 Fee = -1;
	TestEqual(TEXT("멀쩡하면 고칠 것 없음"), GrimTowerShop::TryRepair(Bag, &Fee), GrimDialogueContexts::RepairNothing);
	TestEqual(TEXT("값 0"), Fee, 0);

	Equipment->Wear(EGrimEquipSlot::Wand, 120.f); // 200 중 80 남음 → 25% 단위 재료 몫
	const int32 Units = Equipment->GetRepairUnitsNeeded();
	TestTrue(TEXT("재료 몫 > 0"), Units > 0);
	TestEqual(TEXT("솔 없음 → 모자람"), GrimTowerShop::TryRepair(Bag, &Fee), GrimDialogueContexts::RepairNoMoney);
	TestEqual(TEXT("값 = 몫 × 5"), Fee, Units * 5);
	TestEqual(TEXT("모자라면 그대로"), Equipment->GetSlot(EGrimEquipSlot::Wand).RepairCount, 0);

	Bag->AddItem(FGrimItemInstance(Shop->CurrencyItem, Fee + 7), false);
	TestEqual(TEXT("솔을 내고 고친다"), GrimTowerShop::TryRepair(Bag, &Fee), GrimDialogueContexts::Repair);
	TestEqual(TEXT("솔 − 값"), Bag->CountOf(Shop->CurrencyItem), 7);
	const FGrimItemInstance& Wand = Equipment->GetSlot(EGrimEquipSlot::Wand);
	TestEqual(TEXT("수리 횟수 1(최대치 −10% 는 작업대와 같다)"), Wand.RepairCount, 1);
	TestEqual(TEXT("새 최대치까지 가득"), UGrimEquipmentComponent::GetDurability(Wand), UGrimEquipmentComponent::GetMaxDurability(Wand), 0.5f);
	TestEqual(TEXT("재료는 받지 않는다"), Bag->CountOf(TEXT("Wood")), 0);
	Player->Destroy();
	return true;
}

#endif
