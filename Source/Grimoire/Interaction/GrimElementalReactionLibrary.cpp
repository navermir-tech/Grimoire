#include "Interaction/GrimElementalReactionLibrary.h"
#include "Interaction/GrimElementalStateComponent.h"
#include "Interaction/GrimInteractionSettings.h"
#include "Core/GrimElementalTypes.h"
#include "Engine/DataTable.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/StrongObjectPtr.h"

DEFINE_LOG_CATEGORY(LogGrimInteraction);

// ---------------------------------------------------------------------------
// 상태 배열

const FGrimElementalState* FGrimElementalStateArray::Find(const FGameplayTag& StateTag) const
{
	return Items.FindByPredicate([&StateTag](const FGrimElementalState& Item) { return Item.StateTag == StateTag; });
}

FGrimElementalState* FGrimElementalStateArray::Find(const FGameplayTag& StateTag)
{
	return Items.FindByPredicate([&StateTag](const FGrimElementalState& Item) { return Item.StateTag == StateTag; });
}

float FGrimElementalStateArray::GetIntensity(const FGameplayTag& StateTag) const
{
	const FGrimElementalState* Item = Find(StateTag);
	return Item ? Item->Intensity : 0.f;
}

void FGrimElementalStateArray::SetIntensity(const FGameplayTag& StateTag, float NewIntensity, TArray<FGrimElementalStateChange>& OutChanges)
{
	if (!StateTag.IsValid())
	{
		return;
	}
	if (NewIntensity <= KINDA_SMALL_NUMBER)
	{
		Remove(StateTag, OutChanges);
		return;
	}

	if (FGrimElementalState* Existing = Find(StateTag))
	{
		if (!FMath::IsNearlyEqual(Existing->Intensity, NewIntensity))
		{
			OutChanges.Add({ StateTag, Existing->Intensity, NewIntensity });
			Existing->Intensity = NewIntensity;
			MarkItemDirty(*Existing);
		}
		return;
	}

	FGrimElementalState& Added = Items.AddDefaulted_GetRef();
	Added.StateTag = StateTag;
	Added.Intensity = NewIntensity;
	Added.RemainingTime = -1.f;
	MarkItemDirty(Added);
	OutChanges.Add({ StateTag, 0.f, NewIntensity });
}

void FGrimElementalStateArray::Remove(const FGameplayTag& StateTag, TArray<FGrimElementalStateChange>& OutChanges)
{
	const int32 Index = Items.IndexOfByPredicate([&StateTag](const FGrimElementalState& Item) { return Item.StateTag == StateTag; });
	if (Index == INDEX_NONE)
	{
		return;
	}
	OutChanges.Add({ StateTag, Items[Index].Intensity, 0.f });
	Items.RemoveAt(Index);
	MarkArrayDirty();
}

// 클라이언트 복제 콜백 → 컴포넌트 델리게이트

void FGrimElementalState::PostReplicatedAdd(const FGrimElementalStateArray& InArray)
{
	if (InArray.Owner)
	{
		InArray.Owner->HandleReplicatedStateChange(StateTag, 0.f, Intensity);
	}
	LastNotifiedIntensity = Intensity;
}

void FGrimElementalState::PostReplicatedChange(const FGrimElementalStateArray& InArray)
{
	if (InArray.Owner && !FMath::IsNearlyEqual(LastNotifiedIntensity, Intensity))
	{
		InArray.Owner->HandleReplicatedStateChange(StateTag, LastNotifiedIntensity, Intensity);
	}
	LastNotifiedIntensity = Intensity;
}

void FGrimElementalState::PreReplicatedRemove(const FGrimElementalStateArray& InArray)
{
	if (InArray.Owner)
	{
		InArray.Owner->HandleReplicatedStateChange(StateTag, LastNotifiedIntensity, 0.f);
	}
}

// ---------------------------------------------------------------------------
// 규칙표 읽기

namespace
{
	TStrongObjectPtr<UDataTable> GDefaultReactionTable;
	bool GDefaultReactionTableTried = false;

