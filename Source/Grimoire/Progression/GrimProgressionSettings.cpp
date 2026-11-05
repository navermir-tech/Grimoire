#include "Progression/GrimProgressionSettings.h"
#include "Engine/DataTable.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"

namespace GrimProgressionSettingsLocal // 유니티 빌드에서 이름이 부딪히지 않게
{
	FGrimLevelCurve GCurve;
	bool GCurveLoaded = false;

	FGrimSkillTreeDefinition GDefinition;
	bool GDefinitionLoaded = false;

	FAutoConsoleCommand GReloadLevelCommand(
		TEXT("Grim.Level.Reload"),
		TEXT("레벨 곡선 표(CSV)를 다시 읽는다(이미 걸린 레벨 보너스는 다음 레벨 변화 때 바뀐다)"),
		FConsoleCommandDelegate::CreateStatic(&UGrimLevelSettings::ResetCurve));

	FAutoConsoleCommand GReloadTreeCommand(
		TEXT("Grim.Tree.Reload"),
		TEXT("성장 트리 표(CSV)를 다시 읽는다"),
		FConsoleCommandDelegate::CreateStatic(&UGrimSkillTreeSettings::ResetDefinition));

	UDataTable* MakeTableFromCSV(UScriptStruct* RowStruct, const TCHAR* BaseName, const FString& CSV, TArray<FString>& OutProblems)
	{
		UDataTable* Table = NewObject<UDataTable>(GetTransientPackage(), MakeUniqueObjectName(GetTransientPackage(), UDataTable::StaticClass(), BaseName));
		Table->RowStruct = RowStruct;
		OutProblems.Append(Table->CreateTableFromCSVString(CSV));
		if (Table->GetRowMap().Num() == 0)
		{
			OutProblems.Add(TEXT("행이 없다"));
			return nullptr;
		}
		return Table;
	}

	/** 에셋이 지정돼 있고 행 구조체가 맞으면 그것, 아니면 Content 기준 CSV 를 읽는다. */
	const UDataTable* LoadTable(const TSoftObjectPtr<UDataTable>& Asset, UScriptStruct* RowStruct, const FString& RelativeCSV, const TCHAR* BaseName, TArray<FString>& OutProblems, FString& OutSource)
	{
		if (!Asset.IsNull())
		{
			UDataTable* Loaded = Asset.LoadSynchronous();
			if (Loaded && Loaded->GetRowStruct() == RowStruct)
			{
				OutSource = Asset.ToString();
				return Loaded;
			}
			OutProblems.Add(FString::Printf(TEXT("에셋 %s 을 쓸 수 없다(없거나 행 구조체가 다름) — CSV 로 대신한다"), *Asset.ToString()));
		}
		OutSource = FPaths::Combine(FPaths::ProjectContentDir(), RelativeCSV);
		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *OutSource))
		{
			OutProblems.Add(TEXT("파일을 열 수 없다"));
			return nullptr;
		}
		return MakeTableFromCSV(RowStruct, BaseName, Text, OutProblems);
	}
}

// ───────────────────────── 레벨 ─────────────────────────

UDataTable* UGrimLevelSettings::CreateLevelTableFromCSV(const FString& CSV, TArray<FString>& OutProblems)
{
	return GrimProgressionSettingsLocal::MakeTableFromCSV(FGrimLevelRow::StaticStruct(), TEXT("DT_LevelCurve"), CSV, OutProblems);
}

const FGrimLevelCurve& UGrimLevelSettings::GetCurve()
{
	using namespace GrimProgressionSettingsLocal;
	if (!GCurveLoaded)
	{
		GCurveLoaded = true;
		const UGrimLevelSettings* Settings = Get();
		TArray<FString> Problems;
		FString Source;
		const UDataTable* Table = LoadTable(Settings->LevelTableAsset, FGrimLevelRow::StaticStruct(), Settings->LevelTableCSV, TEXT("DT_LevelCurve"), Problems, Source);
		FGrimLevelMath::ReadCurve(Table, GCurve, Problems);
		for (const FString& Problem : Problems)
		{
			UE_LOG(LogGrimProgression, Warning, TEXT("레벨 표 %s: %s"), *Source, *Problem);
		}
		UE_LOG(LogGrimProgression, Log, TEXT("레벨 %d 개를 읽었다(상한 %d): %s"), GCurve.Steps.Num(), FGrimLevelMath::GetMaxLevel(GCurve, Settings->LevelCap), *Source);
	}
	return GCurve;
}

