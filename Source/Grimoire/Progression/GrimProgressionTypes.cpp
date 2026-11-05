#include "Progression/GrimProgressionTypes.h"

DEFINE_LOG_CATEGORY(LogGrimProgression);

// ───────────────────────── 레벨 ─────────────────────────

bool FGrimLevelMath::ReadCurve(const UDataTable* Table, FGrimLevelCurve& OutCurve, TArray<FString>& OutProblems)
{
	OutCurve.Steps.Reset();
	if (!Table || Table->GetRowStruct() != FGrimLevelRow::StaticStruct())
	{
		OutProblems.Add(TEXT("레벨 표가 없거나 행 구조체가 FGrimLevelRow 가 아니다"));
		return false;
	}

	for (const TPair<FName, uint8*>& Pair : Table->GetRowMap())
	{
		const FGrimLevelRow* Row = reinterpret_cast<const FGrimLevelRow*>(Pair.Value);
		const FString Name = Pair.Key.ToString();
		if (!Name.IsNumeric())
		{
			OutProblems.Add(FString::Printf(TEXT("행 이름 %s 가 레벨 숫자가 아니다"), *Name));
			continue;
		}
		FGrimLevelStep& Step = OutCurve.Steps.AddDefaulted_GetRef();
		Step.Level = FCString::Atoi(*Name);
		Step.TotalXP = Row->TotalXP;
		Step.MaxHealth = Row->MaxHealth;
		Step.MaxMana = Row->MaxMana;
		Step.SkillPoints = Row->SkillPoints;
	}
	OutCurve.Steps.Sort([](const FGrimLevelStep& A, const FGrimLevelStep& B) { return A.Level < B.Level; });

	bool bOk = OutCurve.Steps.Num() > 0;
	if (!bOk)
	{
		OutProblems.Add(TEXT("레벨 행이 없다"));
	}
	for (int32 Index = 0; Index < OutCurve.Steps.Num(); ++Index)
	{
		const FGrimLevelStep& Step = OutCurve.Steps[Index];
		if (Step.Level != Index + 1)
		{
			OutProblems.Add(FString::Printf(TEXT("레벨이 1 부터 빈틈없이 이어지지 않는다(%d 번째 줄 = 레벨 %d)"), Index + 1, Step.Level));
			bOk = false;
		}
		if (Index == 0 && Step.TotalXP != 0.f)
		{
			OutProblems.Add(TEXT("레벨 1 의 TotalXP 는 0 이어야 한다"));
			bOk = false;
		}
		if (Index > 0 && Step.TotalXP <= OutCurve.Steps[Index - 1].TotalXP)
		{
			OutProblems.Add(FString::Printf(TEXT("레벨 %d 의 TotalXP 가 앞 레벨보다 크지 않다"), Step.Level));
			bOk = false;
		}
		if (Step.SkillPoints < 0)
		{
			OutProblems.Add(FString::Printf(TEXT("레벨 %d 의 SkillPoints 가 음수"), Step.Level));
			bOk = false;
		}
	}
	return bOk;
}

int32 FGrimLevelMath::GetMaxLevel(const FGrimLevelCurve& Curve, int32 Cap)
{
	const int32 TableMax = FMath::Max(Curve.Steps.Num(), 1);
	return Cap > 0 ? FMath::Clamp(Cap, 1, TableMax) : TableMax;
}

int32 FGrimLevelMath::GetLevelForXP(const FGrimLevelCurve& Curve, float TotalXP, int32 Cap)
{
	const int32 MaxLevel = GetMaxLevel(Curve, Cap);
	int32 Level = 1;
	for (int32 Index = 1; Index < MaxLevel && Index < Curve.Steps.Num(); ++Index)
	{
		if (TotalXP + KINDA_SMALL_NUMBER >= Curve.Steps[Index].TotalXP)
		{
			Level = Index + 1;
		}
		else
		{
			break;
		}
	}
	return Level;
}

float FGrimLevelMath::GetXPForLevel(const FGrimLevelCurve& Curve, int32 Level)
{
	if (Curve.Steps.IsEmpty())
	{
		return 0.f;
	}
	return Curve.Steps[FMath::Clamp(Level - 1, 0, Curve.Steps.Num() - 1)].TotalXP;
}

float FGrimLevelMath::ClampXP(const FGrimLevelCurve& Curve, float TotalXP, int32 Cap)
{
	return FMath::Clamp(TotalXP, 0.f, GetXPForLevel(Curve, GetMaxLevel(Curve, Cap)));
}

