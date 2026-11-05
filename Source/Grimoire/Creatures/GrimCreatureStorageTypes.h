#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Creatures/GrimTamingTypes.h"
#include "GrimCreatureStorageTypes.generated.h"

/** 길들인 생물이 있는 곳(스펙 「데리고 다니기 · 보관함 · 거점 생물 상자」 R38) */
UENUM(BlueprintType)
enum class EGrimCreaturePlace : uint8
{
	/** 데리고 다님 — 세계에서 주인을 따라오거나 기다린다(파티 수에 든다) */
	Party,
	/** 생물 보관함(노말 스킬) — 세계에 없다 */
	Storage,
	/** 거점 생물 상자 안 — 세계에 없다 */
	Box,
	/** 거점 생물 상자에서 풀어놓음 — 상자 둘레를 돌아다닌다(파티 수에 들지 않는다) */
	Released
};

/** 옮기기 · 길들이기 자리 결과 */
UENUM(BlueprintType)
enum class EGrimCreatureStorageResult : uint8
{
	Ok,
	/** 생물 보관함을 아직 배우지 않았다 */
	NotLearned,
	/** 데리고 다니는 수가 가득 */
	PartyFull,
	/** 보관함 칸이 가득 */
	StorageFull,
	/** 상자 칸이 가득 */
	BoxFull,
	/** 상자의 풀어놓기 상한 */
	ReleasedFull,
	/** 그 생물이 그 자리에 없다 */
	NotFound,
	/** 상자가 없거나 너무 멀다 */
	NoBox,
	/** 옮길 수 없는 길(같은 곳 · 잘못된 인자) */
	Invalid
};

/** 새로 길들인 생물이 갈 곳(굴림 전에 정한다) */
UENUM(BlueprintType)
enum class EGrimNewTameDestination : uint8
{
	/** 데리고 다니는 수에 자리가 있다 — 그대로 곁에 */
	Party,
	/** 파티가 가득 — 보관함으로 */
	Storage,
	/** 파티 · 보관함이 가득(또는 보관함 안 배움) — 세계의 거점 생물 상자로 */
	Box,
	/** 갈 곳이 없다 — 계약이 걸리지 않는다 */
	Refused
};

/** 서클 구간 하나의 값(MinCircle 이상이면 Value). 표는 MinCircle 오름차순이 아니어도 된다 — 맞는 것 중 MinCircle 이 가장 큰 것 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimCircleLimitStep
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Limit", meta = (ClampMin = "1"))
	int32 MinCircle = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Limit", meta = (ClampMin = "0"))
	int32 Value = 0;
};

/** 태그 하나가 있으면 더하는 값(성장 트리 노드 효과 태그 Progression.Node.<키> — 진행이 상한을 올리는 자리) */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimTagLimitBonus
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Limit")
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Limit")
	int32 Bonus = 1;
};

/** 거점 생물 상자의 한 칸 — 개체 기록 그대로(가진 개체 기록) + 풀어놓았나 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimCreatureBoxEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Creature")
	FGrimOwnedCreatureRecord Record;

	/** 풀어놓아 상자 둘레를 돌아다닌다(세계에 액터가 있다) */
	UPROPERTY(BlueprintReadOnly, Category = "Creature")
	bool bReleased = false;
};
