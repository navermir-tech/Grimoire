#pragma once

#include "CoreMinimal.h"
#include "Creatures/GrimTamingTypes.h"

class UGrimTamingSettings;
struct FGrimCaptureTuning;
struct FGrimLoyaltyTuning;
struct FGrimPersonalityTuning;
struct FGrimTamedBehaviorTuning;

/** 길들인 생물 AI 가 한 번 판단할 때 보는 것 */
struct FGrimTamedSenses
{
	bool bRidden = false;
	bool bKnockedOut = false;
	bool bBurning = false;
	/** 주인이 아닌 누군가(생물 · 다른 플레이어)에게 맞았고 그 상대가 아직 있다 */
	bool bProvoked = false;
	bool bCanAttack = true;
	/** 겁 많은 종(Timid) */
	bool bTimid = false;
	float HealthRatio = 1.f;
	float FleeHealthRatio = 0.25f;
	EGrimTamedCommand Command = EGrimTamedCommand::Follow;
	/** 주인 폰까지 거리(주인이 없으면 음수) */
	float OwnerDistance = -1.f;
	float FollowDistance = 350.f;
};

/** 길들이기 · 충성도 · 탑승 순수 계산 — 액터 · 월드를 모른다(테스트가 직접 부른다). */
namespace GrimTamingMath
{
	/**
	 * 포획 확률 = 기본 + 약해짐(+ 약한 정도) + 잠듦 + 지침 + 먹이(좋아하는 먹이면 더 큰 몫) + 속성 일치 + 완드 친화 + 서클 차
	 *           + 숙련도 추가 효과 + 장신구 − 등급(크기 분류 · 거대), Min~Max 로 자른다. 요구 서클보다 MaxCircleDeficit 넘게 낮으면 bCircleTooLow.
	 * WeakenedRatio = 생물 설정 WeakenedHealthRatio(0.3).
	 */
	GRIMOIRE_API FGrimCaptureChance CalcCaptureChance(const FGrimCaptureInput& Input, const FGrimCaptureTuning& Tuning, float WeakenedRatio);

	/** 굴림(0~1)이 확률보다 작으면 성공 */
	GRIMOIRE_API bool RollCapture(float Chance, float Roll);

	/** 충성도 → 단계(Stages 의 MinLoyalty, 0 이하 = Gone) */
	GRIMOIRE_API EGrimLoyaltyStage GetStage(float Loyalty, const FGrimLoyaltyTuning& Tuning);

	/** 오르는 사건인가(기본 변화량 > 0) */
	GRIMOIRE_API bool IsGainEvent(EGrimLoyaltyEvent Event, const FGrimLoyaltyTuning& Tuning);

	/** 성격 배율: 사건별 배율 → 없으면 오름 · 내림 배율. Personality 가 null 이면 1 */
	GRIMOIRE_API float GetPersonalityScale(const FGrimPersonalityTuning* Personality, EGrimLoyaltyEvent Event, bool bGain);

	/**
	 * 사건 하나를 적용한다: 변화 = 기본 변화량 × 성격 배율 × Scale, 0~100 으로 자른다.
	 * 실제 바뀐 값을 돌려주고, 0 이 아니면 Reasons 맨 앞에 넣는다(최대 RecentReasonCount).
	 */
	GRIMOIRE_API float ApplyLoyaltyEvent(float& InOutLoyalty, TArray<FGrimLoyaltyReason>& InOutReasons, EGrimLoyaltyEvent Event,
		const FGrimLoyaltyTuning& Tuning, const FGrimPersonalityTuning* Personality, float Scale = 1.f);

	/** 까닭을 맨 앞에 넣고 Count 개만 남긴다 */
	GRIMOIRE_API void PushReason(TArray<FGrimLoyaltyReason>& InOutReasons, const FGrimLoyaltyReason& Reason, int32 Count);

	/** 명령을 따르나(단계 순종 확률 vs 굴림 0~1) */
	GRIMOIRE_API bool RollObedience(EGrimLoyaltyStage Stage, const FGrimLoyaltyTuning& Tuning, float Roll);

	/** 타기를 거부하나 */
	GRIMOIRE_API bool RollMountRefuse(EGrimLoyaltyStage Stage, const FGrimLoyaltyTuning& Tuning, float Roll);

	/** 탄 사람을 떨어뜨리나(BuckCheckSeconds 한 번) */
	GRIMOIRE_API bool RollBuck(EGrimLoyaltyStage Stage, const FGrimLoyaltyTuning& Tuning, float Roll);

	/** 탑승 속도 배율(단계) */
	GRIMOIRE_API float GetMountSpeedScale(EGrimLoyaltyStage Stage, const FGrimLoyaltyTuning& Tuning);

	/** 작업 속도 배율(단계 × 성격) — M5 일꾼 자리 */
	GRIMOIRE_API float GetWorkSpeedScale(EGrimLoyaltyStage Stage, const FGrimLoyaltyTuning& Tuning, const FGrimPersonalityTuning* Personality);

	/** 접속하지 않은 시간 → 주인 부재 사건 수(한 시간에 하나, 상한) */
	GRIMOIRE_API int32 GetOfflineAbsentEvents(double OfflineSeconds, int32 MaxEvents);

	/**
	 * 길들인 생물 판단(우선순위): 탔음 → Ridden / 기절 → Sleep / 불붙음 → Flee / 맞음 → 체력 낮거나 겁 많으면 Flee, 아니면 Fight /
	 * 기다리기 → Stay / 따라오기 → 멀면 Follow, 가까우면 Idle.
	 */
	GRIMOIRE_API EGrimCreatureAIState DecideTamed(const FGrimTamedSenses& Senses);

	/** 가진 개체 기록 하나를 쓰고 읽는다(세이브 Creatures.Owned v1 의 한 항목). 읽기에 실패하면 false */
	GRIMOIRE_API bool SerializeOwnedRecord(FArchive& Ar, FGrimOwnedCreatureRecord& Record);

	/** 기록 목록(수 + 항목). 읽기에 실패하면 false */
	GRIMOIRE_API bool SerializeOwnedRecords(FArchive& Ar, TArray<FGrimOwnedCreatureRecord>& Records);

	/** 이름 다듬기(앞뒤 공백 · 줄바꿈 · 길이) */
	GRIMOIRE_API FString SanitizeNickname(const FString& Raw, int32 MaxLength);

	GRIMOIRE_API const TCHAR* EventName(EGrimLoyaltyEvent Event);
	/** 화면용 한국어 까닭 */
	GRIMOIRE_API FString EventDisplayText(EGrimLoyaltyEvent Event);
}
