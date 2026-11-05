#pragma once

#include "CoreMinimal.h"
#include "Magic/GrimSpellAbility.h"
#include "Magic/GrimBasicAttackTypes.h"
#include "GrimBasicAttackAbility.generated.h"

/**
 * 기본 공격 (R26) — 완드로 순수 마력(무속성) 볼트를 쏜다. 정의는 DA_Spell_BasicAttack(비용 0 · 쿨다운 0 · 숙련도 없음).
 * 시전 흐름(조준 · 예측 · 가짜 볼트 · 큐 · 책장 넘김)은 부모 UGrimSpellAbility 그대로이고, 여기서 더하는 것:
 *  - 연타 리듬: 한 발 사이 AttackInterval(쿨다운이 아님 — 시전자는 그 전 누름을 무시, 서버는 Tolerance 만큼 봐준다).
 *  - 연타: 첫 발부터 ComboWindow 안의 ComboLength 번째가 강타(피해 × HeavyDamageMultiplier, 밀어냄, 큰 볼트).
 *    서버와 시전자가 각자 시계로 같은 규칙을 센다 — 판정 · 피해는 서버 값, 시전자 값은 가짜 볼트 크기에만 쓴다.
 *  - 원격 시전자는 가짜 볼트를 쏘자마자 끝낸다(끝 RPC 는 조준 RPC 뒤에 가므로 서버는 이미 쏠 수 있다) — 리듬이 왕복 지연에 묶이지 않는다.
 *  - 처음부터 알지 않는다 — 튜토리얼 첫 단계의 「기본 공격 스킬북」(AGrimSkillbook::TeachesSpell = DA_Spell_BasicAttack)으로 배운다.
 *  - 완드가 없으면(부러진 완드 포함 — Q4) 부모처럼 못 쓴다 — 같은 입력의 주먹질(UGrimFistAbility)이 대신 나간다. 배우기 전에도 주먹질.
 */
UCLASS()
class GRIMOIRE_API UGrimBasicAttackAbility : public UGrimSpellAbility
{
	GENERATED_BODY()

public:
	UGrimBasicAttackAbility();

	/** HUD: 이 ASC 의 기본 공격이 지금 몇 번째까지 이어졌나(0 = 연타 아님). 이 기기의 값. */
	static int32 GetVisibleComboStep(const UAbilitySystemComponent* ASC);

	/** 마지막 발의 번째 · 강타 여부(이 기기) */
	int32 GetLastStep() const { return Combo.Step; }
	bool WasLastHeavy() const { return bCurrentHeavy; }

	//~ UGameplayAbility
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	virtual FGameplayEffectSpecHandle MakeDamageSpec(const UGrimSpellDefinition* Definition, const AGrimWand* Wand) const override;
	virtual void OnSpellProjectileSpawning(AGrimSpellProjectile* Projectile) const override;

	FGrimBasicAttackCombo Combo;
	bool bCurrentHeavy = false;
};
