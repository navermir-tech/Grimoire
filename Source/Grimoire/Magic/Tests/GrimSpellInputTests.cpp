#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Magic/GrimBasicAttackAbility.h"
#include "Magic/GrimBasicAttackProjectile.h"
#include "Magic/GrimBasicAttackTypes.h"
#include "Magic/GrimFistAbility.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "Magic/GrimMagicTags.h"
#include "Magic/GrimProficiencySettings.h"
#include "Magic/GrimSpellbookComponent.h"
#include "Magic/GrimSpellDefinition.h"
#include "Magic/GrimSpellSlotInput.h"
#include "Core/GrimPlayerState.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

namespace GrimSpellInputTestsLocal // 유니티 빌드에서 다른 테스트 파일과 이름이 부딪히지 않게
{
	constexpr EAutomationTestFlags InputTestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	struct FInputTestWorld
	{
		UWorld* World = nullptr;

		FInputTestWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false);
			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
		}

		~FInputTestWorld()
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}
	};

	UGrimSpellDefinition* MakeInputTestSpell(const TCHAR* Name, bool bBasic = false)
	{
		UGrimSpellDefinition* Spell = NewObject<UGrimSpellDefinition>(GetTransientPackage(), Name);
		Spell->Circle = 1;
		Spell->bTracksProficiency = !bBasic;
		Spell->SpellTag = bBasic ? GrimMagicTags::Spell_BasicAttack.GetTag() : FGameplayTag();
		return Spell;
	}

	const FGameplayAbilitySpec* FindSpec(const UAbilitySystemComponent* ASC, const UGrimSpellDefinition* Spell)
	{
		for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			if (Spec.SourceObject.Get() == Spell)
			{
				return &Spec;
			}
		}
		return nullptr;
	}
}

// 입력 → 슬롯: 태그 ↔ 칸, 세트 · 칸 ↔ 마법서 번호, 세트 돌기, Spec 이 가질 태그.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimInputSlotMathTest, "Grimoire.Input.SlotMath", GrimSpellInputTestsLocal::InputTestFlags)
bool FGrimInputSlotMathTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("칸 0 → Slot1"), FGrimSpellSlotInputMath::GetSlotInputTag(0), GrimInputTags::Input_Spell_Slot1.GetTag());
	TestEqual(TEXT("칸 3 → Slot4"), FGrimSpellSlotInputMath::GetSlotInputTag(3), GrimInputTags::Input_Spell_Slot4.GetTag());
	TestFalse(TEXT("칸 4 없음"), FGrimSpellSlotInputMath::GetSlotInputTag(4).IsValid());
	TestEqual(TEXT("Slot3 → 칸 2"), FGrimSpellSlotInputMath::GetSlotInSet(GrimInputTags::Input_Spell_Slot3), 2);
	TestEqual(TEXT("기본 공격은 슬롯 아님"), FGrimSpellSlotInputMath::GetSlotInSet(GrimInputTags::Input_Spell_BasicAttack), (int32)INDEX_NONE);
	TestEqual(TEXT("빈 태그"), FGrimSpellSlotInputMath::GetSlotInSet(FGameplayTag()), (int32)INDEX_NONE);

	// 4칸 × 4세트
	TestEqual(TEXT("세트 0 칸 0"), FGrimSpellSlotInputMath::ToAbsoluteSlot(0, 0, 4, 4), 0);
	TestEqual(TEXT("세트 2 칸 1"), FGrimSpellSlotInputMath::ToAbsoluteSlot(2, 1, 4, 4), 9);
	TestEqual(TEXT("세트 범위 밖"), FGrimSpellSlotInputMath::ToAbsoluteSlot(4, 0, 4, 4), (int32)INDEX_NONE);
	TestEqual(TEXT("칸 범위 밖"), FGrimSpellSlotInputMath::ToAbsoluteSlot(0, 4, 4, 4), (int32)INDEX_NONE);
	TestEqual(TEXT("9 는 세트 2 의 칸 1"), FGrimSpellSlotInputMath::ToSlotInActiveSet(9, 2, 4, 4), 1);
	TestEqual(TEXT("9 는 세트 0 이 아님"), FGrimSpellSlotInputMath::ToSlotInActiveSet(9, 0, 4, 4), (int32)INDEX_NONE);
	TestEqual(TEXT("없는 칸"), FGrimSpellSlotInputMath::ToSlotInActiveSet(INDEX_NONE, 0, 4, 4), (int32)INDEX_NONE);

	TestEqual(TEXT("다음 세트"), FGrimSpellSlotInputMath::CycleSet(0, 1, 4), 1);
	TestEqual(TEXT("끝에서 처음으로"), FGrimSpellSlotInputMath::CycleSet(3, 1, 4), 0);
	TestEqual(TEXT("처음에서 끝으로"), FGrimSpellSlotInputMath::CycleSet(0, -1, 4), 3);

	TestEqual(TEXT("기본 공격 태그"), FGrimSpellSlotInputMath::ComputeInputTag(true, INDEX_NONE, 0, 4, 4), GrimInputTags::Input_Spell_BasicAttack.GetTag());
	TestEqual(TEXT("지금 세트 칸 2 → Slot3"), FGrimSpellSlotInputMath::ComputeInputTag(false, 6, 1, 4, 4), GrimInputTags::Input_Spell_Slot3.GetTag());
	TestFalse(TEXT("다른 세트 칸 → 태그 없음"), FGrimSpellSlotInputMath::ComputeInputTag(false, 2, 1, 4, 4).IsValid());
	TestFalse(TEXT("슬롯에 없음 → 태그 없음"), FGrimSpellSlotInputMath::ComputeInputTag(false, INDEX_NONE, 0, 4, 4).IsValid());
	// 세트가 5칸이면 다섯째 칸은 바로 시전 입력이 없다(입력은 4개).
	TestFalse(TEXT("5칸 세트의 다섯째"), FGrimSpellSlotInputMath::ComputeInputTag(false, 4, 0, 5, 2).IsValid());

	TestTrue(TEXT("관리 태그: 슬롯"), FGrimSpellSlotInputMath::IsManagedInputTag(GrimInputTags::Input_Spell_Slot2));
	TestTrue(TEXT("관리 태그: 기본 공격"), FGrimSpellSlotInputMath::IsManagedInputTag(GrimInputTags::Input_Spell_BasicAttack));
	TestFalse(TEXT("방어 자리는 관리 밖"), FGrimSpellSlotInputMath::IsManagedInputTag(GrimInputTags::Input_Action_Protego));
	return true;
}

