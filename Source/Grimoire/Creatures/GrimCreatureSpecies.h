#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Creatures/GrimCreatureTypes.h"
#include "GrimCreatureSpecies.generated.h"

/**
 * 생물 종 정의(R35 도감 확장 룰 · R23 개체 차이). 에셋 DA_Creature_<종> — Scripts/Creatures/make_creature_species.py 가 만든다.
 * 모델은 아직 엔진 기본 도형 조합(BodyParts). 색 영역은 부위마다 다이내믹 머티리얼 Color 파라미터로 칠한다.
 */
UCLASS(BlueprintType)
class GRIMOIRE_API UGrimCreatureSpecies : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override { return FPrimaryAssetId(TEXT("CreatureSpecies"), SpeciesId.IsNone() ? GetFName() : SpeciesId); }

	// ── 도감 ──

	/** 종 id — 개체 기록 · 서식 구역 · 먹이 관계 표 · 콘솔이 이 이름을 쓴다(예: Jackalope) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Codex")
	FName SpeciesId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Codex")
	FText DisplayName;

	/** 바탕이 된 신화 · 민담 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Codex")
	FText Origin;

	/** 주속성 — 전투 역할 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Codex", meta = (Categories = "Element"))
	FGameplayTag PrimaryElement;

	/** 부속성 — 생태 특징(없어도 된다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Codex", meta = (Categories = "Element"))
	FGameplayTag SecondaryElement;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Codex")
	EGrimCreatureSizeClass SizeClass = EGrimCreatureSizeClass::Small;

	/** 고유 능력(Creature.Ability.*) — 하나(R35 「게임 능력을 하나 더한다」) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Codex", meta = (Categories = "Creature.Ability"))
	FGameplayTagContainer AbilityTags;

	// ── 능력치 ──

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "1"))
	float MaxHealth = 30.f;

	/** 공격 피해 배율(개체 StatScale 도 곱해진다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0"))
	float Power = 1.f;

	/** 걷기(배회) 속도 cm/s */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0"))
	float WalkSpeed = 200.f;

	/** 달리기(도망 · 사냥 · 싸움) 속도 cm/s */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0"))
	float RunSpeed = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	EGrimCreatureTemperament Temperament = EGrimCreatureTemperament::Timid;

	/** 플레이어 · 포식자를 알아채는 거리(cm) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0"))
	float PerceptionRadius = 1200.f;

	/** 공격적 종이 먼저 덤비는 거리(cm) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0"))
	float AggroRadius = 800.f;

	/** 체력 비율이 이 아래면 도망(0 = 설정값) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0", ClampMax = "1"))
	float FleeHealthRatio = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	TArray<FGrimCreatureAttack> Attacks;

	// ── 생태 ──

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ecology")
	EGrimCreatureDiet Diet = EGrimCreatureDiet::Herbivore;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ecology")
	EGrimCreatureActivity Activity = EGrimCreatureActivity::Diurnal;

	/** 활동 시간 밖이면 잠들지 않고 사라진다(검은 개 — 밤에만 나온다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ecology")
	bool bHideOutsideActiveTime = false;

	/** 모닥불 · 불빛을 피한다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ecology")
	bool bAvoidsLight = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ecology", meta = (ClampMin = "1"))
	int32 HerdSizeMin = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ecology", meta = (ClampMin = "1"))
	int32 HerdSizeMax = 1;

	/** 사는 곳(Habitat.*) — 서식 구역 볼륨이 맞는지 볼 때 · 도감 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ecology", meta = (Categories = "Habitat"))
	FGameplayTagContainer HabitatTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ecology")
	EGrimCreatureWaterMovement WaterMovement = EGrimCreatureWaterMovement::Swim;

	/** 사냥 · 죽으면 떨구는 것 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ecology")
	TArray<FGrimCreatureLootEntry> Loot;

	// ── 개체 차이 ──

	/** 색 영역 2~6개 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variation")
	TArray<FGrimCreatureColorRegion> ColorRegions;

	/** 돌연변이 색(팔레트 밖, 3~5색) — 돌연변이면 영역 하나가 여기서 나온다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variation")
	TArray<FLinearColor> MutantPalette;

	/** 수컷 비율(0.5 = 반반) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variation", meta = (ClampMin = "0", ClampMax = "1"))
	float MaleRatio = 0.5f;

	/** 거대 개체 확률(스펙 제안 2%) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variation", meta = (ClampMin = "0", ClampMax = "1"))
	float GiantChance = 0.02f;

	// ── 모양(임시 — 기본 도형) ──

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body", meta = (ClampMin = "5"))
	float CapsuleRadius = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body", meta = (ClampMin = "5"))
	float CapsuleHalfHeight = 30.f;

	/** 몸 · 머리 · 다리 · 뿔 · 꼬리 … 성별 부위는 Sex 로 켜고 끈다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Body")
	TArray<FGrimCreatureBodyPart> BodyParts;

	// ── 길들이기 자리(다음 묶음 — 갈래 T) ──

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Taming")
	bool bTameable = true;

	/** 길들이기 요구 서클 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Taming", meta = (ClampMin = "1", ClampMax = "10"))
	int32 TamingCircle = 1;

	/** 잘 듣는 파크툼의 속성(파크툼 + 속성 — 보통 주속성) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Taming", meta = (Categories = "Element"))
	FGameplayTag PreferredPactElement;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Taming")
	bool bMountable = false;

	/** 체력 비율 문턱(0 = 설정 WeakenedHealthRatio) */
	float GetFleeHealthRatio(float Fallback) const { return FleeHealthRatio > 0.f ? FleeHealthRatio : Fallback; }

	const FGrimCreatureAttack* FindAttack(const FGameplayTag& Element) const;

	/** 에디터 파이썬(make_creature_species.py)용: 이름으로 태그 · 태그 묶음. 없는 이름은 빈 태그 */
	UFUNCTION(BlueprintPure, Category = "Grimoire|Creature")
	static FGameplayTag RequestTag(FName TagName);

	UFUNCTION(BlueprintPure, Category = "Grimoire|Creature")
	static FGameplayTagContainer MakeTagContainer(const TArray<FName>& TagNames);

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
