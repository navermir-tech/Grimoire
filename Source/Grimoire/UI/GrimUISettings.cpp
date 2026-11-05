#include "UI/GrimUISettings.h"

const FGrimElementDisplay* UGrimUISettings::FindElementDisplay(const FGameplayTag& Element) const
{
	if (!Element.IsValid())
	{
		return nullptr;
	}
	// 가장 가까운(깊은) 항목: 정확히 같은 것 → 부모 순.
	const FGrimElementDisplay* Best = nullptr;
	int32 BestDepth = -1;
	for (const FGrimElementDisplay& Display : Elements)
	{
		if (Display.Element.IsValid() && Element.MatchesTag(Display.Element))
		{
			const int32 Depth = Display.Element.GetGameplayTagParents().Num();
			if (Depth > BestDepth)
			{
				Best = &Display;
				BestDepth = Depth;
			}
		}
	}
	return Best;
}

FText UGrimUISettings::GetCircleTitle(int32 Circle) const
{
	return CircleTitles.IsValidIndex(Circle - 1) ? CircleTitles[Circle - 1] : FText::GetEmpty();
}