// HUD 키 글자(ini): 키보드 · 패드.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimInputLabelsTest, "Grimoire.Input.Labels", GrimSpellInputTestsLocal::InputTestFlags)
bool FGrimInputLabelsTest::RunTest(const FString& Parameters)
{
	const UGrimSpellInputSettings* Settings = UGrimSpellInputSettings::Get();
	TestEqual(TEXT("키보드 글자 4개"), Settings->KeyboardSlotLabels.Num(), FGrimSpellSlotInputMath::NumSlotInputs);
	TestEqual(TEXT("패드 글자 4개"), Settings->GamepadSlotLabels.Num(), FGrimSpellSlotInputMath::NumSlotInputs);
	TestTrue(TEXT("슬롯 1 = 숫자 1 + 패드"), Settings->GetSlotLabel(0).StartsWith(TEXT("1")) && Settings->GetSlotLabel(0).Contains(Settings->GamepadSlotLabels[0]));
	TestTrue(TEXT("기본 공격 글자에 패드 버튼"), Settings->GetBasicAttackLabel().Contains(Settings->GamepadBasicAttackLabel));
	TestEqual(TEXT("ini 밖 칸은 숫자"), Settings->GetSlotLabel(7).Left(1), FString(TEXT("8")));
	return true;
}

// 마법서 Spec 태그: 지금 세트의 칸 → Slot1~4, 기본 공격 → BasicAttack, 세트를 바꾸면 옮겨 붙는다.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimInputSpecTagsTest, "Grimoire.Input.SpecTags", GrimSpellInputTestsLocal::InputTestFlags)
bool FGrimInputSpecTagsTest::RunTest(const FString& Parameters)
{
	using namespace GrimSpellInputTestsLocal;
	FInputTestWorld TestWorld;
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
	// 2서클 = 슬롯 세트 둘(Q122 서클 표) — 세트 2 를 쓴다.
	ASC->SetNumericAttributeBase(UGrimMagicAttributeSet::GetCircleAttribute(), 2.f);

	UGrimSpellbookComponent* Spellbook = PlayerState->GetSpellbook();
	UGrimSpellDefinition* A = MakeInputTestSpell(TEXT("InputTest_A"));
	UGrimSpellDefinition* B = MakeInputTestSpell(TEXT("InputTest_B"));
	UGrimSpellDefinition* C = MakeInputTestSpell(TEXT("InputTest_C"));
	UGrimSpellDefinition* Basic = MakeInputTestSpell(TEXT("InputTest_Basic"), true);
	TestTrue(TEXT("기본 공격 정의 판별"), GrimSpellSlotInput::IsBasicAttackSpell(Basic));
	TestFalse(TEXT("보통 마법은 아님"), GrimSpellSlotInput::IsBasicAttackSpell(A));

	// 기본 공격 입력은 처음엔 주먹질 하나뿐 — 주먹질은 마법 Spec 이 아니라 태그 관리 밖.
	TestTrue(TEXT("주먹질 줌"), UGrimFistAbility::EnsureGranted(ASC));
	TestTrue(TEXT("두 번 줘도 하나"), UGrimFistAbility::EnsureGranted(ASC));
	int32 FistSpecs = 0;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		FistSpecs += Spec.Ability && Spec.Ability->IsA<UGrimFistAbility>() && Spec.GetDynamicSpecSourceTags().HasTagExact(GrimInputTags::Input_Spell_BasicAttack) ? 1 : 0;
	}
	TestEqual(TEXT("주먹질 Spec 하나(기본 공격 입력)"), FistSpecs, 1);
	TestNull(TEXT("배우기 전엔 기본 공격 마법 없음"), GrimSpellSlotInput::FindSpellForInput(ASC, GrimInputTags::Input_Spell_BasicAttack));

	Spellbook->LearnSpell(A);
	Spellbook->LearnSpell(B);
	Spellbook->LearnSpell(C);
	// 기본 공격은 시작 지급이 아니라 스킬북으로 배운다(튜토리얼 첫 단계).
	TestEqual(TEXT("기본 공격 스킬북 → 배움"), Spellbook->UseSkillbook(Basic), EGrimLearnResult::Learned);
	// A = 칸 0, B = 칸 1, C 를 세트 2 의 칸 3(번호 7)으로 옮긴다.
	Spellbook->AssignSlot(UGrimSpellbookSettings::Get()->SpellSlotsPerSet + 3, C);

	TestTrue(TEXT("처음 붙이기 — 바뀐 Spec 있음"), GrimSpellSlotInput::RefreshSlotInputTags(ASC, Spellbook) > 0);
	TestTrue(TEXT("A = Slot1"), FindSpec(ASC, A)->GetDynamicSpecSourceTags().HasTagExact(GrimInputTags::Input_Spell_Slot1));
	TestTrue(TEXT("B = Slot2"), FindSpec(ASC, B)->GetDynamicSpecSourceTags().HasTagExact(GrimInputTags::Input_Spell_Slot2));
	auto ManagedCount = [ASC](const UGrimSpellDefinition* Spell)
	{
		int32 Count = 0;
		for (const FGameplayTag& Tag : FindSpec(ASC, Spell)->GetDynamicSpecSourceTags())
		{
			Count += FGrimSpellSlotInputMath::IsManagedInputTag(Tag) ? 1 : 0;
		}
		return Count;
	};
	TestEqual(TEXT("C 는 다른 세트 — 태그 없음"), ManagedCount(C), 0);
	TestEqual(TEXT("A 는 태그 하나"), ManagedCount(A), 1);
	TestTrue(TEXT("기본 공격 = BasicAttack"), FindSpec(ASC, Basic)->GetDynamicSpecSourceTags().HasTagExact(GrimInputTags::Input_Spell_BasicAttack));
	TestEqual(TEXT("Slot1 → A"), GrimSpellSlotInput::FindSpellForInput(ASC, GrimInputTags::Input_Spell_Slot1), (const UGrimSpellDefinition*)A);
	TestNull(TEXT("Slot4 → 빈 칸"), GrimSpellSlotInput::FindSpellForInput(ASC, GrimInputTags::Input_Spell_Slot4));
	TestEqual(TEXT("다시 붙여도 바뀐 것 없음"), GrimSpellSlotInput::RefreshSlotInputTags(ASC, Spellbook), 0);

	// 세트 2 로: C 가 Slot4, A · B 는 태그를 잃는다. 기본 공격은 그대로.
	Spellbook->SetActiveSlotSet(1);
	GrimSpellSlotInput::RefreshSlotInputTags(ASC, Spellbook);
	TestTrue(TEXT("세트 2: C = Slot4"), FindSpec(ASC, C)->GetDynamicSpecSourceTags().HasTagExact(GrimInputTags::Input_Spell_Slot4));
	TestFalse(TEXT("세트 2: A 태그 없음"), FindSpec(ASC, A)->GetDynamicSpecSourceTags().HasTagExact(GrimInputTags::Input_Spell_Slot1));
	TestTrue(TEXT("세트 2: 기본 공격 그대로"), FindSpec(ASC, Basic)->GetDynamicSpecSourceTags().HasTagExact(GrimInputTags::Input_Spell_BasicAttack));
	TestNull(TEXT("세트 2: Slot1 빈 칸"), GrimSpellSlotInput::FindSpellForInput(ASC, GrimInputTags::Input_Spell_Slot1));

	PlayerState->Destroy();
	return true;
}

