#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "InputCoreTypes.h"
#include "Creatures/GrimCreatureStorageTypes.h"
#include "GrimCreatureStorageSettings.generated.h"

/**
 * 생물 데리고 다니기 · 보관함 · 거점 생물 상자 수치(R38) — 정본은 Config/DefaultGame.ini 의 [/Script/Grimoire.GrimCreatureStorageSettings]
 * (docs/systems/creatures 「데리고 다니기 · 보관함 · 거점 상자」). 모두 제안값(Q60~Q64). 노말 스킬 이름 = 쿠스토디아(Q42).
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Grimoire Creature Storage"))
class GRIMOIRE_API UGrimCreatureStorageSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UGrimCreatureStorageSettings* Get() { return GetDefault<UGrimCreatureStorageSettings>(); }

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	// ── 데리고 다니는 수(팰월드 파티 참고) ──

	/** 서클 구간별 데리고 다니는 수(맞는 것 중 MinCircle 이 가장 큰 것). 비면 1 */
	UPROPERTY(Config, EditAnywhere, Category = "Party")
	TArray<FGrimCircleLimitStep> PartyLimitByCircle;

	/** 플레이어 ASC 에 이 태그가 있으면 더한다(성장 트리 노드 효과 Progression.Node.CreatureParty …) */
	UPROPERTY(Config, EditAnywhere, Category = "Party")
	TArray<FGrimTagLimitBonus> PartyLimitTagBonuses;

	/** 전체 상한(서버 부담) */
	UPROPERTY(Config, EditAnywhere, Category = "Party", meta = (ClampMin = "1"))
	int32 PartyLimitMax = 8;

	// ── 생물 보관함(노말 스킬, 호그와트 레거시 납색 참고) ──

	/** 노말 스킬 ID — 스킬북 아이템 TeachesSpell = "Skill:<이 값>" */
	UPROPERTY(Config, EditAnywhere, Category = "Storage")
	FName StorageSkillId = TEXT("CreatureStorage");

	/** 보관함을 배우는 데 필요한 서클 */
	UPROPERTY(Config, EditAnywhere, Category = "Storage", meta = (ClampMin = "1"))
	int32 StorageRequiredCircle = 1;

	/** 쿠스토디아를 배우지 않아도 처음부터 쓰는 보관함 칸 수(Q42 답 2026-11-07 — 인벤토리처럼). 배우면 아래 서클 표 + 노드만큼 늘어난다 */
	UPROPERTY(Config, EditAnywhere, Category = "Storage", meta = (ClampMin = "0"))
	int32 StorageBaseCapacity = 10;

	/** 쿠스토디아를 배웠을 때 기본 칸에 더하는 칸 수(서클 구간별) */
	UPROPERTY(Config, EditAnywhere, Category = "Storage")
	TArray<FGrimCircleLimitStep> StorageCapacityByCircle;

	UPROPERTY(Config, EditAnywhere, Category = "Storage")
	TArray<FGrimTagLimitBonus> StorageCapacityTagBonuses;

	/** 보관함 전체 칸 상한(기본 칸 포함) */
	UPROPERTY(Config, EditAnywhere, Category = "Storage", meta = (ClampMin = "1"))
	int32 StorageCapacityMax = 40;

	/** 보관함 · 상자 안에서 1분에 차는 체력(최대 체력 비율) — 꺼낼 때 지난 시간만큼 채운다 */
	UPROPERTY(Config, EditAnywhere, Category = "Storage", meta = (ClampMin = "0"))
	float StoredHealPerMinute = 0.2f;

	// ── 거점 생물 상자(팰월드 팰 상자 참고) ──

	/** 상자 하나의 칸 수(모든 플레이어 합) */
	UPROPERTY(Config, EditAnywhere, Category = "Box", meta = (ClampMin = "1"))
	int32 BoxCapacity = 60;

	/** 상자 둘레 이 거리 안에서 관리 화면을 연다(F) */
	UPROPERTY(Config, EditAnywhere, Category = "Box", meta = (Units = "cm"))
	float BoxManageRadius = 300.f;

	/** 서버 거리 검사 여유(클라이언트 위치 오차) */
	UPROPERTY(Config, EditAnywhere, Category = "Box", meta = (Units = "cm"))
	float BoxServerReachSlack = 250.f;

	/** 풀어놓은 생물이 돌아다니는 반경(상자 기준). M4 거점 핵이 생기면 거점 반경으로 바꾼다 */
	UPROPERTY(Config, EditAnywhere, Category = "Box", meta = (Units = "cm"))
	float BoxRoamRadius = 2000.f;

	/** 상자 하나에 풀어놓을 수 있는 수 */
	UPROPERTY(Config, EditAnywhere, Category = "Box", meta = (ClampMin = "0"))
	int32 BoxMaxReleased = 10;

	/** 풀어놓은 생물이 집(내놓은 곳) 둘레를 배회하는 반경 */
	UPROPERTY(Config, EditAnywhere, Category = "Box", meta = (Units = "cm"))
	float RoamWanderRadius = 600.f;

	/** 풀어놓은 생물이 반경을 벗어나면 집으로 되돌린다(이 배수 × BoxRoamRadius 밖) */
	UPROPERTY(Config, EditAnywhere, Category = "Box", meta = (ClampMin = "1"))
	float RoamLeashScale = 1.5f;

	/** 데리고 다니는 수 · 보관함이 가득일 때 새로 길들인 생물을 세계의 거점 상자로 보낸다(팰월드처럼) */
	UPROPERTY(Config, EditAnywhere, Category = "Box")
	bool bSendOverflowToBox = true;

	// ── 갱신 ──

	/** 서버가 상한(서클 · 태그)을 다시 셈하는 주기 */
	UPROPERTY(Config, EditAnywhere, Category = "Party", meta = (Units = "s", ClampMin = "0.2"))
	float LimitRefreshSeconds = 1.f;

	/** 노말 스킬 스킬북 표기(아이템 표 TeachesSpell) 앞 이름 */
	static const TCHAR* SkillPrefix() { return TEXT("Skill:"); }
};
