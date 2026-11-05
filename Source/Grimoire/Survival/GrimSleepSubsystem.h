#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GrimSleepSubsystem.generated.h"

class UGrimSurvivalComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGrimOnNightSkipBegin, float, FadeSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGrimOnNightSkipped, float, SkippedDays);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGrimOnNightSkipCancelled);

/**
 * 밤 건너뛰기 (스펙 「잠 · 밤 건너뛰기」, 서버 전용). 접속자 중 잠든 비율이 SleepSkipRatio 이상이고 밤이면
 * 페이드(이벤트만) → 해 뜰 때로 시간 이동 → 잠든 사람 피곤함 0 · 건너뛴 시간만큼 배고픔 감소 · 깨움.
 */
UCLASS()
class GRIMOIRE_API UGrimSleepSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	/** 누가 눕거나 일어나면 생존 컴포넌트가 부른다. */
	void NotifySleepStateChanged();

	/** 지금 잠든 수 / 접속자 수(서버). */
	void CountSleepers(int32& OutSleeping, int32& OutPlayers) const;

	bool IsSkipPending() const { return bSkipPending; }

	UPROPERTY(BlueprintAssignable, Category = "Grimoire|Sleep")
	FGrimOnNightSkipBegin OnNightSkipBegin;

	UPROPERTY(BlueprintAssignable, Category = "Grimoire|Sleep")
	FGrimOnNightSkipped OnNightSkipped;

	UPROPERTY(BlueprintAssignable, Category = "Grimoire|Sleep")
	FGrimOnNightSkipCancelled OnNightSkipCancelled;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	bool IsServer() const;
	TArray<UGrimSurvivalComponent*> GatherPlayers() const;
	bool CanSkipNow() const;
	void Evaluate();
	void ExecuteSkip();

	/** 시각을 앞으로만 옮긴다. 자정을 넘어야 하면 하루 끝으로 갔다가 날짜가 바뀐 뒤 목표로(SetTimeOfDay 가 날짜를 유지해서). */
	void AdvanceTimeTo(float TargetTimeOfDay);
	void FinishAdvanceAcrossMidnight();

	FTimerHandle EvaluateTimer;
	FTimerHandle FadeTimer;
	FTimerHandle MidnightTimer;
	bool bSkipPending = false;
	float PendingTargetTimeOfDay = 0.f;
	int32 PendingStartDay = 0;
};
