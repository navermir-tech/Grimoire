#include "World/GrimTerrainTags.h"

namespace GrimTerrainTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Terrain_Site_Tower, "Terrain.Site.Tower", "마법탑 자리(남쪽 언덕 꼭대기 평지)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Terrain_Site_TowerYard, "Terrain.Site.TowerYard", "탑 앞마당");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Terrain_Site_Base, "Terrain.Site.Base", "거점 세울 만한 평지");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Terrain_Path, "Terrain.Path", "길 꺾은점(큰길 · 갈림길)");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Habitat_Jackalope, "Habitat.Jackalope", "재칼로프 서식 구역(초원)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Habitat_Kelpie, "Habitat.Kelpie", "켈피 서식 구역(연못 · 개울)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Habitat_BlackDog, "Habitat.BlackDog", "검은 개 서식 구역(계곡 어두운 숲)");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Landmark, "Landmark", "랜드마크 — 멀리서 방향을 잡는 기준(docs/dev/level-design.md 「2」)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Landmark_Tower, "Landmark.Tower", "마법탑(전역, 가는 수직)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Landmark_GiantTree, "Landmark.GiantTree", "거대 고목(전역, 둥근 큰 덩어리)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Landmark_RockGate, "Landmark.RockGate", "북쪽 고개 바위 문(전역, 두 기둥 사이 틈)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Landmark_StoneCircle, "Landmark.StoneCircle", "선돌 고리(구역, 고리)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Landmark_Waterfall, "Landmark.Waterfall", "폭포(구역, 흰 세로 띠)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Landmark_Watchtower, "Landmark.Watchtower", "폐허 감시탑(구역, 부러진 원통)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Landmark_Windmill, "Landmark.Windmill", "풍차(구역, 날개)");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Poi, "Poi", "들를 곳(POI) — 자리와 실루엣만, 놀이는 각 마일스톤(docs/dev/level-design.md 「5」)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Poi_Puzzle, "Poi.Puzzle", "속성 퍼즐(꺼진 화로 · 마른 덩굴 · 잠긴 제단)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Poi_Resource, "Poi.Resource", "자원 군락(약초 · 광석 · 나무)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Poi_Creature, "Poi.Creature", "생물 자리(굴 · 둥지 · 연못)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Poi_Ruin, "Poi.Ruin", "작은 폐허 · 보물");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Poi_Viewpoint, "Poi.Viewpoint", "전망대(위에서 다른 POI 를 본다)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Poi_Story, "Poi.Story", "이야기 흔적(야영 흔적 · 버려진 짐)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Poi_BaseSite, "Poi.BaseSite", "거점 터(물 · 평지 · 경치)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Poi_Dungeon, "Poi.Dungeon", "던전 입구(동굴 · 지하 계단)");
}
