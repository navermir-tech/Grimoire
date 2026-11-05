#include "Base/GrimBaseTypes.h"
#include "Engine/DataTable.h"
#include "UObject/Package.h"

DEFINE_LOG_CATEGORY(LogGrimBase);

namespace GrimBaseEvents
{
	const FName CorePlaced(TEXT("Base.CorePlaced"));
	const FName SleptInBase(TEXT("Base.SleptInBase"));

	FName PiecePlaced(EGrimBuildCategory Category)
	{
		const UEnum* Enum = StaticEnum<EGrimBuildCategory>();
		return FName(*FString::Printf(TEXT("Base.Placed.%s"), *Enum->GetNameStringByValue(static_cast<int64>(Category))));
	}

	FName Harvested(FName CropId)
	{
		return FName(*FString::Printf(TEXT("Crop.Harvested.%s"), *CropId.ToString()));
	}

	FName Crafted(FName ItemId)
	{
		return FName(*FString::Printf(TEXT("Item.Crafted.%s"), *ItemId.ToString()));
	}
}

namespace GrimBaseTypesLocal // 유니티 빌드에서 이름이 부딪히지 않게
{
	UDataTable* MakeBaseTable(UScriptStruct* RowStruct, const TCHAR* BaseName, const FString& CSV, TArray<FString>& OutProblems)
	{
		UDataTable* Table = NewObject<UDataTable>(GetTransientPackage(), MakeUniqueObjectName(GetTransientPackage(), UDataTable::StaticClass(), BaseName));
		Table->RowStruct = RowStruct;
		if (CSV.IsEmpty())
		{
			OutProblems.Add(FString::Printf(TEXT("%s: 표가 비었다"), BaseName));
			return Table;
		}
		for (const FString& Problem : Table->CreateTableFromCSVString(CSV))
		{
			OutProblems.Add(FString::Printf(TEXT("%s: %s"), BaseName, *Problem));
		}
		return Table;
	}

	constexpr uint8 LayerFoundation = 'F';
	constexpr uint8 LayerRoof = 'R';
	constexpr uint8 LayerStair = 'S';
	constexpr uint8 LayerEdge = 'E';
	constexpr uint8 LayerCorner = 'C';

	constexpr float WallThickness = 20.f;
	constexpr float PillarHalf = 15.f;
	constexpr float DoorWidth = 120.f;
	constexpr float DoorHeight = 220.f;
	constexpr int32 StairSteps = 10;
}

// ───────────────────────── 정의 · 표 ─────────────────────────

FString FGrimBuildPieceDefinition::DescribeCost(const FGrimItemDatabase& Items) const
{
	TArray<FString> Parts;
	for (const FGrimRecipeInput& Input : Cost)
	{
		const FGrimItemDefinition* Item = Items.Find(Input.Id);
		Parts.Add(FString::Printf(TEXT("%s %d"), Item ? *Item->GetDisplayName().ToString() : *Input.Id.ToString(), Input.Count));
	}
	return Parts.Num() > 0 ? FString::Join(Parts, TEXT(" · ")) : TEXT("무료");
}

const FGrimCropDefinition* FGrimBaseDatabase::FindCropBySeed(FName SeedItem) const
{
	for (const TPair<FName, FGrimCropDefinition>& Pair : Crops)
	{
		if (Pair.Value.Row.SeedItem == SeedItem)
		{
			return &Pair.Value;
		}
	}
	return nullptr;
}

