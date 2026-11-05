#include "Appearance/GrimAppearanceTypes.h"
#include "Appearance/GrimAppearanceSettings.h"
#include "Engine/DataTable.h"
#include "Math/RandomStream.h"
#include "Misc/Crc.h"
#include "Serialization/MemoryWriter.h"
#include "UObject/Package.h"

DEFINE_LOG_CATEGORY(LogGrimAppearance);

#define LOCTEXT_NAMESPACE "GrimAppearance"

namespace GrimAppearancePalettes
{
	const FName Skin(TEXT("Skin"));
	const FName Eye(TEXT("Eye"));
	const FName Hair(TEXT("Hair"));
	const FName Cloth(TEXT("Cloth"));
	const FName Element(TEXT("Element"));
	const FName Fixed(TEXT("Fixed"));
}

namespace GrimAppearanceTypesLocal // 유니티 빌드에서 이름이 부딪히지 않게
{
	const TCHAR* const ElementNames[FGrimAppearance::ElementCount + 1] = {
		TEXT("None"), TEXT("Water"), TEXT("Fire"), TEXT("Wind"), TEXT("Earth"), TEXT("Light"), TEXT("Dark") };

	const TCHAR* const ElementKoreanNames[FGrimAppearance::ElementCount + 1] = {
		TEXT("없음"), TEXT("물"), TEXT("불"), TEXT("바람"), TEXT("땅"), TEXT("빛"), TEXT("어둠") };

	const FName FixedLeather(TEXT("Fixed_Leather"));
	const FName FixedMetal(TEXT("Fixed_Metal"));
	const FName FixedUnder(TEXT("Fixed_Under"));

