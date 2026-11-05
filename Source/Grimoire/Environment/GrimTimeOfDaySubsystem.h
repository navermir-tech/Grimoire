#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Environment/GrimTimeOfDayReplicator.h"
#include "Core/GrimSaveTypes.h"
#include "GrimTimeOfDaySubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGrimOnNightChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGrimOnHourChanged, int32, Hour);

/**
 * 낮 · 밤 시간 (R04). 서버가 시간을 진행하고 AGrimTimeOfDayReplicator 로 스냅숏을 복제한다.
 * 클라이언트는 스냅숏에서 서버 월드 시각만큼 외삽하고, 새 스냅숏과의 차이는 짧게 보정해 부드럽게 잇는다.
 * 이벤트(밤 시작 · 끝, 시 바뀜)는 서버 · 클라이언트가 각자 자기 시간으로 낸다.
 */
UCLASS()
class GRIMOIRE_API UGrimTimeOfDaySubsystem : public UTickableWorldSubsystem, public IGrimSaveParticipant
{
	GENERATED_BODY()

public:
	// 세이브 (IGrimSaveParticipant — World 「Environment.TimeOfDay」 v1: 누적 일수). 하루 길이는 서버 옵션이라 저장하지 않는다.
	virtual EGrimSaveScope GetSaveScope() const override { return EGrimSaveScope::World; }
	virtual FName GetSaveId() const override { return TEXT("Environment.TimeOfDay"); }
	virtual void WriteSaveData(FArchive& Ar) override;
	virtual void ReadSaveData(FArchive& Ar, int32 Version) override;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	/** 누적 일수(정수부 = 날짜, 소수부 = 시각). */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Time")
	double GetTotalDays() const;

	/** 0~1, 0 = 자정. */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Time")
	float GetTimeOfDay() const;

	/** 0~24 시. */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Time")
	float GetHours() const { return GetTimeOfDay() * 24.f; }

	UFUNCTION(BlueprintPure, Category = "Grimoire|Time")
	int32 GetDayIndex() const;

	UFUNCTION(BlueprintPure, Category = "Grimoire|Time")
	bool IsNight() const;

	UFUNCTION(BlueprintPure, Category = "Grimoire|Time")
	float GetDayRatio() const { return Snapshot.DayRatio; }

	UFUNCTION(BlueprintPure, Category = "Grimoire|Time")
	float GetDayLengthSeconds() const { return Snapshot.DayLengthSeconds; }

	/** 해 궤도 각(도). FGrimTimeOfDayMath::GetSunAngleDegrees. */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Time")
	float GetSunAngleDegrees() const;

	/** 서버 전용. 오늘 날짜를 유지하고 시각만 바꾼다(0~1). */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Grimoire|Time")
	void SetTimeOfDay(float NewTimeOfDay);

	/** 서버 전용. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Grimoire|Time")
	void SetDayLengthMinutes(float NewDayLengthMinutes);

	/** 복제기에서 받은 스냅숏(클라이언트). */
	void ApplyReplicatedSnapshot(const FGrimTimeSnapshot& NewSnapshot);

	UPROPERTY(BlueprintAssignable, Category = "Grimoire|Time")
	FGrimOnNightChanged OnNightBegin;

	UPROPERTY(BlueprintAssignable, Category = "Grimoire|Time")
	FGrimOnNightChanged OnNightEnd;

	UPROPERTY(BlueprintAssignable, Category = "Grimoire|Time")
	FGrimOnHourChanged OnHourChanged;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	bool IsServer() const;
	double GetServerSeconds() const;
	double ExtrapolateSnapshot() const;

	/** 서버: 지금 값으로 스냅숏을 새로 찍고 복제한다. */
	void PublishSnapshot(double TotalDays);

	void UpdateEvents();

	FGrimTimeSnapshot Snapshot;

	/** 클라이언트 보정(일 단위). 새 스냅숏을 받았을 때 (이전 외삽 − 새 외삽)에서 0 으로 줄어든다. */
	double CorrectionDays = 0.0;

	bool bReceivedServerSnapshot = false;
	bool bEventsPrimed = false;
	bool bWasNight = false;
	int32 LastHour = -1;

	UPROPERTY()
	TObjectPtr<AGrimTimeOfDayReplicator> Replicator;
};
