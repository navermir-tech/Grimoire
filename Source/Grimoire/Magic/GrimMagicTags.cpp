#include "Magic/GrimMagicTags.h"

namespace GrimMagicTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_ManaCost, "SetByCaller.Magic.ManaCost", "마나 비용(음수로 넣는다)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Cooldown, "SetByCaller.Magic.Cooldown", "쿨다운 초");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Damage, "SetByCaller.Magic.Damage", "상성 적용 전 기본 피해");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_ManaRegen, "SetByCaller.Magic.ManaRegen", "주기마다 회복하는 마나");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Spell, "Cooldown.Spell", "마법 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Spell_Ember, "Cooldown.Spell.Ember", "불씨 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Spell_WaterJet, "Cooldown.Spell.WaterJet", "물줄기 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Spell_Breeze, "Cooldown.Spell.Breeze", "미풍 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Spell_Fireball, "Cooldown.Spell.Fireball", "화염구 쿨다운");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Spell, "Spell", "마법");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Spell_Ember, "Spell.Ember", "불씨");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Spell_WaterJet, "Spell.WaterJet", "물줄기");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Spell_Breeze, "Spell.Breeze", "미풍");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Spell_Fireball, "Spell.Fireball", "화염구 (2서클, 스킬북)");
}
