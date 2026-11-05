#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GrimItemEffects.generated.h"

/**
 * 음식 버프(지속형) — 추위 · 더위 · 질병 저항을 더한다. 시간 · 값은 SetByCaller(아이템 표의 Buff* 열).
 * 새로 먹으면 전 버프를 떼고 다시 건다(겹치지 않는다). Status.Buff.Food 태그를 준다.
 */
UCLASS()
class GRIMOIRE_API UGrimFoodBuffEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGrimFoodBuffEffect();

	static const FName DurationMagnitudeName;
	static const FName ColdResistMagnitudeName;
	static const FName HeatResistMagnitudeName;
	static const FName DiseaseResistMagnitudeName;
};
