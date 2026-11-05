#include "Magic/GrimPactumProjectile.h"
#include "Magic/GrimSpellAbility.h"
#include "Magic/GrimSpellDefinition.h"
#include "Magic/GrimSpellbookComponent.h"
#include "Creatures/GrimCreature.h"
#include "Creatures/GrimTamingSubsystem.h"
#include "Core/GrimSpellFXTypes.h"
#include "AbilitySystemComponent.h"

void AGrimPactumProjectile::HandleHit(AActor* HitActor, const FHitResult& Hit, bool bHasHitResult)
{
	AGrimCreature* Creature = Cast<AGrimCreature>(HitActor);
	if (!Creature || !HasAuthority() || IsFake() || IsActorBeingDestroyed() || !Definition)
	{
		Super::HandleHit(HitActor, Hit, bHasHitResult);
		return;
	}
	const TWeakObjectPtr<AActor> HitKey(static_cast<AActor*>(Creature));
	if (HitActors.Contains(HitKey))
	{
		return;
	}
	HitActors.Add(HitKey);

	UAbilitySystemComponent* Caster = SourceASC.Get();
	// 숙련도(Q26): 생물에 맞힌 것은 의미 있는 명중.
	if (UGrimSpellbookComponent* Spellbook = UGrimSpellbookComponent::FindForASC(Caster))
	{
		Spellbook->AwardHitXP(Definition, Creature);
	}
	if (UGrimTamingSubsystem* Taming = UGrimTamingSubsystem::Get(this))
	{
		Taming->TryTame(Definition, Caster, Creature);
	}

	const FVector Location = bHasHitResult && !Hit.ImpactPoint.IsNearlyZero() ? FVector(Hit.ImpactPoint) : GetActorLocation();
	const FVector Normal = bHasHitResult && !Hit.ImpactNormal.IsNearlyZero() ? FVector(Hit.ImpactNormal) : -GetActorForwardVector();
	UGrimSpellAbility::ExecuteSpellCue(Caster, GrimCueTags::Spell_Impact, Definition, GetInstigator(), Location, Normal);
	UE_LOG(LogTemp, Verbose, TEXT("[Pactum] %s 맞음 %s"), *Definition->GetName(), *Creature->GetName());
	Destroy();
}
