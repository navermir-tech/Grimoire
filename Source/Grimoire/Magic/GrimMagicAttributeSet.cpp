#include "Magic/GrimMagicAttributeSet.h"
#include "Net/UnrealNetwork.h"

UGrimMagicAttributeSet::UGrimMagicAttributeSet()
{
}

void UGrimMagicAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UGrimMagicAttributeSet, Mana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGrimMagicAttributeSet, MaxMana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UGrimMagicAttributeSet, Circle, COND_None, REPNOTIFY_Always);
}

void UGrimMagicAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGrimMagicAttributeSet, Mana, OldValue);
}

void UGrimMagicAttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGrimMagicAttributeSet, MaxMana, OldValue);
}

void UGrimMagicAttributeSet::OnRep_Circle(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UGrimMagicAttributeSet, Circle, OldValue);
}
