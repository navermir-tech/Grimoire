#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GrimCreatureAttackAbility.generated.h"

/**
 * 생물 공격(속성 하나). 서버 전용 — 생물 ASC 가 공격마다 Spec 하나를 가진다(동적 소스 태그 = 그 공격의 Element.*).
 * AGrimCreature::TryAttack 이 대상을 정하고 발동한다 → 대상이 사거리 안이면 마법 피해 GE(UGrimGE_SpellDamage — 상성 · 두 속성 Q35 그대로)
 * + 속성 자극(출처 Creature) + 넉백. 플레이어 · 다른 생물 · 사물에 같은 길로 닿는다.
 */
UCLASS()
class GRIMOIRE_API UGrimCreatureAttackAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGrimCreatureAttackAbility();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
