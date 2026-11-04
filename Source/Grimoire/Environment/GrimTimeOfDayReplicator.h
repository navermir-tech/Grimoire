#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "GrimTimeOfDayReplicator.generated.h"

/** 서버 시간 스냅숏: 서버 시각 ServerSeconds(GameState 의 서버 월드 시각)에 누적 일수가 TotalDays 였다. */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimTimeSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Time")
	double TotalDays = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Time")
	double ServerSeconds = 0.0;

	UPROPERTY(BlueprintReadOnly, Category = "Time")
	float DayLengthSeconds = 48.f * 60.f;

	UPROPERTY(BlueprintReadOnly, Category = "Time")
	float DayRatio = 0.625f;
};

/**
 * 시간 상태를 모두에게 복제하는 보이지 않는 액터. 서버의 UGrimTimeOfDaySubsystem 이 만든다(맵에 놓지 않는다).
 * GameState 를 고치지 않고 시스템이 자기 복제 상태를 갖는 방식(docs/systems/environment).
 */
UCLASS(NotPlaceable, Transient)
class GRIMOIRE_API AGrimTimeOfDayReplicator : public AInfo
{
	GENERATED_BODY()

public:
	AGrimTimeOfDayReplicator();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	/** 서버 전용. */
	void SetSnapshot(const FGrimTimeSnapshot& NewSnapshot);
	const FGrimTimeSnapshot& GetSnapshot() const { return Snapshot; }

private:
	UPROPERTY(ReplicatedUsing = OnRep_Snapshot)
	FGrimTimeSnapshot Snapshot;

	UFUNCTION()
	void OnRep_Snapshot();
};
