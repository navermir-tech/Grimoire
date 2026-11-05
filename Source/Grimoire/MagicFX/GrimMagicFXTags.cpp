#include "MagicFX/GrimMagicFXTags.h"

namespace GrimMagicFXTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Swimming, "State.Swimming", "헤엄 중 — 그리모어를 허리춤에 거둔다(Q25)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_BuildMode, "State.BuildMode", "건축 모드 — 그리모어를 거둔다(Q25)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Carrying, "State.Carrying", "두 손으로 나르는 중 — 그리모어를 거둔다(Q25)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Mounted, "State.Mounted", "탈것 위 — 조준 · 시전이 없으면 그리모어를 거둔다(Q25)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Aiming, "State.Aiming", "조준 중 — 탈것 위에서도 그리모어를 띄운다(Q25)");
}