	UDataTable* MakeTable(UScriptStruct* RowStruct, const TCHAR* BaseName, const FString& CSV, TArray<FString>& OutProblems)
	{
		UDataTable* Table = NewObject<UDataTable>(GetTransientPackage(), MakeUniqueObjectName(GetTransientPackage(), UDataTable::StaticClass(), BaseName));
		Table->RowStruct = RowStruct;
		if (!CSV.IsEmpty())
		{
			for (const FString& Problem : Table->CreateTableFromCSVString(CSV))
			{
				OutProblems.Add(FString::Printf(TEXT("%s: %s"), BaseName, *Problem));
			}
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

	/** 표 순서대로 (행 이름, 행) — DataTable 의 행 맵은 넣은 순서를 지킨다 */
	template <typename RowType>
	TArray<TPair<FName, const RowType*>> OrderedRows(const UDataTable* Table)
	{
		TArray<TPair<FName, const RowType*>> Out;
		if (Table && Table->RowStruct && Table->RowStruct->IsChildOf(RowType::StaticStruct()))
		{
			for (const TPair<FName, uint8*>& Pair : Table->GetRowMap())
			{
				Out.Emplace(Pair.Key, reinterpret_cast<const RowType*>(Pair.Value));
			}
		}
		return Out;
	}

	bool NameLess(const FName& A, const FName& B)
	{
		return A.ToString().ToLower() < B.ToString().ToLower();
	}

	/** 가중치 뽑기 — 후보가 없어도 난수를 한 번 쓴다(규칙이 달라도 뒤 칸의 난수 순서가 밀리지 않게) */
	int32 PickWeighted(FRandomStream& Stream, const TArray<float>& Weights)
	{
		const float Roll = Stream.FRand();
		float Total = 0.f;
		for (float Weight : Weights)
		{
			Total += FMath::Max(0.f, Weight);
		}
		if (Total <= 0.f)
		{
			return INDEX_NONE;
		}
		float Acc = 0.f;
		for (int32 Index = 0; Index < Weights.Num(); ++Index)
		{
			Acc += FMath::Max(0.f, Weights[Index]) / Total;
			if (Roll < Acc)
			{
				return Index;
			}
		}
		for (int32 Index = Weights.Num() - 1; Index >= 0; --Index)
		{
			if (Weights[Index] > 0.f)
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}

	uint8 PickByte(FRandomStream& Stream, const TArray<uint8>& Choices, uint8 Fallback)
	{
		const int32 Roll = Stream.RandHelper(FMath::Max(1, Choices.Num()));
		return Choices.IsValidIndex(Roll) ? Choices[Roll] : Fallback;
	}

	void ParseColorList(const FGrimAppearanceCatalog& Catalog, FName Palette, const FString& Text, bool bCreatorOnly, FName RuleId, const TCHAR* Column,
		TArray<uint8>& Out, TArray<FString>& OutProblems)
	{
		TArray<FString> Tokens;
		SplitList(Text, Tokens);
		if (Tokens.Num() == 0 || Tokens.Contains(TEXT("*")))
		{
			Out = Catalog.GetColorChoices(Palette, bCreatorOnly);
			return;
		}
		for (const FString& Token : Tokens)
		{
			const int32 Index = Catalog.FindColorIndex(Palette, FName(*Token));
			if (Index == INDEX_NONE || Index > MAX_uint8)
			{
				OutProblems.Add(FString::Printf(TEXT("규칙 %s.%s: 팔레트 %s 에 색 %s 가 없다"), *RuleId.ToString(), Column, *Palette.ToString(), *Token));
				continue;
			}
			Out.Add(static_cast<uint8>(Index));
		}
	}
}

// ═════════════════════════ FGrimAppearance ═════════════════════════

FName FGrimAppearance::GetPart(EGrimAppearanceSlot Slot) const
{
	const int32 Index = static_cast<int32>(Slot);
	return Parts.IsValidIndex(Index) ? Parts[Index] : NAME_None;
}

void FGrimAppearance::SetPart(EGrimAppearanceSlot Slot, FName PartId)
{
	const int32 Index = static_cast<int32>(Slot);
	if (Index < 0 || Index >= GrimAppearanceSlotCount)
	{
		return;
	}
	if (Parts.Num() < GrimAppearanceSlotCount)
	{
		Parts.SetNum(GrimAppearanceSlotCount);
	}
	Parts[Index] = PartId;
}

float FGrimAppearance::GetMorph(int32 Index) const
{
	return Morphs.IsValidIndex(Index) ? DequantizeUnit(Morphs[Index]) : 0.f;
}

void FGrimAppearance::SetMorph(int32 Index, float Value)
{
	if (Index < 0)
	{
		return;
	}
	if (!Morphs.IsValidIndex(Index))
	{
		Morphs.SetNum(Index + 1);
		for (uint8& Byte : Morphs)
		{
			Byte = Byte == 0 ? NeutralByte : Byte;
		}
	}
	Morphs[Index] = QuantizeUnit(Value);
}

uint8 FGrimAppearance::QuantizeUnit(float Value)
{
	return static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(FMath::Clamp(Value, -1.f, 1.f) * 127.f) + NeutralByte, 1, 255));
}

float FGrimAppearance::DequantizeUnit(uint8 Byte)
{
	return FMath::Clamp((static_cast<int32>(Byte) - NeutralByte) / 127.f, -1.f, 1.f);
}

void FGrimAppearance::Serialize(FArchive& Ar)
{
	const FGrimAppearanceCatalog& Catalog = UGrimAppearanceSettings::GetCatalog();
	uint8 Version = SaveVersion;
	Ar << Version;
	Ar << BodyType << Height << SkinColor << EyeColor << HairColor << ClothPrimary << ClothSecondary << Element << Circle;

	// 조각: (칸 이름, 조각 ID) — 칸 순서 · 카탈로그 순서가 바뀌어도 읽힌다
	const UEnum* SlotEnum = StaticEnum<EGrimAppearanceSlot>();
	int32 NumParts = 0;
	if (Ar.IsSaving())
	{
		for (const FName& Part : Parts)
		{
			NumParts += Part.IsNone() ? 0 : 1;
		}
	}
	Ar << NumParts;
	if (Ar.IsLoading())
	{
		Parts.Reset();
		Parts.SetNum(GrimAppearanceSlotCount);
		NumParts = FMath::Clamp(NumParts, 0, 64);
		for (int32 Index = 0; Index < NumParts && !Ar.IsError(); ++Index)
		{
			FString SlotText, PartText;
			Ar << SlotText << PartText;
			const int64 SlotValue = SlotEnum->GetValueByNameString(SlotText);
			if (SlotValue >= 0 && SlotValue < GrimAppearanceSlotCount)
			{
				Parts[SlotValue] = FName(*PartText);
			}
		}
	}
	else
	{
		for (int32 Index = 0; Index < Parts.Num(); ++Index)
		{
			if (Parts[Index].IsNone())
			{
				continue;
			}
			FString SlotText = FGrimAppearanceMath::SlotName(static_cast<EGrimAppearanceSlot>(Index));
			FString PartText = Parts[Index].ToString();
			Ar << SlotText << PartText;
		}
	}

	// 슬라이더: (ID, 값)
	int32 NumMorphs = Ar.IsSaving() ? FMath::Min(Morphs.Num(), Catalog.Morphs.Num()) : 0;
	Ar << NumMorphs;
	if (Ar.IsLoading())
	{
		Morphs.Init(NeutralByte, Catalog.Morphs.Num());
		NumMorphs = FMath::Clamp(NumMorphs, 0, 255);
		for (int32 Index = 0; Index < NumMorphs && !Ar.IsError(); ++Index)
		{
			FString IdText;
			uint8 Value = NeutralByte;
			Ar << IdText << Value;
			const int32 MorphIndex = Catalog.FindMorph(FName(*IdText));
			if (Morphs.IsValidIndex(MorphIndex))
			{
				Morphs[MorphIndex] = Value;
			}
		}
	}
	else
	{
		for (int32 Index = 0; Index < NumMorphs; ++Index)
		{
			FString IdText = Catalog.Morphs[Index].Id.ToString();
			uint8 Value = Morphs[Index];
			Ar << IdText << Value;
		}
	}

	FString RuleText = Rule.ToString();
	Ar << RuleText << Seed;
	if (Ar.IsLoading())
	{
		Rule = RuleText.IsEmpty() || RuleText == TEXT("None") ? NAME_None : FName(*RuleText);
	}
}

bool FGrimAppearance::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	const FGrimAppearanceCatalog& Catalog = UGrimAppearanceSettings::GetCatalog();

	uint8 NumParts = static_cast<uint8>(FMath::Min(Parts.Num(), 32));
	Ar << NumParts;
	if (Ar.IsLoading())
	{
		Parts.SetNum(FMath::Min<int32>(NumParts, 32));
	}
	for (int32 Index = 0; Index < NumParts && !Ar.IsError(); ++Index)
	{
		uint16 Net = Ar.IsSaving() ? Catalog.PartToNet(Parts[Index]) : 0;
		Ar << Net;
		if (Ar.IsLoading() && Parts.IsValidIndex(Index))
		{
			Parts[Index] = Catalog.PartFromNet(Net);
		}
	}

	uint8 NumMorphs = static_cast<uint8>(FMath::Min(Morphs.Num(), 255));
	Ar << NumMorphs;
	if (Ar.IsLoading())
	{
		Morphs.SetNum(NumMorphs);
	}
	for (int32 Index = 0; Index < NumMorphs && !Ar.IsError(); ++Index)
	{
		Ar << Morphs[Index];
	}

	Ar << BodyType << Height << SkinColor << EyeColor << HairColor << ClothPrimary << ClothSecondary << Element << Circle;

	uint16 RuleNet = Ar.IsSaving() ? Catalog.RuleToNet(Rule) : 0;
	Ar << RuleNet;
	if (Ar.IsLoading())
	{
		Rule = Catalog.RuleFromNet(RuleNet);
	}
	Ar << Seed;

	bOutSuccess = !Ar.IsError();
	return true;
}

