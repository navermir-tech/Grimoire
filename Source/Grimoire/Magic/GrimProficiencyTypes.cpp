#include "Magic/GrimProficiencyTypes.h"

void FGrimProficiencyMath::ReadGrades(const UDataTable* Table, TArray<FGrimProficiencyGrade>& OutGrades)
{
	OutGrades.Reset();
	if (!Table || Table->GetRowStruct() != FGrimProficiencyGradeRow::StaticStruct())
	{
		return;
	}
	for (const TPair<FName, uint8*>& Pair : Table->GetRowMap())
	{
		const FGrimProficiencyGradeRow* Row = reinterpret_cast<const FGrimProficiencyGradeRow*>(Pair.Value);
		if (!Row)
		{
			continue;
		}
		FGrimProficiencyGrade& Grade = OutGrades.AddDefaulted_GetRef();
		Grade.Grade = Pair.Key;
		Grade.DisplayName = Row->DisplayName;
		Grade.XPToReach = Row->XPToReach;
		Grade.ManaCostMultiplier = Row->ManaCostMultiplier;
		Grade.CooldownMultiplier = Row->CooldownMultiplier;
		Grade.ExtraEffectScale = Row->ExtraEffectScale;
	}
}

int32 FGrimProficiencyMath::GetGradeIndex(float XP, TConstArrayView<FGrimProficiencyGrade> Grades, float XPScale)
{
	if (Grades.IsEmpty())
	{
		return INDEX_NONE;
	}
	int32 Result = 0;
	for (int32 Index = 1; Index < Grades.Num(); ++Index)
	{
		// 문턱은 행 순서대로 커진다고 본다. 작아지는 행이 있으면 거기서 멈춘다(표 실수로 등급이 건너뛰지 않게).
		if (Grades[Index].XPToReach < Grades[Index - 1].XPToReach)
		{
			break;
		}
		if (XP + KINDA_SMALL_NUMBER >= GetGradeThreshold(Grades, Index, XPScale))
		{
			Result = Index;
		}
		else
		{
			break;
		}
	}
	return Result;
}

float FGrimProficiencyMath::GetGradeThreshold(TConstArrayView<FGrimProficiencyGrade> Grades, int32 Index, float XPScale)
{
	return Grades.IsValidIndex(Index) ? FMath::Max(0.f, Grades[Index].XPToReach) * FMath::Max(XPScale, KINDA_SMALL_NUMBER) : 0.f;
}

FGrimProficiencyMultipliers FGrimProficiencyMath::GetMultipliers(TConstArrayView<FGrimProficiencyGrade> Grades, int32 Index)
{
	FGrimProficiencyMultipliers Result;
	if (Grades.IsValidIndex(Index))
	{
		Result.ManaCost = FMath::Max(0.f, Grades[Index].ManaCostMultiplier);
		Result.Cooldown = FMath::Max(0.f, Grades[Index].CooldownMultiplier);
		Result.ExtraEffectScale = FMath::Max(0.f, Grades[Index].ExtraEffectScale);
	}
	return Result;
}

FGrimProficiencyView FGrimProficiencyMath::MakeView(float XP, TConstArrayView<FGrimProficiencyGrade> Grades, float XPScale)
{
	FGrimProficiencyView View;
	View.XP = XP;
	View.GradeIndex = GetGradeIndex(XP, Grades, XPScale);
	if (View.GradeIndex == INDEX_NONE)
	{
		return View;
	}
	View.bTracked = true;
	View.Grade = Grades[View.GradeIndex].Grade;
	View.GradeName = Grades[View.GradeIndex].DisplayName;
	View.Multipliers = GetMultipliers(Grades, View.GradeIndex);
	View.CurrentGradeXP = GetGradeThreshold(Grades, View.GradeIndex, XPScale);
	View.bMaxGrade = !Grades.IsValidIndex(View.GradeIndex + 1) || Grades[View.GradeIndex + 1].XPToReach < Grades[View.GradeIndex].XPToReach;
	if (View.bMaxGrade)
	{
		View.NextGradeXP = View.CurrentGradeXP;
		View.Progress = 1.f;
	}
	else
	{
		View.NextGradeXP = GetGradeThreshold(Grades, View.GradeIndex + 1, XPScale);
		const float Span = View.NextGradeXP - View.CurrentGradeXP;
		View.Progress = Span > KINDA_SMALL_NUMBER ? FMath::Clamp((XP - View.CurrentGradeXP) / Span, 0.f, 1.f) : 1.f;
	}
	return View;
}

float FGrimProficiencyMath::ComputeXPGain(float BaseXP, int32 RepeatCount, float RepeatDecay, float GlobalRate)
{
	const float Decay = FMath::Clamp(RepeatDecay, 0.f, 1.f);
	const float RepeatFactor = RepeatCount <= 0 ? 1.f : FMath::Pow(Decay, static_cast<float>(RepeatCount));
	return FMath::Max(0.f, BaseXP) * RepeatFactor * FMath::Max(0.f, GlobalRate);
}

float FGrimProficiencyMath::ComputeSkillbookXP(const FGrimProficiencyView& View, float FractionOfGrade, float FlatXP)
{
	if (!View.bTracked || View.bMaxGrade)
	{
		return 0.f;
	}
	const float Span = FMath::Max(0.f, View.NextGradeXP - View.CurrentGradeXP);
	return Span * FMath::Clamp(FractionOfGrade, 0.f, 1.f) + FMath::Max(0.f, FlatXP);
}

int32 FGrimRepeatTracker::Register(const FGrimRepeatKey& Key, double Now, double WindowSeconds)
{
	FEntry& Entry = Entries.FindOrAdd(Key);
	if (Entry.Count > 0 && Now - Entry.LastTime > WindowSeconds)
	{
		Entry.Count = 0;
	}
	const int32 Before = Entry.Count;
	++Entry.Count;
	Entry.LastTime = Now;
	return Before;
}

void FGrimRepeatTracker::Prune(double Now, double WindowSeconds)
{
	for (auto It = Entries.CreateIterator(); It; ++It)
	{
		if (Now - It.Value().LastTime > WindowSeconds)
		{
			It.RemoveCurrent();
		}
	}
}
