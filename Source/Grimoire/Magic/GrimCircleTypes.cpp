#include "Magic/GrimCircleTypes.h"

const FGrimCircleStep* FGrimCircleTable::FindClamped(int32 Circle) const
{
	if (Steps.IsEmpty())
	{
		return nullptr;
	}
	return &Steps[FMath::Clamp(Circle - 1, 0, Steps.Num() - 1)];
}

const FGrimCircleGradeCheck* FGrimCircleCheck::FirstUnmetGrade() const
{
	return Grades.FindByPredicate([](const FGrimCircleGradeCheck& Grade) { return !Grade.IsMet(); });
}

void FGrimCircleMath::ParseGrades(const FString& Text, TArray<FGrimCircleGradeRequirement>& OutGrades, TArray<FString>& OutProblems)
{
	OutGrades.Reset();
	TArray<FString> Parts;
	Text.ParseIntoArray(Parts, TEXT("|"), true);
	for (FString Part : Parts)
	{
		Part.TrimStartAndEndInline();
		FString GradeText;
		FString CountText;
		if (!Part.Split(TEXT(":"), &GradeText, &CountText))
		{
			OutProblems.Add(FString::Printf(TEXT("승급 자격 「%s」: 「등급:수」 꼴이 아니다"), *Part));
			continue;
		}
		GradeText.TrimStartAndEndInline();
		CountText.TrimStartAndEndInline();
		if (GradeText.IsEmpty() || !CountText.IsNumeric() || FCString::Atoi(*CountText) <= 0)
		{
			OutProblems.Add(FString::Printf(TEXT("승급 자격 「%s」: 등급이 비었거나 수가 1 보다 작다"), *Part));
			continue;
		}
		OutGrades.Add({ FName(*GradeText), FCString::Atoi(*CountText) });
	}
}

bool FGrimCircleMath::ReadTable(const UDataTable* Table, FGrimCircleTable& OutTable, TConstArrayView<FName> GradeOrder, TArray<FString>& OutProblems)
{
	OutTable.Steps.Reset();
	if (!Table || Table->GetRowStruct() != FGrimCircleRow::StaticStruct())
	{
		OutProblems.Add(TEXT("표가 없거나 행 구조체가 FGrimCircleRow 가 아니다"));
		return false;
	}

	TArray<TPair<int32, const FGrimCircleRow*>> Rows;
	for (const TPair<FName, uint8*>& Pair : Table->GetRowMap())
	{
		const FString Name = Pair.Key.ToString();
		if (!Name.IsNumeric())
		{
			OutProblems.Add(FString::Printf(TEXT("행 「%s」: 이름이 서클 번호가 아니다"), *Name));
			continue;
		}
		Rows.Add({ FCString::Atoi(*Name), reinterpret_cast<const FGrimCircleRow*>(Pair.Value) });
	}
	Rows.Sort([](const TPair<int32, const FGrimCircleRow*>& A, const TPair<int32, const FGrimCircleRow*>& B) { return A.Key < B.Key; });

	for (int32 Index = 0; Index < Rows.Num(); ++Index)
	{
		const int32 Circle = Rows[Index].Key;
		if (Circle != Index + 1)
		{
			OutProblems.Add(FString::Printf(TEXT("서클 %d: 1 부터 빈틈없이 이어져야 한다(여기서 멈춘다)"), Circle));
			break;
		}
		const FGrimCircleRow& Row = *Rows[Index].Value;
		FGrimCircleStep& Step = OutTable.Steps.AddDefaulted_GetRef();
		Step.Circle = Circle;
		Step.GapPowerPerCircle = FMath::Max(0.f, Row.GapPowerPerCircle);
		Step.GapPowerCap = FMath::Max(0.f, Row.GapPowerCap);
		Step.GapManaPerCircle = FMath::Clamp(Row.GapManaPerCircle, 0.f, 1.f);
		Step.GapManaCap = FMath::Clamp(Row.GapManaCap, 0.f, 1.f);
		Step.MaxEnchantTier = FMath::Max(1, Row.MaxEnchantTier);
		Step.SlotSets = FMath::Max(1, Row.SlotSets);
		Step.MaxSustainedSpells = FMath::Max(1, Row.MaxSustainedSpells);
		Step.EquipTier = FMath::Max(1, Row.EquipTier);
		Step.UnlockText = Row.UnlockText;
		Step.PromotionCurrentCircleSpells = FMath::Max(0, Row.PromotionCurrentCircleSpells);
		Step.PromotionTask = Row.PromotionTask;

		TArray<FString> UnlockParts;
		Row.Unlocks.ParseIntoArray(UnlockParts, TEXT("|"), true);
		for (FString Part : UnlockParts)
		{
			Part.TrimStartAndEndInline();
			if (!Part.IsEmpty())
			{
				Step.Unlocks.Add(FName(*Part));
			}
		}

		TArray<FString> GradeProblems;
		ParseGrades(Row.PromotionGrades, Step.PromotionGrades, GradeProblems);
		for (const FString& Problem : GradeProblems)
		{
			OutProblems.Add(FString::Printf(TEXT("서클 %d: %s"), Circle, *Problem));
		}
		if (GradeOrder.Num() > 0)
		{
			for (const FGrimCircleGradeRequirement& Requirement : Step.PromotionGrades)
			{
				if (!GradeOrder.Contains(Requirement.Grade))
				{
					OutProblems.Add(FString::Printf(TEXT("서클 %d: 등급 「%s」 가 숙련도 등급표에 없다"), Circle, *Requirement.Grade.ToString()));
				}
			}
		}
		if (Circle > 1 && Step.PromotionGrades.IsEmpty())
		{
			OutProblems.Add(FString::Printf(TEXT("서클 %d: 승급 자격(PromotionGrades)이 비었다 — 자격 없이 오른다"), Circle));
		}
	}
	return OutProblems.IsEmpty();
}

