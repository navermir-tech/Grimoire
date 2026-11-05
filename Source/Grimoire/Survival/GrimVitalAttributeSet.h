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

	/** 섭씨. 보이는 게이지 — 판정은 더위 · 추위 모자람 등급. */
	UPROPERTY(BlueprintReadOnly, Category = "Vital", ReplicatedUsing = OnRep_BodyTemperature)
	FGameplayAttributeData BodyTemperature;
	GRIM_ATTRIBUTE_ACCESSORS(UGrimVitalAttributeSet, BodyTemperature)

	/** 0~MaxFatigue(100). 0 = 쌩쌩. */
	UPROPERTY(BlueprintReadOnly, Category = "Vital", ReplicatedUsing = OnRep_Fatigue)
	FGameplayAttributeData Fatigue;
	GRIM_ATTRIBUTE_ACCESSORS(UGrimVitalAttributeSet, Fatigue)

	/** 더위저항 합(장비 · 인챈트 · 버프 GE 가 더한다). 정수처럼 쓴다. 음수 가능(털가죽 로브 더위 −1). */
	UPROPERTY(BlueprintReadOnly, Category = "Vital", ReplicatedUsing = OnRep_HeatResist)
	FGameplayAttributeData HeatResist;
	GRIM_ATTRIBUTE_ACCESSORS(UGrimVitalAttributeSet, HeatResist)

	/** 추위저항 합. */
	UPROPERTY(BlueprintReadOnly, Category = "Vital", ReplicatedUsing = OnRep_ColdResist)
	FGameplayAttributeData ColdResist;
	GRIM_ATTRIBUTE_ACCESSORS(UGrimVitalAttributeSet, ColdResist)

	/** 0~100. 질병 확률 × (1 − 값/100). */
	UPROPERTY(BlueprintReadOnly, Category = "Vital", ReplicatedUsing = OnRep_DiseaseResist)
	FGameplayAttributeData DiseaseResist;
	GRIM_ATTRIBUTE_ACCESSORS(UGrimVitalAttributeSet, DiseaseResist)

	/** 이동 속도 배율(기본 1). 생존 패널티 GE 가 곱한다 — 생존 컴포넌트가 폰의 MaxWalkSpeed 에 반영. */
	UPROPERTY(BlueprintReadOnly, Category = "Vital", ReplicatedUsing = OnRep_MoveSpeedScale)
	FGameplayAttributeData MoveSpeedScale;
	GRIM_ATTRIBUTE_ACCESSORS(UGrimVitalAttributeSet, MoveSpeedScale)

	/** 시전 속도 배율(기본 1). 추위 · 피곤함이 낮춘다 — 마법 쪽이 읽는다. */
	UPROPERTY(BlueprintReadOnly, Category = "Vital", ReplicatedUsing = OnRep_CastSpeedScale)
	FGameplayAttributeData CastSpeedScale;
	GRIM_ATTRIBUTE_ACCESSORS(UGrimVitalAttributeSet, CastSpeedScale)

	/** 마나 소모 배율(기본 1). 더위가 높인다 — 마법 쪽이 읽는다. */
	UPROPERTY(BlueprintReadOnly, Category = "Vital", ReplicatedUsing = OnRep_ManaCostScale)
	FGameplayAttributeData ManaCostScale;
	GRIM_ATTRIBUTE_ACCESSORS(UGrimVitalAttributeSet, ManaCostScale)

protected:
	/** Health 0~MaxHealth, MaxHealth ≥ 1, Hunger 0~MaxHunger, BodyTemperature Min~Max, Fatigue 0~Max, 저항 · 배율 범위(아래). */
	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Hunger(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_BodyTemperature(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Fatigue(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_HeatResist(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_ColdResist(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_DiseaseResist(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MoveSpeedScale(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_CastSpeedScale(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_ManaCostScale(const FGameplayAttributeData& OldValue);
};
