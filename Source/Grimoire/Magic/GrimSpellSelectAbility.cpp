#include "Magic/GrimSpellSelectAbility.h"
#include "Magic/GrimSpellbookComponent.h"
#include "Core/GrimGameplayTags.h"
#include "Core/GrimPlayerState.h"
#include "AbilitySystemComponent.h"

UGrimSpellSelectAbility::UGrimSpellSelectAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UGrimSpellSelectAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const AGrimPlayerState* PlayerState = Cast<AGrimPlayerState>(ActorInfo->OwnerActor.Get());
	UGrimSpellbookComponent* Spellbook = PlayerState ? PlayerState->GetSpellbook() : nullptr;
	const FGameplayAbilitySpec* Spec = ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle);

	if (Spellbook && Spec)
	{
		const int32 Step = Spec->GetDynamicSpecSourceTags().HasTagExact(GrimTags::Input_Spell_Prev) ? -1 : 1;
		Spellbook->CycleSelection(Step);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
