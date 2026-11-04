#include "Magic/GrimMagicEffects.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "Magic/GrimMagicTags.h"
#include "Magic/GrimDamageExecution.h"

namespace
{
	FGameplayEffectModifierMagnitude MakeSetByCaller(const FGameplayTag& Tag)
	{
		FSetByCallerFloat SetByCaller;
		SetByCaller.DataTag = Tag;
		return FGameplayEffectModifierMagnitude(SetByCaller);
	}

	FGameplayModifierInfo MakeAddModifier(const FGameplayAttribute& Attribute, const FGameplayTag& SetByCallerTag)
	{
		FGameplayModifierInfo Info;
		Info.Attribute = Attribute;
		Info.ModifierOp = EGameplayModOp::AddBase;
		Info.ModifierMagnitude = MakeSetByCaller(SetByCallerTag);
		return Info;
	}
}

UGrimGE_SpellCost::UGrimGE_SpellCost()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	Modifiers.Add(MakeAddModifier(UGrimMagicAttributeSet::GetManaAttribute(), GrimMagicTags::SetByCaller_ManaCost));
}

UGrimGE_SpellCooldown::UGrimGE_SpellCooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = MakeSetByCaller(GrimMagicTags::SetByCaller_Cooldown);
}

UGrimGE_SpellDamage::UGrimGE_SpellDamage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayEffectExecutionDefinition Execution;
	Execution.CalculationClass = UGrimDamageExecution::StaticClass();
	Executions.Add(Execution);
}

UGrimGE_ManaRegen::UGrimGE_ManaRegen()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	// 주기는 구조(1초 단위 회복)라 여기 둔다. 회복량은 설정의 ManaRegenPerSecond.
	Period = 1.f;
	bExecutePeriodicEffectOnApplication = false;
	Modifiers.Add(MakeAddModifier(UGrimMagicAttributeSet::GetManaAttribute(), GrimMagicTags::SetByCaller_ManaRegen));
}