// 연타 리듬(쿨다운이 아님): 간격 · 서버 여유.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimBasicAttackRhythmTest, "Grimoire.Magic.BasicAttack.Rhythm", GrimSpellInputTestsLocal::InputTestFlags)
bool FGrimBasicAttackRhythmTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("처음은 언제나"), FGrimBasicAttackMath::IsReady(0.0, -1.0e9, 0.3f));
	TestFalse(TEXT("0.2초 뒤는 이르다"), FGrimBasicAttackMath::IsReady(10.2, 10.0, 0.3f));
	TestTrue(TEXT("0.3초 뒤는 된다"), FGrimBasicAttackMath::IsReady(10.3, 10.0, 0.3f));
	TestTrue(TEXT("서버 여유 0.08 이면 0.25초도"), FGrimBasicAttackMath::IsReady(10.25, 10.0, 0.3f, 0.08f));
	TestFalse(TEXT("여유를 넘으면 안 된다"), FGrimBasicAttackMath::IsReady(10.2, 10.0, 0.3f, 0.08f));
	TestTrue(TEXT("간격 0 은 늘"), FGrimBasicAttackMath::IsReady(10.0, 10.0, 0.f));

	// ini 값(스펙 R26: 0.3초 · 1.5초 · 4연타)
	const UGrimBasicAttackSettings* Settings = UGrimBasicAttackSettings::Get();
	TestEqual(TEXT("ini 리듬 0.3"), Settings->AttackInterval, 0.3f, 0.001f);
	TestEqual(TEXT("ini 연타 창 1.0(마지막 발부터 — Q120)"), Settings->ComboWindow, 1.f, 0.001f);
	TestEqual(TEXT("ini 4연타"), Settings->ComboLength, 4);
	return true;
}

