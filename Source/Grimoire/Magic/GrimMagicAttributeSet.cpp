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

void UGrimMagicAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetManaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxMana());
	}
	else if (Attribute == GetMaxManaAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
}

void UGrimMagicAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	// 회복 GE 가 기본값을 계속 올려도 상한을 넘지 않게 기본값도 자른다.
	if (Attribute == GetManaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxMana());
	}
}

void UGrimMagicAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	// 최대 마나가 줄면 현재 마나도 따라 줄인다.
	if (Attribute == GetMaxManaAttribute() && GetMana() > NewValue)
	{
		if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
		{
			ASC->SetNumericAttributeBase(GetManaAttribute(), NewValue);
		}
	}
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
