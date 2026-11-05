#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Creatures/GrimCreatureStorageTypes.h"

/** 옮기기 판정이 보는 수(서버가 모은다 — 순수 계산은 이것만 본다) */
struct FGrimCreatureStorageCounts
{
	int32 PartyCount = 0;
	int32 PartyLimit = 0;
	bool bStorageLearned = false;
	int32 StorageCount = 0;
	int32 StorageCapacity = 0;
	/** 곁에 쓸 수 있는 거점 상자가 있다 */
	bool bHasBox = false;
	/** 상자의 모든 칸(풀어놓은 것 포함, 모든 플레이어) */
	int32 BoxCount = 0;
	int32 BoxCapacity = 0;
	int32 ReleasedCount = 0;
	int32 ReleasedMax = 0;
};

/** 데리고 다니기 · 보관함 · 거점 상자 순수 계산(R38) — 액터 · 월드를 모른다(테스트가 직접 부른다). */
namespace GrimCreatureStorageMath
{
	/** 서클 구간 표에서 값(맞는 것 중 MinCircle 이 가장 큰 것). 맞는 것이 없으면 Fallback */
	GRIMOIRE_API int32 GetCircleValue(const TArray<FGrimCircleLimitStep>& Steps, int32 Circle, int32 Fallback);

	/** 서클 값 + 가진 태그마다 보너스, 0~Max 로 자른다 */
	GRIMOIRE_API int32 ComputeLimit(const TArray<FGrimCircleLimitStep>& Steps, int32 Circle, const TArray<FGrimTagLimitBonus>& TagBonuses,
		const FGameplayTagContainer& OwnedTags, int32 Max, int32 Fallback);

	/** 새로 길들인 생물이 갈 곳: 파티 자리 → 보관함(배웠고 자리) → 상자(있고 자리) → 거절 */
	GRIMOIRE_API EGrimNewTameDestination DecideNewTame(const FGrimCreatureStorageCounts& Counts, bool bAllowBox);

	/** From → To 로 옮길 수 있나(자리 · 배움 · 상자). 된다면 Ok */
	GRIMOIRE_API EGrimCreatureStorageResult CheckTransfer(EGrimCreaturePlace From, EGrimCreaturePlace To, const FGrimCreatureStorageCounts& Counts);

	/** 보관 중 회복: 넣은 때(UTC 초)부터 지금까지 분당 PerMinute 만큼(0.01~1). 넣은 때가 없으면(0) 그대로 */
	GRIMOIRE_API float GetHealedRatio(float HealthRatio, double StoredUtcSeconds, double NowUtcSeconds, float PerMinute);

	/** 같은 개체(종 + 씨앗)의 번호. 없으면 INDEX_NONE */
	GRIMOIRE_API int32 FindRecord(const TArray<FGrimOwnedCreatureRecord>& Records, FName SpeciesId, int32 Seed);
	GRIMOIRE_API int32 FindBoxEntry(const TArray<FGrimCreatureBoxEntry>& Entries, FName SpeciesId, int32 Seed);

	/** 상자 칸 목록을 쓰고 읽는다(세이브 CreatureBox v1: int32 수, 항목마다 bool 풀어놓음 + 가진 개체 기록). 읽기에 실패하면 false */
	GRIMOIRE_API bool SerializeBoxEntries(FArchive& Ar, TArray<FGrimCreatureBoxEntry>& Entries);

	/** 스킬북 TeachesSpell 이 노말 스킬("Skill:<ID>")이면 ID 를 꺼낸다 */
	GRIMOIRE_API bool ParseNormalSkill(FName TeachesSpell, FName& OutSkillId);

	/** 화면 · 로그 글 */
	GRIMOIRE_API FString ResultText(EGrimCreatureStorageResult Result);
	GRIMOIRE_API const TCHAR* PlaceName(EGrimCreaturePlace Place);
	GRIMOIRE_API FString PlaceDisplayName(EGrimCreaturePlace Place);
	/** 콘솔 인자 → 자리(Party · Storage · Box · Released, 대소문자 무시). 못 읽으면 false */
	GRIMOIRE_API bool ParsePlace(const FString& Text, EGrimCreaturePlace& OutPlace);
}
