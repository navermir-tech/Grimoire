#pragma once

#include "CoreMinimal.h"

/** 마법 수치 계산 — 어빌리티 · 피해 계산 · 테스트가 같은 식을 쓴다. 수치 자체는 데이터에 있다. */
namespace GrimMagicMath
{
	/** 완드 마나 효율(0 = 그대로, 0.2 = 20% 절약)을 반영한 비용. 음수가 되지 않는다. */
	inline float CalcManaCost(float BaseCost, float ManaEfficiency)
	{
		return FMath::Max(0.f, BaseCost * (1.f - FMath::Clamp(ManaEfficiency, 0.f, 1.f)));
	}

	/** 완드 시전 속도(1 = 그대로, 2 = 두 배 빠름)를 반영한 쿨다운. */
	inline float CalcCooldown(float BaseCooldown, float CastSpeed)
	{
		return FMath::Max(0.f, BaseCooldown) / FMath::Max(CastSpeed, KINDA_SMALL_NUMBER);
	}

	inline bool CanAfford(float CurrentMana, float Cost)
	{
		return CurrentMana + KINDA_SMALL_NUMBER >= Cost;
	}

	/** 최종 피해 = 기본 피해 × 완드 속성 위력 × 상성. */
	inline float CalcDamage(float BaseDamage, float WandPower, float Affinity)
	{
		return FMath::Max(0.f, BaseDamage * WandPower * Affinity);
	}
}
