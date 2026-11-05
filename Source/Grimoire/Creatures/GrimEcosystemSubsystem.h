#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Creatures/GrimCreatureTypes.h"
#include "GrimEcosystemSubsystem.generated.h"

class AGrimCreature;
class AGrimHabitatVolume;
class UGrimCreatureSpecies;

/** 서식 구역 하나의 종 하나 — 개체군 기록(서버) */
USTRUCT()
struct GRIMOIRE_API FGrimHabitatPopulation
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UGrimCreatureSpecies> Species;

	UPROPERTY()
	int32 Cap = 0;

	UPROPERTY()
	EGrimCreatureActivity Activity = EGrimCreatureActivity::Always;

	UPROPERTY()
	float RegrowSeconds = 0.f;

	float RegrowTimer = 0.f;

	/** 접힌 개체(액터 없음) */
	UPROPERTY()
	TArray<FGrimFoldedCreature> Folded;

	/** 펼친 개체 */
	UPROPERTY()
	TArray<TWeakObjectPtr<AGrimCreature>> Active;

	int32 NextHerdId = 1;
	int32 Killed = 0;
	int32 Regrown = 0;

	int32 GetCurrent() const { return Folded.Num() + Active.Num(); }
};

USTRUCT()
struct GRIMOIRE_API FGrimHabitatRuntime
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<AGrimHabitatVolume> Volume;

	UPROPERTY()
	TArray<FGrimHabitatPopulation> Populations;

	FRandomStream Rng;
};

/**
 * 생태계(R14) — 서버 월드 서브시스템. 서식 구역마다 개체군 기록 { 종, 상한, 현재, 회복 } 을 들고,
 * UpdateInterval 마다: 회복(줄어든 만큼 RegrowSeconds 마다 하나) → 멀어진 액터 접기 → 가까운 기록 펼치기(전역 상한 MaxActiveCreatures).
 * 먹이 관계 표(CreatureEcology.csv)도 여기서 읽는다 — 클라이언트도 읽는다(덤프 · 연출용, 판단은 서버).
 */
UCLASS()
class GRIMOIRE_API UGrimEcosystemSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	void RegisterHabitat(AGrimHabitatVolume* Volume);
	void UnregisterHabitat(AGrimHabitatVolume* Volume);

	/** 서버: 생물이 BeginPlay · EndPlay 때 */
	void RegisterCreature(AGrimCreature* Creature);
	void UnregisterCreature(AGrimCreature* Creature);

	/** 서버: 죽음 → 개체군에서 뺀다(현재 −1 → 회복 시작) */
	void HandleCreatureDied(AGrimCreature* Creature);

	/** 살아 있는 펼친 생물(서버) */
	const TArray<TWeakObjectPtr<AGrimCreature>>& GetCreatures() const { return Creatures; }
	int32 GetActiveCount() const;

	/** Predator 가 Prey 를 쫓나(먹이 관계 표) */
	bool IsPreyOf(FName Predator, FName Prey) const;

	/** Prey 를 쫓는 종이 있나 */
	bool HasPredator(FName Prey) const;

	/** Prey 를 쫓는 종의 우선순위(없으면 INDEX_NONE) */
	int32 GetHuntPriority(FName Predator, FName Prey) const;

	/** 지금 서버 시간대(시간 서브시스템이 없으면 낮 12시) */
	void GetTimeOfDay(bool& bOutNight, float& OutHours) const;

	/** 지금 이 활동 시간인가 */
	bool IsActiveNow(EGrimCreatureActivity Activity) const;

	/** 서버: 한 번 갱신(타이머 · 콘솔 · 테스트) */
	void UpdateNow(float DeltaSeconds);

	const TArray<FGrimHabitatRuntime>& GetHabitats() const { return Habitats; }

	/** 먹이 관계 표를 읽는다(테스트도 쓴다) */
	static bool LoadEcologyFromCSV(const FString& FilePath, TArray<FGrimCreatureEcologyRow>& OutRows, TArray<FString>& OutProblems);

	/** 바닥 찾기: 위 · 아래 트레이스. 못 찾으면 false */
	static bool FindGround(UWorld* World, const FVector& Location, FVector& OutGround);

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	bool IsServer() const;
	void LoadEcology();
	void FillPopulation(FGrimHabitatRuntime& Runtime, FGrimHabitatPopulation& Population, int32 Count);
	FGrimFoldedCreature MakeRecord(FGrimHabitatRuntime& Runtime, FGrimHabitatPopulation& Population, const FVector& Around, float Radius, int32 HerdId);
	void FoldCreature(FGrimHabitatPopulation& Population, AGrimCreature* Creature);
	AGrimCreature* UnfoldRecord(FGrimHabitatRuntime& Runtime, FGrimHabitatPopulation& Population, const FGrimFoldedCreature& Record);
	float GetNearestPlayerDistance(const FVector& Location) const;
	void HandleTimer();

	UPROPERTY()
	TArray<FGrimHabitatRuntime> Habitats;

	TArray<TWeakObjectPtr<AGrimCreature>> Creatures;
	TArray<FGrimCreatureEcologyRow> EcologyRows;
	TArray<FVector> PlayerLocations;
	FTimerHandle UpdateTimer;
	double LastUpdateTime = 0.0;
};
