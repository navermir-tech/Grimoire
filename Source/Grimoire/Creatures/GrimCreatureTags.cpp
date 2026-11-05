#include "Creatures/GrimCreatureTags.h"

namespace GrimCreatureTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Element_Earth_Plant, "Element.Earth.Plant", "풀 속성(땅 계열 — 10속성 확정)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Element_Dark_Shadow, "Element.Dark.Shadow", "그림자술 속성(어둠 계열 — 10속성 확정)");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Creature_Ability_SeedSpread, "Creature.Ability.SeedSpread", "씨앗 퍼뜨리기 — 지나간 자리 마른 풀에 풀 자극(재칼로프)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Creature_Ability_WaterRun, "Creature.Ability.WaterRun", "물 위 달리기 — 물 볼륨 위를 달린다(켈피, 탑승은 다음 묶음)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Creature_Ability_ShadowStep, "Creature.Ability.ShadowStep", "그림자 걸음 — 어두운 곳에서 잠깐 안 보인다(검은 개)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Creature_Ability_GiantRoar, "Creature.Ability.GiantRoar", "거대 개체가 더 가지는 기술 자리(아직 효과 없음)");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Creature_State_ShadowHidden, "Creature.State.ShadowHidden", "그림자 걸음으로 숨은 중(조준 · 락온 안 됨)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Creature_State_Dead, "Creature.State.Dead", "죽음");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Creature_State_Weakened, "Creature.State.Weakened", "체력이 약해짐 문턱(30%) 이하 — 길들이기 확률이 오를 자리");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Habitat_Meadow, "Habitat.Meadow", "초원");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Habitat_Field, "Habitat.Field", "밭 둘레");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Habitat_Waterside, "Habitat.Waterside", "개울 · 연못 가");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Habitat_Road, "Habitat.Road", "밤길 · 길가");
}