	FAutoConsoleCommand GReloadReactionsCommand(
		TEXT("Grim.Interaction.ReloadReactions"),
		TEXT("속성 반응 규칙표(CSV)를 다시 읽는다"),
		FConsoleCommandDelegate::CreateStatic(&FGrimElementalReactionLibrary::ResetDefaultTable));
}

const UDataTable* FGrimElementalReactionLibrary::GetDefaultTable()
{
	if (GDefaultReactionTableTried)
	{
		return GDefaultReactionTable.Get();
	}
	GDefaultReactionTableTried = true;

	const UGrimInteractionSettings* Settings = GetDefault<UGrimInteractionSettings>();
	if (!Settings->ReactionTableAsset.IsNull())
	{
		UDataTable* Asset = Settings->ReactionTableAsset.LoadSynchronous();
		if (Asset && Asset->GetRowStruct() == FGrimElementalReactionRow::StaticStruct())
		{
			GDefaultReactionTable.Reset(Asset);
			return Asset;
		}
		UE_LOG(LogGrimInteraction, Error, TEXT("반응 규칙표 에셋 %s 을 쓸 수 없다(없거나 행 구조체가 다름). CSV 로 대신한다."),
			*Settings->ReactionTableAsset.ToString());
	}

	const FString FilePath = FPaths::Combine(FPaths::ProjectContentDir(), Settings->ReactionTableCSV);
	TArray<FString> Problems;
	UDataTable* Table = LoadTableFromCSVFile(FilePath, Problems);
	for (const FString& Problem : Problems)
	{
		UE_LOG(LogGrimInteraction, Warning, TEXT("반응 규칙표 %s: %s"), *FilePath, *Problem);
	}
	if (Table)
	{
		UE_LOG(LogGrimInteraction, Log, TEXT("반응 규칙 %d 행을 읽었다: %s"), Table->GetRowMap().Num(), *FilePath);
	}
	GDefaultReactionTable.Reset(Table);
	return Table;
}

void FGrimElementalReactionLibrary::ResetDefaultTable()
{
	GDefaultReactionTable.Reset();
	GDefaultReactionTableTried = false;
}

UDataTable* FGrimElementalReactionLibrary::LoadTableFromCSVFile(const FString& FilePath, TArray<FString>& OutProblems)
{
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *FilePath))
	{
		OutProblems.Add(FString::Printf(TEXT("파일을 열 수 없다: %s"), *FilePath));
		return nullptr;
	}

	UDataTable* Table = NewObject<UDataTable>(GetTransientPackage(), MakeUniqueObjectName(GetTransientPackage(), UDataTable::StaticClass(), TEXT("DT_ElementalReactions")));
	Table->RowStruct = FGrimElementalReactionRow::StaticStruct();
	OutProblems.Append(Table->CreateTableFromCSVString(Text));

	if (Table->GetRowMap().Num() == 0)
	{
		OutProblems.Add(TEXT("행이 없다"));
		return nullptr;
	}
	return Table;
}

// ---------------------------------------------------------------------------
// 조회 · 적용

namespace
{
	bool MatchesStateAndMaterial(const FGrimElementalReactionRow& Row, const FGrimElementalStateArray& States, const FGameplayTagContainer& Material)
	{
		if (Row.RequiredMaterial.IsValid() && !Material.HasTag(Row.RequiredMaterial))
		{
			return false;
		}
		for (const FGameplayTag& Blocked : Row.BlockedByStates)
		{
			if (States.Has(Blocked))
			{
				return false;
			}
		}
		return true;
	}

	template <typename PredicateType>
	const FGrimElementalReactionRow* FindFirstRow(const UDataTable* Table, PredicateType Predicate)
	{
		if (!Table || Table->GetRowStruct() != FGrimElementalReactionRow::StaticStruct())
		{
			return nullptr;
		}
		// RowMap 은 CSV 행 순서를 그대로 지킨다 → 위의 행이 먼저.
		for (const TPair<FName, uint8*>& Pair : Table->GetRowMap())
		{
			const FGrimElementalReactionRow* Row = reinterpret_cast<const FGrimElementalReactionRow*>(Pair.Value);
			if (Row && Predicate(*Row))
			{
				return Row;
			}
		}
		return nullptr;
	}
}

