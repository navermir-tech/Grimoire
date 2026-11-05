#include "Magic/GrimSpellSlotInput.h"
#include "Magic/GrimMagicTags.h"
#include "Magic/GrimProficiencySettings.h"
#include "Magic/GrimSpellbookComponent.h"
#include "Magic/GrimSpellDefinition.h"
#include "AbilitySystemComponent.h"

namespace GrimInputTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Spell_BasicAttack, "Input.Spell.BasicAttack", "기본 공격 — PC 좌클릭 · 패드 R2/RT 짧게");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Spell_Slot1, "Input.Spell.Slot1", "지금 세트 슬롯 1 바로 시전");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Spell_Slot2, "Input.Spell.Slot2", "지금 세트 슬롯 2 바로 시전");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Spell_Slot3, "Input.Spell.Slot3", "지금 세트 슬롯 3 바로 시전");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Spell_Slot4, "Input.Spell.Slot4", "지금 세트 슬롯 4 바로 시전");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Action_Protego, "Input.Action.Protego", "방어(호그와트 Protego 자리) — 입력 자리만");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Action_Dodge, "Input.Action.Dodge", "회피 구르기 — 입력 자리만");
}

// ───────────────────────── 설정 ─────────────────────────

FString UGrimSpellInputSettings::GetSlotLabel(int32 SlotInSet) const
{
	const FString Keyboard = KeyboardSlotLabels.IsValidIndex(SlotInSet) ? KeyboardSlotLabels[SlotInSet] : FString::FromInt(SlotInSet + 1);
	const FString Gamepad = GamepadSlotLabels.IsValidIndex(SlotInSet) ? GamepadSlotLabels[SlotInSet] : FString();
	return Gamepad.IsEmpty() ? Keyboard : (Keyboard.IsEmpty() ? Gamepad : Keyboard + LabelSeparator + Gamepad);
}

FString UGrimSpellInputSettings::GetBasicAttackLabel() const
{
	if (GamepadBasicAttackLabel.IsEmpty())
	{
		return KeyboardBasicAttackLabel;
	}
	return KeyboardBasicAttackLabel.IsEmpty() ? GamepadBasicAttackLabel : KeyboardBasicAttackLabel + LabelSeparator + GamepadBasicAttackLabel;
}

// ───────────────────────── 순수 계산 ─────────────────────────

FGameplayTag FGrimSpellSlotInputMath::GetSlotInputTag(int32 SlotInSet)
{
	switch (SlotInSet)
	{
	case 0: return GrimInputTags::Input_Spell_Slot1;
	case 1: return GrimInputTags::Input_Spell_Slot2;
	case 2: return GrimInputTags::Input_Spell_Slot3;
	case 3: return GrimInputTags::Input_Spell_Slot4;
	default: return FGameplayTag();
	}
}