namespace GrimCircleMathLocal
{
	float GapBonus(int32 CasterCircle, int32 SpellCircle, float PerCircle, float Cap)
	{
		const int32 Gap = FMath::Max(0, CasterCircle - SpellCircle);
		return FMath::Min(Gap * PerCircle, Cap);
	}
}

float FGrimCircleMath::GetGapPowerBonus(const FGrimCircleTable& Table, int32 CasterCircle, int32 SpellCircle)
{
	const FGrimCircleStep* Step = Table.FindClamped(CasterCircle);
	return Step ? GrimCircleMathLocal::GapBonus(CasterCircle, SpellCircle, Step->GapPowerPerCircle, Step->GapPowerCap) : 0.f;
}

float FGrimCircleMath::GetGapManaBonus(const FGrimCircleTable& Table, int32 CasterCircle, int32 SpellCircle)
{
	const FGrimCircleStep* Step = Table.FindClamped(CasterCircle);
	return Step ? FMath::Clamp(GrimCircleMathLocal::GapBonus(CasterCircle, SpellCircle, Step->GapManaPerCircle, Step->GapManaCap), 0.f, 1.f) : 0.f;
}

int32 FGrimCircleMath::GetSlotSets(const FGrimCircleTable& Table, int32 Circle, int32 Fallback)
{
	const FGrimCircleStep* Step = Table.FindClamped(Circle);
	return Step ? Step->SlotSets : Fallback;
}

int32 FGrimCircleMath::GetMaxSustainedSpells(const FGrimCircleTable& Table, int32 Circle, int32 Fallback)
{
	const FGrimCircleStep* Step = Table.FindClamped(Circle);
	return Step ? Step->MaxSustainedSpells : Fallback;
}

int32 FGrimCircleMath::GetEquipTier(const FGrimCircleTable& Table, int32 Circle, int32 Fallback)
{
	const FGrimCircleStep* Step = Table.FindClamped(Circle);
	return Step ? Step->EquipTier : Fallback;
}

int32 FGrimCircleMath::GetMaxEnchantTier(const FGrimCircleTable& Table, int32 Circle, int32 Fallback)
{
	const FGrimCircleStep* Step = Table.FindClamped(Circle);
	return Step ? Step->MaxEnchantTier : Fallback;
}

