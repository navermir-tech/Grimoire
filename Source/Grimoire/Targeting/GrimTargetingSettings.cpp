#include "Targeting/GrimTargetingSettings.h"
#include "Core/GrimGameplayTags.h"
#include "Interaction/GrimInteractionTags.h"

DEFINE_LOG_CATEGORY(LogGrimTargeting);

UGrimTargetingSettings::UGrimTargetingSettings()
{
	CategoryName = TEXT("Game");
	SectionName = TEXT("Grimoire Targeting");

	// 적 · 생물(Pawn), 사물(WorldDynamic · PhysicsBody). WorldStatic(지형 · 건물)은 오버랩 대상이 아니다.
	CandidateObjectChannels = { ECC_WorldDynamic, ECC_Pawn, ECC_PhysicsBody };

	// 줌인 조준 정보 글자(R24). 한국어라 ini 대신 여기 둔다 — ini 에서 +ZoomStateLabels 로 더할 수 있다.
	auto AddLabel = [this](const FGameplayTag& State, const FText& Label)
	{
		FGrimStateLabel& Entry = ZoomStateLabels.AddDefaulted_GetRef();
		Entry.State = State;
		Entry.Label = Label;
	};
	AddLabel(GrimTags::State_Wet, NSLOCTEXT("GrimTargeting", "StateWet", "젖음"));
	AddLabel(GrimTags::State_Burning, NSLOCTEXT("GrimTargeting", "StateBurning", "불붙음"));
	AddLabel(GrimInteractionTags::State_Charred, NSLOCTEXT("GrimTargeting", "StateCharred", "다 탐"));
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
