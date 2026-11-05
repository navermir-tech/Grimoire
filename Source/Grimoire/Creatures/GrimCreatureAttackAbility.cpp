#include "Creatures/GrimCreatureAttackAbility.h"
#include "Creatures/GrimCreature.h"
#include "Creatures/GrimCreatureSpecies.h"
#include "Core/GrimElementalTypes.h"
#include "Magic/GrimDamageExecution.h"
#include "Magic/GrimMagicEffects.h"
#include "Magic/GrimMagicTags.h"
#include "Magic/GrimSpellAbility.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"

UGrimCreatureAttackAbility::UGrimCreatureAttackAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UGrimCreatureAttackAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AGrimCreature* Creature = Cast<AGrimCreature>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	const FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec();
	if (!Creature || !SourceASC || !Spec || !Creature->GetSpecies() || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FGameplayTag Element = UGrimDamageExecution::FindElementTag(Spec->GetDynamicSpecSourceTags());
	const FGrimCreatureAttack* Attack = Creature->GetSpecies()->FindAttack(Element);
	AActor* Target = Creature->GetPendingAttackTarget();
	if (!Attack || !Target || Creature->GetSurfaceDistanceTo(Target) > Attack->Range)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	float AppliedDamage = 0.f;
	UAbilitySystemComponent* TargetASC = UGrimSpellAbility::FindTargetASC(Target);
	if (TargetASC && TargetASC != SourceASC && !UGrimSpellAbility::IsCombatBlocked(SourceASC, TargetASC))
	{
		FGameplayEffectSpecHandle Damage = MakeOutgoingGameplayEffectSpec(UGrimGE_SpellDamage::StaticClass(), GetAbilityLevel());
		if (Damage.IsValid())
		{
			AppliedDamage = Attack->Damage * Creature->GetSpecies()->Power * Creature->GetInstance().StatScale;
			Damage.Data->SetSetByCallerMagnitude(GrimMagicTags::SetByCaller_Damage, AppliedDamage);
			Damage.Data->AddDynamicAssetTag(Attack->Element);
			SourceASC->ApplyGameplayEffectSpecToTarget(*Damage.Data.Get(), TargetASC);
		}
	}

	if (Attack->StimulusIntensity > 0.f)
	{
		FGrimElementalStimulus Stimulus;
		Stimulus.Element = Attack->Element;
		Stimulus.Intensity = Attack->StimulusIntensity;
		Stimulus.Source = EGrimStimulusSource::Creature;
		UGrimElementalStatics::ApplyStimulusToActor(Target, Stimulus, Creature);
	}

	if (Attack->Knockback > 0.f)
	{
		if (ACharacter* TargetCharacter = Cast<ACharacter>(Target))
		{
			const FVector Away = (Target->GetActorLocation() - Creature->GetActorLocation()).GetSafeNormal2D();
			TargetCharacter->LaunchCharacter(Away * Attack->Knockback + FVector(0.f, 0.f, Attack->Knockback * 0.25f), true, false);
		}
	}

	UE_LOG(LogGrimCreatures, Display, TEXT("[Server] 생물 공격 %s(%s) seed=%d → %s %s 피해 %.1f"),
		*Creature->GetInstance().SpeciesId.ToString(), *Attack->AttackName.ToString(), Creature->GetInstance().Seed,
		*Target->GetName(), *Attack->Element.ToString(), AppliedDamage);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
