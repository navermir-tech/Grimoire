#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Creatures/GrimCreatureTypes.h"
#include "GrimHabitatVolume.generated.h"

class UBoxComponent;
class UGrimCreatureSpecies;

/** 서식 구역의 종 하나 — 상한 · 시간대 · 회복 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimHabitatSpeciesEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Habitat")
	TObjectPtr<UGrimCreatureSpecies> Species;

	/** 이 구역의 개체군 상한(접힌 기록 + 펼친 액터) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Habitat", meta = (ClampMin = "0"))
	int32 MaxCount = 6;

	/** 켜면 종의 활동 시간 대신 아래 시간대 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Habitat")
	bool bOverrideActivity = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Habitat", meta = (EditCondition = "bOverrideActivity"))
	EGrimCreatureActivity Activity = EGrimCreatureActivity::Always;

	/** 하나 회복하는 시간(0 = 설정 RegrowSeconds) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Habitat", meta = (ClampMin = "0", Units = "s"))
	float RegrowSeconds = 0.f;
};

/**
 * 서식 구역(R14). 상자 안에 종마다 개체군이 산다 — 서버의 UGrimEcosystemSubsystem 이 기록을 들고,
 * 플레이어 근처의 기록만 액터로 펼친다(멀면 접힌 기록). 충돌 없음 · 복제 없음(생물 액터만 복제된다).
 * 맵에 놓거나(Scripts/Creatures/place_creature_test.py · 갈래 W 의 지형 통합) 콘솔 Grim.Creature.Habitat 으로 만든다.
 */
UCLASS(ClassGroup = (Grimoire), HideCategories = (Input, Physics, Collision, Replication, HLOD))
class GRIMOIRE_API AGrimHabitatVolume : public AActor
{
	GENERATED_BODY()

public:
	AGrimHabitatVolume();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Habitat")
	TArray<FGrimHabitatSpeciesEntry> Species;

	/** 이 구역이 어떤 곳인가(Habitat.*) — 종의 HabitatTags 와 안 맞으면 경고만 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Habitat", meta = (Categories = "Habitat"))
	FGameplayTagContainer HabitatTags;

	/** 처음부터 상한만큼 산다(끄면 0 에서 회복) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Habitat")
	bool bStartFull = true;

	/** 굴림 씨앗(0 = 매번 다름) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Habitat")
	int32 RandomSeed = 0;

	UBoxComponent* GetBounds() const { return Bounds; }

	/** 상자 안 무작위 점(높이는 상자 가운데 — 바닥은 펼칠 때 찾는다) */
	FVector GetRandomPoint(FRandomStream& Rng) const;

	/** 점에서 상자까지 거리(안이면 0) */
	float GetDistanceTo(const FVector& Location) const;

	bool Contains(const FVector& Location) const { return GetDistanceTo(Location) <= 0.f; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UBoxComponent> Bounds;
};