int32 FGrimCircleMath::GetCircleForEquipTier(const FGrimCircleTable& Table, int32 Tier)
{
	for (const FGrimCircleStep& Step : Table.Steps)
	{
		if (Step.EquipTier >= Tier)
		{
			return Step.Circle;
		}
	}
	return INDEX_NONE;
}

bool FGrimCircleMath::CanWearTier(const FGrimCircleTable& Table, int32 Circle, int32 Tier, int32& OutRequiredCircle)
{
	OutRequiredCircle = 1;
	if (Table.IsEmpty() || Tier <= GetEquipTier(Table, Circle, Tier))
	{
		return true;
	}
	const int32 Needed = GetCircleForEquipTier(Table, Tier);
	OutRequiredCircle = Needed != INDEX_NONE ? Needed : Table.GetMaxCircle() + 1;
	return false;
}

int32 FGrimCircleMath::GetUnlockCircle(const FGrimCircleTable& Table, FName UnlockId)
{
	for (const FGrimCircleStep& Step : Table.Steps)
	{
		if (Step.Unlocks.Contains(UnlockId))
		{
			return Step.Circle;
		}
	}
	return INDEX_NONE;
}

bool FGrimCircleMath::IsUnlocked(const FGrimCircleTable& Table, int32 Circle, FName UnlockId)
{
	const int32 Needed = GetUnlockCircle(Table, UnlockId);
	return Needed != INDEX_NONE && Circle >= Needed;
}

FGrimCircleCheck FGrimCircleMath::EvaluatePromotion(const FGrimCircleTable& Table, int32 CurrentCircle, TConstArrayView<FGrimSpellGradeEntry> Spells, TConstArrayView<FName> GradeOrder)
{
	FGrimCircleCheck Check;
	Check.CurrentCircle = CurrentCircle;
	const FGrimCircleStep* Next = Table.Find(CurrentCircle + 1);
	if (!Next)
	{
		return Check;
	}
	Check.bAtMax = false;
	Check.NextCircle = Next->Circle;
	Check.Task = Next->PromotionTask;
	Check.RequiredCurrentCircleSpells = Next->PromotionCurrentCircleSpells;

	int32 LowestGradeIndex = MAX_int32;
	for (const FGrimCircleGradeRequirement& Requirement : Next->PromotionGrades)
	{
		FGrimCircleGradeCheck& Grade = Check.Grades.AddDefaulted_GetRef();
		Grade.Grade = Requirement.Grade;
		Grade.Required = Requirement.Count;
		// 등급표에 없는 등급은 넘을 수 없는 문턱으로 본다(표 검사가 따로 알린다).
		const int32 Found = GradeOrder.IndexOfByKey(Requirement.Grade);
		Grade.GradeIndex = Found != INDEX_NONE ? Found : MAX_int32 / 2;
		LowestGradeIndex = FMath::Min(LowestGradeIndex, Grade.GradeIndex);
		for (const FGrimSpellGradeEntry& Spell : Spells)
		{
			Grade.Have += Spell.GradeIndex >= Grade.GradeIndex ? 1 : 0;
		}
	}

	if (LowestGradeIndex == MAX_int32)
	{
		LowestGradeIndex = 0; // 등급 조건이 없으면 아는 마법 전부
	}
	for (const FGrimSpellGradeEntry& Spell : Spells)
	{
		Check.CurrentCircleSpells += (Spell.GradeIndex >= LowestGradeIndex && Spell.SpellCircle >= CurrentCircle) ? 1 : 0;
	}

	Check.bMet = Check.FirstUnmetGrade() == nullptr && Check.IsCurrentCircleMet();
	return Check;
}

int32 FGrimCircleMath::PackGradeParam(int32 GradeIndex, int32 Required, int32 Have)
{
	return FMath::Clamp(GradeIndex, 0, 99) * 10000 + FMath::Clamp(Required, 0, 99) * 100 + FMath::Clamp(Have, 0, 99);
}

void FGrimCircleMath::UnpackGradeParam(int32 Param, int32& OutGradeIndex, int32& OutRequired, int32& OutHave)
{
	Param = FMath::Max(Param, 0);
	OutGradeIndex = Param / 10000;
	OutRequired = (Param / 100) % 100;
	OutHave = Param % 100;
}
