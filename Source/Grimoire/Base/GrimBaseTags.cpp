#include "Base/GrimBaseTags.h"

namespace GrimBaseTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_WandBroken, "Status.WandBroken", "완드가 부러짐(Q22) — 마법 시전 불가, 주먹만. 수리로 되살린다");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Spell_Repair, "Spell.Repair", "「수리」 — 무속성 마법. 조각 · 시설 · 장비를 재료로 고친다(R18 · R28)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Cooldown_Spell_Repair, "Cooldown.Spell.Repair", "「수리」 쿨다운");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Material_Wood, "Material.Wood", "건축 나무 — 센 불에만 붙어 잠시 타며 옆 나무 조각으로 번진다(모닥불 곁불로는 안 붙음)");
}