bool FGrimBaseDatabase::BuildFromCSV(const FString& PiecesCSV, const FString& CropsCSV, const FString& EquipmentCSV, const FGrimItemDatabase* Items,
	FGrimBaseDatabase& Out, TArray<FString>& OutProblems)
{
	using namespace GrimBaseTypesLocal;
	const int32 ProblemsBefore = OutProblems.Num();
	Out = FGrimBaseDatabase();

	auto CheckItem = [&](const FString& Where, FName ItemId, bool bAllowNone)
	{
		if (ItemId.IsNone())
		{
			if (!bAllowNone)
			{
				OutProblems.Add(FString::Printf(TEXT("%s: 아이템이 비었다"), *Where));
			}
			return;
		}
		if (Items && !Items->Find(ItemId))
		{
			OutProblems.Add(FString::Printf(TEXT("%s: 아이템 표에 없는 %s"), *Where, *ItemId.ToString()));
		}
	};

	// 건축 조각
	const UDataTable* PieceTable = MakeBaseTable(FGrimBuildPieceRow::StaticStruct(), TEXT("DT_BuildPieces"), PiecesCSV, OutProblems);
	TArray<TPair<int32, FName>> Order;
	bool bHasCore = false;
	for (const TPair<FName, uint8*>& Pair : PieceTable->GetRowMap())
	{
		FGrimBuildPieceDefinition& Def = Out.Pieces.Add(Pair.Key);
		Def.Id = Pair.Key;
		Def.Row = *reinterpret_cast<const FGrimBuildPieceRow*>(Pair.Value);
		const FString Where = FString::Printf(TEXT("조각 %s"), *Pair.Key.ToString());

		TArray<FString> CostProblems;
		FGrimItemDatabase::ParseInputs(Def.Row.Cost, Def.Cost, CostProblems);
		for (const FString& Problem : CostProblems)
		{
			OutProblems.Add(FString::Printf(TEXT("%s 비용: %s"), *Where, *Problem));
		}
		for (const FGrimRecipeInput& Input : Def.Cost)
		{
			if (Input.bGroup)
			{
				OutProblems.Add(FString::Printf(TEXT("%s: 비용에 묶음(#%s)은 쓰지 않는다"), *Where, *Input.Id.ToString()));
			}
			CheckItem(Where + TEXT(" 비용"), Input.Id, false);
		}
		CheckItem(Where + TEXT(" 수리 재료"), Def.Row.RepairItem, true);
		if (Def.Row.MaxHealth <= 0.f)
		{
			OutProblems.Add(FString::Printf(TEXT("%s: MaxHealth 가 0 이하"), *Where));
		}
		if (Def.IsFreePlaced())
		{
			if (Def.Row.SpawnClass.IsEmpty())
			{
				OutProblems.Add(FString::Printf(TEXT("%s: 시설 · 거점 핵인데 SpawnClass 가 비었다"), *Where));
			}
			if (Def.Row.Extent.GetMin() <= 0.f)
			{
				OutProblems.Add(FString::Printf(TEXT("%s: Extent 가 0 이하"), *Where));
			}
		}
		bHasCore |= Def.Row.Category == EGrimBuildCategory::Core;
		Order.Add(TPair<int32, FName>(Def.Row.SortOrder, Pair.Key));
	}
	Order.Sort([](const TPair<int32, FName>& A, const TPair<int32, FName>& B)
	{
		return A.Key != B.Key ? A.Key < B.Key : A.Value.LexicalLess(B.Value);
	});
	for (const TPair<int32, FName>& Entry : Order)
	{
		Out.PieceOrder.Add(Entry.Value);
	}
	if (!bHasCore)
	{
		OutProblems.Add(TEXT("조각 표에 거점 핵(Category=Core)이 없다"));
	}

	// 작물
	const UDataTable* CropTable = MakeBaseTable(FGrimCropRow::StaticStruct(), TEXT("DT_Crops"), CropsCSV, OutProblems);
	for (const TPair<FName, uint8*>& Pair : CropTable->GetRowMap())
	{
		FGrimCropDefinition& Def = Out.Crops.Add(Pair.Key);
		Def.Id = Pair.Key;
		Def.Row = *reinterpret_cast<const FGrimCropRow*>(Pair.Value);
		const FString Where = FString::Printf(TEXT("작물 %s"), *Pair.Key.ToString());
		CheckItem(Where + TEXT(" 씨앗"), Def.Row.SeedItem, false);
		CheckItem(Where + TEXT(" 수확"), Def.Row.HarvestItem, false);
		if (Def.Row.GrowSeconds <= 0.f || Def.Row.Stages < 1 || Def.Row.HarvestCount < 1)
		{
			OutProblems.Add(FString::Printf(TEXT("%s: GrowSeconds · Stages · HarvestCount 는 양수"), *Where));
		}
	}

	// 장비
	const UDataTable* EquipmentTable = MakeBaseTable(FGrimEquipmentRow::StaticStruct(), TEXT("DT_Equipment"), EquipmentCSV, OutProblems);
	for (const TPair<FName, uint8*>& Pair : EquipmentTable->GetRowMap())
	{
		FGrimEquipmentDefinition& Def = Out.Equipment.Add(Pair.Key);
		Def.ItemId = Pair.Key;
		Def.Row = *reinterpret_cast<const FGrimEquipmentRow*>(Pair.Value);
		const FString Where = FString::Printf(TEXT("장비 %s"), *Pair.Key.ToString());
		CheckItem(Where, Pair.Key, false);
		CheckItem(Where + TEXT(" 수리 재료"), Def.Row.RepairItem, true);
		if (Items)
		{
			const FGrimItemDefinition* Item = Items->Find(Pair.Key);
			if (Item && (Item->Row.Kind != EGrimItemKind::Equipment || Item->Row.MaxStack != 1))
			{
				OutProblems.Add(FString::Printf(TEXT("%s: 아이템 표에서 Kind=Equipment · MaxStack=1 이어야 한다"), *Where));
			}
		}
		if (Def.Row.MaxDurability <= 0.f || Def.Row.Slot == EGrimEquipSlot::Count)
		{
			OutProblems.Add(FString::Printf(TEXT("%s: MaxDurability 는 양수 · 칸은 Wand~Necklace"), *Where));
		}
	}
	if (Items)
	{
		for (const TPair<FName, FGrimItemDefinition>& Pair : Items->Items)
		{
			if (Pair.Value.Row.Kind == EGrimItemKind::Equipment && !Out.Equipment.Contains(Pair.Key))
			{
				OutProblems.Add(FString::Printf(TEXT("아이템 %s: Kind=Equipment 인데 장비 표에 없다"), *Pair.Key.ToString()));
			}
			if (Pair.Value.Row.Kind == EGrimItemKind::Seed && !Out.FindCropBySeed(Pair.Key))
			{
				OutProblems.Add(FString::Printf(TEXT("아이템 %s: Kind=Seed 인데 작물 표에 없다"), *Pair.Key.ToString()));
			}
		}
	}
	return OutProblems.Num() == ProblemsBefore;
}

