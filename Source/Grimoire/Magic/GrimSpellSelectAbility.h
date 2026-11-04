#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GrimSpellSelectAbility.generated.h"

/**
 * 다음 / 이전 마법 선택. 마법서가 Input.Spell.Next · Input.Spell.Prev 를 붙여 두 번 지급한다.
 * 서버에서만 돈다 — 선택 상태(UGrimSpellbookComponent::SelectedIndex)가 복제된다.
 */
UCLASS()
class GRIMOIRE_API UGrimSpellSelectAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGrimSpellSelectAbility();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
