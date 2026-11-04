#include "Interaction/GrimInteractionTags.h"

namespace GrimInteractionTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Material_Flammable, "Material.Flammable", "불이 붙는 재질");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Material_Consumable, "Material.Consumable", "다 타면 없어지는 재질(마른 풀 · 장작)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Material_Conductive, "Material.Conductive", "전기가 통하는 재질");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Material_Absorbent, "Material.Absorbent", "물을 머금는 재질");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Charred, "State.Charred", "다 타 버림. 다시 불이 붙지 않는다");
}
