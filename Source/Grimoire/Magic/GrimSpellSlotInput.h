#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "GrimSpellSlotInput.generated.h"

class UAbilitySystemComponent;
class UGrimSpellbookComponent;
class UGrimSpellDefinition;

/**
 * 조작(Q29 — 호그와트 레거시 배치)의 어빌리티 입력 태그. 캐릭터는 키가 무엇에 묶였는지 모른 채 이 태그를 ASC 에 보낸다
 * (UGrimAbilitySystemComponent::AbilityInputTagPressed). 어떤 Spec 이 어떤 태그를 갖는지는 서버가 정한다(RefreshSlotInputTags).
 */
namespace GrimInputTags
{
	/** 기본 공격 (R26) — PC 좌클릭 · 패드 R2/RT 짧게 */
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Spell_BasicAttack);
	/** 지금 세트의 슬롯 1~4 바로 시전 — PC 숫자키 1~4 · 패드 R2/RT 누른 채 얼굴 버튼 */
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Spell_Slot1);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Spell_Slot2);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Spell_Slot3);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Spell_Slot4);
	/** 방어(호그와트의 Protego 자리) — 지금은 입력 자리만(어빌리티 없음) */
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Action_Protego);
	/** 회피 구르기 자리 — 지금은 입력 자리만(어빌리티 없음) */
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Action_Dodge);
}

/**
 * 조작 설정 — 정본은 Config/DefaultGame.ini 의 [/Script/Grimoire.GrimSpellInputSettings].
 * 키 배치 자체는 입력 에셋(Content/Grimoire/Input, Scripts/Character/make_input.py)에 있고, 여기는 HUD 에 보일 글자와 보조 동작만.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Grimoire Spell Input"))
class GRIMOIRE_API UGrimSpellInputSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UGrimSpellInputSettings* Get() { return GetDefault<UGrimSpellInputSettings>(); }

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	/** 슬롯 i 의 키 글자: 「키보드 · 패드」. 한쪽만 있으면 그것만 */
	FString GetSlotLabel(int32 SlotInSet) const;

	/** 기본 공격 칸의 키 글자 */
	FString GetBasicAttackLabel() const;

	/** HUD 슬롯 칸 키 글자 — 키보드 · 마우스(숫자키) */
	UPROPERTY(Config, EditAnywhere, Category = "Labels")
	TArray<FString> KeyboardSlotLabels;

	/** HUD 슬롯 칸 키 글자 — 패드(R2/RT 누른 채 얼굴 버튼). 아이콘 대신 글자 */
	UPROPERTY(Config, EditAnywhere, Category = "Labels")
	TArray<FString> GamepadSlotLabels;

	UPROPERTY(Config, EditAnywhere, Category = "Labels")
	FString KeyboardBasicAttackLabel;

	UPROPERTY(Config, EditAnywhere, Category = "Labels")
	FString GamepadBasicAttackLabel;

	/** 키 글자 사이 구분 */
	UPROPERTY(Config, EditAnywhere, Category = "Labels")
	FString LabelSeparator = TEXT(" · ");

	/** 마우스 휠 한 칸 = 다음 · 이전 슬롯 세트(호그와트 PC 의 보조 — 공략 하나만 확인, 불확실). 끄면 휠은 아무것도 안 한다 */
	UPROPERTY(Config, EditAnywhere, Category = "Sets")
	bool bMouseWheelCyclesSets = true;
};

/** 입력 → 슬롯의 순수 계산 (자동화 테스트 Grimoire.Input.*) */
struct GRIMOIRE_API FGrimSpellSlotInputMath
{
	/** 바로 시전 입력 수(세트 하나의 칸 수와 같다 — 호그와트 4칸) */
	static constexpr int32 NumSlotInputs = 4;

	/** 칸 번호(0~3) → 입력 태그. 범위 밖이면 빈 태그 */
	static FGameplayTag GetSlotInputTag(int32 SlotInSet);

	/** 입력 태그 → 칸 번호(0~3). 슬롯 입력이 아니면 INDEX_NONE */
	static int32 GetSlotInSet(const FGameplayTag& InputTag);

	/** 세트 · 칸 → 마법서 SlotSpells 의 번호. 범위 밖(세트 · 칸)이면 INDEX_NONE */
	static int32 ToAbsoluteSlot(int32 ActiveSet, int32 SlotInSet, int32 SlotsPerSet, int32 Sets);

	/** 마법서 SlotSpells 번호 → 지금 세트의 칸 번호. 지금 세트가 아니면 INDEX_NONE */
	static int32 ToSlotInActiveSet(int32 AbsoluteSlot, int32 ActiveSet, int32 SlotsPerSet, int32 Sets);

	/** 다음 · 이전 세트(끝에서 돈다) */
	static int32 CycleSet(int32 Current, int32 Step, int32 Sets);

	/**
	 * 마법 Spec 하나가 가져야 할 입력 태그: 기본 공격이면 Input.Spell.BasicAttack, 지금 세트의 칸 i 에 있으면 Input.Spell.Slot(i+1), 아니면 빈 태그.
	 * AbsoluteSlot = 그 마법이 놓인 SlotSpells 번호(없으면 INDEX_NONE).
	 */
	static FGameplayTag ComputeInputTag(bool bIsBasicAttack, int32 AbsoluteSlot, int32 ActiveSet, int32 SlotsPerSet, int32 Sets);

	/** 이 시스템이 관리하는 입력 태그(슬롯 1~4 · 기본 공격)인가 — 다시 붙일 때 이것만 지운다 */
	static bool IsManagedInputTag(const FGameplayTag& Tag);
};

namespace GrimSpellSlotInput
{
	/** 기본 공격 정의인가(SpellTag == Spell.BasicAttack) */
	GRIMOIRE_API bool IsBasicAttackSpell(const UGrimSpellDefinition* Spell);

	/**
	 * 서버: 마법서의 슬롯 · 지금 세트에 맞춰 마법 Spec 의 입력 태그(Input.Spell.Slot1~4 · BasicAttack)를 다시 붙인다.
	 * 바뀐 Spec 만 MarkAbilitySpecDirty — 소유 클라이언트로 복제되어 예측 발동도 같은 마법을 고른다. 바뀐 Spec 수를 돌려준다.
	 */
	GRIMOIRE_API int32 RefreshSlotInputTags(UAbilitySystemComponent* ASC, const UGrimSpellbookComponent* Spellbook);

	/** 이 입력 태그를 가진 마법(서버 · 클라이언트 — 복제된 Spec 에서). 없으면 nullptr */
	GRIMOIRE_API const UGrimSpellDefinition* FindSpellForInput(const UAbilitySystemComponent* ASC, const FGameplayTag& InputTag);
}