uint32 FGrimAppearance::GetHash() const
{
	FGrimAppearance Copy = *this;
	TArray<uint8> Bytes;
	FMemoryWriter Writer(Bytes);
	Copy.Serialize(Writer);
	return FCrc::MemCrc32(Bytes.GetData(), Bytes.Num());
}

FString FGrimAppearance::ToString() const
{
	if (!IsSet())
	{
		return TEXT("(외형 없음)");
	}
	FString Out = FString::Printf(TEXT("Body=%s Height=%d"), BodyType == 0 ? TEXT("A") : TEXT("B"), Height);
	for (int32 Index = 0; Index < Parts.Num(); ++Index)
	{
		if (!Parts[Index].IsNone())
		{
			Out += FString::Printf(TEXT(" %s=%s"), *FGrimAppearanceMath::SlotName(static_cast<EGrimAppearanceSlot>(Index)), *Parts[Index].ToString());
		}
	}
	Out += FString::Printf(TEXT(" Skin=%d Eye=%d HairColor=%d Cloth=%d/%d Element=%s Circle=%d Morphs=%d"),
		SkinColor, EyeColor, HairColor, ClothPrimary, ClothSecondary, *FGrimAppearanceMath::ElementName(Element), Circle, Morphs.Num());
	if (!Rule.IsNone())
	{
		Out += FString::Printf(TEXT(" Rule=%s Seed=%d"), *Rule.ToString(), Seed);
	}
	return Out;
}

bool FGrimAppearance::operator==(const FGrimAppearance& Other) const
{
	return BodyType == Other.BodyType && Height == Other.Height && Parts == Other.Parts && Morphs == Other.Morphs
		&& SkinColor == Other.SkinColor && EyeColor == Other.EyeColor && HairColor == Other.HairColor
		&& ClothPrimary == Other.ClothPrimary && ClothSecondary == Other.ClothSecondary
		&& Element == Other.Element && Circle == Other.Circle && Rule == Other.Rule && Seed == Other.Seed;
}

// ═════════════════════════ 카탈로그 ═════════════════════════

void FGrimAppearanceCatalog::BuildFromCSV(const FString& PartsCSV, const FString& PiecesCSV, const FString& PalettesCSV, const FString& MorphsCSV,
	const FString& RulesCSV, const FString& NpcsCSV, FGrimAppearanceCatalog& Out, TArray<FString>& OutProblems)
{
	using namespace GrimAppearanceTypesLocal;
	UDataTable* Parts = MakeTable(FGrimAppearancePartRow::StaticStruct(), TEXT("AppearanceParts"), PartsCSV, OutProblems);
	UDataTable* Pieces = MakeTable(FGrimAppearancePieceRow::StaticStruct(), TEXT("AppearancePieces"), PiecesCSV, OutProblems);
	UDataTable* Palettes = MakeTable(FGrimAppearancePaletteRow::StaticStruct(), TEXT("AppearancePalettes"), PalettesCSV, OutProblems);
	UDataTable* Morphs = MakeTable(FGrimAppearanceMorphRow::StaticStruct(), TEXT("AppearanceMorphs"), MorphsCSV, OutProblems);
	UDataTable* Rules = MakeTable(FGrimNpcAppearanceRuleRow::StaticStruct(), TEXT("NpcAppearanceRules"), RulesCSV, OutProblems);
	UDataTable* Npcs = MakeTable(FGrimNpcAppearanceRow::StaticStruct(), TEXT("NpcAppearance"), NpcsCSV, OutProblems);
	BuildFromTables(Parts, Pieces, Palettes, Morphs, Rules, Npcs, Out, OutProblems);
	for (UDataTable* Table : { Parts, Pieces, Palettes, Morphs, Rules, Npcs })
	{
		Table->MarkAsGarbage();
	}
}

