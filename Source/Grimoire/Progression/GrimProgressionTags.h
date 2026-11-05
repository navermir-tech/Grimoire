#pragma once

#include "NativeGameplayTags.h"

// 성장(레벨 · 성장 트리) 전용 네이티브 GameplayTag — M2 후반 갈래 L.
namespace GrimProgressionTags
{
	// 경험치 출처 (UGrimLevelComponent::AwardExperience 의 Source). 출처별 기본값 · 반복 감쇠는 UGrimLevelSettings::Sources.
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(XPSource);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(XPSource_Skillbook);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(XPSource_CircleUp);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(XPSource_RegionDiscovery);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(XPSource_RuleDiscovery);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(XPSource_Quest);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(XPSource_Hunt);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(XPSource_Craft);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(XPSource_Debug);

	// 성장 보너스 GE(UGrimGE_ProgressionBonus)의 SetByCaller 키 — 어트리뷰트마다 하나
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_MaxHealth);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_MaxMana);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_HeatResist);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_ColdResist);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_DiseaseResist);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_MoveSpeedScale);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_CastSpeedScale);
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_ManaCostScale);

	/** 성장 트리 노드 효과 태그의 뿌리. 어트리뷰트가 없는 효과 키 K 는 태그 "Progression.Node.K" 로 붙는다(GrimProgressionTags.cpp 목록). */
	GRIMOIRE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Node);

	/** 효과 키(예: "Fire.IgniteStrength")의 노드 태그. 등록되지 않았으면 빈 태그. */
	GRIMOIRE_API FGameplayTag FindNodeEffectTag(FName EffectKey);
}
