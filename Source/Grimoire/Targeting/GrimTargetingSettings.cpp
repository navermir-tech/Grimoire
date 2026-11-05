#include "Targeting/GrimTargetingSettings.h"

DEFINE_LOG_CATEGORY(LogGrimTargeting);

UGrimTargetingSettings::UGrimTargetingSettings()
{
	CategoryName = TEXT("Game");
	SectionName = TEXT("Grimoire Targeting");

	// 적 · 생물(Pawn), 사물(WorldDynamic · PhysicsBody). WorldStatic(지형 · 건물)은 오버랩 대상이 아니다.
	CandidateObjectChannels = { ECC_WorldDynamic, ECC_Pawn, ECC_PhysicsBody };
}

FColor UGrimTargetingSettings::GetElementColor(const FGameplayTag& Element) const
{
	for (FGameplayTag Tag = Element; Tag.IsValid(); Tag = Tag.RequestDirectParent())
	{
		for (const FGrimElementColor& Entry : ElementColors)
		{
			if (Entry.Element == Tag)
			{
				return Entry.Color;
			}
		}
	}
	return MarkerDefaultColor;
}
