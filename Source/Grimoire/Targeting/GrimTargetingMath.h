#pragma once

#include "CoreMinimal.h"
#include "Core/GrimTargetingTypes.h"

struct FGrimTargetScoreWeights;

/** 점수 · 우선순위에 쓰는 대상 분류. EGrimTargetKind + 「지금 마법에 반응하는가」. */
enum class EGrimTargetCategory : uint8
{
	/** 대상이 아님(아군 · 종류 없음) */
	None,
	Enemy,
	Creature,
	/** 지금 고른 마법의 속성에 반응하는 사물 */
	ReactiveObject,
	Object
};

/** 소프트 락온 후보 하나. 순수 계산이 쓰는 값만 담는다(액터는 결과를 되찾는 데만 쓴다). */
struct FGrimTargetCandidate
{
	TWeakObjectPtr<AActor> Actor;
	EGrimTargetKind Kind = EGrimTargetKind::None;
	EGrimTargetCategory Category = EGrimTargetCategory::None;
	/** 조준점(월드) */
	FVector Point = FVector::ZeroVector;
	/** 원뿔 축과 이루는 각(도) */
	float AngleDegrees = 0.f;
	/** 캐릭터에서 조준점까지(cm) */
	float Distance = 0.f;
	bool bIsCurrent = false;
	float Score = 0.f;
};

/**
 * 소프트 락온의 순수 계산. 월드 · 액터를 모르고 숫자만 다뤄 자동화 테스트(Grimoire.Targeting.*)가 직접 부른다.
 * 오버랩 · 트레이스 · 입력은 UGrimTargetingComponent 가 하고 결과를 여기 넘긴다.
 */
struct GRIMOIRE_API FGrimTargetingMath
{
	/** Origin 에서 Point 쪽이 Axis 와 이루는 각(도, 0~180). Axis 가 0 이거나 Point == Origin 이면 0 */
	static float AngleFromAxisDegrees(const FVector& Origin, const FVector& Axis, const FVector& Point);

	/** 원뿔 안(각도 ≤ 반각, 거리 ≤ 사거리)인가. 경계는 안으로 친다 */
	static bool IsInsideCone(float AngleDegrees, float Distance, float HalfAngleDegrees, float Range);

	/** 종류 + 반응 여부 → 분류. 아군 · None 은 None(잡지 않는다) */
	static EGrimTargetCategory Categorize(EGrimTargetKind Kind, bool bReactsToSelectedSpell);

	static float GetCategoryWeight(EGrimTargetCategory Category, const FGrimTargetScoreWeights& Weights);

	/** 점수. 각도 · 거리는 [0, 1] 로 잘라 쓴다(지금 대상이 반각 밖 여유 구간에 있어도 음수가 되지 않게) */
	static float Score(float AngleDegrees, float HalfAngleDegrees, float Distance, float Range,
		EGrimTargetCategory Category, bool bIsCurrent, const FGrimTargetScoreWeights& Weights);

	/** 후보의 Score 를 채운다 */
	static void ScoreCandidate(FGrimTargetCandidate& Candidate, float HalfAngleDegrees, float Range, const FGrimTargetScoreWeights& Weights);

	/**
	 * 좋은 순으로 정렬하고 MaxCount 개로 자른다.
	 * bEnemiesFirst 면 적이 점수와 상관없이 다른 종류보다 앞(전투 중 적 우선), 그 안에서는 점수 순.
	 */
	static void SortAndCap(TArray<FGrimTargetCandidate>& Candidates, bool bEnemiesFirst, int32 MaxCount);

	/**
	 * 고정 락온 옆 대상 넘기기. From(지금 대상의 화면 위치)에서 Direction(화면 좌표, y 아래) 쪽으로
	 * MaxAngleDegrees 안에 있는 후보 중 가깝고 방향이 잘 맞는 것. 없으면 INDEX_NONE.
	 */
	static int32 FindSwitchTarget(const FVector2D& From, const FVector2D& Direction, TConstArrayView<FVector2D> Candidates, float MaxAngleDegrees);

	/**
	 * 카메라 평면 투영(원근, 뷰포트 없이). 결과 = (오른쪽/앞, 아래/앞) × 1000 — 화면 좌표와 같은 방향(y 아래) · 같은 비율.
	 * 카메라 뒤(앞 거리 ≤ 0)면 false. 헤드리스(-nullrhi)에서도 되도록 ProjectWorldLocationToScreen 대신 쓴다.
	 */
	static bool ProjectToViewPlane(const FVector& ViewLocation, const FRotator& ViewRotation, const FVector& Point, FVector2D& OutPlane);

	/** 시점 입력(요 · 피치, 피치 + = 위) → 화면 좌표 방향(y 아래) */
	static FVector2D LookInputToScreenDirection(const FVector2D& YawPitch) { return FVector2D(YawPitch.X, -YawPitch.Y); }
};

/** 「0.3초 넘게 가려지면 풀린다」 타이머. */
struct GRIMOIRE_API FGrimOcclusionTimer
{
	float OccludedSeconds = 0.f;

	void Reset() { OccludedSeconds = 0.f; }

	/** 이번 검사 결과를 반영한다. 보이거나 가려진 시간이 GraceSeconds 이하면 true(대상 유지) */
	bool Update(bool bVisible, float DeltaSeconds, float GraceSeconds);
};

/**
 * 고정 중 시점 입력을 모아 「툭 튕김」을 알아낸다. 마우스와 오른쪽 스틱이 같은 길(AddYaw/PitchInput)로 온다.
 * 입력이 ResetSeconds 동안 없으면 모은 것을 버리고, 넘긴 뒤에는 CooldownSeconds 동안 무시한다.
 */
struct GRIMOIRE_API FGrimLookFlickDetector
{
	FVector2D Accumulated = FVector2D::ZeroVector;
	float IdleSeconds = 0.f;
	float CooldownRemaining = 0.f;

	/** 입력을 더한다. 문턱을 넘으면 true 와 방향(요 · 피치, 단위 벡터)을 주고 모은 값을 비운다 */
	bool AddInput(const FVector2D& YawPitch, float Threshold, FVector2D& OutDirection);

	void Tick(float DeltaSeconds, float ResetSeconds);

	void StartCooldown(float Seconds) { CooldownRemaining = Seconds; Accumulated = FVector2D::ZeroVector; }

	void Reset() { Accumulated = FVector2D::ZeroVector; IdleSeconds = 0.f; CooldownRemaining = 0.f; }
};
