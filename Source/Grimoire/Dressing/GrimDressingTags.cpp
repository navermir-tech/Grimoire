#include "Dressing/GrimDressingTags.h"

namespace GrimDressingTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Material_Seedhead, "Material.Seedhead", "바람에 날리는 솜 · 씨앗머리(바람솜꽃 D01)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Material_Thirsty, "Material.Thirsty", "물을 받으면 일어서는 시든 꽃(D03)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Material_Paper, "Material.Paper", "센 바람에 날려 옮겨 가는 종이(책장 D21)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Material_Swarm, "Material.Swarm", "바람에 흩어지는 날벌레 무리(나비 D16)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Material_Floating, "Material.Floating", "충격에 튕겼다 돌아오는 떠 있는 돌(D29)");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Spent, "State.Spent", "날리거나 거둬서 비었다(다시 날 때까지)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Wilted, "State.Wilted", "시들었다 — 물을 받으면 핀다");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Bloomed, "State.Bloomed", "물을 받고 피었다");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Blown, "State.Blown", "센 바람에 날리는 중(잠깐)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Scattered, "State.Scattered", "흩어졌다(잠깐)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Struck, "State.Struck", "충격을 받고 튕기는 중(잠깐)");
}
