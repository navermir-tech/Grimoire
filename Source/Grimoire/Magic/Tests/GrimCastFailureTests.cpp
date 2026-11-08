#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Magic/GrimCastFailure.h"
#include "Magic/GrimFistAbility.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "Magic/GrimMagicTags.h"
#include "Magic/GrimSpellAbility.h"
#include "Magic/GrimSpellbookComponent.h"
#include "Magic/GrimSpellDefinition.h"
#include "Base/GrimBaseSettings.h"
#include "Base/GrimBaseTags.h"
#include "Base/GrimEquipmentComponent.h"
#include "Core/GrimAbilitySystemComponent.h"
#include "Core/GrimPlayerState.h"
#include "Items/GrimItemSettings.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

namespace GrimCastFailureTestsLocal // 유니티 빌드에서 다른 테스트 파일과 이름이 부딪히지 않게
{
	constexpr EAutomationTestFlags PureFlags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;
	constexpr EAutomationTestFlags WorldFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	struct FCastFailureTestWorld
	{
		UWorld* World = nullptr;

		FCastFailureTestWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("GrimCastFailureTestWorld"));
			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
			World->BeginPlay();
			World->GetWorldSettings()->NotifyBeginPlay();
		}

		~FCastFailureTestWorld()
		{
			World->EndPlay(EEndPlayReason::Quit);
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}
	};

	AGrimPlayerState* SpawnPlayer(UWorld* World)
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
		ASC->SetNumericAttributeBase(UGrimMagicAttributeSet::GetCircleAttribute(), 1.f);
		return PlayerState;
	}

	UGrimEquipmentComponent* EnsureEquipment(AGrimPlayerState* PlayerState)
	{
		// 게임에서는 거점 서브시스템이 붙인다 — 붙어 있지 않으면 같은 이름으로 붙인다.
		UGrimEquipmentComponent* Equipment = UGrimEquipmentComponent::FindForActor(PlayerState);
		if (!Equipment && PlayerState)
		{
			Equipment = NewObject<UGrimEquipmentComponent>(PlayerState, UGrimEquipmentComponent::StaticClass(), UGrimEquipmentComponent::ComponentName);
			PlayerState->AddInstanceComponent(Equipment);
			Equipment->RegisterComponent();
		}
		return Equipment;
	}

	FGameplayAbilitySpecHandle FindHandle(const UAbilitySystemComponent* ASC, const UGrimSpellDefinition* Spell)
	{
		for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			if (Spec.SourceObject.Get() == Spell)
			{
				return Spec.Handle;
			}
		}
		return FGameplayAbilitySpecHandle();
	}
}

