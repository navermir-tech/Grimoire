#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Core/GrimAttributeMacros.h"
#include "GrimMagicAttributeSet.generated.h"

/**
 * 마법 어트리뷰트 — 마나와 서클 (R06).
 * 초기값은 UGrimSpellbookComponent 가 UGrimMagicSettings 에서 넣는다. Mana 는 [0, MaxMana] 로 자른다.
 */
UCLASS()
class GRIMOIRE_API UGrimMagicAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UGrimMagicAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	UPROPERTY(BlueprintReadOnly, Category = "Magic", ReplicatedUsing = OnRep_Mana)
	FGameplayAttributeData Mana;
	GRIM_ATTRIBUTE_ACCESSORS(UGrimMagicAttributeSet, Mana)

	UPROPERTY(BlueprintReadOnly, Category = "Magic", ReplicatedUsing = OnRep_MaxMana)
	FGameplayAttributeData MaxMana;
	GRIM_ATTRIBUTE_ACCESSORS(UGrimMagicAttributeSet, MaxMana)

	/** 1~10 (R06). */
	UPROPERTY(BlueprintReadOnly, Category = "Magic", ReplicatedUsing = OnRep_Circle)
	FGameplayAttributeData Circle;
	GRIM_ATTRIBUTE_ACCESSORS(UGrimMagicAttributeSet, Circle)

protected:
	UFUNCTION()
	void OnRep_Mana(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxMana(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Circle(const FGameplayAttributeData& OldValue);
};
