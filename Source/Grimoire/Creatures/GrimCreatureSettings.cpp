#include "Creatures/GrimCreatureSettings.h"
#include "Creatures/GrimCreatureSpecies.h"

UGrimCreatureSpecies* UGrimCreatureSettings::FindSpecies(FName SpeciesId) const
{
	if (SpeciesId.IsNone())
	{
		return nullptr;
	}
	for (const TSoftObjectPtr<UGrimCreatureSpecies>& Ref : Species)
	{
		UGrimCreatureSpecies* Loaded = Ref.LoadSynchronous();
		if (Loaded && (Loaded->SpeciesId == SpeciesId || Loaded->GetFName() == SpeciesId))
		{
			return Loaded;
		}
	}
	return nullptr;
}

FGameplayTagContainer UGrimCreatureSettings::GetMaterialsForElements(const FGameplayTag& Primary, const FGameplayTag& Secondary) const
{
	FGameplayTagContainer Result;
	for (const FGrimCreatureElementMaterial& Row : ElementMaterials)
	{
		if (!Row.Element.IsValid())
		{
			continue;
		}
		if ((Primary.IsValid() && Primary.MatchesTag(Row.Element)) || (Secondary.IsValid() && Secondary.MatchesTag(Row.Element)))
		{
			Result.AppendTags(Row.Materials);
		}
	}
	return Result;
}

FName UGrimCreatureSettings::GetQuestEventName(const FString& Prefix, EGrimCreatureSizeClass SizeClass)
{
	const UEnum* Enum = StaticEnum<EGrimCreatureSizeClass>();
	const FString SizeName = Enum ? Enum->GetNameStringByValue(static_cast<int64>(SizeClass)) : TEXT("Small");
	return FName(*(Prefix + SizeName));
}
