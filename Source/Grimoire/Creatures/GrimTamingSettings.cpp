#include "Creatures/GrimTamingSettings.h"

const FGrimPersonalityTuning* UGrimTamingSettings::FindPersonality(EGrimCreaturePersonality Personality) const
{
	return Personalities.FindByPredicate([Personality](const FGrimPersonalityTuning& Row) { return Row.Personality == Personality; });
}

const FGrimLoyaltyStageTuning* UGrimTamingSettings::FindStage(EGrimLoyaltyStage Stage) const
{
	return Loyalty.Stages.FindByPredicate([Stage](const FGrimLoyaltyStageTuning& Row) { return Row.Stage == Stage; });
}

const FGrimMountProfile* UGrimTamingSettings::FindMountProfile(FName SpeciesId) const
{
	return Mount.Profiles.FindByPredicate([SpeciesId](const FGrimMountProfile& Row) { return Row.SpeciesId == SpeciesId; });
}

const FGrimCreatureFoodRule* UGrimTamingSettings::FindFoodRule(FName SpeciesId) const
{
	return FoodRules.FindByPredicate([SpeciesId](const FGrimCreatureFoodRule& Row) { return Row.SpeciesId == SpeciesId; });
}

TArray<FName> UGrimTamingSettings::GetAcceptedGroups(FName SpeciesId, EGrimCreatureDiet Diet) const
{
	if (const FGrimCreatureFoodRule* Rule = FindFoodRule(SpeciesId); Rule && Rule->AcceptedGroups.Num() > 0)
	{
		return Rule->AcceptedGroups;
	}
	if (const FGrimDietGroups* Row = DietGroups.FindByPredicate([Diet](const FGrimDietGroups& R) { return R.Diet == Diet; }))
	{
		return Row->Groups;
	}
	return {};
}

float UGrimTamingSettings::GetEventDelta(EGrimLoyaltyEvent Event) const
{
	const FGrimLoyaltyEventDelta* Row = Loyalty.EventDeltas.FindByPredicate([Event](const FGrimLoyaltyEventDelta& R) { return R.Event == Event; });
	return Row ? Row->Delta : 0.f;
}