void FGrimAppearanceCatalog::BuildFromTables(const UDataTable* PartsTable, const UDataTable* PiecesTable, const UDataTable* PalettesTable, const UDataTable* MorphsTable,
	const UDataTable* RulesTable, const UDataTable* NpcsTable, FGrimAppearanceCatalog& Out, TArray<FString>& OutProblems)
{
	using namespace GrimAppearanceTypesLocal;
	Out = FGrimAppearanceCatalog();

	// 팔레트 — 표 순서
	for (const TPair<FName, const FGrimAppearancePaletteRow*>& Pair : OrderedRows<FGrimAppearancePaletteRow>(PalettesTable))
	{
		FGrimAppearanceColor Color;
		Color.Id = Pair.Key;
		Color.DisplayName = Pair.Value->DisplayName.IsEmpty() ? Pair.Key.ToString() : Pair.Value->DisplayName;
		Color.bCreator = Pair.Value->Creator;
		FString Hex = Pair.Value->Hex.TrimStartAndEnd();
		Hex.RemoveFromStart(TEXT("#"));
		if (Hex.Len() != 6 && Hex.Len() != 8)
		{
			OutProblems.Add(FString::Printf(TEXT("팔레트 %s: Hex %s 를 읽을 수 없다"), *Pair.Key.ToString(), *Pair.Value->Hex));
		}
		Color.Color = FLinearColor::FromSRGBColor(FColor::FromHex(Hex));
		Out.Palettes.FindOrAdd(Pair.Value->Palette).Add(Color);
	}

	// 슬라이더 — 표 순서(복제 순서)
	for (const TPair<FName, const FGrimAppearanceMorphRow*>& Pair : OrderedRows<FGrimAppearanceMorphRow>(MorphsTable))
	{
		Out.MorphIndex.Add(Pair.Key, Out.Morphs.Num());
		Out.Morphs.Add({ Pair.Key, *Pair.Value });
	}

	// 조각 — ID 정렬(복제 번호)
	for (const TPair<FName, const FGrimAppearancePartRow*>& Pair : OrderedRows<FGrimAppearancePartRow>(PartsTable))
	{
		FGrimAppearancePart Part;
		Part.Id = Pair.Key;
		Part.Row = *Pair.Value;
		Part.BodyMask = FGrimAppearanceMath::ParseBodyMask(Pair.Value->BodyTypes);
		Out.Parts.Add(MoveTemp(Part));
	}
	Out.Parts.Sort([](const FGrimAppearancePart& A, const FGrimAppearancePart& B) { return NameLess(A.Id, B.Id); });
	for (int32 Index = 0; Index < Out.Parts.Num(); ++Index)
	{
		FGrimAppearancePart& Part = Out.Parts[Index];
		Out.PartIndex.Add(Part.Id, Index);
		Part.BaseMorphs.Init(0.f, Out.Morphs.Num());
		TArray<FString> Entries;
		SplitList(Part.Row.Morphs, Entries);
		for (const FString& Entry : Entries)
		{
			FString Key, Value;
			const int32 MorphIndex = Entry.Split(TEXT("="), &Key, &Value) ? Out.FindMorph(FName(*Key.TrimStartAndEnd())) : INDEX_NONE;
			if (MorphIndex == INDEX_NONE)
			{
				OutProblems.Add(FString::Printf(TEXT("조각 %s: 슬라이더 %s 를 모른다"), *Part.Id.ToString(), *Entry));
				continue;
			}
			Part.BaseMorphs[MorphIndex] = FMath::Clamp(FCString::Atof(*Value), -1.f, 1.f);
		}
		if (Part.Row.Slot >= EGrimAppearanceSlot::Count)
		{
			OutProblems.Add(FString::Printf(TEXT("조각 %s: 칸이 잘못됐다"), *Part.Id.ToString()));
		}
	}

	// 소품 — 표 순서
	for (const TPair<FName, const FGrimAppearancePieceRow*>& Pair : OrderedRows<FGrimAppearancePieceRow>(PiecesTable))
	{
		const int32 PartIndex = Out.FindPartIndex(Pair.Value->Part);
		if (PartIndex == INDEX_NONE)
		{
			OutProblems.Add(FString::Printf(TEXT("소품 %s: 조각 %s 가 없다"), *Pair.Key.ToString(), *Pair.Value->Part.ToString()));
			continue;
		}
		FGrimAppearancePiece Piece;
		Piece.Id = Pair.Key;
		Piece.Row = *Pair.Value;
		Piece.BodyMask = FGrimAppearanceMath::ParseBodyMask(Pair.Value->BodyTypes);
		Piece.bRepeatCircle = Pair.Value->Repeat.TrimStartAndEnd().Equals(TEXT("Circle"), ESearchCase::IgnoreCase);
		if (Piece.Row.Mesh.IsNull())
		{
			OutProblems.Add(FString::Printf(TEXT("소품 %s: 메시가 비었다"), *Pair.Key.ToString()));
		}
		Out.Parts[PartIndex].Pieces.Add(Out.Pieces.Num());
		Out.Pieces.Add(MoveTemp(Piece));
	}

	// 규칙 — ID 정렬(복제 번호)
	for (const TPair<FName, const FGrimNpcAppearanceRuleRow*>& Pair : OrderedRows<FGrimNpcAppearanceRuleRow>(RulesTable))
	{
		FGrimAppearanceRule Rule;
		Rule.Id = Pair.Key;
		Rule.Row = *Pair.Value;
		const FGrimNpcAppearanceRuleRow& Row = *Pair.Value;
		const FString* SlotColumns[GrimAppearanceSlotCount] = {
			&Row.Face, &Row.Hair, &Row.Beard, &Row.Hat, &Row.Robe, &Row.Stole, &Row.Belt, &Row.Cloak, &Row.Gloves, &Row.Boots, &Row.Legs };
		for (int32 SlotIndex = 0; SlotIndex < GrimAppearanceSlotCount; ++SlotIndex)
		{
			TArray<FString> Entries;
			SplitList(*SlotColumns[SlotIndex], Entries);
			for (const FString& Entry : Entries)
			{
				FString IdText = Entry, WeightText;
				float Weight = 1.f;
				if (Entry.Split(TEXT(":"), &IdText, &WeightText))
				{
					Weight = FCString::Atof(*WeightText);
				}
				IdText.TrimStartAndEndInline();
				FGrimAppearanceCandidate Candidate;
				Candidate.Weight = Weight;
				if (IdText != TEXT("-"))
				{
					const FGrimAppearancePart* Part = Out.FindPart(FName(*IdText));
					if (!Part || static_cast<int32>(Part->Row.Slot) != SlotIndex)
					{
						OutProblems.Add(FString::Printf(TEXT("규칙 %s: %s 칸 후보 %s 가 없거나 칸이 다르다"), *Rule.Id.ToString(),
							*FGrimAppearanceMath::SlotName(static_cast<EGrimAppearanceSlot>(SlotIndex)), *IdText));
						continue;
					}
					if (Row.Player && !Part->Row.Creator)
					{
						OutProblems.Add(FString::Printf(TEXT("규칙 %s(플레이어): 후보 %s 는 생성 화면 조각이 아니다"), *Rule.Id.ToString(), *IdText));
						continue;
					}
					Candidate.Part = Part->Id;
				}
				Rule.Slots[SlotIndex].Add(Candidate);
			}
		}
		const bool bCreator = Row.Player;
		ParseColorList(Out, GrimAppearancePalettes::Skin, Row.Skin, bCreator, Rule.Id, TEXT("Skin"), Rule.Skin, OutProblems);
		ParseColorList(Out, GrimAppearancePalettes::Eye, Row.Eye, bCreator, Rule.Id, TEXT("Eye"), Rule.Eye, OutProblems);
		ParseColorList(Out, GrimAppearancePalettes::Hair, Row.HairColor, bCreator, Rule.Id, TEXT("HairColor"), Rule.Hair, OutProblems);
		if (!Row.GreyHairColor.IsEmpty())
		{
			ParseColorList(Out, GrimAppearancePalettes::Hair, Row.GreyHairColor, false, Rule.Id, TEXT("GreyHairColor"), Rule.GreyHair, OutProblems);
		}
		ParseColorList(Out, GrimAppearancePalettes::Cloth, Row.ClothPrimary, bCreator, Rule.Id, TEXT("ClothPrimary"), Rule.ClothPrimary, OutProblems);
		ParseColorList(Out, GrimAppearancePalettes::Cloth, Row.ClothSecondary, bCreator, Rule.Id, TEXT("ClothSecondary"), Rule.ClothSecondary, OutProblems);

		TArray<FString> ElementTokens;
		SplitList(Row.Elements, ElementTokens);
		for (const FString& Token : ElementTokens)
		{
			if (Token == TEXT("*"))
			{
				for (uint8 Element = 1; Element <= FGrimAppearance::ElementCount; ++Element)
				{
					Rule.Elements.AddUnique(Element);
				}
				continue;
			}
			const uint8 Element = FGrimAppearanceMath::ElementFromName(Token);
			if (Element == 0)
			{
				OutProblems.Add(FString::Printf(TEXT("규칙 %s: 속성 %s 를 모른다"), *Rule.Id.ToString(), *Token));
				continue;
			}
			Rule.Elements.AddUnique(Element);
		}
		if (Row.CircleMin > Row.CircleMax || Row.CircleMin < 0 || Row.CircleMax > FGrimAppearance::MaxCircle)
		{
			OutProblems.Add(FString::Printf(TEXT("규칙 %s: 서클 범위 %d~%d 가 잘못됐다"), *Rule.Id.ToString(), Row.CircleMin, Row.CircleMax));
		}
		Out.Rules.Add(MoveTemp(Rule));
	}
	Out.Rules.Sort([](const FGrimAppearanceRule& A, const FGrimAppearanceRule& B) { return NameLess(A.Id, B.Id); });
	for (int32 Index = 0; Index < Out.Rules.Num(); ++Index)
	{
		Out.RuleIndex.Add(Out.Rules[Index].Id, Index);
	}

	// 이름 있는 NPC
	for (const TPair<FName, const FGrimNpcAppearanceRow*>& Pair : OrderedRows<FGrimNpcAppearanceRow>(NpcsTable))
	{
		if (!Out.FindRule(Pair.Value->Rule))
		{
			OutProblems.Add(FString::Printf(TEXT("NPC %s: 규칙 %s 가 없다"), *Pair.Key.ToString(), *Pair.Value->Rule.ToString()));
		}
		FGrimNpcAppearanceBinding Binding;
		Binding.Rule = Pair.Value->Rule;
		Binding.Seed = Pair.Value->Seed;
		Binding.Circle = Pair.Value->Circle;
		Binding.bElementOverride = !Pair.Value->Element.TrimStartAndEnd().IsEmpty();
		Binding.Element = Binding.bElementOverride ? FGrimAppearanceMath::ElementFromName(Pair.Value->Element) : 0;
		Out.Npcs.Add(Pair.Key, Binding);
	}

	for (const FName& Required : { GrimAppearancePalettes::Skin, GrimAppearancePalettes::Eye, GrimAppearancePalettes::Hair, GrimAppearancePalettes::Cloth })
	{
		if (Out.GetPalette(Required).Num() == 0)
		{
			OutProblems.Add(FString::Printf(TEXT("팔레트 %s 가 비었다"), *Required.ToString()));
		}
	}
	if (Out.GetPalette(GrimAppearancePalettes::Element).Num() < FGrimAppearance::ElementCount)
	{
		OutProblems.Add(TEXT("팔레트 Element 는 여섯 색(물 · 불 · 바람 · 땅 · 빛 · 어둠)이어야 한다"));
	}
}

