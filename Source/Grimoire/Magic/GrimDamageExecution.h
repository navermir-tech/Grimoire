#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayEffectExecutionCalculation.h"
#include "GrimDamageExecution.generated.h"

/**
 * 마법 피해 계산 (R07).
 * 입력: SetByCaller.Magic.Damage(완드 보정까지 들어간 기본 피해), Spec 의 에셋 태그 중 Element.*(공격 속성),
 *       대상 태그 중 Element.*(대상 속성 — 하나 또는 둘).
 * 출력: UGrimVitalAttributeSet::Health -= 기본 피해 × 상성.
 *
 * 두 속성 대상(R35 · Q35): 대상이 Element.* 를 둘 가지면 그중 부속성은 같은 이름의 표지 태그 SubElement.*(예: Element.Earth.Plant → SubElement.Earth.Plant)도 함께 가진다.
 *   배율 = 상성(공격, 주) × (1 + W × (상성(공격, 부) − 1)), W = UGrimDamageSettings::SecondaryElementWeight(0.5).
 */
UCLASS()
class GRIMOIRE_API UGrimDamageExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UGrimDamageExecution();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

	/** 태그 묶음에서 첫 Element.* 태그. 없으면 빈 태그(무속성). */
	static FGameplayTag FindElementTag(const FGameplayTagContainer& Tags);

	/**
	 * 대상 태그에서 주 · 부속성을 가린다(Element.None 은 속성으로 보지 않는다).
	 * 부속성 = SubElement 표지가 붙은 Element.*. 표지가 없는데 속성이 둘이면 묶음 순서대로(첫째 = 주) — 경고 대상.
	 * 속성이 하나면 OutSecondary 는 빈 태그. 셋 이상이면 앞의 둘만 쓴다.
	 */
	static void ResolveDefenderElements(const FGameplayTagContainer& Tags, FGameplayTag& OutPrimary, FGameplayTag& OutSecondary);

	/** 부속성 표지 태그(Element.X → SubElement.X). 등록되지 않은 이름이면 빈 태그. */
	static FGameplayTag GetSecondaryMarkerTag(const FGameplayTag& ElementTag);

	/** Q35 식 — 부속성 상성이 없으면(무속성 · 하나만) 주 상성 그대로. */
	static float CombineDualAffinity(float PrimaryAffinity, float SecondaryAffinity, bool bHasSecondary, float SecondaryWeight);

	/** 상성 표 + 두 속성 식으로 최종 배율(테스트 · 생물 AI 가 같은 식을 쓴다). */
	static float CalcAffinityAgainst(const FGameplayTag& AttackElement, const FGameplayTag& DefendPrimary, const FGameplayTag& DefendSecondary);
};

/**
 * 피해 계산 수치 — 정본은 Config/DefaultGame.ini 의 [/Script/Grimoire.GrimDamageSettings].
 * 두 속성 상성(Q35) 만 여기 둔다(상성 표 자체는 UGrimMagicSettings).
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Grimoire Damage"))
class GRIMOIRE_API UGrimDamageSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UGrimDamageSettings* Get() { return GetDefault<UGrimDamageSettings>(); }

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	/** 부속성 상성 반영 비율(Q35 — 0.5 = 절반). 0 이면 주속성만, 1 이면 두 상성을 곱한 것과 같다. */
	UPROPERTY(Config, EditAnywhere, Category = "Combat", meta = (ClampMin = "0", ClampMax = "1"))
	float SecondaryElementWeight = 0.5f;
};
