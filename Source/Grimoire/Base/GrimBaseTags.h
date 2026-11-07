#pragma once

#include "NativeGameplayTags.h"

// 거점 · 건축 · 장비 전용 태그(M4).
namespace GrimBaseTags
{
	/** 완드가 부러졌다(Q22) — 마법은 못 쓰고 주먹만(UGrimSpellAbility::GetWand 가 완드 없음으로 본다) */
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_WandBroken);

	/** 완드 칸이 비었다(Q71) — 마법은 못 쓰고 주먹만. 부러짐과 같은 길(GetWand 가 완드 없음으로 본다) */
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_WandMissing);

	/** 「수리」 마법(무속성) */
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Spell_Repair);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Spell_Repair);

	/** 건축 나무(조각) — 센 불(세기 1 이상)에만 붙어 잠시 타며 옆 나무 조각으로 번진다(반응 표 Wood_Ignite). 모닥불 곁불로는 안 붙는다 */
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Material_Wood);
}
