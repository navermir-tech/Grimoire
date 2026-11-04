#pragma once

#include "NativeGameplayTags.h"

// 생존 시스템 전용 GameplayTag. 서버가 임계값에 따라 ASC 에 복제 루스 태그로 붙인다.
namespace GrimSurvivalTags
{
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Hungry);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Starving);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Hypothermia);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Hyperthermia);
}
