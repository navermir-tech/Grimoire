#include "Magic/GrimProficiencySettings.h"
#include "Magic/GrimSpellDefinition.h"
#include "Magic/GrimSpellDelivery.h"
#include "Engine/DataTable.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

namespace GrimProficiencySettingsLocal
{
	TStrongObjectPtr<UDataTable> GDefaultGradeTable;
	bool GDefaultGradeTableTried = false;
	TArray<FGrimProficiencyGrade> GDefaultGrades;

	FAutoConsoleCommand GReloadCommand(
		TEXT("Grim.Magic.ReloadProficiency"),
		TEXT("숙련도 등급표(CSV)를 다시 읽는다"),
		FConsoleCommandDelegate::CreateStatic(&UGrimProficiencySettings::ResetDefaultGradeTable));
}

UDataTable* UGrimProficiencySettings::CreateGradeTableFromCSV(const FString& CSV, TArray<FString>& OutProblems)
{
	UDataTable* Table = NewObject<UDataTable>(GetTransientPackage(), MakeUniqueObjectName(GetTransientPackage(), UDataTable::StaticClass(), TEXT("DT_ProficiencyGrades")));
	Table->RowStruct = FGrimProficiencyGradeRow::StaticStruct();
	OutProblems.Append(Table->CreateTableFromCSVString(CSV));
	if (Table->GetRowMap().Num() == 0)
	{
		OutProblems.Add(TEXT("행이 없다"));
		return nullptr;
	}
	return Table;
}

const UDataTable* UGrimProficiencySettings::GetDefaultGradeTable()
{
	using namespace GrimProficiencySettingsLocal;
	if (GDefaultGradeTableTried)
	{
		return GDefaultGradeTable.Get();
	}
	GDefaultGradeTableTried = true;
	GDefaultGrades.Reset();

	const UGrimProficiencySettings* Settings = Get();
	if (!Settings->GradeTableAsset.IsNull())
	{
		UDataTable* Asset = Settings->GradeTableAsset.LoadSynchronous();
		if (Asset && Asset->GetRowStruct() == FGrimProficiencyGradeRow::StaticStruct())
		{
			GDefaultGradeTable.Reset(Asset);
			FGrimProficiencyMath::ReadGrades(Asset, GDefaultGrades);
			return Asset;
		}
		UE_LOG(LogGrimMagic, Error, TEXT("숙련도 등급표 에셋 %s 을 쓸 수 없다(없거나 행 구조체가 다름). CSV 로 대신한다."), *Settings->GradeTableAsset.ToString());
	}

	const FString FilePath = FPaths::Combine(FPaths::ProjectContentDir(), Settings->GradeTableCSV);
	FString Text;
	TArray<FString> Problems;
	UDataTable* Table = nullptr;
	if (FFileHelper::LoadFileToString(Text, *FilePath))
	{
		Table = CreateGradeTableFromCSV(Text, Problems);
	}
	else
	{
		Problems.Add(TEXT("파일을 열 수 없다"));
	}
	for (const FString& Problem : Problems)
	{
		UE_LOG(LogGrimMagic, Warning, TEXT("숙련도 등급표 %s: %s"), *FilePath, *Problem);
	}
	GDefaultGradeTable.Reset(Table);
	FGrimProficiencyMath::ReadGrades(Table, GDefaultGrades);
	if (Table)
	{
		UE_LOG(LogGrimMagic, Log, TEXT("숙련도 등급 %d 개를 읽었다: %s"), GDefaultGrades.Num(), *FilePath);
	}
	return Table;
}

void UGrimProficiencySettings::ResetDefaultGradeTable()
{
	using namespace GrimProficiencySettingsLocal;
	GDefaultGradeTable.Reset();
	GDefaultGradeTableTried = false;
	GDefaultGrades.Reset();
}

void UGrimProficiencySettings::GetGradesFor(const UGrimSpellDefinition* Spell, TArray<FGrimProficiencyGrade>& OutGrades)
{
	if (Spell && Spell->ProficiencyProfile && !Spell->ProficiencyProfile->GradeTable.IsNull())
	{
		if (const UDataTable* Table = Spell->ProficiencyProfile->GradeTable.LoadSynchronous())
		{
			FGrimProficiencyMath::ReadGrades(Table, OutGrades);
			if (OutGrades.Num() > 0)
			{
				return;
			}
		}
	}
	GetDefaultGradeTable();
	OutGrades = GrimProficiencySettingsLocal::GDefaultGrades;
}

