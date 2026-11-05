#pragma once

#include "CoreMinimal.h"
#include "Creatures/GrimCreatureTypes.h"

class UGrimCreatureSpecies;
struct FGrimCreatureRollTuning;
struct FGrimCreatureAITuning;

/** AI 가 한 번 판단할 때 보는 것(서버가 모아 넣는다 — 순수 계산은 이것만 본다) */
struct FGrimCreatureSenses
{
	float HealthRatio = 1.f;
	bool bBurning = false;
	bool bActiveTime = true;
	/** 플레이어 · 포식자가 알아채는 거리 안에 있다 */
	bool bThreatNear = false;
	/** 공격적 종이 덤빌 거리 안에 플레이어가 있다 */
	bool bHostileInAggroRange = false;
	/** 최근 누군가에게 맞았고 그 상대가 아직 있다 */
	bool bProvoked = false;
	/** 사냥할 먹이가 보인다 */
	bool bPreyNear = false;
	/** 피하는 불빛이 가깝다(bAvoidsLight 종만 의미) */
	bool bLightNear = false;
	/** 계약에 저항해 화났다(Creature.State.Enraged — 갈래 T) — 겁 많은 종도 덤빈다 */
	bool bEnraged = false;
};

/** 종의 기질(종 정의에서) */
struct FGrimCreatureTemperamentInfo
{
	EGrimCreatureTemperament Temperament = EGrimCreatureTemperament::Timid;
	EGrimCreatureDiet Diet = EGrimCreatureDiet::Herbivore;
	float FleeHealthRatio = 0.25f;
	bool bAvoidsLight = false;
	bool bCanAttack = true;
};

/** 생물 순수 계산 — 개체 굴림 · 전리품 · 활동 시간 · AI 판단 · 개체군. 액터 · 월드를 모른다(테스트가 직접 부른다). */
namespace GrimCreatureMath
{
	/** 개체 하나를 굴린다. 같은 종 + 같은 씨앗 = 같은 결과. */
	GRIMOIRE_API FGrimCreatureInstance RollInstance(const UGrimCreatureSpecies& Species, int32 Seed, const FGrimCreatureRollTuning& Tuning);

	/** 색조 ±HueDeg · 채도 ±Sat · 밝기 ±Val 흔들림 */
	GRIMOIRE_API FLinearColor JitterColor(const FLinearColor& Base, FRandomStream& Rng, float HueDeg, float Sat, float Val);

	/** 크기 → 능력치 배율 */
	GRIMOIRE_API float CalcStatScale(float SizeScale, bool bGiant, const FGrimCreatureRollTuning& Tuning);

	/** 이 성별에 붙는 부위인가 */
	GRIMOIRE_API bool IsPartForSex(EGrimCreatureSexFilter Filter, EGrimCreatureSex Sex);

	/** 성별에 맞는 팔레트(덮어쓰기가 비면 기본) */
	GRIMOIRE_API const TArray<FLinearColor>& GetPaletteForSex(const FGrimCreatureColorRegion& Region, EGrimCreatureSex Sex);

	GRIMOIRE_API TArray<FGrimCreatureLootDrop> RollLoot(const TArray<FGrimCreatureLootEntry>& Table, bool bGiant, float GiantMultiplier, FRandomStream& Rng);

	/** 지금 활동 시간인가 */
	GRIMOIRE_API bool IsActiveTime(EGrimCreatureActivity Activity, bool bNight, float Hours, float DuskStartHour, float DuskEndHour);

	/**
	 * AI 판단(우선순위): 불붙음 → 도망 / 빛 피함 → 도망 / 체력 낮음 + 위협 → 도망 / 겁 많음 + 위협 · 맞음 → 도망 /
	 * 맞음 → 싸움 / 공격적 + 가까움 + 활동 시간 → 싸움 / 활동 시간 밖 → 잠 / 육식 + 먹이 → 사냥 / 그 밖 → Idle(한가함).
	 */
	GRIMOIRE_API EGrimCreatureAIState Decide(const FGrimCreatureSenses& Senses, const FGrimCreatureTemperamentInfo& Info);

	/** 한가할 때 활동 하나(가만히 · 배회 · 풀 뜯기). Roll = 0~1 난수. 풀 뜯기는 초식만. */
	GRIMOIRE_API EGrimCreatureAIState PickCalmActivity(float Roll, EGrimCreatureDiet Diet, const FVector& Weights);

	/**
	 * 개체군 회복: 현재 < 상한이면 타이머를 Dt 만큼 진행하고, RegrowSeconds 마다 하나씩 늘 몫을 돌려준다(상한까지).
	 * 상한이면 타이머를 0 으로 둔다.
	 */
	GRIMOIRE_API int32 StepRegrow(int32 Current, int32 Cap, float& InOutTimer, float Dt, float RegrowSeconds);

	/** 접힌 기록을 펼치나 — 가장 가까운 플레이어 거리 · 반경 · 활동 시간 · 숨는 종 · 전역 상한 */
	GRIMOIRE_API bool ShouldUnfold(float NearestPlayerDistance, float SpawnRadius, bool bActiveTime, bool bHideOutsideActiveTime, int32 TotalActive, int32 MaxActive);

	/** 펼친 액터를 접나 */
	GRIMOIRE_API bool ShouldFold(float NearestPlayerDistance, float DespawnRadius, bool bActiveTime, bool bHideOutsideActiveTime);

	/** 무리 크기 하나를 굴린다(최소~최대, 남은 수를 넘지 않음) */
	GRIMOIRE_API int32 RollHerdSize(int32 Min, int32 Max, int32 Remaining, FRandomStream& Rng);
}