int32 FGrimSpellSlotInputMath::GetSlotInSet(const FGameplayTag& InputTag)
{
	for (int32 Index = 0; Index < NumSlotInputs; ++Index)
	{
		if (InputTag.IsValid() && InputTag == GetSlotInputTag(Index))
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

int32 FGrimSpellSlotInputMath::ToAbsoluteSlot(int32 ActiveSet, int32 SlotInSet, int32 SlotsPerSet, int32 Sets)
{
	if (SlotsPerSet <= 0 || Sets <= 0 || ActiveSet < 0 || ActiveSet >= Sets || SlotInSet < 0 || SlotInSet >= SlotsPerSet)
	{
		return INDEX_NONE;
	}
	return ActiveSet * SlotsPerSet + SlotInSet;
}

int32 FGrimSpellSlotInputMath::ToSlotInActiveSet(int32 AbsoluteSlot, int32 ActiveSet, int32 SlotsPerSet, int32 Sets)
{
	if (AbsoluteSlot < 0 || SlotsPerSet <= 0 || AbsoluteSlot >= SlotsPerSet * Sets || AbsoluteSlot / SlotsPerSet != ActiveSet)
	{
		return INDEX_NONE;
	}
	return AbsoluteSlot % SlotsPerSet;
}

int32 FGrimSpellSlotInputMath::CycleSet(int32 Current, int32 Step, int32 Sets)
{
	if (Sets <= 0)
	{
		return 0;
	}
	return ((Current + Step) % Sets + Sets) % Sets;
}

FGameplayTag FGrimSpellSlotInputMath::ComputeInputTag(bool bIsBasicAttack, int32 AbsoluteSlot, int32 ActiveSet, int32 SlotsPerSet, int32 Sets)
{
	if (bIsBasicAttack)
	{
		return GrimInputTags::Input_Spell_BasicAttack;
	}
	const int32 SlotInSet = ToSlotInActiveSet(AbsoluteSlot, ActiveSet, SlotsPerSet, Sets);
	// 세트가 4칸보다 크면 앞 4칸만 바로 시전 입력이 있다.
	return SlotInSet != INDEX_NONE && SlotInSet < NumSlotInputs ? GetSlotInputTag(SlotInSet) : FGameplayTag();
}

bool FGrimSpellSlotInputMath::IsManagedInputTag(const FGameplayTag& Tag)
{
	return Tag.IsValid() && (Tag == GrimInputTags::Input_Spell_BasicAttack || GetSlotInSet(Tag) != INDEX_NONE);
}

// ───────────────────────── Spec 태그 ─────────────────────────

namespace GrimSpellSlotInput
{
	bool IsBasicAttackSpell(const UGrimSpellDefinition* Spell)
	{
		return Spell && Spell->SpellTag.IsValid() && Spell->SpellTag == GrimMagicTags::Spell_BasicAttack;
	}

	int32 RefreshSlotInputTags(UAbilitySystemComponent* ASC, const UGrimSpellbookComponent* Spellbook)
	{
		if (!ASC || !Spellbook || !ASC->IsOwnerActorAuthoritative())
		{
			return 0;
		}
		const UGrimSpellbookSettings* Settings = UGrimSpellbookSettings::Get();
		const TArray<TObjectPtr<UGrimSpellDefinition>>& SlotSpells = Spellbook->GetSlotSpells();

		int32 Changed = 0;
		for (FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			const UGrimSpellDefinition* Spell = Cast<UGrimSpellDefinition>(Spec.SourceObject.Get());
			if (!Spell)
			{
				continue;
			}
			const int32 AbsoluteSlot = SlotSpells.IndexOfByKey(Spell);
			const FGameplayTag Desired = FGrimSpellSlotInputMath::ComputeInputTag(IsBasicAttackSpell(Spell), AbsoluteSlot,
				Spellbook->GetActiveSlotSet(), Settings->SpellSlotsPerSet, Settings->SpellSlotSets);

			FGameplayTagContainer& Tags = Spec.GetDynamicSpecSourceTags();
			FGameplayTagContainer ToRemove;
			for (const FGameplayTag& Tag : Tags)
			{
				if (FGrimSpellSlotInputMath::IsManagedInputTag(Tag) && Tag != Desired)
				{
					ToRemove.AddTag(Tag);
				}
			}
			const bool bAdd = Desired.IsValid() && !Tags.HasTagExact(Desired);
			if (ToRemove.IsEmpty() && !bAdd)
			{
				continue;
			}
			Tags.RemoveTags(ToRemove);
			if (bAdd)
			{
				Tags.AddTag(Desired);
			}
			ASC->MarkAbilitySpecDirty(Spec);
			++Changed;
		}
		return Changed;
	}

	const UGrimSpellDefinition* FindSpellForInput(const UAbilitySystemComponent* ASC, const FGameplayTag& InputTag)
	{
		if (!ASC || !InputTag.IsValid())
		{
			return nullptr;
		}
		for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			if (Spec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
			{
				if (const UGrimSpellDefinition* Spell = Cast<UGrimSpellDefinition>(Spec.SourceObject.Get()))
				{
					return Spell;
				}
			}
		}
		return nullptr;
	}
}