const FGrimAppearancePart* FGrimAppearanceCatalog::FindPart(FName Id) const
{
	const int32 Index = FindPartIndex(Id);
	return Index == INDEX_NONE ? nullptr : &Parts[Index];
}

int32 FGrimAppearanceCatalog::FindPartIndex(FName Id) const
{
	const int32* Found = Id.IsNone() ? nullptr : PartIndex.Find(Id);
	return Found ? *Found : INDEX_NONE;
}

const FGrimAppearanceRule* FGrimAppearanceCatalog::FindRule(FName Id) const
{
	const int32 Index = FindRuleIndex(Id);
	return Index == INDEX_NONE ? nullptr : &Rules[Index];
}

int32 FGrimAppearanceCatalog::FindRuleIndex(FName Id) const
{
	const int32* Found = Id.IsNone() ? nullptr : RuleIndex.Find(Id);
	return Found ? *Found : INDEX_NONE;
}

int32 FGrimAppearanceCatalog::FindMorph(FName Id) const
{
	const int32* Found = MorphIndex.Find(Id);
	return Found ? *Found : INDEX_NONE;
}

const TArray<FGrimAppearanceColor>& FGrimAppearanceCatalog::GetPalette(FName Palette) const
{
	static const TArray<FGrimAppearanceColor> Empty;
	const TArray<FGrimAppearanceColor>* Found = Palettes.Find(Palette);
	return Found ? *Found : Empty;
}

int32 FGrimAppearanceCatalog::FindColorIndex(FName Palette, FName ColorId) const
{
	return GetPalette(Palette).IndexOfByPredicate([ColorId](const FGrimAppearanceColor& Color) { return Color.Id == ColorId; });
}

FLinearColor FGrimAppearanceCatalog::GetColor(FName Palette, int32 Index, const FLinearColor& Fallback) const
{
	const TArray<FGrimAppearanceColor>& Colors = GetPalette(Palette);
	return Colors.IsValidIndex(Index) ? Colors[Index].Color : Fallback;
}

