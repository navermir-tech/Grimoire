#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Core/GrimAttributeMacros.h"
#include "GrimVitalAttributeSet.generated.h"

/**
 * 생존 어트리뷰트 (R03). 플레이어 · 생물 공용.
 * 클램프 범위는 UGrimSurvivalSettings, 초기값 · 주기 변화는 UGrimSurvivalComponent 가 서버에서 넣는다.
 */
UCLASS()
class GRIMOIRE_API UGrimVitalAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UGrimVitalAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	UPROPERTY(BlueprintReadOnly, Category = "Vital", ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;
	GRIM_ATTRIBUTE_ACCESSORS(UGrimVitalAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, Category = "Vital", ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	GRIM_ATTRIBUTE_ACCESSORS(UGrimVitalAttributeSet, MaxHealth)

	/** 0~100. 100 = 배부름. */
	UPROPERTY(BlueprintReadOnly, Category = "Vital", ReplicatedUsing = OnRep_Hunger)
	FGameplayAttributeData Hunger;
	GRIM_ATTRIBUTE_ACCESSORS(UGrimVitalAttributeSet, Hunger)

	/** 섭씨. */
	UPROPERTY(BlueprintReadOnly, Category = "Vital", ReplicatedUsing = OnRep_BodyTemperature)
	FGameplayAttributeData BodyTemperature;
	GRIM_ATTRIBUTE_ACCESSORS(UGrimVitalAttributeSet, BodyTemperature)

protected:
	/** Health 0~MaxHealth, MaxHealth ≥ 1, Hunger 0~MaxHunger, BodyTemperature Min~Max(설정). */
	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Hunger(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_BodyTemperature(const FGameplayAttributeData& OldValue);
};
