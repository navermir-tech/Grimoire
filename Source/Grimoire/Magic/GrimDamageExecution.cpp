#include "Magic/GrimDamageExecution.h"
#include "Magic/GrimMagicMath.h"
#include "Magic/GrimMagicSettings.h"
#include "Magic/GrimMagicTags.h"
#include "Survival/GrimVitalAttributeSet.h"
#include "GameplayTagsManager.h"

UGrimDamageExecution::UGrimDamageExecution()
{
}

FGameplayTag UGrimDamageExecution::FindElementTag(const FGameplayTagContainer& Tags)
{
	static const FGameplayTag ElementRoot = UGameplayTagsManager::Get().RequestGameplayTag(TEXT("Element"), false);
	for (const FGameplayTag& Tag : Tags)
	{
		// Element 자체가 아니라 그 아래 태그(Element.Fire …)만 속성으로 본다.
		if (Tag != ElementRoot && Tag.MatchesTag(ElementRoot))
		{
			return Tag;
		}
	}
	return FGameplayTag();
}

void UGrimDamageExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	const float BaseDamage = Spec.GetSetByCallerMagnitude(GrimMagicTags::SetByCaller_Damage, false, 0.f);
	if (BaseDamage <= 0.f)
	{
		return;
	}

	FGameplayTagContainer SpecTags;
	Spec.GetAllAssetTags(SpecTags);
	const FGameplayTag AttackElement = FindElementTag(SpecTags);

	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	const FGameplayTag DefendElement = TargetTags ? FindElementTag(*TargetTags) : FGameplayTag();

	const float Affinity = UGrimMagicSettings::Get()->GetAffinity(AttackElement, DefendElement);
	const float FinalDamage = GrimMagicMath::CalcDamage(BaseDamage, 1.f, Affinity);
	if (FinalDamage > 0.f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UGrimVitalAttributeSet::GetHealthAttribute(), EGameplayModOp::AddBase, -FinalDamage));
	}
}
