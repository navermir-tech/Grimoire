#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "Creatures/GrimCreatureTypes.h"
#include "GrimCreatureSettings.generated.h"

class UGrimCreatureSpecies;

/** 속성 → 재료 태그(R16 상호작용). 두 속성 생물은 두 속성 몫을 함께 가진다. */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimCreatureElementMaterial
{
	GENERATED_BODY()

	/** 이 속성(하위 태그도 맞음) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature", meta = (Categories = "Element"))
	FGameplayTag Element;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature", meta = (Categories = "Material"))
	FGameplayTagContainer Materials;
};

/** 개체 굴림 수치(스펙 「개체 차이」 산정 항목) */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimCreatureRollTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Roll")
	float SizeMin = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Roll")
	float SizeMax = 1.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Roll")
	float GiantSizeMin = 1.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Roll")
	float GiantSizeMax = 1.5f;

	/** 크기 1 차이당 능력치 차이(0.5 → 0.9~1.1 이 ±5%) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Roll")
	float StatPerSize = 0.5f;

	/** 거대 개체 능력치 배율(크기 몫에 곱한다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Roll")
	float GiantStatMultiplier = 1.5f;

	/** 야생 개체 돌연변이 확률(Q8 의 3% 를 따른다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Roll", meta = (ClampMin = "0", ClampMax = "1"))
	float WildMutantChance = 0.03f;

	/** 색조 흔들림 ±도 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Roll")
	float HueJitterDegrees = 4.f;

	/** 채도 흔들림 ±(0~1 비율) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Roll")
	float SaturationJitter = 0.08f;

	/** 밝기 흔들림 ± */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Roll")
	float ValueJitter = 0.08f;

	/** 거대 개체 전리품 개수 배율 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Roll")
	float GiantLootMultiplier = 2.f;
};

/** AI 판단 수치 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimCreatureAITuning
{
	GENERATED_BODY()

	/** 서버 판단 주기(초). 생물 수 × 이 주기가 서버 비용 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (ClampMin = "0.05", Units = "s"))
	float ThinkInterval = 0.25f;

	/** 체력이 이 비율 아래면 도망(종 FleeHealthRatio 가 0 일 때) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (ClampMin = "0", ClampMax = "1"))
	float FleeHealthRatio = 0.25f;

	/** 맞은 뒤 그 상대를 기억하는 시간 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (Units = "s"))
	float ProvokedMemorySeconds = 12.f;

	/** 쫓던 대상이 이 거리 밖이면 놓는다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (Units = "cm"))
	float ChaseGiveUpDistance = 3000.f;

	/** 사냥 표적을 찾는 거리 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (Units = "cm"))
	float HuntRadius = 2500.f;

	/** 배회 반경(집 = 처음 자리) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (Units = "cm"))
	float WanderRadius = 800.f;

	/** 한가할 때 한 활동(가만히 · 배회 · 풀 뜯기)을 이어 가는 시간 범위 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (Units = "s"))
	FVector2D CalmActivitySeconds = FVector2D(3.f, 8.f);

	/** 한가할 때 고르는 가중치: 가만히 · 배회 · 풀 뜯기(초식만) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	FVector CalmWeights = FVector(1.f, 2.f, 2.f);

	/** 잠든 생물이 알아채는 거리 배율 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (ClampMin = "0"))
	float SleepPerceptionScale = 0.5f;

	/** 도망 · 배회 목표를 한 번에 이만큼 앞에 둔다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (Units = "cm"))
	float MoveStepDistance = 600.f;

	/** 도망칠 때 위협에서 이만큼 떨어지면 멈춘다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (Units = "cm"))
	float FleeSafeDistance = 2000.f;

	/** 빛을 피하는 종이 불빛(타는 사물)을 알아채는 거리 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (Units = "cm"))
	float LightAvoidRadius = 900.f;

	/** 해 질 녘(DayAndDusk 활동) 시각 범위 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	float DuskStartHour = 17.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	float DuskEndHour = 21.f;
};

/** 고유 능력 수치 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimCreatureAbilityTuning
{
	GENERATED_BODY()

	/** 씨앗 퍼뜨리기: 주기 · 반경 · 자극 세기(Element.Earth.Plant, 출처 Creature) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SeedSpread", meta = (Units = "s"))
	float SeedSpreadInterval = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SeedSpread", meta = (Units = "cm"))
	float SeedSpreadRadius = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SeedSpread")
	float SeedSpreadIntensity = 0.5f;

	/** 자극을 줄 재질(마른 풀) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SeedSpread", meta = (Categories = "Material"))
	FGameplayTag SeedSpreadMaterial;

	/** 그림자 걸음: 숨는 시간 · 다시 쓰기까지 · 숨을 수 있는 대상 거리 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShadowStep", meta = (Units = "s"))
	float ShadowStepSeconds = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShadowStep", meta = (Units = "s"))
	float ShadowStepCooldown = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShadowStep", meta = (Units = "cm"))
	float ShadowStepMinTargetDistance = 400.f;

	/** 이 속성(하위 포함) 자극을 맞으면 그림자 걸음이 풀린다 — 빛(신성 · 빛 마법) · 불빛 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ShadowStep", meta = (Categories = "Element"))
	FGameplayTagContainer ShadowRevealElements;

	/** 물 위 달리기: 물 볼륨 안 부력(1 초과 = 수면으로 떠오름) · 헤엄 속도 = 달리기 속도 × 배율 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WaterRun")
	float WaterRunBuoyancy = 1.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WaterRun")
	float WaterRunSpeedScale = 1.f;

	/** 불붙은 생물: 세기 1 당 초당 피해(Element.Fire) · 몇 초 뒤 뒹굴어 끄나(Element.Earth 자극) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Burning")
	float BurnDamagePerSecond = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Burning", meta = (Units = "s"))
	float BurnSelfExtinguishSeconds = 5.f;
};

/** 생태계 스폰 · 서버 성능 상한 */
USTRUCT(BlueprintType)
struct GRIMOIRE_API FGrimEcosystemTuning
{
	GENERATED_BODY()

	/** 서버 생태계 갱신 주기 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ecosystem", meta = (ClampMin = "0.1", Units = "s"))
	float UpdateInterval = 1.f;

	/** 접힌 기록이 플레이어에게서 이 거리 안이면 액터로 펼친다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ecosystem", meta = (Units = "cm"))
	float SpawnRadius = 6000.f;

	/** 액터가 모든 플레이어에게서 이 거리 밖이면 기록으로 접는다(SpawnRadius 보다 커야 한다 — 되풀이 방지) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ecosystem", meta = (Units = "cm"))
	float DespawnRadius = 8000.f;

	/** 서버 성능 상한 — 세계 전체 펼친 생물 수 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ecosystem", meta = (ClampMin = "0"))
	int32 MaxActiveCreatures = 40;

	/** 한 갱신에 펼치는 최대 수(한 프레임에 몰리지 않게) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ecosystem", meta = (ClampMin = "1"))
	int32 MaxSpawnsPerUpdate = 6;

	/** 줄어든 개체군이 하나 회복되는 시간(서식 구역이 0 이면) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ecosystem", meta = (Units = "s"))
	float RegrowSeconds = 240.f;

	/** 무리 구성원이 무리 가운데에서 떨어지는 거리 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ecosystem", meta = (Units = "cm"))
	float HerdRadius = 400.f;

	/** 죽은 몸이 남는 시간 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ecosystem", meta = (Units = "s"))
	float CorpseSeconds = 6.f;

	/** 바닥 찾기 트레이스: 위로 · 아래로 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ecosystem", meta = (Units = "cm"))
	float GroundTraceUp = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ecosystem", meta = (Units = "cm"))
	float GroundTraceDown = 5000.f;
};

/**
 * 생물 · 생태계 수치 — 정본은 Config/DefaultGame.ini 의 [/Script/Grimoire.GrimCreatureSettings].
 * 종마다 다른 값은 종 정의(DA_Creature_*), 먹이 관계는 Content/Grimoire/Data/CreatureEcology.csv.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Grimoire Creatures"))
class GRIMOIRE_API UGrimCreatureSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UGrimCreatureSettings* Get() { return GetDefault<UGrimCreatureSettings>(); }

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	/** 게임에 있는 종(콘솔 · 개체 기록의 SpeciesId 로 찾는다) */
	UPROPERTY(Config, EditAnywhere, Category = "Species")
	TArray<TSoftObjectPtr<UGrimCreatureSpecies>> Species;

	/** 먹이 관계 표 CSV(Content 기준) */
	UPROPERTY(Config, EditAnywhere, Category = "Species")
	FString EcologyCSV = TEXT("Grimoire/Data/CreatureEcology.csv");

	UPROPERTY(Config, EditAnywhere, Category = "Species")
	TArray<FGrimCreatureElementMaterial> ElementMaterials;

	UPROPERTY(Config, EditAnywhere, Category = "Roll")
	FGrimCreatureRollTuning Roll;

	UPROPERTY(Config, EditAnywhere, Category = "AI")
	FGrimCreatureAITuning AI;

	UPROPERTY(Config, EditAnywhere, Category = "Abilities")
	FGrimCreatureAbilityTuning Abilities;

	UPROPERTY(Config, EditAnywhere, Category = "Ecosystem")
	FGrimEcosystemTuning Ecosystem;

	/** 이 체력 비율 이하로 처음 내려가면 「약해짐」(튜토리얼 · 다음 묶음의 포획 확률) */
	UPROPERTY(Config, EditAnywhere, Category = "Quest", meta = (ClampMin = "0", ClampMax = "1"))
	float WeakenedHealthRatio = 0.3f;

	/** 죽였을 때 · 약하게 만들었을 때 퀘스트 이벤트 앞 이름 — 뒤에 크기 분류(Small · Medium …)가 붙는다 */
	UPROPERTY(Config, EditAnywhere, Category = "Quest")
	FString KilledEventPrefix = TEXT("Creature.Killed.");

	UPROPERTY(Config, EditAnywhere, Category = "Quest")
	FString WeakenedEventPrefix = TEXT("Creature.Weakened.");

	/** 종 id 로 종 정의를 찾는다(동기 로드) */
	UGrimCreatureSpecies* FindSpecies(FName SpeciesId) const;

	/** 속성 둘의 재료 태그를 합친다 */
	FGameplayTagContainer GetMaterialsForElements(const FGameplayTag& Primary, const FGameplayTag& Secondary) const;

	static FName GetQuestEventName(const FString& Prefix, EGrimCreatureSizeClass SizeClass);
};
