#pragma once

#include "NativeGameplayTags.h"

// 여러 시스템이 함께 쓰는 네이티브 GameplayTag. 한 시스템만 쓰는 태그는 그 시스템 폴더의 파일에 따로 선언한다.
namespace GrimTags
{
	// 속성 (R07)
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Element_Water);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Element_Fire);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Element_Wind);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Element_Earth);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Element_Light);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Element_Dark);

	// 속성 상태 (R16) — 사물 · 생물 · 플레이어 공용
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Burning);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Wet);

	// 어빌리티 입력. 캐릭터가 누르면 ASC 가 이 태그를 DynamicSpecSourceTags 로 가진 어빌리티를 발동한다.
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Spell_Cast);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Spell_Next);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Spell_Prev);
}