FLinearColor FGrimAppearanceCatalog::GetFixedColor(FName ColorId, const FLinearColor& Fallback) const
{
	return GetColor(GrimAppearancePalettes::Fixed, FindColorIndex(GrimAppearancePalettes::Fixed, ColorId), Fallback);
}

TArray<FName> FGrimAppearanceCatalog::GetPartsForSlot(EGrimAppearanceSlot Slot, uint8 BodyType, bool bCreatorOnly) const
{
	TArray<FName> Out;
	for (const FGrimAppearancePart& Part : Parts)
	{
		if (Part.Row.Slot == Slot && Part.FitsBody(BodyType) && (!bCreatorOnly || Part.Row.Creator))
		{
			Out.Add(Part.Id);
		}
	}
	return Out;
}

TArray<uint8> FGrimAppearanceCatalog::GetColorChoices(FName Palette, bool bCreatorOnly) const
{
	TArray<uint8> Out;
	const TArray<FGrimAppearanceColor>& Colors = GetPalette(Palette);
	for (int32 Index = 0; Index < Colors.Num() && Index <= MAX_uint8; ++Index)
	{
		if (!bCreatorOnly || Colors[Index].bCreator)
		{
			Out.Add(static_cast<uint8>(Index));
		}
	}
	return Out;
}

uint16 FGrimAppearanceCatalog::PartToNet(FName Id) const
{
	const int32 Index = FindPartIndex(Id);
	return Index == INDEX_NONE || Index >= MAX_uint16 ? 0 : static_cast<uint16>(Index + 1);
}

FName FGrimAppearanceCatalog::PartFromNet(uint16 Net) const
{
	return Net > 0 && Parts.IsValidIndex(Net - 1) ? Parts[Net - 1].Id : NAME_None;
}

uint16 FGrimAppearanceCatalog::RuleToNet(FName Id) const
{
	const int32 Index = FindRuleIndex(Id);
	return Index == INDEX_NONE || Index >= MAX_uint16 ? 0 : static_cast<uint16>(Index + 1);
}

FName FGrimAppearanceCatalog::RuleFromNet(uint16 Net) const
{
	return Net > 0 && Rules.IsValidIndex(Net - 1) ? Rules[Net - 1].Id : NAME_None;
}

// ═════════════════════════ 계산 ═════════════════════════

bool FGrimAppearanceMath::Sanitize(FGrimAppearance& InOut, const FGrimAppearanceCatalog& Catalog, bool bPlayer)
{
	const FGrimAppearance Before = InOut;

	InOut.BodyType = FMath::Min<uint8>(InOut.BodyType, 1);
	InOut.Parts.SetNum(GrimAppearanceSlotCount);
	for (int32 SlotIndex = 0; SlotIndex < GrimAppearanceSlotCount; ++SlotIndex)
	{
		const EGrimAppearanceSlot Slot = static_cast<EGrimAppearanceSlot>(SlotIndex);
		FName& PartId = InOut.Parts[SlotIndex];
		const FGrimAppearancePart* Part = Catalog.FindPart(PartId);
		const bool bValid = Part && Part->Row.Slot == Slot && Part->FitsBody(InOut.BodyType) && (!bPlayer || Part->Row.Creator);
		if (!bValid)
		{
			PartId = NAME_None;
		}
		// 얼굴은 비울 수 없다 — 그 체형의 첫 얼굴
		if (Slot == EGrimAppearanceSlot::Face && PartId.IsNone())
		{
			const TArray<FName> Faces = Catalog.GetPartsForSlot(Slot, InOut.BodyType, bPlayer);
			PartId = Faces.Num() > 0 ? Faces[0] : NAME_None;
		}
	}

	if (InOut.Morphs.Num() != Catalog.Morphs.Num())
	{
		const int32 OldNum = InOut.Morphs.Num();
		InOut.Morphs.SetNum(Catalog.Morphs.Num());
		for (int32 Index = OldNum; Index < InOut.Morphs.Num(); ++Index)
		{
			InOut.Morphs[Index] = FGrimAppearance::NeutralByte;
		}
	}
	for (uint8& Morph : InOut.Morphs)
	{
		Morph = FMath::Max<uint8>(Morph, 1); // 0 은 1 과 같은 −1 — 양쪽 대칭으로
	}

	auto FixColor = [&Catalog, bPlayer](uint8& Index, FName Palette)
	{
		const TArray<FGrimAppearanceColor>& Colors = Catalog.GetPalette(Palette);
		if (!Colors.IsValidIndex(Index) || (bPlayer && !Colors[Index].bCreator))
		{
			const TArray<uint8> Choices = Catalog.GetColorChoices(Palette, bPlayer);
			Index = Choices.Num() > 0 ? Choices[0] : 0;
		}
	};
	FixColor(InOut.SkinColor, GrimAppearancePalettes::Skin);
	FixColor(InOut.EyeColor, GrimAppearancePalettes::Eye);
	FixColor(InOut.HairColor, GrimAppearancePalettes::Hair);
	FixColor(InOut.ClothPrimary, GrimAppearancePalettes::Cloth);
	FixColor(InOut.ClothSecondary, GrimAppearancePalettes::Cloth);

	InOut.Element = FMath::Min<uint8>(InOut.Element, FGrimAppearance::ElementCount);
	InOut.Circle = FMath::Min<uint8>(InOut.Circle, FGrimAppearance::MaxCircle);
	if (bPlayer)
	{
		InOut.Rule = NAME_None;
		InOut.Seed = 0;
	}
	else if (!InOut.Rule.IsNone() && !Catalog.FindRule(InOut.Rule))
	{
		InOut.Rule = NAME_None;
	}
	return InOut != Before;
}