FGrimLevelProgress FGrimLevelMath::GetProgress(const FGrimLevelCurve& Curve, float TotalXP, int32 Cap)
{
	FGrimLevelProgress Progress;
	Progress.MaxLevel = GetMaxLevel(Curve, Cap);
	Progress.TotalXP = TotalXP;
	Progress.Level = GetLevelForXP(Curve, TotalXP, Cap);
	Progress.bAtCap = Progress.Level >= Progress.MaxLevel;
	Progress.LevelStartXP = GetXPForLevel(Curve, Progress.Level);
	Progress.NextLevelXP = Progress.bAtCap ? Progress.LevelStartXP : GetXPForLevel(Curve, Progress.Level + 1);
	const float Span = Progress.NextLevelXP - Progress.LevelStartXP;
	Progress.Fraction = Progress.bAtCap ? 1.f : (Span > 0.f ? FMath::Clamp((TotalXP - Progress.LevelStartXP) / Span, 0.f, 1.f) : 0.f);
	return Progress;
}

int32 FGrimLevelMath::GetSkillPointsAtLevel(const FGrimLevelCurve& Curve, int32 Level)
{
	int32 Sum = 0;
	for (int32 Index = 1; Index < Level && Index < Curve.Steps.Num(); ++Index)
	{
		Sum += Curve.Steps[Index].SkillPoints;
	}
	return Sum;
}

float FGrimLevelMath::GetMaxHealthBonusAtLevel(const FGrimLevelCurve& Curve, int32 Level)
{
	float Sum = 0.f;
	for (int32 Index = 1; Index < Level && Index < Curve.Steps.Num(); ++Index)
	{
		Sum += Curve.Steps[Index].MaxHealth;
	}
	return Sum;
}

float FGrimLevelMath::GetMaxManaBonusAtLevel(const FGrimLevelCurve& Curve, int32 Level)
{
	float Sum = 0.f;
	for (int32 Index = 1; Index < Level && Index < Curve.Steps.Num(); ++Index)
	{
		Sum += Curve.Steps[Index].MaxMana;
	}
	return Sum;
}

float FGrimLevelMath::ComputeXPGain(float Amount, int32 RepeatCount, float Decay, float MinMultiplier, float GlobalRate)
{
	if (Amount <= 0.f)
	{
		return 0.f;
	}
	const float Multiplier = FMath::Max(FMath::Clamp(MinMultiplier, 0.f, 1.f), FMath::Pow(FMath::Clamp(Decay, 0.f, 1.f), static_cast<float>(FMath::Max(RepeatCount, 0))));
	return Amount * Multiplier * FMath::Max(GlobalRate, 0.f);
}

int32 FGrimXPRepeatTracker::Register(const FGameplayTag& Source, double Now, double WindowSeconds)
{
	FEntry& Entry = Entries.FindOrAdd(Source);
	if (Entry.Count > 0 && Now - Entry.LastTime > WindowSeconds)
	{
		Entry.Count = 0;
	}
	const int32 Repeat = Entry.Count;
	Entry.Count += 1;
	Entry.LastTime = Now;
	return Repeat;
}

// ───────────────────────── 성장 트리 ─────────────────────────

TArray<FName> FGrimSkillTreeDefinition::GetTreeIds() const
{
	TArray<FName> Result;
	for (const FGrimSkillNode& Node : Nodes)
	{
		Result.AddUnique(Node.Tree);
	}
	return Result;
}

bool FGrimSkillTreeMath::ParseEffects(const FString& Text, TArray<FGrimSkillEffect>& OutEffects, FString* OutProblem)
{
	OutEffects.Reset();
	bool bOk = true;
	TArray<FString> Parts;
	Text.ParseIntoArray(Parts, TEXT(";"), true);
	for (FString Part : Parts)
	{
		Part.TrimStartAndEndInline();
		if (Part.IsEmpty())
		{
			continue;
		}
		FString Key;
		FString Value;
		if (!Part.Split(TEXT("="), &Key, &Value))
		{
			if (OutProblem)
			{
				*OutProblem += FString::Printf(TEXT("「%s」에 = 가 없다. "), *Part);
			}
			bOk = false;
			continue;
		}
		Key.TrimStartAndEndInline();
		Value.TrimStartAndEndInline();
		FGrimSkillEffect Effect;
		Effect.Key = FName(*Key);
		if (Value.StartsWith(TEXT("*")))
		{
			Effect.bMultiply = true;
			Value.RightChopInline(1);
		}
		else if (Value.StartsWith(TEXT("+")))
		{
			Value.RightChopInline(1);
		}
		Value.TrimStartAndEndInline();
		if (Key.IsEmpty() || !Value.IsNumeric())
		{
			if (OutProblem)
			{
				*OutProblem += FString::Printf(TEXT("「%s」를 읽을 수 없다. "), *Part);
			}
			bOk = false;
			continue;
		}
		Effect.Value = FCString::Atof(*Value);
		OutEffects.Add(Effect);
	}
	return bOk;
}

