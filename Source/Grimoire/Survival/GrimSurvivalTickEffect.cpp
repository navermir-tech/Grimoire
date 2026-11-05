#include "Survival/GrimSurvivalTickEffect.h"
#include "Survival/GrimSurvivalTags.h"
#include "Survival/GrimVitalAttributeSet.h"
#include "Magic/GrimMagicAttributeSet.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

const FName UGrimSurvivalTickEffect::HealthMagnitudeName(TEXT("Survival.Health"));
const FName UGrimSurvivalTickEffect::HungerMagnitudeName(TEXT("Survival.Hunger"));
const FName UGrimSurvivalTickEffect::BodyTemperatureMagnitudeName(TEXT("Survival.BodyTemperature"));

const FName UGrimSurvivalExtraTickEffect::FatigueMagnitudeName(TEXT("Survival.Fatigue"));
const FName UGrimSurvivalExtraTickEffect::ManaMagnitudeName(TEXT("Survival.Mana"));

const FName UGrimSurvivalPenaltyEffect::MoveSpeedMagnitudeName(TEXT("Survival.Penalty.MoveSpeed"));
const FName UGrimSurvivalPenaltyEffect::CastSpeedMagnitudeName(TEXT("Survival.Penalty.CastSpeed"));
const FName UGrimSurvivalPenaltyEffect::ManaCostMagnitudeName(TEXT("Survival.Penalty.ManaCost"));

const FName UGrimDiseaseEffect::DurationMagnitudeName(TEXT("Survival.Disease.Duration"));

const FName UGrimClimateResistEffect::HeatMagnitudeName(TEXT("Survival.Resist.Heat"));
const FName UGrimClimateResistEffect::ColdMagnitudeName(TEXT("Survival.Resist.Cold"));

// 유니티 빌드에서 다른 파일의 익명 네임스페이스 함수와 부딪히지 않게 이름을 붙인다.
namespace GrimSurvivalEffectsPrivate
{
	FGameplayModifierInfo MakeSetByCallerModifier(const FGameplayAttribute& Attribute, FName DataName, EGameplayModOp::Type Op)
	{
		FSetByCallerFloat SetByCaller;
		SetByCaller.DataName = DataName;

		FGameplayModifierInfo Modifier;
		Modifier.Attribute = Attribute;
		Modifier.ModifierOp = Op;
		Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
		return Modifier;
	}
}

UGrimSurvivalTickEffect::UGrimSurvivalTickEffect()
{
	using namespace GrimSurvivalEffectsPrivate;
	DurationPolicy = EGameplayEffectDurationType::Instant;

	Modifiers.Add(MakeSetByCallerModifier(UGrimVitalAttributeSet::GetHealthAttribute(), HealthMagnitudeName, EGameplayModOp::AddBase));
	Modifiers.Add(MakeSetByCallerModifier(UGrimVitalAttributeSet::GetHungerAttribute(), HungerMagnitudeName, EGameplayModOp::AddBase));
	Modifiers.Add(MakeSetByCallerModifier(UGrimVitalAttributeSet::GetBodyTemperatureAttribute(), BodyTemperatureMagnitudeName, EGameplayModOp::AddBase));
}

UGrimSurvivalExtraTickEffect::UGrimSurvivalExtraTickEffect()
{
	using namespace GrimSurvivalEffectsPrivate;
	DurationPolicy = EGameplayEffectDurationType::Instant;

	Modifiers.Add(MakeSetByCallerModifier(UGrimVitalAttributeSet::GetFatigueAttribute(), FatigueMagnitudeName, EGameplayModOp::AddBase));
	Modifiers.Add(MakeSetByCallerModifier(UGrimMagicAttributeSet::GetManaAttribute(), ManaMagnitudeName, EGameplayModOp::AddBase));
}

UGrimSurvivalPenaltyEffect::UGrimSurvivalPenaltyEffect()
{
	using namespace GrimSurvivalEffectsPrivate;
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	Modifiers.Add(MakeSetByCallerModifier(UGrimVitalAttributeSet::GetMoveSpeedScaleAttribute(), MoveSpeedMagnitudeName, EGameplayModOp::MultiplyCompound));
	Modifiers.Add(MakeSetByCallerModifier(UGrimVitalAttributeSet::GetCastSpeedScaleAttribute(), CastSpeedMagnitudeName, EGameplayModOp::MultiplyCompound));
	Modifiers.Add(MakeSetByCallerModifier(UGrimVitalAttributeSet::GetManaCostScaleAttribute(), ManaCostMagnitudeName, EGameplayModOp::MultiplyCompound));
}

UGrimDiseaseEffect::UGrimDiseaseEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat Duration;
	Duration.DataName = DurationMagnitudeName;
	DurationMagnitude = FGameplayEffectModifierMagnitude(Duration);
}

void UGrimDiseaseEffect::GrantTag(const FGameplayTag& Tag)
{
	UTargetTagsGameplayEffectComponent* TargetTags = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("DiseaseTag"));
	FInheritedTagContainer GrantedTags;
	GrantedTags.Added.AddTag(Tag);
	TargetTags->SetAndApplyTargetTagChanges(GrantedTags);
	GEComponents.Add(TargetTags);
}

UGrimDiseaseStomachEffect::UGrimDiseaseStomachEffect()
{
	GrantTag(GrimSurvivalTags::Status_Sick_Stomach);
}

UGrimDiseaseColdEffect::UGrimDiseaseColdEffect()
{
	GrantTag(GrimSurvivalTags::Status_Sick_Cold);
}

UGrimClimateResistEffect::UGrimClimateResistEffect()
{
	using namespace GrimSurvivalEffectsPrivate;
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	Modifiers.Add(MakeSetByCallerModifier(UGrimVitalAttributeSet::GetHeatResistAttribute(), HeatMagnitudeName, EGameplayModOp::AddBase));
	Modifiers.Add(MakeSetByCallerModifier(UGrimVitalAttributeSet::GetColdResistAttribute(), ColdMagnitudeName, EGameplayModOp::AddBase));
}
