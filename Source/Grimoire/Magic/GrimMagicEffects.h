#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GrimMagicEffects.generated.h"

// 마법 공용 GE. 수치는 모두 SetByCaller(GrimMagicTags::SetByCaller_*)로 들어오므로 GE 자체에는 숫자가 없다.

/** 마나 비용 — 즉시, Mana += SetByCaller.Magic.ManaCost(음수). */
UCLASS()
class GRIMOIRE_API UGrimGE_SpellCost : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGrimGE_SpellCost();
};

/** 쿨다운 — 지속 SetByCaller.Magic.Cooldown 초. 쿨다운 태그는 Spec 의 DynamicGrantedTags 로 붙인다. */
UCLASS()
class GRIMOIRE_API UGrimGE_SpellCooldown : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGrimGE_SpellCooldown();
};

/** 마법 피해 — 즉시, UGrimDamageExecution 이 상성을 적용해 Health 를 깎는다. */
UCLASS()
class GRIMOIRE_API UGrimGE_SpellDamage : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGrimGE_SpellDamage();
};

/** 마나 회복 — 무한 지속, 1초마다 Mana += SetByCaller.Magic.ManaRegen. 상한은 어트리뷰트 셋이 자른다. */
UCLASS()
class GRIMOIRE_API UGrimGE_ManaRegen : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGrimGE_ManaRegen();
};
