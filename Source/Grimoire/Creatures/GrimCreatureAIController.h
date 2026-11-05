#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Creatures/GrimCreatureTypes.h"
#include "Creatures/GrimCreatureMath.h"
#include "GrimCreatureAIController.generated.h"

class AGrimCreature;

/**
 * 생물 AI(서버). ThinkInterval 마다 감각을 모아(FGrimCreatureSenses) 순수 판단 GrimCreatureMath::Decide 로 상태를 고르고,
 * 매 프레임 목표 지점으로 AddMovementInput(내비 메시 없이 곧게 — 지형 통합 때 MoveTo 로 바꿀 자리).
 * StateTree · BT 를 쓰지 않은 까닭은 docs/systems/creatures 「AI」.
 */
UCLASS()
class GRIMOIRE_API AGrimCreatureAIController : public AAIController
{
	GENERATED_BODY()

public:
	AGrimCreatureAIController();

	virtual void Tick(float DeltaSeconds) override;

	/** 마지막 판단에 쓴 감각(덤프) */
	const FGrimCreatureSenses& GetLastSenses() const { return LastSenses; }

	/** 서버: 지금 바로 판단(콘솔 · 시험) */
	void ThinkNow() { Think(); }

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

private:
	void Think();
	void GatherSenses(AGrimCreature* Creature, FGrimCreatureSenses& OutSenses);
	void MoveTowards(const FVector& Goal, float Acceptance);
	void StopMoving();
	bool IsDark() const;

	FTimerHandle ThinkTimer;
	FGrimCreatureSenses LastSenses;

	EGrimCreatureAIState CalmState = EGrimCreatureAIState::Idle;
	double CalmUntil = 0.0;

	FVector MoveGoal = FVector::ZeroVector;
	float MoveAcceptance = 50.f;
	bool bHasMoveGoal = false;

	// 감각을 모을 때 찾은 것들
	TWeakObjectPtr<AActor> NearestPlayer;
	float NearestPlayerDistance = TNumericLimits<float>::Max();
	TWeakObjectPtr<AActor> NearestPredator;
	TWeakObjectPtr<AActor> BestPrey;
	TWeakObjectPtr<AActor> NearestLight;
	bool bLightSeen = false;
};
