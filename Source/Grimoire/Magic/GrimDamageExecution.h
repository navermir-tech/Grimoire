#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "GrimDamageExecution.generated.h"

/**
 * 마법 피해 계산 (R07).
 * 입력: SetByCaller.Magic.Damage(완드 보정까지 들어간 기본 피해), Spec 의 에셋 태그 중 Element.*(공격 속성),
 *       대상 태그 중 Element.*(대상 속성).
 * 출력: UGrimVitalAttributeSet::Health -= 기본 피해 × 상성(UGrimMagicSettings::ElementAffinities).
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
};
