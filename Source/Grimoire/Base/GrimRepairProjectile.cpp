#include "Base/GrimRepairProjectile.h"
#include "Base/GrimBaseCore.h"
#include "Base/GrimBaseStations.h"
#include "Base/GrimBuilderComponent.h"
#include "Base/GrimStructureComponent.h"
#include "Magic/GrimSpellAbility.h"
#include "Magic/GrimSpellbookComponent.h"
#include "Core/GrimSpellFXTypes.h"
#include "AbilitySystemComponent.h"

void AGrimRepairProjectile::HandleHit(AActor* HitActor, const FHitResult& Hit, bool bHasHitResult)
{
	const bool bRepairable = HitActor && (UGrimStructureComponent::FindOn(HitActor) || Cast<AGrimWorkbench>(HitActor) || Cast<AGrimBaseCore>(HitActor));
	if (!bRepairable || !HasAuthority() || IsFake() || IsActorBeingDestroyed() || !Definition)
	{
		Super::HandleHit(HitActor, Hit, bHasHitResult);
		return;
	}
	const TWeakObjectPtr<AActor> HitKey(HitActor);
	if (HitActors.Contains(HitKey))
	{
		return;
	}
	HitActors.Add(HitKey);

	UAbilitySystemComponent* Caster = SourceASC.Get();
	if (UGrimBuilderComponent* Builder = UGrimBuilderComponent::FindForActor(Caster ? Caster->GetOwnerActor() : nullptr))
	{
		if (Builder->PerformRepair(HitActor, true, false) > 0)
		{
			// 숙련도(Q26): 무언가를 고친 것은 의미 있는 명중.
			if (UGrimSpellbookComponent* Spellbook = UGrimSpellbookComponent::FindForASC(Caster))
			{
				Spellbook->AwardHitXP(Definition, HitActor);
			}
		}
	}

	const FVector Location = bHasHitResult && !Hit.ImpactPoint.IsNearlyZero() ? FVector(Hit.ImpactPoint) : GetActorLocation();
	const FVector Normal = bHasHitResult && !Hit.ImpactNormal.IsNearlyZero() ? FVector(Hit.ImpactNormal) : -GetActorForwardVector();
	UGrimSpellAbility::ExecuteSpellCue(Caster, GrimCueTags::Spell_Impact, Definition, GetInstigator(), Location, Normal);
	Destroy();
}
