#include "Survival/GrimSurvivalTickEffect.h"
#include "Survival/GrimVitalAttributeSet.h"

const FName UGrimSurvivalTickEffect::HealthMagnitudeName(TEXT("Survival.Health"));
const FName UGrimSurvivalTickEffect::HungerMagnitudeName(TEXT("Survival.Hunger"));
const FName UGrimSurvivalTickEffect::BodyTemperatureMagnitudeName(TEXT("Survival.BodyTemperature"));

namespace
{
	FGameplayModifierInfo MakeSetByCallerModifier(const FGameplayAttribute& Attribute, FName DataName)
	{
		FSetByCallerFloat SetByCaller;
		SetByCaller.DataName = DataName;

		FGameplayModifierInfo Modifier;
		Modifier.Attribute = Attribute;
		Modifier.ModifierOp = EGameplayModOp::AddBase;
		Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
		return Modifier;
	}
}

UGrimSurvivalTickEffect::UGrimSurvivalTickEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	Modifiers.Add(MakeSetByCallerModifier(UGrimVitalAttributeSet::GetHealthAttribute(), HealthMagnitudeName));
	Modifiers.Add(MakeSetByCallerModifier(UGrimVitalAttributeSet::GetHungerAttribute(), HungerMagnitudeName));
	Modifiers.Add(MakeSetByCallerModifier(UGrimVitalAttributeSet::GetBodyTemperatureAttribute(), BodyTemperatureMagnitudeName));
}