// 연타 · 강타(Q120): 마지막 발부터 1초 안에 이어 쏘면 이어지고 4번째가 강타, 창을 넘기거나 강타 뒤면 처음부터.
// 사이에 쓴 슬롯 마법 · 회피 · 방패(KeepAlive)는 끊지도 세지도 않는다.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimBasicAttackComboTest, "Grimoire.Magic.BasicAttack.Combo", GrimSpellInputTestsLocal::InputTestFlags)
bool FGrimBasicAttackComboTest::RunTest(const FString& Parameters)
{
	constexpr float W = 1.f;
	FGrimBasicAttackCombo Combo;
	TestFalse(TEXT("처음엔 연타 아님"), FGrimBasicAttackMath::IsChainAlive(Combo, 0.0, W));
	TestEqual(TEXT("1번째"), FGrimBasicAttackMath::Register(Combo, 0.0, W, 4), 1);
	TestEqual(TEXT("2번째"), FGrimBasicAttackMath::Register(Combo, 0.3, W, 4), 2);
	TestEqual(TEXT("3번째"), FGrimBasicAttackMath::Register(Combo, 0.6, W, 4), 3);
	TestEqual(TEXT("보이는 번째 3"), FGrimBasicAttackMath::VisibleStep(Combo, 0.7, W), 3);
	const int32 Fourth = FGrimBasicAttackMath::Register(Combo, 0.9, W, 4);
	TestEqual(TEXT("4번째"), Fourth, 4);
	TestTrue(TEXT("4번째 = 강타"), FGrimBasicAttackMath::IsHeavy(Fourth, 4));
	TestFalse(TEXT("3번째는 강타 아님"), FGrimBasicAttackMath::IsHeavy(3, 4));
	TestEqual(TEXT("강타 뒤는 처음부터"), FGrimBasicAttackMath::Register(Combo, 1.2, W, 4), 1);

	// 창은 「마지막 발부터」: 0.9초 간격으로 계속 쏘면 첫 발부터 1초가 훨씬 지나도 이어진다(옛 규칙 — 첫 발부터 1.5초 — 이면 끊겼다).
	FGrimBasicAttackCombo Steady;
	FGrimBasicAttackMath::Register(Steady, 0.0, W, 4);
	FGrimBasicAttackMath::Register(Steady, 0.9, W, 4);
	TestEqual(TEXT("1.8초의 3번째"), FGrimBasicAttackMath::Register(Steady, 1.8, W, 4), 3);
	TestEqual(TEXT("2.7초의 4번째 = 강타"), FGrimBasicAttackMath::Register(Steady, 2.7, W, 4), 4);

	// 경계: 마지막 발부터 정확히 1초는 이어지고, 1초를 넘기면 1부터.
	FGrimBasicAttackCombo Edge;
	FGrimBasicAttackMath::Register(Edge, 0.0, W, 4);
	TestEqual(TEXT("정확히 1초 = 이어짐"), FGrimBasicAttackMath::Register(Edge, 1.0, W, 4), 2);
	TestEqual(TEXT("1.01초 = 처음부터"), FGrimBasicAttackMath::Register(Edge, 2.01, W, 4), 1);
	TestEqual(TEXT("창이 지나면 보이는 번째 0"), FGrimBasicAttackMath::VisibleStep(Edge, 3.5, W), 0);

	// 사이의 슬롯 마법 · 회피 · 방패: 2번째 뒤 0.8초에 슬롯 마법, 다시 0.8초 뒤 기본 공격 → 3번째(끊기지도, 세지도 않음).
	FGrimBasicAttackCombo Weave;
	FGrimBasicAttackMath::Register(Weave, 0.0, W, 4);
	FGrimBasicAttackMath::Register(Weave, 0.3, W, 4);
	TestTrue(TEXT("창 안의 다른 행동은 연타를 살린다"), FGrimBasicAttackMath::KeepAlive(Weave, 1.1, W));
	TestEqual(TEXT("번째는 그대로(세지 않음)"), FGrimBasicAttackMath::VisibleStep(Weave, 1.1, W), 2);
	TestTrue(TEXT("방패를 쥔 동안 · 놓을 때도 살린다"), FGrimBasicAttackMath::KeepAlive(Weave, 1.9, W));
	TestEqual(TEXT("슬롯 마법 · 방패 뒤 3번째"), FGrimBasicAttackMath::Register(Weave, 2.7, W, 4), 3);
	TestEqual(TEXT("리듬은 기본 공격만 본다"), Weave.LastAttackTime, 2.7, 1e-9);

	// 이미 끊긴 연타는 다른 행동이 되살리지 않는다(전투를 벗어나면 자연히 처음으로).
	FGrimBasicAttackCombo Dead;
	FGrimBasicAttackMath::Register(Dead, 0.0, W, 4);
	TestFalse(TEXT("창 밖 행동은 살리지 않는다"), FGrimBasicAttackMath::KeepAlive(Dead, 1.5, W));
	TestEqual(TEXT("끊긴 뒤 기본 공격은 1번째"), FGrimBasicAttackMath::Register(Dead, 1.7, W, 4), 1);
	FGrimBasicAttackCombo Fresh;
	TestFalse(TEXT("연타가 없으면 살릴 것도 없다"), FGrimBasicAttackMath::KeepAlive(Fresh, 0.0, W));
	TestEqual(TEXT("슬롯 마법만 쓴 뒤 기본 공격은 1번째"), FGrimBasicAttackMath::Register(Fresh, 0.2, W, 4), 1);

	// 강타 뒤에 다른 행동이 있어도 다음 발은 1번째.
	FGrimBasicAttackCombo AfterHeavy;
	for (int32 i = 0; i < 4; ++i)
	{
		FGrimBasicAttackMath::Register(AfterHeavy, 0.3 * i, W, 4);
	}
	FGrimBasicAttackMath::KeepAlive(AfterHeavy, 1.2, W);
	TestEqual(TEXT("강타 + 다른 행동 뒤 1번째"), FGrimBasicAttackMath::Register(AfterHeavy, 1.5, W, 4), 1);

	// 피해 배율 · 밀어냄
	TestEqual(TEXT("강타 배율"), FGrimBasicAttackMath::DamageMultiplier(4, 4, 3.f), 3.f, 0.001f);
	TestEqual(TEXT("보통 배율"), FGrimBasicAttackMath::DamageMultiplier(2, 4, 3.f), 1.f, 0.001f);
	const FVector Knock = FGrimBasicAttackMath::KnockbackVelocity(FVector(1.f, 0.f, -1.f).GetSafeNormal(), 900.f, 250.f);
	TestEqual(TEXT("밀어냄 수평은 날아온 쪽"), Knock.X, 900.0, 0.01);
	TestEqual(TEXT("밀어냄 위로"), Knock.Z, 250.0, 0.01);
	TestEqual(TEXT("수직으로 맞으면 수평 0"), FGrimBasicAttackMath::KnockbackVelocity(FVector::DownVector, 900.f, 250.f).X, 0.0, 0.01);
	return true;
}

