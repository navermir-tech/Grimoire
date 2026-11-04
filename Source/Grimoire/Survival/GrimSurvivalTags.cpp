#include "Survival/GrimSurvivalTags.h"

namespace GrimSurvivalTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Hungry, "Status.Hungry", "배고픔이 임계값 아래. HP 자연 회복 · 마나 회복 정지");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Starving, "Status.Starving", "배고픔 0. HP 가 서서히 줄어든다");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Hypothermia, "Status.Hypothermia", "체온이 안전 범위 아래. HP 감소");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Hyperthermia, "Status.Hyperthermia", "체온이 안전 범위 위. HP 감소");
}