void UGrimLevelSettings::ResetCurve()
{
	GrimProgressionSettingsLocal::GCurveLoaded = false;
	GrimProgressionSettingsLocal::GCurve = FGrimLevelCurve();
}

FGrimXPSourceRule UGrimLevelSettings::GetSourceRule(const FGameplayTag& Source) const
{
	for (const FGrimXPSourceRule& Rule : Sources)
	{
		if (Rule.Source == Source)
		{
			return Rule;
		}
	}
	FGrimXPSourceRule Rule;
	Rule.Source = Source;
	Rule.RepeatDecay = DefaultRepeatDecay;
	Rule.RepeatWindowSeconds = DefaultRepeatWindowSeconds;
	Rule.MinMultiplier = DefaultMinMultiplier;
	return Rule;
}

// ───────────────────────── 성장 트리 ─────────────────────────

UDataTable* UGrimSkillTreeSettings::CreateNodeTableFromCSV(const FString& CSV, TArray<FString>& OutProblems)
{
	return GrimProgressionSettingsLocal::MakeTableFromCSV(FGrimSkillTreeNodeRow::StaticStruct(), TEXT("DT_SkillTreeNodes"), CSV, OutProblems);
}

const FGrimSkillTreeDefinition& UGrimSkillTreeSettings::GetDefinition()
{
	using namespace GrimProgressionSettingsLocal;
	if (!GDefinitionLoaded)
	{
		GDefinitionLoaded = true;
		const UGrimSkillTreeSettings* Settings = Get();
		TArray<FString> Problems;
		FString Source;
		const UDataTable* Table = LoadTable(Settings->NodeTableAsset, FGrimSkillTreeNodeRow::StaticStruct(), Settings->NodeTableCSV, TEXT("DT_SkillTreeNodes"), Problems, Source);
		FGrimSkillTreeMath::BuildDefinition(Table, GDefinition, Problems);
		for (const FString& Problem : Problems)
		{
			UE_LOG(LogGrimProgression, Warning, TEXT("성장 트리 표 %s: %s"), *Source, *Problem);
		}
		UE_LOG(LogGrimProgression, Log, TEXT("성장 트리 노드 %d 개(트리 %d 개, 비용 합 %d)를 읽었다: %s"), GDefinition.Nodes.Num(), GDefinition.GetTreeIds().Num(),
			FGrimSkillTreeMath::GetTotalCost(GDefinition), *Source);
	}
	return GDefinition;
}

void UGrimSkillTreeSettings::ResetDefinition()
{
	GrimProgressionSettingsLocal::GDefinitionLoaded = false;
	GrimProgressionSettingsLocal::GDefinition = FGrimSkillTreeDefinition();
}

const FGrimSkillTreeDisplay* UGrimSkillTreeSettings::FindTree(FName Id) const
{
	return Trees.FindByPredicate([Id](const FGrimSkillTreeDisplay& Display) { return Display.Id == Id; });
}

const FGrimSkillBranchDisplay* UGrimSkillTreeSettings::FindBranch(FName Id) const
{
	return Branches.FindByPredicate([Id](const FGrimSkillBranchDisplay& Display) { return Display.Id == Id; });
}

TArray<FName> UGrimSkillTreeSettings::GetTreeOrder(const FGrimSkillTreeDefinition& Definition) const
{
	const TArray<FName> InTable = Definition.GetTreeIds();
	TArray<FName> Order;
	for (const FGrimSkillTreeDisplay& Display : Trees)
	{
		if (InTable.Contains(Display.Id))
		{
			Order.AddUnique(Display.Id);
		}
	}
	for (const FName Id : InTable)
	{
		Order.AddUnique(Id);
	}
	return Order;
}
