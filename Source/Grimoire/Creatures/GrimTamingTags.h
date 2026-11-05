#pragma once

#include "NativeGameplayTags.h"
#include "Creatures/GrimTamingTypes.h"

/** 길들이기 · 충성도 · 탑승 태그(M3 갈래 T). 탑승 중 그리모어 거두기는 MagicFX 의 State.Mounted 를 쓴다. */
namespace GrimTamingTags
{
	// 충성도 단계(생물 ASC 의 루즈 태그 — 각 기기가 복제된 충성도로 붙인다)
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Creature_Loyalty);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Creature_Loyalty_Devoted);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Creature_Loyalty_Normal);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Creature_Loyalty_Discontent);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Creature_Loyalty_Rebellious);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Creature_Loyalty_Gone);

	// 생물 상태
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Creature_State_Tamed);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Creature_State_Exhausted);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Creature_State_Enraged);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Creature_State_KnockedOut);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Creature_State_Ridden);

	// 숙련도 추가 효과(Q26) — 계약 마법의 포획 확률 몫
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Taming_CaptureBonus);

	// 파크툼 주문(spell · cooldown) — 무속성 + 속성 5개(이그니스 · 아쿠아 · 벤투스 · 테라 · 움브라)
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Spell_Pactum);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Spell_Pactum_Ignis);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Spell_Pactum_Aqua);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Spell_Pactum_Ventus);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Spell_Pactum_Terra);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Spell_Pactum_Umbra);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Spell_Pactum);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Spell_Pactum_Ignis);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Spell_Pactum_Aqua);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Spell_Pactum_Ventus);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Spell_Pactum_Terra);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Spell_Pactum_Umbra);

	/** 단계 → 태그 */
	GRIMOIRE_API FGameplayTag GetStageTag(EGrimLoyaltyStage Stage);
}
