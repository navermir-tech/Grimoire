#include "Survival/GrimSurvivalSettings.h"

FGrimSurvivalRules UGrimSurvivalSettings::MakeRules() const
{
	FGrimSurvivalRules Rules;
	Rules.Vital = Tuning;
	Rules.Climate = Climate;
	Rules.Fatigue = Fatigue;
	Rules.Disease = Disease;
	Rules.Sleep = Sleep;
	Rules.Downed = Downed;
	return Rules;
}
