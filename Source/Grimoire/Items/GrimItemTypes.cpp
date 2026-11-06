#include "Items/GrimItemTypes.h"
#include "Engine/DataTable.h"
#include "UObject/Package.h"

DEFINE_LOG_CATEGORY(LogGrimItems);

namespace GrimItemEvents
{
	const FName FoodEaten(TEXT("Food.Eaten"));
	const FName CookingIngredientsAdded(TEXT("Cooking.IngredientsAdded"));
	const FName CookingCompleted(TEXT("Cooking.Completed"));
	const FName FoodCooked(TEXT("Cooking.Cooked"));

	FName Obtained(FName IdOrGroup)
	{
		return FName(*FString::Printf(TEXT("Item.Obtained.%s"), *IdOrGroup.ToString()));
	}
}

namespace GrimItemTypesLocal // 유니티 빌드에서 이름이 부딪히지 않게
{
	UDataTable* MakeItemTable(UScriptStruct* RowStruct, const TCHAR* BaseName, const FString& CSV, TArray<FString>& OutProblems)
	{
		UDataTable* Table = NewObject<UDataTable>(GetTransientPackage(), MakeUniqueObjectName(GetTransientPackage(), UDataTable::StaticClass(), BaseName));
		Table->RowStruct = RowStruct;
		for (const FString& Problem : Table->CreateTableFromCSVString(CSV))
		{
			OutProblems.Add(FString::Printf(TEXT("%s: %s"), BaseName, *Problem));
		}
		return Table;
	}

	void SplitList(const FString& Text, TArray<FString>& Out)
	{
		TArray<FString> Parts;
		Text.ParseIntoArray(Parts, TEXT("|"), true);
		for (FString& Part : Parts)
		{
			Part.TrimStartAndEndInline();
			if (!Part.IsEmpty())
			{
				Out.Add(Part);
			}
		}
	}
}

// ───── 인스턴스 ─────

bool FGrimItemInstance::CanStackWith(const FGrimItemInstance& Other) const
{
	return !IsEmpty() && !Other.IsEmpty() && ItemId == Other.ItemId
		&& Durability < 0.f && Other.Durability < 0.f && Enchantments.Num() == 0 && Other.Enchantments.Num() == 0
		&& RepairCount == 0 && Other.RepairCount == 0;
}

void FGrimItemInstance::Serialize(FArchive& Ar)
{
	// 인챈트 수의 높은 비트 = 끝에 수리 횟수가 붙음(M4). 옛 형식에는 이 비트가 없다.
	constexpr int32 RepairCountFlag = 0x40000000;
	FString IdText = ItemId.ToString();
	Ar << IdText;
	Ar << Count;
	Ar << Durability;
	int32 NumEnchantments = Enchantments.Num() | (RepairCount > 0 ? RepairCountFlag : 0);
	Ar << NumEnchantments;
	const bool bHasRepairCount = (NumEnchantments & RepairCountFlag) != 0;
	NumEnchantments &= ~RepairCountFlag;
	if (Ar.IsLoading())
	{
		ItemId = IdText.IsEmpty() || IdText == TEXT("None") ? NAME_None : FName(*IdText);
		Enchantments.Reset();
		RepairCount = 0;
		NumEnchantments = FMath::Clamp(NumEnchantments, 0, 64);
		for (int32 Index = 0; Index < NumEnchantments && !Ar.IsError(); ++Index)
		{
			FString Enchantment;
			Ar << Enchantment;
			Enchantments.Add(FName(*Enchantment));
		}
	}
	else
	{
		for (const FName& Enchantment : Enchantments)
		{
			FString EnchantmentText = Enchantment.ToString();
			Ar << EnchantmentText;
		}
	}
	if (bHasRepairCount)
	{
		Ar << RepairCount;
		RepairCount = FMath::Max(0, RepairCount);
	}
}

FText FGrimItemDefinition::GetDisplayName() const
{
	return FText::FromString(Row.DisplayName.IsEmpty() ? Id.ToString() : Row.DisplayName);
}

