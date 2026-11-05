#pragma once

#include "NativeGameplayTags.h"

/** 생물 시스템 태그. 속성 태그 중 생물이 먼저 쓰는 10속성 하위 태그(풀 · 그림자술)도 여기서 등록한다(마법 쪽이 10속성을 넣을 때 옮겨도 이름은 같다). */
namespace GrimCreatureTags
{
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Element_Earth_Plant);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Element_Dark_Shadow);

	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Creature_Ability_SeedSpread);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Creature_Ability_WaterRun);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Creature_Ability_ShadowStep);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Creature_Ability_GiantRoar);

	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Creature_State_ShadowHidden);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Creature_State_Dead);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Creature_State_Weakened);

	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Habitat_Meadow);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Habitat_Field);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Habitat_Waterside);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Habitat_Road);
}