FGrimAppearance FGrimAppearanceMath::GenerateFromRule(const FGrimAppearanceCatalog& Catalog, FName RuleId, int32 Seed)
{
	using namespace GrimAppearanceTypesLocal;
	FGrimAppearance Out;
	const FGrimAppearanceRule* Rule = Catalog.FindRule(RuleId);
	if (!Rule)
	{
		return Out;
	}
	const FGrimNpcAppearanceRuleRow& Row = Rule->Row;
	// 규칙 이름을 섞어 같은 씨앗이라도 규칙마다 다른 사람이 나오게(이름 문자열 CRC — 실행마다 같다)
	// FRandomStream 은 첫 난수가 이웃 씨앗(10 · 11 · 12)끼리 비슷하다 — 씨앗을 한 번 섞어(murmur 마무리) 줄 세운 NPC 가 닮지 않게
	uint32 Mixed = static_cast<uint32>(Seed) ^ static_cast<uint32>(MakeSeed(Rule->Id.ToString()));
	Mixed ^= Mixed >> 16;
	Mixed *= 0x85ebca6bu;
	Mixed ^= Mixed >> 13;
	Mixed *= 0xc2b2ae35u;
	Mixed ^= Mixed >> 16;
	FRandomStream Stream(static_cast<int32>(Mixed));

	// 난수를 쓰는 순서가 곧 형식이다 — 바꾸면 모든 NPC 얼굴이 바뀐다(이름 있는 NPC 도). 뽑기는 후보가 없어도 난수를 한 번 쓴다.
	const int32 BodyPick = PickWeighted(Stream, { Row.BodyA, Row.BodyB });
	Out.BodyType = BodyPick == 1 ? 1 : 0;
	Out.Height = FGrimAppearance::QuantizeUnit(Stream.FRandRange(-1.f, 1.f) * FMath::Clamp(Row.HeightJitter, 0.f, 1.f));

	Out.Parts.SetNum(GrimAppearanceSlotCount);
	for (int32 SlotIndex = 0; SlotIndex < GrimAppearanceSlotCount; ++SlotIndex)
	{
		TArray<FName> Ids;
		TArray<float> Weights;
		for (const FGrimAppearanceCandidate& Candidate : Rule->Slots[SlotIndex])
		{
			const FGrimAppearancePart* Part = Catalog.FindPart(Candidate.Part);
			if (Candidate.Part.IsNone() || (Part && Part->FitsBody(Out.BodyType)))
			{
				Ids.Add(Candidate.Part);
				Weights.Add(Candidate.Weight);
			}
		}
		const int32 Pick = PickWeighted(Stream, Weights);
		Out.Parts[SlotIndex] = Ids.IsValidIndex(Pick) ? Ids[Pick] : NAME_None;
	}

	Out.SkinColor = PickByte(Stream, Rule->Skin, 0);
	Out.EyeColor = PickByte(Stream, Rule->Eye, 0);
	const bool bGrey = Stream.FRand() < Row.GreyHair && Rule->GreyHair.Num() > 0;
	const uint8 Hair = PickByte(Stream, Rule->Hair, 0);
	const uint8 Grey = PickByte(Stream, Rule->GreyHair, Hair);
	Out.HairColor = bGrey ? Grey : Hair;
	Out.ClothPrimary = PickByte(Stream, Rule->ClothPrimary, 0);
	Out.ClothSecondary = PickByte(Stream, Rule->ClothSecondary, 0);
	Out.Circle = static_cast<uint8>(FMath::Clamp(Stream.RandRange(Row.CircleMin, FMath::Max(Row.CircleMin, Row.CircleMax)), 0, FGrimAppearance::MaxCircle));
	Out.Element = PickByte(Stream, Rule->Elements, 0);

	ApplyFaceDefaults(Out, Catalog);
	const float Jitter = FMath::Clamp(Row.MorphJitter, 0.f, 1.f);
	for (int32 Index = 0; Index < Out.Morphs.Num(); ++Index)
	{
		Out.SetMorph(Index, Out.GetMorph(Index) + Stream.FRandRange(-Jitter, Jitter));
	}

	Out.Rule = Rule->Id;
	Out.Seed = Seed;
	Sanitize(Out, Catalog, Row.Player);
	if (Row.Player)
	{
		// 플레이어 규칙도 무작위 결과는 규칙 이름을 남기지 않는다(플레이어 외형은 규칙이 없다)
		Out.Rule = NAME_None;
		Out.Seed = 0;
	}
	return Out;
}

FGrimAppearance FGrimAppearanceMath::GenerateForNpc(const FGrimAppearanceCatalog& Catalog, FName NpcId, FName FallbackRule)
{
	const FGrimNpcAppearanceBinding* Binding = Catalog.Npcs.Find(NpcId);
	const FName RuleId = Binding ? Binding->Rule : FallbackRule;
	const int32 Seed = Binding && Binding->Seed != 0 ? Binding->Seed : MakeSeed(NpcId.ToString());
	FGrimAppearance Out = GenerateFromRule(Catalog, RuleId, Seed);
	if (Binding && Out.IsSet())
	{
		if (Binding->Circle > 0)
		{
			Out.Circle = static_cast<uint8>(FMath::Clamp(Binding->Circle, 0, FGrimAppearance::MaxCircle));
		}
		if (Binding->bElementOverride)
		{
			Out.Element = Binding->Element;
		}
	}
	return Out;
}

int32 FGrimAppearanceMath::MakeSeed(const FString& Text)
{
	return static_cast<int32>(FCrc::StrCrc32(*Text.ToLower()));
}

void FGrimAppearanceMath::ApplyFaceDefaults(FGrimAppearance& InOut, const FGrimAppearanceCatalog& Catalog)
{
	InOut.Morphs.Init(FGrimAppearance::NeutralByte, Catalog.Morphs.Num());
	if (const FGrimAppearancePart* Face = Catalog.FindPart(InOut.GetPart(EGrimAppearanceSlot::Face)))
	{
		for (int32 Index = 0; Index < Face->BaseMorphs.Num() && Index < InOut.Morphs.Num(); ++Index)
		{
			InOut.SetMorph(Index, Face->BaseMorphs[Index]);
		}
	}
}

