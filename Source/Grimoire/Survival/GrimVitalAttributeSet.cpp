#include "Survival/GrimVitalAttributeSet.h"
#include "Survival/GrimSurvivalSettings.h"
#include "Net/UnrealNetwork.h"

UGrimVitalAttributeSet::UGrimVitalAttributeSet()
{
}

void UGrimVitalAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UGrimVitalAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGrimVitalAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGrimVitalAttributeSet, Hunger, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGrimVitalAttributeSet, BodyTemperature, COND_None, REPNOTIFY_Always);
}

void UGrimVitalAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);
	ClampAttribute(Attribute, NewValue);
}

void UGrimVitalAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	ClampAttribute(Attribute, NewValue);
}

void UGrimVitalAttributeSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	const FGrimSurvivalTuning& Tuning = GetDefault<UGrimSurvivalSettings>()->Tuning;

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.f);
	}
	else if (Attribute == GetHungerAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, Tuning.MaxHunger);
	}
	else if (Attribute == GetBodyTemperatureAttribute())
	{
		NewValue = FMath::Clamp(NewValue, Tuning.MinBodyTemperature, Tuning.MaxBodyTemperature);
	}
}

void UGrimVitalAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGrimVitalAttributeSet, Health, OldValue);
}

void UGrimVitalAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGrimVitalAttributeSet, MaxHealth, OldValue);
}

void UGrimVitalAttributeSet::OnRep_Hunger(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGrimVitalAttributeSet, Hunger, OldValue);
}

void UGrimVitalAttributeSet::OnRep_BodyTemperature(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGrimVitalAttributeSet, BodyTemperature, OldValue);
}