// ───────────────────────── 격자 ─────────────────────────

FString FGrimBuildSlot::ToString() const
{
	return FString::Printf(TEXT("%s(%d,%d,%d,%d)"), *StaticEnum<EGrimBuildCategory>()->GetNameStringByValue(static_cast<int64>(Category)), X, Y, Level, Dir);
}

uint64 FGrimBuildSlot::MakeKey(uint8 Layer, int32 InX, int32 InY, int32 InLevel, int32 InDir)
{
	const uint64 PackedX = static_cast<uint64>(static_cast<uint16>(InX + 32768));
	const uint64 PackedY = static_cast<uint64>(static_cast<uint16>(InY + 32768));
	return (static_cast<uint64>(Layer) << 48) | (PackedX << 32) | (PackedY << 16) | (static_cast<uint64>(InLevel & 0xFF) << 8) | static_cast<uint64>(InDir & 0xFF);
}

uint64 FGrimBuildSlot::GetOccupancyKey() const
{
	using namespace GrimBaseTypesLocal;
	switch (Category)
	{
	case EGrimBuildCategory::Foundation: return MakeKey(LayerFoundation, X, Y, Level, 0);
	case EGrimBuildCategory::Roof: return MakeKey(LayerRoof, X, Y, Level, 0);
	case EGrimBuildCategory::Stair: return MakeKey(LayerStair, X, Y, Level, 0);
	case EGrimBuildCategory::Wall:
	case EGrimBuildCategory::Door: return MakeKey(LayerEdge, X, Y, Level, Dir & 1);
	case EGrimBuildCategory::Pillar: return MakeKey(LayerCorner, X, Y, Level, 0);
	default: return 0;
	}
}

FVector FGrimBuildGrid::LocalToWorld(const FVector& Local) const
{
	return Origin + FRotator(0.f, Yaw, 0.f).RotateVector(Local);
}

FVector FGrimBuildGrid::WorldToLocal(const FVector& World) const
{
	return FRotator(0.f, Yaw, 0.f).UnrotateVector(World - Origin);
}