TArray<FName> FGrimSkillTreeMath::ParsePrerequisites(const FString& Text)
{
	TArray<FName> Result;
	TArray<FString> Parts;
	Text.ParseIntoArray(Parts, TEXT("|"), true);
	for (FString Part : Parts)
	{
		Part.TrimStartAndEndInline();
		if (!Part.IsEmpty())
		{
			Result.AddUnique(FName(*Part));
		}
	}
	return Result;
}

namespace GrimSkillTreeMathLocal // 유니티 빌드에서 이름이 부딪히지 않게
{
	/** 선행 관계 순환 검사(0 = 안 봄, 1 = 보는 중, 2 = 끝) */
	bool VisitForCycle(const FGrimSkillTreeDefinition& Definition, int32 Index, TArray<uint8>& Marks)
	{
		if (Marks[Index] == 2)
		{
			return false;
		}
		if (Marks[Index] == 1)
		{
			return true;
		}
		Marks[Index] = 1;
		for (const FName Prerequisite : Definition.Nodes[Index].Prerequisites)
		{
			if (const int32* Next = Definition.IndexById.Find(Prerequisite))
			{
				if (VisitForCycle(Definition, *Next, Marks))
				{
					return true;
				}
			}
		}
		Marks[Index] = 2;
		return false;
	}
}

bool FGrimSkillTreeMath::BuildDefinition(const UDataTable* Table, FGrimSkillTreeDefinition& OutDefinition, TArray<FString>& OutProblems)
{
	OutDefinition.Nodes.Reset();
	OutDefinition.IndexById.Reset();
	if (!Table || Table->GetRowStruct() != FGrimSkillTreeNodeRow::StaticStruct())
	{
		OutProblems.Add(TEXT("성장 트리 표가 없거나 행 구조체가 FGrimSkillTreeNodeRow 가 아니다"));
		return false;
	}

	bool bOk = true;
	// 표의 줄 순서를 지킨다(GetRowMap 은 넣은 순서).
	for (const TPair<FName, uint8*>& Pair : Table->GetRowMap())
	{
		const FGrimSkillTreeNodeRow* Row = reinterpret_cast<const FGrimSkillTreeNodeRow*>(Pair.Value);
		FGrimSkillNode Node;
		Node.Id = Pair.Key;
		Node.Tree = Row->Tree;
		Node.Branch = Row->Branch;
		Node.Kind = Row->Kind;
		Node.DisplayName = Row->DisplayName.IsEmpty() ? FText::FromName(Pair.Key) : Row->DisplayName;
		Node.Description = Row->Description;
		Node.Grid = FVector2D(Row->GridX, Row->GridY);
		Node.Prerequisites = ParsePrerequisites(Row->Prerequisites);
		Node.Cost = FMath::Max(Row->Cost, 0);
		Node.RequiredCircle = FMath::Max(Row->RequiredCircle, 1);
		FString Problem;
		if (!ParseEffects(Row->Effects, Node.Effects, &Problem))
		{
			OutProblems.Add(FString::Printf(TEXT("노드 %s 효과: %s"), *Pair.Key.ToString(), *Problem));
			bOk = false;
		}
		if (Node.Tree.IsNone())
		{
			OutProblems.Add(FString::Printf(TEXT("노드 %s 에 트리가 없다"), *Pair.Key.ToString()));
			bOk = false;
		}
		OutDefinition.IndexById.Add(Node.Id, OutDefinition.Nodes.Num());
		OutDefinition.Nodes.Add(MoveTemp(Node));
	}

	for (const FGrimSkillNode& Node : OutDefinition.Nodes)
	{
		for (const FName Prerequisite : Node.Prerequisites)
		{
			const FGrimSkillNode* Other = OutDefinition.Find(Prerequisite);
			if (!Other)
			{
				OutProblems.Add(FString::Printf(TEXT("노드 %s 의 선행 %s 가 표에 없다"), *Node.Id.ToString(), *Prerequisite.ToString()));
				bOk = false;
			}
			else if (Other->Tree != Node.Tree)
			{
				OutProblems.Add(FString::Printf(TEXT("노드 %s 의 선행 %s 가 다른 트리다"), *Node.Id.ToString(), *Prerequisite.ToString()));
				bOk = false;
			}
		}
	}

	TArray<uint8> Marks;
	Marks.SetNumZeroed(OutDefinition.Nodes.Num());
	for (int32 Index = 0; Index < OutDefinition.Nodes.Num(); ++Index)
	{
		if (GrimSkillTreeMathLocal::VisitForCycle(OutDefinition, Index, Marks))
		{
			OutProblems.Add(FString::Printf(TEXT("노드 %s 에서 선행 관계가 돈다"), *OutDefinition.Nodes[Index].Id.ToString()));
			bOk = false;
			break;
		}
	}

	if (OutDefinition.Nodes.IsEmpty())
	{
		OutProblems.Add(TEXT("노드가 없다"));
		bOk = false;
	}
	return bOk;
}

