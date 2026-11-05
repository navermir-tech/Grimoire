#include "Magic/GrimMagicSettings.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

float UGrimMagicSettings::GetAffinity(const FGameplayTag& Attacker, const FGameplayTag& Defender) const
{
	return FindAffinity(ElementAffinities, DefaultAffinity, Attacker, Defender);
}

float UGrimMagicSettings::FindAffinity(const TArray<FGrimElementAffinity>& Table, float DefaultMultiplier, const FGameplayTag& Attacker, const FGameplayTag& Defender)
{
	if (!Attacker.IsValid() || !Defender.IsValid())
	{
		return 1.f;
	}

	for (const FGrimElementAffinity& Row : Table)
	{
		if (Row.Attacker == Attacker && Row.Defender == Defender)
		{
			return Row.Multiplier;
		}
	}
	return DefaultMultiplier;
}

float UGrimMagicSettings::GetMaxManaForCircle(int32 InCircle) const
{
	if (MaxManaByCircle.IsEmpty())
	{
		return 0.f;
	}
	const int32 Index = FMath::Clamp(InCircle - 1, 0, MaxManaByCircle.Num() - 1);
	return MaxManaByCircle[Index];
}

bool UGrimMagicSettings::ShouldGrantStartingSpells() const
{
	return bGrantStartingSpells || FParse::Param(FCommandLine::Get(), TEXT("GrimStartingSpells"));
}

