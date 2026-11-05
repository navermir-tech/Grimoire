#include "Base/GrimBaseEffects.h"
#include "Survival/GrimVitalAttributeSet.h"

const FName UGrimEquipmentResistEffect::ColdResistMagnitudeName(TEXT("Base.Equipment.ColdResist"));
const FName UGrimEquipmentResistEffect::HeatResistMagnitudeName(TEXT("Base.Equipment.HeatResist"));

namespace GrimBaseEffectsLocal // 유니티 빌드에서 이름이 부딪히지 않게
{
	FGameplayModifierInfo MakeResistModifier(const FGameplayAttribute& Attribute, FName DataName)
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

UGrimEquipmentResistEffect::UGrimEquipmentResistEffect()
{
	using namespace GrimBaseEffectsLocal;
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Modifiers.Add(MakeResistModifier(UGrimVitalAttributeSet::GetColdResistAttribute(), ColdResistMagnitudeName));
	Modifiers.Add(MakeResistModifier(UGrimVitalAttributeSet::GetHeatResistAttribute(), HeatResistMagnitudeName));
}
