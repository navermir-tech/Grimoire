#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Magic/GrimBasicAttackTypes.h"
#include "GrimFistAbility.generated.h"

/**
 * 주먹질 (Q4 · R26, 임시) — 기본 공격 입력(Input.Spell.BasicAttack: 좌클릭 · 패드 R2/RT 짧게)의 대신.
 *  - 기본 공격을 아직 배우지 않았거나(튜토리얼 첫 단계의 「기본 공격 스킬북」 전), 완드가 없을 때(부러진 완드 포함) 나간다.
 *    기본 공격을 알고 완드가 있으면 CanActivate 가 false — 같은 입력의 기본 공격(UGrimBasicAttackAbility)이 나간다.
 *  - 서버가 앞쪽 짧은 구 판정: 받는 쪽 · ASC 가 있는 첫 대상에 작은 피해 · 무속성 「충격」 자극. 동작 · 책장 · 이펙트 없음(임시).
 *  - 연타 리듬은 기본 공격과 같은 AttackInterval(강타 없음). 수치는 UGrimBasicAttackSettings 의 Fist*.
 * 마법 정의가 없는 어빌리티라 마법서가 아니라 캐릭터가 서버에서 한 번 준다(EnsureGranted).
 */
UCLASS()
class GRIMOIRE_API UGrimFistAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGrimFistAbility();

	/** 서버: 이 ASC 에 주먹질이 없으면 Input.Spell.BasicAttack 태그로 준다. 있거나 줬으면 true. */
	static bool EnsureGranted(UAbilitySystemComponent* ASC);

	/** 이 ASC 가 지금 기본 공격(마법)을 쓸 수 있나 — 기본 공격 Spec 이 있고 완드가 있다. 그러면 주먹질은 물러난다. */
	static bool CanUseBasicAttackSpell(const FGameplayAbilityActorInfo* ActorInfo);

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	/** 서버: 앞쪽 짧은 구 판정 */
	void PerformFist();

	FGrimBasicAttackCombo Combo;
};
