#pragma once

#include "NativeGameplayTags.h"

// 지형 표식 GameplayTag(갈래 W). 생물 갈래의 서식 볼륨 · 스포너는 Habitat.* 표식 자리에 통합 때 놓는다.
namespace GrimTerrainTags
{
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Terrain_Site_Tower);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Terrain_Site_TowerYard);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Terrain_Site_Base);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Terrain_Path);

	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Habitat_Jackalope);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Habitat_Kelpie);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Habitat_BlackDog);
}
