#include "Survival/GrimVitalAttributeSet.h"
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
