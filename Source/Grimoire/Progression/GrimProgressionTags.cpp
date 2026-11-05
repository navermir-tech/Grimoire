#include "Progression/GrimProgressionTags.h"
#include "GameplayTagsManager.h"

namespace GrimProgressionTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(XPSource, "Progression.XPSource", "경험치 출처");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(XPSource_Skillbook, "Progression.XPSource.Skillbook", "스킬북으로 새 마법을 배움");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(XPSource_CircleUp, "Progression.XPSource.CircleUp", "서클 승급");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(XPSource_RegionDiscovery, "Progression.XPSource.RegionDiscovery", "새 기후 지역 첫 방문");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(XPSource_RuleDiscovery, "Progression.XPSource.RuleDiscovery", "상호작용 규칙 첫 발견(도감)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(XPSource_Quest, "Progression.XPSource.Quest", "퀘스트 보상");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(XPSource_Hunt, "Progression.XPSource.Hunt", "생물 사냥 · 길들이기 · 사냥꾼 격파");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(XPSource_Craft, "Progression.XPSource.Craft", "제작 · 건축");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(XPSource_Debug, "Progression.XPSource.Debug", "개발 콘솔");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_MaxHealth, "SetByCaller.Progression.MaxHealth", "성장 보너스: 최대 HP 더하기");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_MaxMana, "SetByCaller.Progression.MaxMana", "성장 보너스: 최대 마나 더하기");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_HeatResist, "SetByCaller.Progression.HeatResist", "성장 보너스: 더위저항 더하기");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_ColdResist, "SetByCaller.Progression.ColdResist", "성장 보너스: 추위저항 더하기");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_DiseaseResist, "SetByCaller.Progression.DiseaseResist", "성장 보너스: 질병저항 더하기");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_MoveSpeedScale, "SetByCaller.Progression.MoveSpeedScale", "성장 보너스: 이동 속도 배율 곱하기");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_CastSpeedScale, "SetByCaller.Progression.CastSpeedScale", "성장 보너스: 시전 속도 배율 곱하기");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_ManaCostScale, "SetByCaller.Progression.ManaCostScale", "성장 보너스: 마나 소모 배율 곱하기");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Node, "Progression.Node", "성장 트리 노드 효과(어트리뷰트가 없는 것)");

	// 어트리뷰트가 없는 노드 효과 — Content/Grimoire/Data/SkillTreeNodes.csv 의 Effects 키마다 하나.
	// 효과를 실제로 쓰는 코드(마법 위력 · 상호작용 세기 · 특성)는 이 태그 · UGrimSkillTreeComponent::GetEffectValue 를 읽는다(TODO: 각 시스템).
	// 표에 새 키를 더하면 여기에도 더한다(테스트 Grimoire.Progression.TreeTable 이 빠진 것을 잡는다).
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_BasicAttack_Power, "Progression.Node.BasicAttack.Power");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_BasicAttack_Speed, "Progression.Node.BasicAttack.Speed");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_BasicAttack_Knockback, "Progression.Node.BasicAttack.Knockback");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_BasicAttack_Impact, "Progression.Node.BasicAttack.Impact");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_BasicAttack_Trait_EmberFourth, "Progression.Node.BasicAttack.Trait.EmberFourth");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Arcane_Power, "Progression.Node.Arcane.Power");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Arcane_ManaCost, "Progression.Node.Arcane.ManaCost");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Arcane_Pierce, "Progression.Node.Arcane.Pierce");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Arcane_Resonance, "Progression.Node.Arcane.Resonance");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Arcane_Trait_Burst, "Progression.Node.Arcane.Trait.Burst");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Mana_Regen, "Progression.Node.Mana.Regen");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Mana_ProficiencyXP, "Progression.Node.Mana.ProficiencyXP");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Mana_Trait_BloodCast, "Progression.Node.Mana.Trait.BloodCast");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Wand_AimAssist, "Progression.Node.Wand.AimAssist");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Wand_Durability, "Progression.Node.Wand.Durability");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Wand_Trait_Resonance, "Progression.Node.Wand.Trait.Resonance");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Water_Power, "Progression.Node.Water.Power");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Water_ManaCost, "Progression.Node.Water.ManaCost");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Water_SoakDuration, "Progression.Node.Water.SoakDuration");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Water_SoakRadius, "Progression.Node.Water.SoakRadius");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Water_Trait_ForkedJet, "Progression.Node.Water.Trait.ForkedJet");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Ice_Power, "Progression.Node.Ice.Power");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Ice_ManaCost, "Progression.Node.Ice.ManaCost");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Ice_FreezeChance, "Progression.Node.Ice.FreezeChance");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Ice_FreezeStrength, "Progression.Node.Ice.FreezeStrength");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Ice_Trait_Shatter, "Progression.Node.Ice.Trait.Shatter");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Fire_Power, "Progression.Node.Fire.Power");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Fire_ManaCost, "Progression.Node.Fire.ManaCost");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Fire_BurnDuration, "Progression.Node.Fire.BurnDuration");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Fire_IgniteStrength, "Progression.Node.Fire.IgniteStrength");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Fire_Trait_Wildfire, "Progression.Node.Fire.Trait.Wildfire");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Wind_Power, "Progression.Node.Wind.Power");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Wind_ManaCost, "Progression.Node.Wind.ManaCost");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Wind_Knockback, "Progression.Node.Wind.Knockback");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Wind_FanFlame, "Progression.Node.Wind.FanFlame");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Wind_Trait_Whirlwind, "Progression.Node.Wind.Trait.Whirlwind");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Earth_Power, "Progression.Node.Earth.Power");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Earth_ManaCost, "Progression.Node.Earth.ManaCost");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Earth_StunChance, "Progression.Node.Earth.StunChance");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Earth_Smother, "Progression.Node.Earth.Smother");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Earth_Trait_StoneWall, "Progression.Node.Earth.Trait.StoneWall");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Plant_Power, "Progression.Node.Plant.Power");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Plant_ManaCost, "Progression.Node.Plant.ManaCost");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Plant_EntangleDuration, "Progression.Node.Plant.EntangleDuration");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Plant_Growth, "Progression.Node.Plant.Growth");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Plant_Trait_Thorns, "Progression.Node.Plant.Trait.Thorns");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Holy_Power, "Progression.Node.Holy.Power");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Holy_ManaCost, "Progression.Node.Holy.ManaCost");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Holy_Healing, "Progression.Node.Holy.Healing");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Holy_Purify, "Progression.Node.Holy.Purify");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Holy_Trait_Ward, "Progression.Node.Holy.Trait.Ward");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Light_Power, "Progression.Node.Light.Power");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Light_ManaCost, "Progression.Node.Light.ManaCost");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Light_BlindDuration, "Progression.Node.Light.BlindDuration");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Light_Illuminate, "Progression.Node.Light.Illuminate");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Light_Trait_Prism, "Progression.Node.Light.Trait.Prism");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Hex_Power, "Progression.Node.Hex.Power");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Hex_ManaCost, "Progression.Node.Hex.ManaCost");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Hex_CurseDuration, "Progression.Node.Hex.CurseDuration");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Hex_Decay, "Progression.Node.Hex.Decay");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Hex_Trait_Drain, "Progression.Node.Hex.Trait.Drain");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Shadow_Power, "Progression.Node.Shadow.Power");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Shadow_ManaCost, "Progression.Node.Shadow.ManaCost");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Shadow_StealthDuration, "Progression.Node.Shadow.StealthDuration");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Shadow_Silence, "Progression.Node.Shadow.Silence");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(Node_Shadow_Trait_Step, "Progression.Node.Shadow.Trait.Step");

	FGameplayTag FindNodeEffectTag(FName EffectKey)
	{
		if (EffectKey.IsNone())
		{
			return FGameplayTag();
		}
		const FName TagName(*FString::Printf(TEXT("Progression.Node.%s"), *EffectKey.ToString()));
		return UGameplayTagsManager::Get().RequestGameplayTag(TagName, false);
	}
}
