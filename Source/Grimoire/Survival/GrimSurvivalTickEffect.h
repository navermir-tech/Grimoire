#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GrimSurvivalTickEffect.generated.h"

// 생존 GE 들. 모두 에셋 없이 C++ CDO 로 쓴다: MakeOutgoingSpec(UGrim...::StaticClass(), ...).

/**
 * GE_Survival_Tick — 생존 틱 하나의 HP · 배고픔 · 체온 변화(즉시, 베이스 값 가산). 크기는 SetByCaller 이름으로 넣는다.
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

/** GE_Survival_TickExtra — 같은 틱의 피곤함 · 마나(감기) 변화(즉시, 베이스 가산). 0 이 아닐 때만 적용한다. */
UCLASS()
class GRIMOIRE_API UGrimSurvivalExtraTickEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGrimSurvivalExtraTickEffect();

	static const FName FatigueMagnitudeName;
	static const FName ManaMagnitudeName;
};

/**
 * GE_Survival_Penalty — 체온 · 피곤함 · 잠 · 쓰러짐 패널티 배율(무한, MultiplyCompound).
 * 배율이 바뀌면 컴포넌트가 떼고 새 값으로 다시 붙인다(동시에 하나).
 */
UCLASS()
class GRIMOIRE_API UGrimSurvivalPenaltyEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGrimSurvivalPenaltyEffect();

	static const FName MoveSpeedMagnitudeName;
	static const FName CastSpeedMagnitudeName;
	static const FName ManaCostMagnitudeName;
};

/** 질병 공통 — 지속 시간은 SetByCaller(설정 값). 시간이 다 되면 낫는다. */
UCLASS(Abstract)
class GRIMOIRE_API UGrimDiseaseEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGrimDiseaseEffect();

	static const FName DurationMagnitudeName;

protected:
	/** 하위 클래스 생성자에서 부른다 — 대상에게 태그를 준다. */
	void GrantTag(const FGameplayTag& Tag);
};

/** GE_Disease_Stomach — 배탈(Status.Sick.Stomach). */
UCLASS()
class GRIMOIRE_API UGrimDiseaseStomachEffect : public UGrimDiseaseEffect
{
	GENERATED_BODY()

public:
	UGrimDiseaseStomachEffect();
};

/** GE_Disease_Cold — 감기(Status.Sick.Cold). */
UCLASS()
class GRIMOIRE_API UGrimDiseaseColdEffect : public UGrimDiseaseEffect
{
	GENERATED_BODY()

public:
	UGrimDiseaseColdEffect();
};

/**
 * GE_ClimateResist — 더위 · 추위저항 가산(무한). 장비(M4) 전 시험용: 콘솔 grim.Survival.Resist.
 * 장비 · 인챈트 · 마법 버프도 같은 모양(HeatResist · ColdResist 에 더하는 GE)으로 만든다.
 */
UCLASS()
class GRIMOIRE_API UGrimClimateResistEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGrimClimateResistEffect();

	static const FName HeatMagnitudeName;
	static const FName ColdMagnitudeName;
};