// 기본 공격 정의(DA_Spell_BasicAttack): 무속성 · Bolt · 비용 0 · 쿨다운 0 · 숙련도 없음 · 기본 공격 어빌리티 · 기본 공격 볼트.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGrimBasicAttackDefinitionTest, "Grimoire.Magic.BasicAttack.Definition", GrimSpellInputTestsLocal::InputTestFlags)
bool FGrimBasicAttackDefinitionTest::RunTest(const FString& Parameters)
{
	const UGrimSpellDefinition* Spell = UGrimBasicAttackSettings::Get()->BasicAttackSpell.LoadSynchronous();
	if (!TestNotNull(TEXT("ini 의 기본 공격 정의가 있다"), Spell))
	{
		return false;
	}
	TestTrue(TEXT("SpellTag = Spell.BasicAttack"), GrimSpellSlotInput::IsBasicAttackSpell(Spell));
	TestEqual(TEXT("무속성"), Spell->Element, GrimMagicTags::Element_None.GetTag());
	TestTrue(TEXT("Bolt"), Spell->Delivery == EGrimSpellDelivery::Bolt);
	TestEqual(TEXT("마나 0"), Spell->ManaCost, 0.f);
	TestEqual(TEXT("쿨다운 0"), Spell->Cooldown, 0.f);
	TestFalse(TEXT("숙련도 없음"), Spell->bTracksProficiency);
	TestEqual(TEXT("1서클"), Spell->Circle, 1);
	TestTrue(TEXT("기본 공격 어빌리티"), Spell->AbilityClass && Spell->AbilityClass->IsChildOf(UGrimBasicAttackAbility::StaticClass()));
	TestTrue(TEXT("기본 공격 볼트"), Spell->ProjectileClass && Spell->ProjectileClass->IsChildOf(AGrimBasicAttackProjectile::StaticClass()));
	TestEqual(TEXT("자극도 무속성(사물엔 충격만)"), Spell->Stimulus.Element, GrimMagicTags::Element_None.GetTag());
	TestTrue(TEXT("피해는 작다"), Spell->Damage > 0.f && Spell->Damage <= 10.f);
	TestNotNull(TEXT("연출 묶음"), Spell->VisualSet.Get());
	return true;
}

#endif
