#include "Survival/GrimVitalAttributeSet.h"
#include "Survival/GrimSurvivalSettings.h"
#include "Net/UnrealNetwork.h"

UGrimVitalAttributeSet::UGrimVitalAttributeSet()
{
	// 배율은 1 이 기본 — 패널티 GE 가 없으면 그대로.
	InitMoveSpeedScale(1.f);
	InitCastSpeedScale(1.f);
	InitManaCostScale(1.f);
}

void UGrimVitalAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UGrimVitalAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGrimVitalAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGrimVitalAttributeSet, Hunger, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGrimVitalAttributeSet, BodyTemperature, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGrimVitalAttributeSet, Fatigue, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGrimVitalAttributeSet, HeatResist, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGrimVitalAttributeSet, ColdResist, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGrimVitalAttributeSet, DiseaseResist, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGrimVitalAttributeSet, MoveSpeedScale, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGrimVitalAttributeSet, CastSpeedScale, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGrimVitalAttributeSet, ManaCostScale, COND_None, REPNOTIFY_Always);
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
	const UGrimSurvivalSettings* Settings = GetDefault<UGrimSurvivalSettings>();
	const FGrimSurvivalTuning& Tuning = Settings->Tuning;

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
	else if (Attribute == GetFatigueAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, Settings->Fatigue.MaxFatigue);
	}
	else if (Attribute == GetDiseaseResistAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, 100.f);
	}
	else if (Attribute == GetMoveSpeedScaleAttribute() || Attribute == GetCastSpeedScaleAttribute() || Attribute == GetManaCostScaleAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
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

void UGrimVitalAttributeSet::OnRep_Fatigue(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGrimVitalAttributeSet, Fatigue, OldValue);
}

void UGrimVitalAttributeSet::OnRep_HeatResist(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGrimVitalAttributeSet, HeatResist, OldValue);
}

void UGrimVitalAttributeSet::OnRep_ColdResist(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGrimVitalAttributeSet, ColdResist, OldValue);
}

void UGrimVitalAttributeSet::OnRep_DiseaseResist(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGrimVitalAttributeSet, DiseaseResist, OldValue);
}

void UGrimVitalAttributeSet::OnRep_MoveSpeedScale(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGrimVitalAttributeSet, MoveSpeedScale, OldValue);
}

void UGrimVitalAttributeSet::OnRep_CastSpeedScale(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGrimVitalAttributeSet, CastSpeedScale, OldValue);
}

void UGrimVitalAttributeSet::OnRep_ManaCostScale(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGrimVitalAttributeSet, ManaCostScale, OldValue);
}
