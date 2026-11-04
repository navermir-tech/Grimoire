#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GrimSurvivalTickEffect.generated.h"

/**
 * GE_Survival_Tick — 생존 틱 하나의 변화(즉시, 베이스 값 가산). 크기는 SetByCaller 이름으로 넣는다.
 * 에셋 없이 C++ CDO 로 쓴다: MakeOutgoingSpec(UGrimSurvivalTickEffect::StaticClass(), ...).
 */
UCLASS()
class GRIMOIRE_API UGrimSurvivalTickEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGrimSurvivalTickEffect();

	static const FName HealthMagnitudeName;
	static const FName HungerMagnitudeName;
	static const FName BodyTemperatureMagnitudeName;
};