bool FGrimRecipeProgress::IsComplete() const
{
	if (!Recipe)
	{
		return false;
	}
	for (int32 Index = 0; Index < Recipe->Inputs.Num(); ++Index)
	{
		if (!Filled.IsValidIndex(Index) || Filled[Index] < Recipe->Inputs[Index].Count)
		{
			return false;
		}
	}
	return true;
}

// ───── 표 ─────

bool FGrimItemDatabase::MatchesInput(FName ItemId, const FGrimRecipeInput& Input) const
{
	if (!Input.bGroup)
	{
		return ItemId == Input.Id;
	}
	const FGrimItemDefinition* Def = Find(ItemId);
	return Def && Def->IsInGroup(Input.Id);
}

FGrimRecipeProgress FGrimItemDatabase::EvaluateRecipe(const FGrimRecipeDefinition& Recipe, const TArray<FGrimItemInstance>& Contents) const
{
	FGrimRecipeProgress Progress;
	Progress.Recipe = &Recipe;
	Progress.Filled.Init(0, Recipe.Inputs.Num());

	TArray<int32> Remaining;
	for (const FGrimItemInstance& Item : Contents)
	{
		Remaining.Add(Item.IsEmpty() ? 0 : Item.Count);
	}

	// ID 재료 먼저 — 묶음 재료가 꼭 필요한 아이템을 먼저 가져가지 않게.
	for (int32 Pass = 0; Pass < 2; ++Pass)
	{
		for (int32 InputIndex = 0; InputIndex < Recipe.Inputs.Num(); ++InputIndex)
		{
			const FGrimRecipeInput& Input = Recipe.Inputs[InputIndex];
			if (Input.bGroup != (Pass == 1))
			{
				continue;
			}
			for (int32 ContentIndex = 0; ContentIndex < Contents.Num() && Progress.Filled[InputIndex] < Input.Count; ++ContentIndex)
			{
				if (Remaining[ContentIndex] > 0 && MatchesInput(Contents[ContentIndex].ItemId, Input))
				{
					const int32 Take = FMath::Min(Remaining[ContentIndex], Input.Count - Progress.Filled[InputIndex]);
					Remaining[ContentIndex] -= Take;
					Progress.Filled[InputIndex] += Take;
				}
			}
		}
	}
	Progress.bHasExtra = Remaining.ContainsByPredicate([](int32 Count) { return Count > 0; });
	return Progress;
}

const FGrimRecipeDefinition* FGrimItemDatabase::FindCompleteRecipe(FName Station, const TArray<FGrimItemInstance>& Contents) const
{
	for (const FGrimRecipeDefinition& Recipe : Recipes)
	{
		if (Recipe.Row.Station != Station)
		{
			continue;
		}
		const FGrimRecipeProgress Progress = EvaluateRecipe(Recipe, Contents);
		if (Progress.IsComplete() && !Progress.bHasExtra)
		{
			return &Recipe;
		}
	}
	return nullptr;
}

bool FGrimItemDatabase::ParseInputs(const FString& Text, TArray<FGrimRecipeInput>& OutInputs, TArray<FString>& OutProblems)
{
	TArray<FString> Parts;
	GrimItemTypesLocal::SplitList(Text, Parts);
	bool bOk = Parts.Num() > 0;
	if (!bOk)
	{
		OutProblems.Add(TEXT("재료가 비었다"));
	}
	for (const FString& Part : Parts)
	{
		FString Left = Part, Right;
		int32 Count = 1;
		if (Part.Split(TEXT(":"), &Left, &Right))
		{
			Right.TrimStartAndEndInline();
			if (!Right.IsNumeric() || FCString::Atoi(*Right) < 1)
			{
				OutProblems.Add(FString::Printf(TEXT("재료 \"%s\" 의 개수를 못 읽는다"), *Part));
				bOk = false;
				continue;
			}
			Count = FCString::Atoi(*Right);
		}
		Left.TrimStartAndEndInline();
		FGrimRecipeInput Input;
		Input.bGroup = Left.StartsWith(TEXT("#"));
		if (Input.bGroup)
		{
			Left.RightChopInline(1);
		}
		if (Left.IsEmpty())
		{
			OutProblems.Add(FString::Printf(TEXT("재료 \"%s\" 의 이름이 비었다"), *Part));
			bOk = false;
			continue;
		}
		Input.Id = FName(*Left);
		Input.Count = Count;
		OutInputs.Add(Input);
	}
	return bOk;
}

