#pragma once

#include "NativeGameplayTags.h"

// 아이템 · 요리 전용 태그(M3 갈래 I).
namespace GrimItemTags
{
	/** 익음(R16) — 바닥 · 불 위의 날것이 불 자극을 충분히 받아 익은 것(픽업이 세기 1 로 알린다) */
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Cooked);

	/** 음식 버프가 붙어 있다(UGrimFoodBuffEffect) */
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Buff_Food);
}