FGrimBuildSlot FGrimBuildGrid::Snap(EGrimBuildCategory Category, const FVector& AimWorld, int32 RotationQuarter) const
{
	const FVector Local = WorldToLocal(AimWorld);
	const float FX = Local.X / CellSize;
	const float FY = Local.Y / CellSize;
	const int32 I = FMath::RoundToInt(FX);
	const int32 J = FMath::RoundToInt(FY);
	const float DX = FX - I;
	const float DY = FY - J;
	const float LevelHeight = GetLevelHeight();

	FGrimBuildSlot Slot;
	Slot.Category = Category;
	Slot.X = I;
	Slot.Y = J;
	Slot.Level = FMath::Clamp(FMath::FloorToInt((Local.Z - FloorTopOffset + 0.25f * LevelHeight) / LevelHeight), 0, MaxLevel);

	switch (Category)
	{
	case EGrimBuildCategory::Foundation:
		Slot.Level = 0;
		break;
	case EGrimBuildCategory::Roof:
		// 벽 꼭대기를 겨누면 그 층의 지붕. 땅을 겨누면 0층.
		Slot.Level = FMath::Clamp(FMath::RoundToInt((Local.Z - FloorTopOffset - WallHeight) / LevelHeight), 0, MaxLevel);
		break;
	case EGrimBuildCategory::Stair:
		Slot.Dir = ((RotationQuarter % 4) + 4) % 4;
		break;
	case EGrimBuildCategory::Wall:
	case EGrimBuildCategory::Door:
		if (FMath::Abs(DX) >= FMath::Abs(DY))
		{
			Slot.Dir = 0;
			Slot.X = DX >= 0.f ? I : I - 1;
		}
		else
		{
			Slot.Dir = 1;
			Slot.Y = DY >= 0.f ? J : J - 1;
		}
		break;
	case EGrimBuildCategory::Pillar:
		Slot.X = DX >= 0.f ? I : I - 1;
		Slot.Y = DY >= 0.f ? J : J - 1;
		break;
	default:
		break;
	}
	return Slot;
}

FVector FGrimBuildGrid::GetSlotAnchorLocal(const FGrimBuildSlot& Slot) const
{
	const float FloorTop = GetFloorTop(Slot.Level);
	switch (Slot.Category)
	{
	case EGrimBuildCategory::Foundation:
		return FVector(Slot.X * CellSize, Slot.Y * CellSize, GetFloorTop(0));
	case EGrimBuildCategory::Roof:
		return FVector(Slot.X * CellSize, Slot.Y * CellSize, FloorTop + WallHeight);
	case EGrimBuildCategory::Wall:
	case EGrimBuildCategory::Door:
		return (Slot.Dir & 1) == 0
			? FVector((Slot.X + 0.5f) * CellSize, Slot.Y * CellSize, FloorTop)
			: FVector(Slot.X * CellSize, (Slot.Y + 0.5f) * CellSize, FloorTop);
	case EGrimBuildCategory::Pillar:
		return FVector((Slot.X + 0.5f) * CellSize, (Slot.Y + 0.5f) * CellSize, FloorTop);
	default:
		return FVector(Slot.X * CellSize, Slot.Y * CellSize, FloorTop);
	}
}

FTransform FGrimBuildGrid::GetSlotTransform(const FGrimBuildSlot& Slot) const
{
	float LocalYaw = 0.f;
	if (Slot.Category == EGrimBuildCategory::Wall || Slot.Category == EGrimBuildCategory::Door)
	{
		LocalYaw = (Slot.Dir & 1) == 0 ? 0.f : 90.f;
	}
	else if (Slot.Category == EGrimBuildCategory::Stair)
	{
		LocalYaw = 90.f * Slot.Dir;
	}
	return FTransform(FRotator(0.f, Yaw + LocalYaw, 0.f), LocalToWorld(GetSlotAnchorLocal(Slot)));
}

