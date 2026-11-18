#pragma once

#include "NativeGameplayTags.h"

// 초원 꾸밈 소품(docs/spec/meadow-dressing.md) 전용 상태 · 재질. 반응은 ElementalReactions.csv 의 행으로만 준다(소품마다 코드를 짜지 않는다).
namespace GrimDressingTags
{
	// 재질 — 반응 규칙의 조건
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Material_Seedhead);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Material_Thirsty);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Material_Paper);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Material_Swarm);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Material_Floating);

	// 상태
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Spent);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Wilted);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Bloomed);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Blown);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Scattered);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Struck);
}
