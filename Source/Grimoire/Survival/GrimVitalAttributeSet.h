#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Core/GrimAttributeMacros.h"
#include "GrimVitalAttributeSet.generated.h"

/** 생존 어트리뷰트 (R03). 플레이어 · 생물 공용. 초기값 · 클램프 · 주기 감소는 Survival 시스템이 채운다. */
UCLASS()
class GRIMOIRE_API UGrimVitalAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UGrimVitalAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

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
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Hunger(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_BodyTemperature(const FGameplayAttributeData& OldValue);
};