bool FGrimItemDatabase::BuildFromCSV(const FString& ItemsCSV, const FString& RecipesCSV, FGrimItemDatabase& Out, TArray<FString>& OutProblems)
{
	using namespace GrimItemTypesLocal;
	const int32 ProblemsBefore = OutProblems.Num();
	Out = FGrimItemDatabase();

	const UDataTable* ItemTable = MakeItemTable(FGrimItemRow::StaticStruct(), TEXT("DT_Items"), ItemsCSV, OutProblems);
	for (const TPair<FName, uint8*>& Pair : ItemTable->GetRowMap())
	{
		FGrimItemDefinition& Def = Out.Items.Add(Pair.Key);
		Def.Id = Pair.Key;
		Def.Row = *reinterpret_cast<const FGrimItemRow*>(Pair.Value);
		TArray<FString> Groups;
		SplitList(Def.Row.Groups, Groups);
		for (const FString& Group : Groups)
		{
			Def.Groups.AddUnique(FName(*Group));
		}
	}
	if (Out.Items.Num() == 0)
	{
		OutProblems.Add(TEXT("아이템 표에 행이 없다"));
	}

	// 아이템 검사
	for (const TPair<FName, FGrimItemDefinition>& Pair : Out.Items)
	{
		const FGrimItemDefinition& Def = Pair.Value;
		const FString Id = Pair.Key.ToString();
		if (Def.Row.MaxStack < 1)
		{
			OutProblems.Add(FString::Printf(TEXT("아이템 %s: MaxStack 이 1 보다 작다"), *Id));
		}
		if (Def.Row.Kind == EGrimItemKind::Food && Def.Row.HungerRestore <= 0.f)
		{
			OutProblems.Add(FString::Printf(TEXT("아이템 %s: 음식인데 HungerRestore 가 0 이하"), *Id));
		}
		if (Def.Row.DiseaseChance > 1.f)
		{
			OutProblems.Add(FString::Printf(TEXT("아이템 %s: DiseaseChance 는 0~1(음수 = 기본)"), *Id));
		}
		if (Def.Row.Kind == EGrimItemKind::Skillbook && Def.Row.TeachesSpell.IsNone())
		{
			OutProblems.Add(FString::Printf(TEXT("아이템 %s: 스킬북인데 TeachesSpell 이 없다"), *Id));
		}
		if (!Def.Row.CookResult.IsNone())
		{
			if (!Out.Items.Contains(Def.Row.CookResult))
			{
				OutProblems.Add(FString::Printf(TEXT("아이템 %s: 익은 결과 %s 가 표에 없다"), *Id, *Def.Row.CookResult.ToString()));
			}
			if (Def.Row.CookSeconds <= 0.f)
			{
				OutProblems.Add(FString::Printf(TEXT("아이템 %s: CookResult 가 있는데 CookSeconds 가 0 이하"), *Id));
			}
			if (Def.Row.CookResult == Pair.Key)
			{
				OutProblems.Add(FString::Printf(TEXT("아이템 %s: 익은 결과가 자기 자신"), *Id));
			}
		}
		if (Def.Row.Size <= 0.f)
		{
			OutProblems.Add(FString::Printf(TEXT("아이템 %s: Size 가 0 이하"), *Id));
		}
	}

	// 레시피
	if (!RecipesCSV.IsEmpty())
	{
		const UDataTable* RecipeTable = MakeItemTable(FGrimRecipeRow::StaticStruct(), TEXT("DT_Recipes"), RecipesCSV, OutProblems);
		for (const TPair<FName, uint8*>& Pair : RecipeTable->GetRowMap())
		{
			FGrimRecipeDefinition& Recipe = Out.Recipes.AddDefaulted_GetRef();
			Recipe.Id = Pair.Key;
			Recipe.Row = *reinterpret_cast<const FGrimRecipeRow*>(Pair.Value);
			TArray<FString> InputProblems;
			ParseInputs(Recipe.Row.Inputs, Recipe.Inputs, InputProblems);
			for (const FString& Problem : InputProblems)
			{
				OutProblems.Add(FString::Printf(TEXT("레시피 %s: %s"), *Pair.Key.ToString(), *Problem));
			}
		}
	}
	for (const FGrimRecipeDefinition& Recipe : Out.Recipes)
	{
		const FString Id = Recipe.Id.ToString();
		if (!Out.Items.Contains(Recipe.Row.Output))
		{
			OutProblems.Add(FString::Printf(TEXT("레시피 %s: 결과 %s 가 아이템 표에 없다"), *Id, *Recipe.Row.Output.ToString()));
		}
		if (Recipe.Row.OutputCount < 1)
		{
			OutProblems.Add(FString::Printf(TEXT("레시피 %s: OutputCount 가 1 보다 작다"), *Id));
		}
		if (Recipe.Row.CookSeconds <= 0.f)
		{
			OutProblems.Add(FString::Printf(TEXT("레시피 %s: CookSeconds 가 0 이하"), *Id));
		}
		if (Recipe.Row.Station.IsNone())
		{
			OutProblems.Add(FString::Printf(TEXT("레시피 %s: Station 이 없다"), *Id));
		}
		for (const FGrimRecipeInput& Input : Recipe.Inputs)
		{
			if (Input.bGroup)
			{
				bool bAnyMember = false;
				for (const TPair<FName, FGrimItemDefinition>& Pair : Out.Items)
				{
					bAnyMember |= Pair.Value.IsInGroup(Input.Id);
				}
				if (!bAnyMember)
				{
					OutProblems.Add(FString::Printf(TEXT("레시피 %s: 묶음 #%s 에 드는 아이템이 없다"), *Id, *Input.Id.ToString()));
				}
			}
			else if (!Out.Items.Contains(Input.Id))
			{
				OutProblems.Add(FString::Printf(TEXT("레시피 %s: 재료 %s 가 아이템 표에 없다"), *Id, *Input.Id.ToString()));
			}
		}
	}
	return OutProblems.Num() == ProblemsBefore;
}

