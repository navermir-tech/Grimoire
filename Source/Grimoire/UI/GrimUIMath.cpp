#include "UI/GrimUIMath.h"

float FGrimUIMath::BarFraction(float Value, float Min, float Max)
{
	const float Range = Max - Min;
	if (Range <= KINDA_SMALL_NUMBER)
	{
		return 0.f;
	}
	return FMath::Clamp((Value - Min) / Range, 0.f, 1.f);
}

bool FGrimUIMath::SplitAttributeName(const FString& Name, FString& OutSet, FString& OutProperty)
{
	const FString Trimmed = Name.TrimStartAndEnd();
	FString Left, Right;
	if (Trimmed.Split(TEXT("."), &Left, &Right, ESearchCase::CaseSensitive, ESearchDir::FromEnd) && !Left.IsEmpty() && !Right.IsEmpty())
	{
		OutSet = Left.TrimStartAndEnd();
		OutProperty = Right.TrimStartAndEnd();
		return true;
	}
	OutSet.Reset();
	OutProperty = Trimmed;
	return false;
}

bool FGrimUIMath::AttributeSetNameMatches(const FString& ClassName, const FString& Wanted)
{
	if (Wanted.IsEmpty())
	{
		return true;
	}
	if (ClassName.Equals(Wanted, ESearchCase::IgnoreCase))
	{
		return true;
	}
	// 접두 U 를 붙이거나 뗀 이름
	if (Wanted.StartsWith(TEXT("U")) && ClassName.Equals(Wanted.Mid(1), ESearchCase::IgnoreCase))
	{
		return true;
	}
	return ClassName.StartsWith(TEXT("U")) && ClassName.Mid(1).Equals(Wanted, ESearchCase::IgnoreCase);
}

float FGrimUIMath::CooldownFraction(float Remaining, float Duration)
{
	if (Duration <= KINDA_SMALL_NUMBER || Remaining <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(Remaining / Duration, 0.f, 1.f);
}

bool FGrimUIMath::MatchesElementFilter(const FGameplayTag& Element, const FGameplayTag& Filter)
{
	return !Filter.IsValid() || (Element.IsValid() && Element.MatchesTag(Filter));
}

TArray<int32> FGrimUIMath::BuildGrimoireOrder(TConstArrayView<FGrimGrimoireEntryKey> Entries, const FGameplayTag& Filter)
{
	TArray<int32> Order;
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		if (MatchesElementFilter(Entries[Index].Element, Filter))
		{
			Order.Add(Index);
		}
	}
	Order.StableSort([&Entries](int32 A, int32 B)
	{
		const FGrimGrimoireEntryKey& KA = Entries[A];
		const FGrimGrimoireEntryKey& KB = Entries[B];
		if (KA.bTracked != KB.bTracked)
		{
			return !KA.bTracked;
		}
		if (KA.Circle != KB.Circle)
		{
			return KA.Circle < KB.Circle;
		}
		return KA.KnownIndex < KB.KnownIndex;
	});
	return Order;
}

FGrimSlotLayout FGrimUIMath::ComputeSlotLayout(TConstArrayView<int32> SlotKnownIndices, TConstArrayView<bool> bTracked, int32 ActiveSet, int32 SlotsPerSet, int32 Sets, bool bSeparateBasic)
{
	FGrimSlotLayout Layout;
	SlotsPerSet = FMath::Max(SlotsPerSet, 1);
	Layout.Sets = FMath::Max(Sets, 1);
	Layout.SetIndex = FMath::Clamp(ActiveSet, 0, Layout.Sets - 1);

	if (bSeparateBasic)
	{
		Layout.BasicIndex = bTracked.IndexOfByKey(false);
	}

	Layout.Slots.Init(INDEX_NONE, SlotsPerSet);
	for (int32 SlotIndex = 0; SlotIndex < SlotsPerSet; ++SlotIndex)
	{
		const int32 Position = Layout.SetIndex * SlotsPerSet + SlotIndex;
		const int32 Known = SlotKnownIndices.IsValidIndex(Position) ? SlotKnownIndices[Position] : INDEX_NONE;
		// 배운 범위 밖 · 따로 둔 기본 공격은 빈 칸으로
		if (bTracked.IsValidIndex(Known) && !(bSeparateBasic && Known == Layout.BasicIndex))
		{
			Layout.Slots[SlotIndex] = Known;
		}
	}
	return Layout;
}

FText FGrimUIMath::GradeLabel(bool bTracked, FName Grade)
{
	if (!bTracked || Grade.IsNone())
	{
		return FText::FromString(TEXT("—"));
	}
	return FText::FromName(Grade);
}

FText FGrimUIMath::FormatCooldown(float Seconds)
{
	if (Seconds <= 0.f)
	{
		return FText::GetEmpty();
	}
	if (Seconds < 1.f)
	{
		return FText::FromString(FString::Printf(TEXT("%.1f"), Seconds));
	}
	return FText::AsNumber(FMath::CeilToInt(Seconds));
}
