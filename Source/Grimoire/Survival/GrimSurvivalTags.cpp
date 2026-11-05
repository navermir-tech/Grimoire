#include "Survival/GrimSurvivalTags.h"

namespace GrimSurvivalTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Hungry, "Status.Hungry", "배고픔이 임계값 아래. HP 자연 회복 · 마나 회복 정지");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Starving, "Status.Starving", "배고픔 0. HP 가 서서히 줄어든다");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Chilly, "Status.Chilly", "추위 모자람 1 — 쌀쌀함. 이동 · 시전 조금 느려짐");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Hypothermia, "Status.Hypothermia", "추위 모자람 2 이상 — 저체온. 이동 · 시전 느려짐, 피곤함 가속");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Hypothermia_Severe, "Status.Hypothermia.Severe", "추위 모자람 3 이상 — 심한 저체온. HP 감소");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Warm, "Status.Warm", "더위 모자람 1 — 더움. 마나 소모 조금 늘어남");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Hyperthermia, "Status.Hyperthermia", "더위 모자람 2 이상 — 과열. 마나 소모 늘어남, 피곤함 가속");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Hyperthermia_Severe, "Status.Hyperthermia.Severe", "더위 모자람 3 이상 — 열사병. HP 감소");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Exhausted, "Status.Exhausted", "피곤함 80 이상. 시전 속도 −30%");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Collapsed, "Status.Collapsed", "피곤함 100 — 기절(강제 수면)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Sleeping, "Status.Sleeping", "자는 중(침대 · 침낭 · 기절). 움직이지 못한다");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Downed, "Status.Downed", "HP 0 — 쓰러짐. 동료가 일으키지 않으면 사망 후 부활");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Sick, "Status.Sick", "질병(하위 태그가 종류). 빛 마법 정화가 이 태그를 지운다(나중)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Sick_Stomach, "Status.Sick.Stomach", "배탈 — 날것 섭취. 배고픔이 빨리 준다");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Sick_Cold, "Status.Sick.Cold", "감기 — 젖은 채 추위. 마나 회복 감소, 시전 가끔 끊김(마법 쪽이 읽는다)");
}