// ───── 가방 모델 ─────

int32 FGrimInventoryModel::Add(TArray<FGrimItemInstance>& Slots, const FGrimItemDatabase& Database, const FGrimItemInstance& Item)
{
	const FGrimItemDefinition* Def = Database.Find(Item.ItemId);
	if (!Def || Item.Count <= 0)
	{
		return 0;
	}
	const int32 MaxStack = Def->GetMaxStack();
	int32 Remaining = Item.Count;

	// 1) 쌓을 수 있는 칸
	for (FGrimItemInstance& Slot : Slots)
	{
		if (Remaining <= 0)
		{
			break;
		}
		if (Slot.CanStackWith(Item) && Slot.Count < MaxStack)
		{
			const int32 Take = FMath::Min(Remaining, MaxStack - Slot.Count);
			Slot.Count += Take;
			Remaining -= Take;
		}
	}
	// 2) 빈 칸
	for (FGrimItemInstance& Slot : Slots)
	{
		if (Remaining <= 0)
		{
			break;
		}
		if (Slot.IsEmpty())
		{
			Slot = Item;
			Slot.Count = FMath::Min(Remaining, MaxStack);
			Remaining -= Slot.Count;
		}
	}
	return Item.Count - Remaining;
}

int32 FGrimInventoryModel::GetAddableCount(const TArray<FGrimItemInstance>& Slots, const FGrimItemDatabase& Database, FName ItemId)
{
	const FGrimItemDefinition* Def = Database.Find(ItemId);
	if (!Def)
	{
		return 0;
	}
	const int32 MaxStack = Def->GetMaxStack();
	const FGrimItemInstance Probe(ItemId, 1);
	int32 Total = 0;
	for (const FGrimItemInstance& Slot : Slots)
	{
		if (Slot.IsEmpty())
		{
			Total += MaxStack;
		}
		else if (Slot.CanStackWith(Probe))
		{
			Total += FMath::Max(0, MaxStack - Slot.Count);
		}
	}
	return Total;
}