// 까닭 고르기 순서 · 글 · 연출 여부 · ini.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimCastFailureClassifyTest, "Grimoire.Magic.CastFailure.Classify", GrimCastFailureTestsLocal::PureFlags)
bool FGrimCastFailureClassifyTest::RunTest(const FString& Parameters)
{
	using ER = EGrimCastFailReason;
	int32 Param = -1;
	FGrimCastFailInputs Ok;
	TestEqual(TEXT("다 되면 알리지 않음"), GrimCastFailure::Classify(Ok, Param), ER::None);

	FGrimCastFailInputs NoWand;
	NoWand.bWandMissing = true;
	NoWand.Circle = 1;
	NoWand.RequiredCircle = 3;
	NoWand.bCanAfford = false;
	TestEqual(TEXT("완드 없음이 서클 · 마나보다 먼저"), GrimCastFailure::Classify(NoWand, Param), ER::NoWand);
	FGrimCastFailInputs Broken = NoWand;
	Broken.bWandBroken = true;
	TestEqual(TEXT("부러짐"), GrimCastFailure::Classify(Broken, Param), ER::WandBroken);
	FGrimCastFailInputs Basic = NoWand;
	Basic.bBasicAttack = true;
	TestEqual(TEXT("기본 공격 + 완드 없음 → 주먹질이 나가니 알리지 않음(Q71)"), GrimCastFailure::Classify(Basic, Param), ER::None);
	FGrimCastFailInputs Blocked = NoWand;
	Blocked.bBlocked = true;
	TestEqual(TEXT("잠 · 쓰러짐은 알리지 않음"), GrimCastFailure::Classify(Blocked, Param), ER::None);

	// 탈진(R29 · Q116): 마법만 막힘 — 슬롯 마법은 알리고, 기본 공격은 주먹질이 나가니 알리지 않는다. 완드 · 서클보다 먼저.
	FGrimCastFailInputs Exhausted = NoWand;
	Exhausted.bMagicBlocked = true;
	TestEqual(TEXT("탈진은 완드 없음보다 먼저"), GrimCastFailure::Classify(Exhausted, Param), ER::Exhausted);
	Exhausted.bBasicAttack = true;
	TestEqual(TEXT("탈진 + 기본 공격 → 주먹질(알리지 않음)"), GrimCastFailure::Classify(Exhausted, Param), ER::None);
	Exhausted.bBasicAttack = false;
	Exhausted.bBlocked = true;
	TestEqual(TEXT("탈진하고 잠들었으면 잠이 먼저(알리지 않음)"), GrimCastFailure::Classify(Exhausted, Param), ER::None);

	FGrimCastFailInputs Circle;
	Circle.Circle = 1;
	Circle.RequiredCircle = 3;
	Circle.bOnCooldown = true;
	TestEqual(TEXT("서클이 쿨타임보다 먼저"), GrimCastFailure::Classify(Circle, Param), ER::CircleTooLow);
	TestEqual(TEXT("값 = 필요한 서클"), Param, 3);

	FGrimCastFailInputs Cooldown;
	Cooldown.bOnCooldown = true;
	Cooldown.CooldownRemaining = 1.2f;
	Cooldown.bCanAfford = false;
	TestEqual(TEXT("쿨타임이 마나보다 먼저"), GrimCastFailure::Classify(Cooldown, Param), ER::Cooldown);
	TestEqual(TEXT("남은 초 올림"), Param, 2);

	FGrimCastFailInputs Mana;
	Mana.bCanAfford = false;
	Mana.ManaCost = 14.2f;
	TestEqual(TEXT("마나 부족"), GrimCastFailure::Classify(Mana, Param), ER::NotEnoughMana);
	TestEqual(TEXT("필요 마나 올림"), Param, 15);

	for (const ER Reason : { ER::NoWand, ER::WandBroken, ER::CircleTooLow, ER::Cooldown, ER::NotEnoughMana, ER::TameNoRoom, ER::TameCircleTooLow, ER::TameRefused, ER::Exhausted })
	{
		TestFalse(FString::Printf(TEXT("글 있음 %s"), GrimCastFailure::ReasonName(Reason)), GrimCastFailure::GetMessage(Reason, 3).IsEmpty());
		TestTrue(FString::Printf(TEXT("연출 %s"), GrimCastFailure::ReasonName(Reason)), GrimCastFailure::ShouldFizzle(Reason));
	}
	TestTrue(TEXT("None 은 글 없음"), GrimCastFailure::GetMessage(ER::None, 0).IsEmpty());
	TestFalse(TEXT("None 은 연출 없음"), GrimCastFailure::ShouldFizzle(ER::None));
	TestTrue(TEXT("글에 값이 들어간다"), GrimCastFailure::GetMessage(ER::NotEnoughMana, 15).ToString().Contains(TEXT("15")));

	const UGrimCastFailureSettings* Settings = UGrimCastFailureSettings::Get();
	TestTrue(TEXT("ini: 알림 간격"), Settings->NoticeIntervalSeconds > 0.f);
	TestFalse(TEXT("ini: 임시 연출 이펙트(엔진 기본 템플릿)"), Settings->FizzleFX.IsNull());
	TestTrue(TEXT("큐 태그"), GrimCastFailureTags::GameplayCue_Magic_Fizzle.GetTag().IsValid());
	return true;
}