void FGrimAppearanceMath::ResolveColors(const FGrimAppearance& Appearance, const FGrimAppearanceCatalog& Catalog, FLinearColor (&OutColors)[GrimAppearanceChannelCount])
{
	using namespace GrimAppearanceTypesLocal;
	auto Set = [&OutColors](EGrimAppearanceChannel Channel, const FLinearColor& Color) { OutColors[static_cast<int32>(Channel)] = Color; };
	const FLinearColor Primary = Catalog.GetColor(GrimAppearancePalettes::Cloth, Appearance.ClothPrimary, FLinearColor(0.02f, 0.025f, 0.06f));
	const FLinearColor Secondary = Catalog.GetColor(GrimAppearancePalettes::Cloth, Appearance.ClothSecondary, FLinearColor(0.78f, 0.67f, 0.42f));
	Set(EGrimAppearanceChannel::None, FLinearColor::White);
	Set(EGrimAppearanceChannel::Primary, Primary);
	Set(EGrimAppearanceChannel::Secondary, Secondary);
	Set(EGrimAppearanceChannel::Leather, Catalog.GetFixedColor(FixedLeather, FLinearColor(0.14f, 0.07f, 0.03f)));
	Set(EGrimAppearanceChannel::Metal, Catalog.GetFixedColor(FixedMetal, FLinearColor(0.48f, 0.25f, 0.05f)));
	Set(EGrimAppearanceChannel::Under, Catalog.GetFixedColor(FixedUnder, FLinearColor(0.06f, 0.05f, 0.04f)));
	Set(EGrimAppearanceChannel::Lining, Appearance.Element > 0
		? Catalog.GetColor(GrimAppearancePalettes::Element, Appearance.Element - 1, Secondary) : Secondary);
	Set(EGrimAppearanceChannel::Skin, Catalog.GetColor(GrimAppearancePalettes::Skin, Appearance.SkinColor, FLinearColor(0.7f, 0.5f, 0.38f)));
	Set(EGrimAppearanceChannel::Hair, Catalog.GetColor(GrimAppearancePalettes::Hair, Appearance.HairColor, FLinearColor(0.05f, 0.03f, 0.02f)));
	Set(EGrimAppearanceChannel::Eye, Catalog.GetColor(GrimAppearancePalettes::Eye, Appearance.EyeColor, FLinearColor(0.1f, 0.06f, 0.03f)));
}

uint8 FGrimAppearanceMath::ElementFromName(const FString& Name)
{
	using namespace GrimAppearanceTypesLocal;
	FString Text = Name.TrimStartAndEnd();
	Text.RemoveFromStart(TEXT("Element."));
	for (uint8 Index = 1; Index <= FGrimAppearance::ElementCount; ++Index)
	{
		if (Text.Equals(ElementNames[Index], ESearchCase::IgnoreCase) || Text == ElementKoreanNames[Index])
		{
			return Index;
		}
	}
	return 0;
}

FString FGrimAppearanceMath::ElementName(uint8 Element)
{
	return GrimAppearanceTypesLocal::ElementNames[FMath::Min<uint8>(Element, FGrimAppearance::ElementCount)];
}

FText FGrimAppearanceMath::ElementDisplayName(uint8 Element)
{
	return FText::FromString(GrimAppearanceTypesLocal::ElementKoreanNames[FMath::Min<uint8>(Element, FGrimAppearance::ElementCount)]);
}

uint8 FGrimAppearanceMath::ParseBodyMask(const FString& Text)
{
	TArray<FString> Tokens;
	GrimAppearanceTypesLocal::SplitList(Text, Tokens);
	uint8 Mask = 0;
	for (const FString& Token : Tokens)
	{
		Mask |= Token.Equals(TEXT("A"), ESearchCase::IgnoreCase) ? 1 : 0;
		Mask |= Token.Equals(TEXT("B"), ESearchCase::IgnoreCase) ? 2 : 0;
	}
	return Mask == 0 ? 3 : Mask;
}

FString FGrimAppearanceMath::SlotName(EGrimAppearanceSlot Slot)
{
	return StaticEnum<EGrimAppearanceSlot>()->GetNameStringByValue(static_cast<int64>(Slot));
}

FText FGrimAppearanceMath::SlotDisplayName(EGrimAppearanceSlot Slot)
{
	switch (Slot)
	{
	case EGrimAppearanceSlot::Face: return LOCTEXT("SlotFace", "얼굴");
	case EGrimAppearanceSlot::Hair: return LOCTEXT("SlotHair", "머리 모양");
	case EGrimAppearanceSlot::Beard: return LOCTEXT("SlotBeard", "수염");
	case EGrimAppearanceSlot::Hat: return LOCTEXT("SlotHat", "모자");
	case EGrimAppearanceSlot::Robe: return LOCTEXT("SlotRobe", "로브");
	case EGrimAppearanceSlot::Stole: return LOCTEXT("SlotStole", "스톨");
	case EGrimAppearanceSlot::Belt: return LOCTEXT("SlotBelt", "벨트 · 주머니");
	case EGrimAppearanceSlot::Cloak: return LOCTEXT("SlotCloak", "망토");
	case EGrimAppearanceSlot::Gloves: return LOCTEXT("SlotGloves", "장갑");
	case EGrimAppearanceSlot::Boots: return LOCTEXT("SlotBoots", "신발");
	case EGrimAppearanceSlot::Legs: return LOCTEXT("SlotLegs", "바지");
	default: return FText::GetEmpty();
	}
}

FName FGrimAppearanceMath::ChannelParameterName(EGrimAppearanceChannel Channel)
{
	if (Channel == EGrimAppearanceChannel::None || Channel >= EGrimAppearanceChannel::Count)
	{
		return NAME_None;
	}
	return FName(*StaticEnum<EGrimAppearanceChannel>()->GetNameStringByValue(static_cast<int64>(Channel)));
}

#undef LOCTEXT_NAMESPACE