int32 FGrimInventoryModel::Remove(TArray<FGrimItemInstance>& Slots, FName ItemId, int32 Count)
{
	int32 Removed = 0;
	for (int32 Index = Slots.Num() - 1; Index >= 0 && Removed < Count; --Index)
	{
		FGrimItemInstance& Slot = Slots[Index];
		if (!Slot.IsEmpty() && Slot.ItemId == ItemId)
		{
			const int32 Take = FMath::Min(Slot.Count, Count - Removed);
			Slot.Count -= Take;
			Removed += Take;
			if (Slot.Count <= 0)
			{
				Slot.Clear();
			}
		}
	}
	return Removed;
}

FGrimItemInstance FGrimInventoryModel::RemoveAt(TArray<FGrimItemInstance>& Slots, int32 SlotIndex, int32 Count)
{
	if (!Slots.IsValidIndex(SlotIndex) || Slots[SlotIndex].IsEmpty() || Count <= 0)
	{
		return FGrimItemInstance();
	}
	FGrimItemInstance& Slot = Slots[SlotIndex];
	FGrimItemInstance Taken = Slot;
	Taken.Count = FMath::Min(Count, Slot.Count);
	Slot.Count -= Taken.Count;
	if (Slot.Count <= 0)
	{
		Slot.Clear();
	}
	return Taken;
}

bool FGrimInventoryModel::Move(TArray<FGrimItemInstance>& Slots, const FGrimItemDatabase& Database, int32 From, int32 To)
{
	if (From == To || !Slots.IsValidIndex(From) || !Slots.IsValidIndex(To) || Slots[From].IsEmpty())
	{
		return false;
	}
	FGrimItemInstance& Source = Slots[From];
	FGrimItemInstance& Target = Slots[To];
	if (Target.CanStackWith(Source))
	{
		const FGrimItemDefinition* Def = Database.Find(Source.ItemId);
		const int32 MaxStack = Def ? Def->GetMaxStack() : 1;
		const int32 Take = FMath::Min(Source.Count, MaxStack - Target.Count);
		if (Take <= 0)
		{
			return false;
		}
		Target.Count += Take;
		Source.Count -= Take;
		if (Source.Count <= 0)
		{
			Source.Clear();
		}
		return true;
	}
	Swap(Source, Target);
	return true;
}

int32 FGrimInventoryModel::CountOf(const TArray<FGrimItemInstance>& Slots, FName ItemId)
{
	int32 Total = 0;
	for (const FGrimItemInstance& Slot : Slots)
	{
		if (!Slot.IsEmpty() && Slot.ItemId == ItemId)
		{
			Total += Slot.Count;
		}
	}
	return Total;
}

// ───── 요리 · 먹기 ─────

float FGrimCookingModel::GetCookRate(float HeatIntensity, float ExtraRatePerIntensity)
{
	if (HeatIntensity <= 0.f)
	{
		return 0.f;
	}
	return 1.f + FMath::Max(0.f, HeatIntensity - 1.f) * FMath::Max(0.f, ExtraRatePerIntensity);
}

bool FGrimCookingModel::Advance(float& InOutProgress, float HeatIntensity, float DeltaSeconds, float CookSeconds, float ExtraRatePerIntensity)
{
	InOutProgress += GetCookRate(HeatIntensity, ExtraRatePerIntensity) * FMath::Max(0.f, DeltaSeconds);
	return CookSeconds > 0.f && InOutProgress >= CookSeconds - KINDA_SMALL_NUMBER;
}

float FGrimFoodModel::GetBaseDiseaseChance(const FGrimItemDefinition& Food, float DefaultRawChance)
{
	if (!Food.Row.bRaw)
	{
		return 0.f;
	}
	return Food.Row.DiseaseChance < 0.f ? DefaultRawChance : Food.Row.DiseaseChance;
}
