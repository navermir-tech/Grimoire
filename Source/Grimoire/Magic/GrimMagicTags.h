#pragma once

#include "NativeGameplayTags.h"

// Magic 시스템 전용 네이티브 GameplayTag.
namespace GrimMagicTags
{
	// GE 수치를 코드에서 넣는 SetByCaller 키
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_ManaCost);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Cooldown);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Damage);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_ManaRegen);

	// 마법별 쿨다운 태그 — UGrimSpellDefinition::CooldownTag 가 가리킨다
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Spell);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Spell_Ember);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Spell_WaterJet);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Spell_Breeze);

	// 마법 이름 태그 (어빌리티 · GE 에셋 태그)
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Spell);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Spell_Ember);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Spell_WaterJet);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Spell_Breeze);
}
