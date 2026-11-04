#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GrimSpellAbility.generated.h"

class UGrimSpellDefinition;
class AGrimWand;
class AGrimPlayerState;

/**
 * 마법 시전 공용 어빌리티 — 투사체 · 광선 (R01 · R07 · R16).
 * 무슨 마법인지는 Spec 의 SourceObject(UGrimSpellDefinition)가 정한다. 그래서 한 클래스로 여러 마법을 낸다.
 * 비용 · 쿨다운은 공용 GE(UGrimGE_SpellCost · UGrimGE_SpellCooldown)에 정의의 수치를 SetByCaller 로 넣는다.
 */
UCLASS()
class GRIMOIRE_API UGrimSpellAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGrimSpellAbility();

	/** Spec 의 SourceObject 에서 정의를 꺼낸다. */
	static const UGrimSpellDefinition* GetSpellDefinition(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo);

	/** 이 시전자가 쥔 완드(PlayerState 의 마법서). 없으면 nullptr. */
	static AGrimWand* GetWand(const FGameplayAbilityActorInfo* ActorInfo);

	/**
	 * 서버: 맞은 대상 하나에 마법을 적용한다.
	 * 대상에 ASC 가 있으면 DamageSpec 을 적용하고, 속성 자극은 받는 쪽이 있으면 언제나 준다.
	 */
	static void ApplySpellHit(const UGrimSpellDefinition* Definition, UAbilitySystemComponent* SourceASC, const FGameplayEffectSpecHandle& DamageSpec, AActor* Instigator, AActor* HitActor);

	/** 대상 액터의 ASC. 폰이 직접 갖지 않으면 그 폰의 PlayerState 에서 찾는다. */
	static UAbilitySystemComponent* FindTargetASC(AActor* Actor);

	//~ UGameplayAbility
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	virtual bool CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual UGameplayEffect* GetCostGameplayEffect() const override;
	virtual UGameplayEffect* GetCooldownGameplayEffect() const override;

	/** 완드 보정까지 넣은 마나 비용. */
	static float GetEffectiveManaCost(const UGrimSpellDefinition* Definition, const AGrimWand* Wand);

	/** 완드 보정까지 넣은 쿨다운(초). */
	static float GetEffectiveCooldown(const UGrimSpellDefinition* Definition, const AGrimWand* Wand);

protected:
	/** 서버: 투사체를 쏘거나 광선을 긋는다. */
	void Deliver();

	UFUNCTION()
	void OnCastTimeFinished();

	/** 조준: 시점에서 사거리만큼 앞의 점. */
	FVector GetAimPoint(const UGrimSpellDefinition* Definition, const FVector& Muzzle) const;

	FGameplayEffectSpecHandle MakeDamageSpec(const UGrimSpellDefinition* Definition, const AGrimWand* Wand) const;

	void DeliverProjectile(const UGrimSpellDefinition* Definition, AGrimWand* Wand, const FVector& Muzzle, const FVector& AimPoint, const FGameplayEffectSpecHandle& DamageSpec);
	void DeliverBeam(const UGrimSpellDefinition* Definition, const FVector& Muzzle, const FVector& AimPoint, const FGameplayEffectSpecHandle& DamageSpec);

	/** GetCooldownTags 가 돌려줄 임시 컨테이너(인스턴스별). */
	mutable FGameplayTagContainer CooldownTagsScratch;
};