void FGrimBuildGrid::GetShape(EGrimBuildCategory Category, TArray<FGrimShapeBox>& OutBoxes) const
{
	using namespace GrimBaseTypesLocal;
	const float Half = CellSize * 0.5f;
	auto Add = [&OutBoxes](const FVector& Center, const FVector& Extent, bool bLeaf = false)
	{
		FGrimShapeBox& Box = OutBoxes.AddDefaulted_GetRef();
		Box.Center = Center;
		Box.Extent = Extent;
		Box.bDoorLeaf = bLeaf;
	};

	switch (Category)
	{
	case EGrimBuildCategory::Foundation:
		Add(FVector(0.f, 0.f, -FoundationDepth * 0.5f), FVector(Half, Half, FoundationDepth * 0.5f));
		break;
	case EGrimBuildCategory::Wall:
		// 벽의 법선이 로컬 X — 두께는 X, 길이는 Y.
		Add(FVector(0.f, 0.f, WallHeight * 0.5f), FVector(WallThickness * 0.5f, Half, WallHeight * 0.5f));
		break;
	case EGrimBuildCategory::Door:
	{
		const float PostHalf = (CellSize - DoorWidth) * 0.25f;
		const float PostY = DoorWidth * 0.5f + PostHalf;
		Add(FVector(0.f, -PostY, WallHeight * 0.5f), FVector(WallThickness * 0.5f, PostHalf, WallHeight * 0.5f));
		Add(FVector(0.f, PostY, WallHeight * 0.5f), FVector(WallThickness * 0.5f, PostHalf, WallHeight * 0.5f));
		Add(FVector(0.f, 0.f, (DoorHeight + WallHeight) * 0.5f), FVector(WallThickness * 0.5f, DoorWidth * 0.5f, (WallHeight - DoorHeight) * 0.5f));
		Add(FVector(0.f, 0.f, DoorHeight * 0.5f), FVector(4.f, DoorWidth * 0.5f - 2.f, DoorHeight * 0.5f), true);
		break;
	}
	case EGrimBuildCategory::Roof:
		// 이웃 지붕과 틈이 없게 살짝 겹친다(지붕 판정 트레이스가 이음매로 새지 않게).
		Add(FVector(0.f, 0.f, SlabThickness * 0.5f), FVector(Half + 2.f, Half + 2.f, SlabThickness * 0.5f));
		break;
	case EGrimBuildCategory::Pillar:
		Add(FVector(0.f, 0.f, WallHeight * 0.5f), FVector(PillarHalf, PillarHalf, WallHeight * 0.5f));
		break;
	case EGrimBuildCategory::Stair:
	{
		// 계단 열 칸 — 한 칸 높이(층 높이 / 10 = 32 cm)는 캐릭터 기본 오를 높이(45 cm) 안이다.
		const float StepDepth = CellSize / StairSteps;
		const float StepRise = GetLevelHeight() / StairSteps;
		for (int32 Step = 0; Step < StairSteps; ++Step)
		{
			const float Top = StepRise * (Step + 1);
			Add(FVector(-Half + StepDepth * (Step + 0.5f), 0.f, Top * 0.5f), FVector(StepDepth * 0.5f, Half - 20.f, Top * 0.5f));
		}
		break;
	}
	default:
		break;
	}
}

// ───────────────────────── 지지 ─────────────────────────