FGrimProficiencyCurve UGrimProficiencySettings::GetCurveFor(const UGrimSpellDefinition* Spell)
{
	if (Spell && Spell->ProficiencyProfile)
	{
		return Spell->ProficiencyProfile->Curve;
	}
	const TArray<FGrimProficiencyCurve>& Curves = Get()->DefaultCurveByCircle;
	if (Curves.IsEmpty())
	{
		return FGrimProficiencyCurve();
	}
	const int32 Circle = Spell ? Spell->Circle : 1;
	return Curves[FMath::Clamp(Circle - 1, 0, Curves.Num() - 1)];
}

namespace GrimCircleSettingsLocal
{
	FGrimCircleTable GCircleTable;
	bool GCircleTableTried = false;
	TStrongObjectPtr<UDataTable> GCircleDataTable;

	FAutoConsoleCommand GReloadCirclesCommand(
		TEXT("Grim.Magic.ReloadCircles"),
		TEXT("서클 표(Circles.csv)를 다시 읽는다"),
		FConsoleCommandDelegate::CreateStatic(&UGrimProgressionSettings::ResetCircleTable));
}

UDataTable* UGrimProgressionSettings::CreateCircleTableFromCSV(const FString& CSV, TArray<FString>& OutProblems)
{
	UDataTable* Table = NewObject<UDataTable>(GetTransientPackage(), MakeUniqueObjectName(GetTransientPackage(), UDataTable::StaticClass(), TEXT("DT_Circles")));
	Table->RowStruct = FGrimCircleRow::StaticStruct();
	OutProblems.Append(Table->CreateTableFromCSVString(CSV));
	if (Table->GetRowMap().Num() == 0)
	{
		OutProblems.Add(TEXT("행이 없다"));
		return nullptr;
	}
	return Table;
}

void UGrimProgressionSettings::GetGradeOrder(TArray<FName>& OutOrder)
{
	TArray<FGrimProficiencyGrade> Grades;
	UGrimProficiencySettings::GetGradesFor(nullptr, Grades);
	OutOrder.Reset(Grades.Num());
	for (const FGrimProficiencyGrade& Grade : Grades)
	{
		OutOrder.Add(Grade.Grade);
	}
}

const FGrimCircleTable& UGrimProgressionSettings::GetCircleTable()
{
	using namespace GrimCircleSettingsLocal;
	if (GCircleTableTried)
	{
		return GCircleTable;
	}
	GCircleTableTried = true;
	GCircleTable = FGrimCircleTable();

	const UGrimProgressionSettings* Settings = Get();
	TArray<FString> Problems;
	UDataTable* Table = nullptr;
	FString Source = Settings->CircleTableAsset.ToString();
	if (!Settings->CircleTableAsset.IsNull())
	{
		Table = Settings->CircleTableAsset.LoadSynchronous();
		if (!Table || Table->GetRowStruct() != FGrimCircleRow::StaticStruct())
		{
			UE_LOG(LogGrimMagic, Error, TEXT("서클 표 에셋 %s 을 쓸 수 없다(없거나 행 구조체가 다름). CSV 로 대신한다."), *Source);
			Table = nullptr;
		}
	}
	if (!Table)
	{
		Source = FPaths::Combine(FPaths::ProjectContentDir(), Settings->CircleTableCSV);
		FString Text;
		if (FFileHelper::LoadFileToString(Text, *Source))
		{
			Table = CreateCircleTableFromCSV(Text, Problems);
		}
		else
		{
			Problems.Add(TEXT("파일을 열 수 없다"));
		}
	}
	GCircleDataTable.Reset(Table);

	TArray<FName> GradeOrder;
	GetGradeOrder(GradeOrder);
	if (Table)
	{
		FGrimCircleMath::ReadTable(Table, GCircleTable, GradeOrder, Problems);
	}
	for (const FString& Problem : Problems)
	{
		UE_LOG(LogGrimMagic, Warning, TEXT("서클 표 %s: %s"), *Source, *Problem);
	}
	UE_LOG(LogGrimMagic, Log, TEXT("서클 %d 개를 읽었다: %s"), GCircleTable.GetMaxCircle(), *Source);
	return GCircleTable;
}

void UGrimProgressionSettings::ResetCircleTable()
{
	using namespace GrimCircleSettingsLocal;
	GCircleTable = FGrimCircleTable();
	GCircleTableTried = false;
	GCircleDataTable.Reset();
}
