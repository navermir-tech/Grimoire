#pragma once

#include "NativeGameplayTags.h"

/**
 * 연출 · 동작(MagicFX) 이 읽는 상태 태그. 그리모어 거두기(Q25) 판정에 쓴다.
 * M1.5 에는 이 상태를 붙이는 시스템이 아직 없다 — 이름만 정해 두고, 헤엄 · 건축 · 나르기 · 탈것을 만드는 쪽이 이 태그를 ASC 에 붙인다.
 * (헤엄은 태그가 없어도 캐릭터 이동 컴포넌트의 IsSwimming 으로도 본다.)
 */
namespace GrimMagicFXTags
{
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Swimming);   // State.Swimming
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_BuildMode);  // State.BuildMode
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Carrying);   // State.Carrying — 두 손으로 나르기
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Mounted);    // State.Mounted — 탈것 위
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Aiming);     // State.Aiming — 조준 중(탈것 전투에서 책을 띄운다)
}