const FGrimElementalReactionRow* FGrimElementalReactionLibrary::FindStimulusRule(const UDataTable* Table, const FGrimElementalStateArray& States,
	const FGameplayTagContainer& Material, const FGrimElementalStimulus& Stimulus)
{
	if (!Stimulus.Element.IsValid())
	{
		return nullptr;
	}
	return FindFirstRow(Table, [&](const FGrimElementalReactionRow& Row)
	{
		return Row.Trigger == EGrimReactionTrigger::Stimulus
			&& Row.Element.IsValid() && Stimulus.Element.MatchesTag(Row.Element)
			&& Stimulus.Intensity >= Row.MinIntensity
			&& (!Row.RequiredState.IsValid() || States.Has(Row.RequiredState))
			&& MatchesStateAndMaterial(Row, States, Material);
	});
}

const FGrimElementalReactionRow* FGrimElementalReactionLibrary::FindExpireRule(const UDataTable* Table, const FGrimElementalStateArray& States,
	const FGameplayTagContainer& Material, const FGameplayTag& ExpiredState)
{
	return FindFirstRow(Table, [&](const FGrimElementalReactionRow& Row)
	{
		return Row.Trigger == EGrimReactionTrigger::Expire
			&& Row.RequiredState == ExpiredState
			&& MatchesStateAndMaterial(Row, States, Material);
	});
}

void FGrimElementalReactionLibrary::ApplyRule(const FGrimElementalReactionRow& Rule, FGrimElementalStateArray& States,
	float MaxPropagationRadius, TArray<FGrimElementalStateChange>& OutChanges)
{
	for (const FGameplayTag& Clear : Rule.ClearStates)
	{
		States.Remove(Clear, OutChanges);
	}

	if (!Rule.ResultState.IsValid())
	{
		return;
	}

	float NewIntensity = States.GetIntensity(Rule.ResultState) + Rule.IntensityDelta;
	if (Rule.MaxIntensity > 0.f)
	{
		NewIntensity = FMath::Min(NewIntensity, Rule.MaxIntensity);
	}
	States.SetIntensity(Rule.ResultState, NewIntensity, OutChanges);

	if (FGrimElementalState* Item = States.Find(Rule.ResultState))
	{
		if (Rule.Duration > 0.f)
		{
			Item->RemainingTime = Rule.Duration;
		}
		if (Rule.Propagate.IsActive())
		{
			Item->Propagation = Rule.Propagate;
			Item->Propagation.Radius = FMath::Min(Rule.Propagate.Radius, MaxPropagationRadius);
		}
	}
}

bool FGrimElementalReactionLibrary::ApplyStimulus(const UDataTable* Table, FGrimElementalStateArray& States, const FGameplayTagContainer& Material,
	const FGrimElementalStimulus& Stimulus, float MaxPropagationRadius, TArray<FGrimElementalStateChange>& OutChanges)
{
	if (const FGrimElementalReactionRow* Rule = FindStimulusRule(Table, States, Material, Stimulus))
	{
		ApplyRule(*Rule, States, MaxPropagationRadius, OutChanges);
		return true;
	}
	return false;
}

void FGrimElementalReactionLibrary::AdvanceTime(const UDataTable* Table, FGrimElementalStateArray& States, const FGameplayTagContainer& Material,
	float DeltaSeconds, float MaxPropagationRadius, TArray<FGrimElementalStateChange>& OutChanges)
{
	TArray<FGameplayTag, TInlineAllocator<4>> Expired;
	for (FGrimElementalState& Item : States.Items)
	{
		if (Item.IsTimed())
		{
			// 남은 시간은 복제용으로 더럽히지 않는다(세기가 바뀔 때만 보낸다).
			Item.RemainingTime = FMath::Max(0.f, Item.RemainingTime - DeltaSeconds);
			if (Item.RemainingTime <= 0.f)
			{
				Expired.Add(Item.StateTag);
			}
		}
	}

	for (const FGameplayTag& StateTag : Expired)
	{
		States.Remove(StateTag, OutChanges);
		if (const FGrimElementalReactionRow* Rule = FindExpireRule(Table, States, Material, StateTag))
		{
			ApplyRule(*Rule, States, MaxPropagationRadius, OutChanges);
		}
	}
}