// Q71: 기본 완드 = 장비 표의 실제 아이템(내구도) · 처음 입고 시작 · 빈 칸 = 주먹질 · 옛 세이브는 빈 칸에 넣어 줌.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimCastFailureBasicWandTest, "Grimoire.Magic.CastFailure.BasicWand", GrimCastFailureTestsLocal::WorldFlags)
bool FGrimCastFailureBasicWandTest::RunTest(const FString& Parameters)
{
	using namespace GrimCastFailureTestsLocal;
	const FName BasicWand(TEXT("Wand_Basic"));
	const FGrimEquipmentDefinition* Def = UGrimBaseSettings::GetDatabase().FindEquipment(BasicWand);
	if (!TestNotNull(TEXT("장비 표: 기본 완드"), Def))
	{
		return false;
	}
	TestEqual(TEXT("완드 칸"), Def->Row.Slot, EGrimEquipSlot::Wand);
	TestEqual(TEXT("내구도 200(제안)"), Def->Row.MaxDurability, 200.f, 0.01f);
	TestEqual(TEXT("수리 재료 나무(기존 M4 규칙)"), Def->Row.RepairItem, FName(TEXT("Wood")));
	TestNotNull(TEXT("아이템 표: 기본 완드"), UGrimItemSettings::GetDatabase().Find(BasicWand));
	TestTrue(TEXT("ini: 시작 장비에 기본 완드"), UGrimBaseSettings::Get()->StartingEquipment.Contains(BasicWand));

	FCastFailureTestWorld TestWorld;
	AGrimPlayerState* Player = SpawnPlayer(TestWorld.World);
	UGrimEquipmentComponent* Equipment = EnsureEquipment(Player);
	UAbilitySystemComponent* ASC = Player ? Player->GetAbilitySystemComponent() : nullptr;
	if (!TestNotNull(TEXT("장비 컴포넌트"), Equipment) || !TestNotNull(TEXT("ASC"), ASC))
	{
		return false;
	}
	Equipment->GiveStartingEquipment();
	Equipment->SetSlotItem(EGrimEquipSlot::Wand, Equipment->GetSlot(EGrimEquipSlot::Wand)); // 태그를 다시 맞춘다(ASC 를 붙인 뒤)
	TestEqual(TEXT("처음 들어오면 기본 완드를 쥔다"), Equipment->GetSlot(EGrimEquipSlot::Wand).ItemId, BasicWand);
	TestFalse(TEXT("완드 있음 → 빈 칸 태그 없음"), ASC->HasMatchingGameplayTag(GrimBaseTags::Status_WandMissing));
	TestFalse(TEXT("마법 쪽도 완드 있음으로 본다"), UGrimSpellAbility::IsWandMissing(ASC));

	// 닳기 → 부러짐 → 주먹질(Q22 그대로)
	Equipment->Wear(EGrimEquipSlot::Wand, 199.f);
	TestEqual(TEXT("닳는다"), UGrimEquipmentComponent::GetDurability(Equipment->GetSlot(EGrimEquipSlot::Wand)), 1.f, 0.01f);
	Equipment->Wear(EGrimEquipSlot::Wand, 5.f);
	TestTrue(TEXT("부러졌다"), Equipment->IsWandBroken());
	TestTrue(TEXT("부러진 완드 태그"), UGrimSpellAbility::IsWandBroken(ASC));
	TestEqual(TEXT("부러진 완드는 칸에 남는다(수리로 되살림)"), Equipment->GetSlot(EGrimEquipSlot::Wand).ItemId, BasicWand);

	// 빈 칸 → Status.WandMissing → 마법은 못 쓰고 주먹만
	Equipment->SetSlotItem(EGrimEquipSlot::Wand, FGrimItemInstance());
	TestTrue(TEXT("빈 칸 태그"), ASC->HasMatchingGameplayTag(GrimBaseTags::Status_WandMissing));
	TestFalse(TEXT("빈 칸이면 부러짐 아님"), UGrimSpellAbility::IsWandBroken(ASC));

	UGrimSpellDefinition* Spell = NewObject<UGrimSpellDefinition>(GetTransientPackage(), TEXT("CastFailureTest_Spell"));
	Spell->Circle = 1;
	Player->GetSpellbook()->LearnSpell(Spell);
	const FGameplayAbilitySpecHandle Handle = FindHandle(ASC, Spell);
	int32 Param = 0;
	TestEqual(TEXT("판정: 완드 없음"), GrimCastFailure::Judge(ASC, Handle, Param), EGrimCastFailReason::NoWand);
	TestFalse(TEXT("완드가 없으면 기본 공격 대신 주먹질"), UGrimFistAbility::CanUseBasicAttackSpell(ASC->AbilityActorInfo.Get()));

	// 서버 알림: 소유 클라이언트(여기선 단독 — 바로 돈다), 같은 까닭은 간격 안에 한 번.
	UGrimAbilitySystemComponent* GrimASC = Cast<UGrimAbilitySystemComponent>(ASC);
	if (TestNotNull(TEXT("그리모리 ASC"), GrimASC))
	{
		int32 Notices = 0;
		uint8 LastReason = 0;
		const FDelegateHandle Bound = GrimASC->OnCastFailedNotice.AddLambda([&Notices, &LastReason](uint8 Reason, int32 Value) { ++Notices; LastReason = Reason; });
		GrimCastFailure::Report(ASC, EGrimCastFailReason::NoWand, 0);
		GrimCastFailure::Report(ASC, EGrimCastFailReason::NoWand, 0);
		TestEqual(TEXT("알림 한 번(간격)"), Notices, 1);
		TestEqual(TEXT("까닭 = 완드 없음"), LastReason, static_cast<uint8>(EGrimCastFailReason::NoWand));
		GrimCastFailure::Report(ASC, EGrimCastFailReason::NotEnoughMana, 12);
		TestEqual(TEXT("다른 까닭은 바로"), Notices, 2);
		GrimCastFailure::Report(ASC, EGrimCastFailReason::None, 0);
		TestEqual(TEXT("None 은 알리지 않음"), Notices, 2);
		GrimASC->OnCastFailedNotice.Remove(Bound);
	}

	// 세이브: v2 빈 칸은 그대로(벗은 것) · v1(보이지 않는 기본 완드 시절)은 빈 칸에 기본 완드
	TArray<uint8> Bytes;
	FMemoryWriter Writer(Bytes, true);
	Equipment->WriteSaveData(Writer);
	FMemoryReader ReaderV2(Bytes, true);
	Equipment->ReadSaveData(ReaderV2, 2);
	TestTrue(TEXT("v2: 빈 칸 그대로"), Equipment->IsWandMissing());
	FMemoryReader ReaderV1(Bytes, true);
	Equipment->ReadSaveData(ReaderV1, 1);
	TestEqual(TEXT("v1: 빈 칸에 기본 완드"), Equipment->GetSlot(EGrimEquipSlot::Wand).ItemId, BasicWand);
	TestFalse(TEXT("v1: 다시 완드 있음"), UGrimSpellAbility::IsWandMissing(ASC));
	TestEqual(TEXT("저장 버전 2"), Equipment->GetSaveVersion(), 2);

	Player->Destroy();
	return true;
}

#endif
