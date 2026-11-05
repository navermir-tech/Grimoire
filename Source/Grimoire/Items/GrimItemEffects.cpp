#include "Items/GrimItemEffects.h"
#include "Items/GrimItemTags.h"
#include "Survival/GrimVitalAttributeSet.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

const FName UGrimFoodBuffEffect::DurationMagnitudeName(TEXT("Item.Buff.Duration"));
const FName UGrimFoodBuffEffect::ColdResistMagnitudeName(TEXT("Item.Buff.ColdResist"));
const FName UGrimFoodBuffEffect::HeatResistMagnitudeName(TEXT("Item.Buff.HeatResist"));
const FName UGrimFoodBuffEffect::DiseaseResistMagnitudeName(TEXT("Item.Buff.DiseaseResist"));

namespace GrimItemEffectsPrivate // 유니티 빌드에서 다른 파일의 도우미와 부딪히지 않게
{
	FGameplayModifierInfo MakeFoodModifier(const FGameplayAttribute& Attribute, FName DataName)
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

UGrimFoodBuffEffect::UGrimFoodBuffEffect()
{
	using namespace GrimItemEffectsPrivate;
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat Duration;
	Duration.DataName = DurationMagnitudeName;
	DurationMagnitude = FGameplayEffectModifierMagnitude(Duration);

	Modifiers.Add(MakeFoodModifier(UGrimVitalAttributeSet::GetColdResistAttribute(), ColdResistMagnitudeName));
	Modifiers.Add(MakeFoodModifier(UGrimVitalAttributeSet::GetHeatResistAttribute(), HeatResistMagnitudeName));
	Modifiers.Add(MakeFoodModifier(UGrimVitalAttributeSet::GetDiseaseResistAttribute(), DiseaseResistMagnitudeName));

	UTargetTagsGameplayEffectComponent* TargetTags = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("FoodBuffTag"));
	FInheritedTagContainer GrantedTags;
	GrantedTags.Added.AddTag(GrimItemTags::Status_Buff_Food);
	TargetTags->SetAndApplyTargetTagChanges(GrantedTags);
	GEComponents.Add(TargetTags);
}