int32 FGrimSkillTreeMath::GetSpentPoints(const FGrimSkillTreeDefinition& Definition, TConstArrayView<FName> Unlocked, FName Tree)
{
	int32 Sum = 0;
	for (const FName Id : Unlocked)
	{
		if (const FGrimSkillNode* Node = Definition.Find(Id); Node && (Tree.IsNone() || Node->Tree == Tree))
		{
			Sum += Node->Cost;
		}
	}
	return Sum;
}

EGrimSkillUnlockResult FGrimSkillTreeMath::CanUnlock(const FGrimSkillTreeDefinition& Definition, TConstArrayView<FName> Unlocked, FName NodeId, int32 AvailablePoints, int32 Circle)
{
	const FGrimSkillNode* Node = Definition.Find(NodeId);
	if (!Node)
	{
		return EGrimSkillUnlockResult::UnknownNode;
	}
	if (Unlocked.Contains(NodeId))
	{
		return EGrimSkillUnlockResult::AlreadyUnlocked;
	}
	for (const FName Prerequisite : Node->Prerequisites)
	{
		if (!Unlocked.Contains(Prerequisite))
		{
			return EGrimSkillUnlockResult::MissingPrerequisite;
		}
	}
	if (Circle < Node->RequiredCircle)
	{
		return EGrimSkillUnlockResult::CircleTooLow;
	}
	if (AvailablePoints < Node->Cost)
	{
		return EGrimSkillUnlockResult::NotEnoughPoints;
	}
	return EGrimSkillUnlockResult::Unlocked;
}

void FGrimSkillTreeMath::SumEffects(const FGrimSkillTreeDefinition& Definition, TConstArrayView<FName> Unlocked, TMap<FName, FGrimSkillEffectTotal>& OutTotals)
{
	OutTotals.Reset();
	for (const FName Id : Unlocked)
	{
		const FGrimSkillNode* Node = Definition.Find(Id);
		if (!Node)
		{
			continue;
		}
		for (const FGrimSkillEffect& Effect : Node->Effects)
		{
			FGrimSkillEffectTotal& Total = OutTotals.FindOrAdd(Effect.Key);
			if (Effect.bMultiply)
			{
				Total.Mul *= Effect.Value;
			}
			else
			{
				Total.Add += Effect.Value;
			}
		}
	}
}

int32 FGrimSkillTreeMath::GetNodeCount(const FGrimSkillTreeDefinition& Definition, FName Tree)
{
	int32 Count = 0;
	for (const FGrimSkillNode& Node : Definition.Nodes)
	{
		Count += (Tree.IsNone() || Node.Tree == Tree) ? 1 : 0;
	}
	return Count;
}

int32 FGrimSkillTreeMath::GetTotalCost(const FGrimSkillTreeDefinition& Definition, FName Tree)
{
	int32 Sum = 0;
	for (const FGrimSkillNode& Node : Definition.Nodes)
	{
		Sum += (Tree.IsNone() || Node.Tree == Tree) ? Node.Cost : 0;
	}
	return Sum;
}

int32 FGrimSkillTreeMath::GetMaxNodesForPoints(const FGrimSkillTreeDefinition& Definition, int32 Points, int32* OutSpent)
{
	TArray<FName> Unlocked;
	int32 Remaining = Points;
	for (;;)
	{
		const FGrimSkillNode* Cheapest = nullptr;
		for (const FGrimSkillNode& Node : Definition.Nodes)
		{
			if (CanUnlock(Definition, Unlocked, Node.Id, Remaining, MAX_int32) == EGrimSkillUnlockResult::Unlocked
				&& (!Cheapest || Node.Cost < Cheapest->Cost))
			{
				Cheapest = &Node;
			}
		}
		if (!Cheapest)
		{
			break;
		}
		Unlocked.Add(Cheapest->Id);
		Remaining -= Cheapest->Cost;
	}
	if (OutSpent)
	{
		*OutSpent = Points - Remaining;
	}
	return Unlocked.Num();
}

TArray<FName> FGrimSkillTreeMath::RemoveTree(const FGrimSkillTreeDefinition& Definition, TConstArrayView<FName> Unlocked, FName Tree)
{
	TArray<FName> Result;
	for (const FName Id : Unlocked)
	{
		const FGrimSkillNode* Node = Definition.Find(Id);
		if (!Node || Node->Tree != Tree)
		{
			Result.Add(Id);
		}
	}
	return Result;
}
