#pragma once

#include "NativeGameplayTags.h"

// 속성 상호작용(R16) 전용 태그. 공용 태그(Element.* · State.Burning · State.Wet)는 Core/GrimGameplayTags.
namespace GrimInteractionTags
{
	// 재질 — 반응 규칙의 조건
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Material_Flammable);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Material_Consumable);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Material_Conductive);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Material_Absorbent);

	// 사물 전용 상태
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Charred);
}
