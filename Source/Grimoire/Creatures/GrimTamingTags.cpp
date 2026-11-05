#include "Creatures/GrimTamingTags.h"

namespace GrimTamingTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Creature_Loyalty, "Creature.Loyalty", "길들인 생물의 충성도 단계");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Creature_Loyalty_Devoted, "Creature.Loyalty.Devoted", "충직(80~100) — 작업 · 탑승 속도 ↑, 명령을 잘 따른다");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Creature_Loyalty_Normal, "Creature.Loyalty.Normal", "보통(50~79)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Creature_Loyalty_Discontent, "Creature.Loyalty.Discontent", "불만(20~49) — 작업이 느리고 명령을 가끔 무시한다");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Creature_Loyalty_Rebellious, "Creature.Loyalty.Rebellious", "반항(1~19) — 일을 거부하고 태운 사람을 떨어뜨리기도 한다");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Creature_Loyalty_Gone, "Creature.Loyalty.Gone", "떠남(0) — 야생으로 돌아간다");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Creature_State_Tamed, "Creature.State.Tamed", "주인이 있는 생물");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Creature_State_Exhausted, "Creature.State.Exhausted", "지침 — 오래 싸우거나 달림(포획 확률 ↑, 길들인 생물이면 혹사)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Creature_State_Enraged, "Creature.State.Enraged", "계약에 저항해 화남 — 겁 많은 종도 덤빈다");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Creature_State_KnockedOut, "Creature.State.KnockedOut", "길들인 생물이 쓰러짐(죽지 않고 잠시 뒤 일어난다)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Creature_State_Ridden, "Creature.State.Ridden", "누군가 타고 있다");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Taming_CaptureBonus, "Taming.CaptureBonus", "계약 마법 숙련도 추가 효과 — 포획 확률 몫(Q26)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Progression_Node_CreatureParty, "Progression.Node.CreatureParty", "성장 트리 노드 효과 — 데리고 다니는 생물 수 +(R38, 생물 보관함 설정 PartyLimitTagBonuses)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Progression_Node_CreatureStorage, "Progression.Node.CreatureStorage", "성장 트리 노드 효과 — 생물 보관함 칸 +(R38, StorageCapacityTagBonuses)");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Spell_Pactum, "Spell.Pactum", "파크툼 — 무속성 기본 계약(1서클)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Spell_Pactum_Ignis, "Spell.Pactum.Ignis", "파크툼 이그니스 — 불 생물 계약");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Spell_Pactum_Aqua, "Spell.Pactum.Aqua", "파크툼 아쿠아 — 물 생물 계약");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Spell_Pactum_Ventus, "Spell.Pactum.Ventus", "파크툼 벤투스 — 바람 생물 계약");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Spell_Pactum_Terra, "Spell.Pactum.Terra", "파크툼 테라 — 땅 생물 계약");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Spell_Pactum_Umbra, "Spell.Pactum.Umbra", "파크툼 움브라 — 그림자술 생물 계약");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Spell_Pactum, "Cooldown.Spell.Pactum", "파크툼 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Spell_Pactum_Ignis, "Cooldown.Spell.Pactum.Ignis", "파크툼 이그니스 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Spell_Pactum_Aqua, "Cooldown.Spell.Pactum.Aqua", "파크툼 아쿠아 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Spell_Pactum_Ventus, "Cooldown.Spell.Pactum.Ventus", "파크툼 벤투스 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Spell_Pactum_Terra, "Cooldown.Spell.Pactum.Terra", "파크툼 테라 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Spell_Pactum_Umbra, "Cooldown.Spell.Pactum.Umbra", "파크툼 움브라 쿨다운");

	FGameplayTag GetStageTag(EGrimLoyaltyStage Stage)
	{
		switch (Stage)
		{
		case EGrimLoyaltyStage::Devoted: return Creature_Loyalty_Devoted;
		case EGrimLoyaltyStage::Normal: return Creature_Loyalty_Normal;
		case EGrimLoyaltyStage::Discontent: return Creature_Loyalty_Discontent;
		case EGrimLoyaltyStage::Rebellious: return Creature_Loyalty_Rebellious;
		default: return Creature_Loyalty_Gone;
		}
	}
}