bool FGrimBuildSupport::HasSupport(const FGrimBuildSlot& Slot, const TMap<uint64, bool>& Stable, bool* bOutDirect)
{
	using namespace GrimBaseTypesLocal;
	auto Has = [&Stable](uint8 Layer, int32 X, int32 Y, int32 Level, int32 Dir)
	{
		return Level >= 0 && Stable.Contains(FGrimBuildSlot::MakeKey(Layer, X, Y, Level, Dir));
	};
	auto Floor = [&](int32 X, int32 Y)
	{
		return Slot.Level == 0 ? Has(LayerFoundation, X, Y, 0, 0) : Has(LayerRoof, X, Y, Slot.Level - 1, 0);
	};
	if (bOutDirect)
	{
		*bOutDirect = true;
	}

	const int32 X = Slot.X;
	const int32 Y = Slot.Y;
	const int32 L = Slot.Level;
	switch (Slot.Category)
	{
	case EGrimBuildCategory::Foundation:
		return L == 0;
	case EGrimBuildCategory::Wall:
	case EGrimBuildCategory::Door:
		return (Slot.Dir & 1) == 0 ? (Floor(X, Y) || Floor(X + 1, Y)) : (Floor(X, Y) || Floor(X, Y + 1));
	case EGrimBuildCategory::Pillar:
		return Floor(X, Y) || Floor(X + 1, Y) || Floor(X, Y + 1) || Floor(X + 1, Y + 1);
	case EGrimBuildCategory::Stair:
		return Floor(X, Y);
	case EGrimBuildCategory::Roof:
	{
		const bool bDirect = Has(LayerEdge, X, Y, L, 0) || Has(LayerEdge, X - 1, Y, L, 0) || Has(LayerEdge, X, Y, L, 1) || Has(LayerEdge, X, Y - 1, L, 1)
			|| Has(LayerCorner, X, Y, L, 0) || Has(LayerCorner, X - 1, Y, L, 0) || Has(LayerCorner, X, Y - 1, L, 0) || Has(LayerCorner, X - 1, Y - 1, L, 0);
		if (bDirect)
		{
			return true;
		}
		// 한 칸 내밀기: 벽 · 기둥으로 직접 받쳐진 이웃 지붕.
		const int32 Offsets[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
		for (const auto& Offset : Offsets)
		{
			const bool* bNeighborDirect = Stable.Find(FGrimBuildSlot::MakeKey(LayerRoof, X + Offset[0], Y + Offset[1], L, 0));
			if (bNeighborDirect && *bNeighborDirect)
			{
				if (bOutDirect)
				{
					*bOutDirect = false;
				}
				return true;
			}
		}
		return false;
	}
	default:
		return false;
	}
}

TMap<uint64, bool> FGrimBuildSupport::ComputeStable(const TArray<FGrimBuildSlot>& Slots, TArray<int32>* OutUnsupported)
{
	TMap<uint64, bool> Stable;
	TArray<int32> State; // 0 = 아직, 1 = 간접, 2 = 직접
	State.Init(0, Slots.Num());

	bool bChanged = true;
	while (bChanged)
	{
		bChanged = false;
		for (int32 Index = 0; Index < Slots.Num(); ++Index)
		{
			if (State[Index] == 2)
			{
				continue;
			}
			bool bDirect = false;
			if (HasSupport(Slots[Index], Stable, &bDirect))
			{
				const int32 NewState = bDirect ? 2 : 1;
				if (NewState > State[Index])
				{
					State[Index] = NewState;
					Stable.Add(Slots[Index].GetOccupancyKey(), bDirect);
					bChanged = true;
				}
			}
		}
	}
	if (OutUnsupported)
	{
		for (int32 Index = 0; Index < Slots.Num(); ++Index)
		{
			if (State[Index] == 0)
			{
				OutUnsupported->Add(Index);
			}
		}
	}
	return Stable;
}

TArray<int32> FGrimBuildSupport::FindUnsupported(const TArray<FGrimBuildSlot>& Slots)
{
	TArray<int32> Unsupported;
	ComputeStable(Slots, &Unsupported);
	return Unsupported;
}

// ───────────────────────── 내구도 ─────────────────────────

float FGrimDurabilityMath::GetEquipmentMax(float BaseMax, int32 RepairCount, float LossPerRepair, float MinFraction)
{
	const float Fraction = FMath::Max(FMath::Clamp(MinFraction, 0.f, 1.f), 1.f - FMath::Max(0.f, LossPerRepair) * FMath::Max(0, RepairCount));
	return FMath::Max(0.f, BaseMax) * Fraction;
}

int32 FGrimDurabilityMath::GetMaterialsNeeded(float Current, float Max, float FractionPerMaterial)
{
	const float Missing = Max - Current;
	const float PerMaterial = Max * FractionPerMaterial;
	if (Missing <= KINDA_SMALL_NUMBER || PerMaterial <= KINDA_SMALL_NUMBER)
	{
		return 0;
	}
	return FMath::CeilToInt(Missing / PerMaterial - KINDA_SMALL_NUMBER);
}

float FGrimDurabilityMath::Repair(float Current, float Max, int32 Materials, float FractionPerMaterial)
{
	return FMath::Clamp(Current + Max * FractionPerMaterial * FMath::Max(0, Materials), 0.f, Max);
}

float FGrimDurabilityMath::GetDecayPerSecond(float Max, bool bInsideBase, float InsideFractionPerHour, float OutsideFractionPerHour)
{
	return FMath::Max(0.f, Max * (bInsideBase ? InsideFractionPerHour : OutsideFractionPerHour) / 3600.f);
}

float FGrimDurabilityMath::GetArmorWear(float Damage, float WearPerUse, float DamageUnit)
{
	return DamageUnit > 0.f ? FMath::Max(0.f, Damage) / DamageUnit * FMath::Max(0.f, WearPerUse) : 0.f;
}

int32 FGrimDurabilityMath::GetRefundCount(int32 Cost, float Fraction)
{
	return FMath::Max(0, FMath::FloorToInt(Cost * FMath::Clamp(Fraction, 0.f, 1.f) + KINDA_SMALL_NUMBER));
}

// ───────────────────────── 작물 ─────────────────────────

float FGrimCropMath::GetGrowthRate(const FGrimCropRow& Crop, const FGrimCropConditions& Conditions)
{
	if (Crop.GrowSeconds <= 0.f)
	{
		return 0.f;
	}
	if (Conditions.ColdGrade > Crop.MaxColdGrade || Conditions.HeatGrade > Crop.MaxHeatGrade)
	{
		return 0.f;
	}
	const float Water = Conditions.bWet ? 1.f : FMath::Max(0.f, Crop.DryGrowthScale);
	const float Light = (Conditions.bDaylight || Conditions.bIlluminated) ? 1.f : FMath::Max(0.f, Crop.NightGrowthScale);
	const float Ash = Conditions.bAsh ? 1.f + FMath::Max(0.f, Crop.AshBonus) : 1.f;
	return Water * Light * Ash / Crop.GrowSeconds;
}

int32 FGrimCropMath::GetStage(float Growth, int32 Stages)
{
	Stages = FMath::Max(1, Stages);
	if (IsMature(Growth))
	{
		return Stages;
	}
	return FMath::Clamp(FMath::FloorToInt(FMath::Max(0.f, Growth) * Stages), 0, Stages - 1);
}

// ───────────────────────── 옮기기 · 제작 ─────────────────────────

int32 FGrimSlotTransferMath::Transfer(TArray<FGrimItemInstance>& From, int32 FromIndex, int32 Count, TArray<FGrimItemInstance>& To, const FGrimItemDatabase& Database)
{
	if (!From.IsValidIndex(FromIndex) || From[FromIndex].IsEmpty() || Count == 0)
	{
		return 0;
	}
	FGrimItemInstance Moving = From[FromIndex];
	Moving.Count = Count < 0 ? Moving.Count : FMath::Min(Count, Moving.Count);
	const int32 Added = FGrimInventoryModel::Add(To, Database, Moving);
	if (Added > 0)
	{
		From[FromIndex].Count -= Added;
		if (From[FromIndex].Count <= 0)
		{
			From[FromIndex].Clear();
		}
	}
	return Added;
}

bool FGrimSlotTransferMath::ConsumeInputs(TArray<FGrimItemInstance>& Slots, const FGrimItemDatabase& Database, const TArray<FGrimRecipeInput>& Inputs)
{
	TArray<FGrimItemInstance> Work = Slots;
	for (int32 Pass = 0; Pass < 2; ++Pass)
	{
		for (const FGrimRecipeInput& Input : Inputs)
		{
			if (Input.bGroup != (Pass == 1))
			{
				continue;
			}
			if (!Input.bGroup)
			{
				if (FGrimInventoryModel::Remove(Work, Input.Id, Input.Count) < Input.Count)
				{
					return false;
				}
				continue;
			}
			int32 Remaining = Input.Count;
			for (int32 Index = Work.Num() - 1; Index >= 0 && Remaining > 0; --Index)
			{
				if (!Work[Index].IsEmpty() && Database.MatchesInput(Work[Index].ItemId, Input))
				{
					const int32 Take = FMath::Min(Remaining, Work[Index].Count);
					FGrimInventoryModel::RemoveAt(Work, Index, Take);
					Remaining -= Take;
				}
			}
			if (Remaining > 0)
			{
				return false;
			}
		}
	}
	Slots = MoveTemp(Work);
	return true;
}

bool FGrimSlotTransferMath::HasInputs(const TArray<FGrimItemInstance>& Slots, const FGrimItemDatabase& Database, const TArray<FGrimRecipeInput>& Inputs)
{
	TArray<FGrimItemInstance> Copy = Slots;
	return ConsumeInputs(Copy, Database, Inputs);
}
