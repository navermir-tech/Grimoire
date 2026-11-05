#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Creatures/GrimCreatureTypes.h"
#include "GrimTamingTypes.generated.h"

GRIMOIRE_API DECLARE_LOG_CATEGORY_EXTERN(LogGrimTaming, Log, All);

/** 충성도 단계(스펙 「충성도」 표). 태그 Creature.Loyalty.<이름> 이 생물 ASC 에 붙는다. */
UENUM(BlueprintType)
enum class EGrimLoyaltyStage : uint8
{
	/** 0 — 야생으로 돌아간다 */
	Gone,
	/** 1~19 — 일을 거부하고, 태운 사람을 떨어뜨리기도 한다 */
	Rebellious,
	/** 20~49 — 작업이 느리고 명령을 가끔 무시한다 */
	Discontent,
	/** 50~79 — 기본 */
	Normal,
	/** 80~100 — 작업 · 탑승 속도 ↑, 명령을 잘 따른다 */
	Devoted
};

/** 충성도를 오르내리게 하는 사건(스펙 「오르는 것 · 내리는 것」 표). 값(변화량)은 설정 EventDeltas, 성격 배율은 Personalities. */
UENUM(BlueprintType)
enum class EGrimLoyaltyEvent : uint8
{
	None,
	/** 길들임(처음 값 — 까닭 줄에만) */
	Tamed,
	Fed,
	/** 좋아하는 먹이(두 배) */
	FedFavorite,
	/** 잠자리 · 쉬는 시간(기다리기 · 잠) */
	Rested,
	/** 다쳤을 때 치료(체력이 오름 — 신성 치유 마법 R16 자리) */
	Healed,
	/** 함께 싸워 이김(곁에서 주인 · 이 생물이 쓰러뜨림) */
	WonTogether,
	/** 함께 탐험(주인과 함께 먼 곳까지) */
	Explored,
	/** 맞는 환경(서식지가 맞는 구역) */
	GoodEnvironment,
	/** 오래 함께 지낸 날 수(천천히) */
	DaysTogether,
	/** 굶김 */
	Starved,
	/** 혹사(지쳤는데 계속 싸움 · 탑승) */
	Overworked,
	/** 다친 채 내버려 둠 */
	LeftInjured,
	/** 쓰러지게 함(전투에서 기절) */
	KnockedOut,
	/** 맞지 않는 환경(불 생물이 젖음 · 물 생물이 불붙음) */
	BadEnvironment,
	/** 다른 생물과 자꾸 다툼 */
	Quarrel,
	/** 주인이 오래 없음(멀리 · 접속하지 않음) */
	OwnerAbsent
};

/** 길들인 생물에게 내리는 명령(놓아주기 · 타기는 따로) */
UENUM(BlueprintType)
enum class EGrimTamedCommand : uint8
{
	Follow,
	Wait,
	/** 거점 생물 상자에서 풀어놓음 — 집(상자 둘레) 곁을 돌아다닌다(R38) */
	Roam
};

/** 포획 판정 결과 */
UENUM(BlueprintType)
enum class EGrimTamingResult : uint8
{
	Success,
	/** 굴림 실패 — 저항(생물이 화내 덤빈다) */
	Failed,
	/** 길들일 수 없는 종 · 죽음 · 이미 주인이 있음 */
	NotTameable,
	/** 내 서클이 종의 요구 서클보다 너무 낮다 */
	CircleTooLow,
	/** 데리고 다니는 수 · 보관함 · 거점 상자가 모두 가득 — 굴림 전에 막는다(R38) */
	NoRoom,
	Invalid
};

/** 최근에 오르내린 까닭 한 줄 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimLoyaltyReason
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Taming")
	EGrimLoyaltyEvent Event = EGrimLoyaltyEvent::None;

	/** 실제로 바뀐 값(성격 배율 · 0~100 자르기 뒤) */
	UPROPERTY(BlueprintReadOnly, Category = "Taming")
	float Delta = 0.f;

	bool operator==(const FGrimLoyaltyReason& Other) const { return Event == Other.Event && FMath::IsNearlyEqual(Delta, Other.Delta, 1e-3f); }
};

/** 플레이어가 가진 개체 하나의 기록 — 세이브(Player, Creatures.Owned) · 접속 때 다시 부르기에 쓴다 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimOwnedCreatureRecord
{
	GENERATED_BODY()

	/** 개체 기록 그대로(OwnerId · Loyalty · Nickname 포함) */
	UPROPERTY(BlueprintReadOnly, Category = "Taming")
	FGrimCreatureInstance Instance;

	UPROPERTY(BlueprintReadOnly, Category = "Taming")
	float HealthRatio = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "Taming")
	EGrimTamedCommand Command = EGrimTamedCommand::Follow;

	/** 최근 까닭(최대 3) */
	UPROPERTY(BlueprintReadOnly, Category = "Taming")
	TArray<FGrimLoyaltyReason> Reasons;

	/** 마지막으로 기록한 때(UTC, 초) — 주인이 오래 없음 계산 · 보관함 · 상자에 넣은 때(체력 회복 계산, 화면이 쓰려고 복제) */
	UPROPERTY()
	double SavedUtcSeconds = 0.0;

	/** 같은 개체인가(종 + 씨앗) */
	bool IsSameCreature(const FGrimCreatureInstance& Other) const { return Instance.SpeciesId == Other.SpeciesId && Instance.Seed == Other.Seed; }
};

/** 포획 확률 계산의 입력(서버가 모아 넣는다 — 순수 계산은 이것만 본다) */
struct FGrimCaptureInput
{
	float HealthRatio = 1.f;
	bool bAsleep = false;
	/** 지침(Creature.State.Exhausted — 오래 달리거나 싸움) */
	bool bExhausted = false;
	/** 최근 먹이를 줬다(FedMemorySeconds 안) */
	bool bFed = false;
	bool bFedFavorite = false;
	/** 파크툼 속성이 그 생물 속성(잘 듣는 파크툼 · 주 · 부)과 맞다 */
	bool bElementMatch = false;
	/** 완드의 그 생물 주속성 위력 배율(1 = 친화 없음) */
	float WandElementPower = 1.f;
	int32 CasterCircle = 1;
	int32 RequiredCircle = 1;
	EGrimCreatureSizeClass SizeClass = EGrimCreatureSizeClass::Small;
	bool bGiant = false;
	/** 계약 마법 숙련도 추가 효과(Q26 — ProficiencyExtraEffect 값, 확률 몫) */
	float ProficiencyBonus = 0.f;
	/** 장신구 자리(M4) */
	float AccessoryBonus = 0.f;
};

/** 포획 확률의 몫별 값(로그 · 화면 · 테스트) */
struct GRIMOIRE_API FGrimCaptureChance
{
	float Base = 0.f;
	float Weakened = 0.f;
	float Asleep = 0.f;
	float Exhausted = 0.f;
	float Food = 0.f;
	float Element = 0.f;
	float Wand = 0.f;
	float Circle = 0.f;
	float Proficiency = 0.f;
	float Accessory = 0.f;
	/** 등급(크기 분류 · 거대) 빼기 — 양수로 담는다 */
	float GradePenalty = 0.f;
	/** 자르기 전 합 */
	float Raw = 0.f;
	/** 최종(Min~Max) */
	float Total = 0.f;
	bool bCircleTooLow = false;

	FString ToString() const;
};
