#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GrimBaseEffects.generated.h"

/**
 * 장비 체온 보정(무한) — 입은 장비의 추위 · 더위 저항 합(SetByCaller). 장비가 바뀌면 떼고 다시 건다(UGrimEquipmentComponent).
 */
UCLASS()
class GRIMOIRE_API UGrimEquipmentResistEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGrimEquipmentResistEffect();

	static const FName ColdResistMagnitudeName;
	static const FName HeatResistMagnitudeName;
};
