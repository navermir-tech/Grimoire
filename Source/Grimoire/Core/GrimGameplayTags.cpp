#include "Core/GrimGameplayTags.h"

namespace GrimTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Element_Water, "Element.Water", "물 속성");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Element_Fire, "Element.Fire", "불 속성");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Element_Wind, "Element.Wind", "바람 속성");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Element_Earth, "Element.Earth", "땅 속성");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Element_Light, "Element.Light", "빛 속성");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Element_Dark, "Element.Dark", "어둠 속성");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Burning, "State.Burning", "불붙음. 세기 = 불꽃 크기");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Wet, "State.Wet", "젖음. 세기 = 젖은 정도");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Spell_Cast, "Input.Spell.Cast", "선택한 마법 시전");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Spell_Next, "Input.Spell.Next", "다음 마법 선택");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Input_Spell_Prev, "Input.Spell.Prev", "이전 마법 선택");
}
